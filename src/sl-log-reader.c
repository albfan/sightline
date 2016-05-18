/* sl-log-reader.c
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#define G_LOG_DOMAIN "sl-log-reader"

#include <glib/gi18n.h>
#include <string.h>

#include "sl-line-reader.h"
#include "sl-log-reader.h"

struct _SlLogReader
{
  GObject  parent_instance;
  gchar   *clang_include_path;
};

enum {
  FLAGS_EXTRACTED,
  N_SIGNALS
};

static guint signals [N_SIGNALS];

G_DEFINE_TYPE (SlLogReader, sl_log_reader, G_TYPE_OBJECT)

static inline gboolean
str_empty0 (const gchar *str)
{
  return !str || !*str;
}

static void
sl_log_reader_discover_clang_include_path (SlLogReader *self)
{
  g_autoptr(GSubprocess) subprocess = NULL;
  g_autofree gchar *stdoutstr = NULL;

  g_assert (SL_IS_LOG_READER (self));
  g_assert (self->clang_include_path == NULL);

  subprocess = g_subprocess_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE,
                                 NULL,
                                 "clang",
                                 "-print-file-name=include",
                                 NULL);

  if (!subprocess ||
      !g_subprocess_communicate_utf8 (subprocess, NULL, NULL, &stdoutstr, NULL, NULL))
    {
      g_warning ("Failed to discover clang include path");
      return;
    }

  g_strstrip (stdoutstr);

  if (!g_str_equal (stdoutstr, "include"))
    self->clang_include_path = g_strdup_printf ("-I%s", stdoutstr);
}

static void
sl_log_reader_constructed (GObject *object)
{
  SlLogReader *self = (SlLogReader *)object;

  sl_log_reader_discover_clang_include_path (self);

  G_OBJECT_CLASS (sl_log_reader_parent_class)->constructed (object);
}

static void
sl_log_reader_finalize (GObject *object)
{
  SlLogReader *self = (SlLogReader *)object;

  g_clear_pointer (&self->clang_include_path, g_free);

  G_OBJECT_CLASS (sl_log_reader_parent_class)->finalize (object);
}

static void
sl_log_reader_class_init (SlLogReaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = sl_log_reader_constructed;
  object_class->finalize = sl_log_reader_finalize;

  signals [FLAGS_EXTRACTED] =
    g_signal_new ("flags-extracted",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRV);
}

static void
sl_log_reader_init (SlLogReader *self)
{
}

static void
sl_log_reader_parse_c_cxx_include (SlLogReader  *self,
                                   GPtrArray    *ret,
                                   const gchar  *part1,
                                   const gchar  *part2,
                                   const gchar  *subdir)
{
  static const gchar *dummy = "-I";
  g_autofree gchar *adjusted = NULL;

  g_assert (self != NULL);
  g_assert (ret != NULL);
  g_assert (part1 != NULL);
  g_assert (subdir != NULL);

  /*
   * We will get parts either like ("-Ifoo", NULL) or ("-I", "foo").
   * Canonicalize things so we are always dealing with something that looks
   * like ("-I", "foo") since we might need to mutate the path.
   */

  if (part2 == NULL)
    {
      g_assert (strlen (part1) > 2);
      part2 = &part1 [2];
      part1 = dummy;
    }

  g_assert (!str_empty0 (part1));
  g_assert (!str_empty0 (part2));

  /*
   * If the path is relative, then we need to adjust it to be relative to the
   * target file rather than relative to the makefile. Clang expects the
   * path information to be as such.
   */

  if (!g_path_is_absolute (part2))
    {
      adjusted = g_build_filename (subdir, part2, NULL);
      part2 = adjusted;
    }

  g_ptr_array_add (ret, g_strdup_printf ("%s%s", part1, part2));
}

static void
sl_log_reader_parse_c_cxx (SlLogReader *self,
                           const gchar *line,
                           const gchar *subdir,
                           GPtrArray   *filenames,
                           GPtrArray   *ret)
{
  g_auto(GStrv) argv = NULL;
  g_autoptr(GError) error = NULL;
  gboolean in_expand = FALSE;
  gsize i;
  gint argc = 0;

  g_assert (line != NULL);
  g_assert (ret != NULL);
  g_assert (subdir != NULL);
  g_assert (filenames != NULL);
  g_assert (ret != NULL);

  while (g_ascii_isspace (*line))
    line++;

  if (!g_shell_parse_argv (line, &argc, &argv, &error))
    {
      /* TODO: Line might have ended with \, or inside a ( */
      g_debug ("Incomplete command: %s\n", error->message);
      return;
    }

  g_ptr_array_add (ret, g_strdup (self->clang_include_path));

  for (i = 0; argv[i]; i++)
    {
      const gchar *flag = argv [i];
      const gchar *dot;
      const gchar *tick;

    try_again:

      if (NULL != (tick = strchr (flag, '`')))
        {
          in_expand = !in_expand;

          if (!in_expand)
            {
              flag = ++tick;
              goto try_again;
            }

          continue;
        }

      if (in_expand || strlen (flag) < 2)
        continue;

      /* Try to extract filenames of source files */
      if (NULL != (dot = strrchr (flag, '.')))
        {
          if ((strcmp (dot, ".c") == 0) ||
              (strcmp (dot, ".h") == 0) ||
              (strcmp (dot, ".cc") == 0) ||
              (strcmp (dot, ".hh") == 0) ||
              (strcmp (dot, ".cxx") == 0) ||
              (strcmp (dot, ".cpp") == 0))
            {
              const gchar *tmp;

              if ((tmp = strstr (flag, "`")))
                flag = ++tmp;

              if (g_path_is_absolute (flag))
                g_ptr_array_add (filenames, g_strdup (flag));
              else
                g_ptr_array_add (filenames, g_build_filename (subdir, flag, NULL));

              continue;
            }
        }

      if (flag [0] != '-')
        continue;

      /* Extract preprorcesser/warnings/link information */
      switch (flag [1])
        {
        case 'I': /* -I./includes/ -I ./includes/ */
          {
            const gchar *part1 = flag;
            const gchar *part2 = NULL;

            if (strcmp (flag, "-I") == 0 && argv[i+1])
              part2 = argv [++i];

            sl_log_reader_parse_c_cxx_include (self, ret, part1, part2, subdir);
          }
          break;

        case 'f': /* -fPIC... */
        case 'W': /* -Werror... */
        case 'm': /* -m64 -mtune=native */
          g_ptr_array_add (ret, g_strdup (flag));
          break;

        case 'D': /* -Dfoo -D foo */
        case 'x': /* -xc++ */
          g_ptr_array_add (ret, g_strdup (flag));
          if ((strlen (flag) == 2) && argv[i+1])
            g_ptr_array_add (ret, g_strdup (argv [++i]));
          break;

        default:
          if (g_str_has_prefix (flag, "-std=") ||
              g_str_has_prefix (flag, "-pthread"))
            g_ptr_array_add (ret, g_strdup (flag));
          break;
        }
    }
}

static void
sl_log_reader_parse_command (SlLogReader  *self,
                             const gchar  *subdir,
                             gchar        *command,
                             gsize         len,
                             SlLineReader *reader)
{
  g_autoptr(GPtrArray) argv = NULL;
  g_autoptr(GPtrArray) filenames = NULL;

  g_assert (SL_IS_LOG_READER (self));
  g_assert (command != NULL);
  g_assert (len > 0);
  g_assert (reader != NULL);

  command[len] = '\0';

  argv = g_ptr_array_new_with_free_func (g_free);
  filenames = g_ptr_array_new_with_free_func (g_free);

  sl_log_reader_parse_c_cxx (self, command, subdir, filenames, argv);

  if (argv->len > 0)
    {
      guint i;

      g_ptr_array_add (argv, NULL);

      for (i = 0; i < filenames->len; i++)
        {
          const gchar *filename = g_ptr_array_index (filenames, i);

          g_signal_emit (self, signals [FLAGS_EXTRACTED], 0,
                         subdir, filename, argv->pdata);
        }
    }
}

gboolean
sl_log_reader_ingest (SlLogReader  *self,
                      const gchar  *filename,
                      GError      **error)
{
  g_autofree gchar *subdir = NULL;
  g_autofree gchar *data = NULL;
  SlLineReader *reader;
  const gchar *end;
  gchar *line;
  gsize data_len;
  gsize len;

  g_return_val_if_fail (SL_IS_LOG_READER (self), TRUE);
  g_return_val_if_fail (filename != NULL, TRUE);

  if (!g_file_get_contents (filename, &data, &data_len, error))
    return FALSE;

  if (!g_utf8_validate (data, data_len, &end))
    {
      g_set_error (error,
                   G_FILE_ERROR,
                   G_FILE_ERROR_FAILED,
                   "The file contained invalid UTF-8");
      return FALSE;
    }

  reader = sl_line_reader_new (data, data_len);

  while (NULL != (line = (gchar *)sl_line_reader_next (reader, &len)))
    {
      struct { const gchar *command; gsize len; } commands[] = {
        { "gcc", 3 },
        { "clang", 5 },
      };
      const gchar *change_dir;
      guint i;

      /*
       * Keep track of subdirectory changes. On some systems we can look for subdir=
       * but if subdir-objects is disabled, we sadly cannot.
       */
      if (NULL != (change_dir = memmem (line, len, ": Entering directory '", 22)))
        {
          g_free (subdir);
          subdir = g_strndup (change_dir + 22, len - (change_dir - line) - 22 - 1);
          continue;
        }

      /*
       * Look to see if this line starts calling gcc somewhere in it.
       *
       * We can probably speed this up by using a regex that does all
       * of the lookups at once rather than multiple lookups/scans.
       */
      for (i = 0; i < G_N_ELEMENTS (commands); i++)
        {
          gchar *cmd_begin;

          if (NULL == (cmd_begin = memmem (line, len, commands[i].command, commands[i].len)))
            continue;

          if (cmd_begin == line || g_ascii_isspace (cmd_begin[-1]))
            sl_log_reader_parse_command (self,
                                         subdir ? subdir : ".",
                                         cmd_begin,
                                         len - (cmd_begin - line),
                                         reader);
        }
    }

  return TRUE;
}

SlLogReader *
sl_log_reader_new (void)
{
  return g_object_new (SL_TYPE_LOG_READER, NULL);
}
