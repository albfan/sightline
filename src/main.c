/* main.c
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

#define G_LOG_DOMAIN "sightline"

#include <clang-c/Index.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include "sl-log-reader.h"

typedef struct
{
  guint count;
  gchar name[0];
} CallCount;

typedef struct
{
  GHashTable *callcounts;
  GHashTable *parsed;
} Sightline;

static void
sightline_inc_call_count (Sightline *self,
                          CXCursor   cursor)
{
  CXString str;
  const gchar *cstr;

  str = clang_getCursorSpelling (cursor);
  cstr = clang_getCString (str);

  if (cstr && *cstr)
    {
      CallCount *count = g_hash_table_lookup (self->callcounts, cstr);

      if (count == NULL)
        {
          gsize len = strlen (cstr);

          count = g_malloc (sizeof (CallCount) + len + 1);
          count->count = 0;
          memcpy (count->name, cstr, len);
          count->name[len] = '\0';

          g_hash_table_insert (self->callcounts, count->name, count);
        }

      count->count++;
    }

  clang_disposeString (str);
}

static enum CXChildVisitResult
cursor_visitor (CXCursor     cursor,
                CXCursor     parent,
                CXClientData client_data)
{
  enum CXCursorKind kind = clang_getCursorKind (cursor);
  Sightline *self = client_data;

  switch ((int)kind)
    {
    case CXCursor_CallExpr:
      sightline_inc_call_count (self, cursor);
      break;

    default:
      break;
    }

  return CXChildVisit_Recurse;
}

static void
flags_extracted (SlLogReader         *reader,
                 const gchar         *subdir,
                 const gchar         *filename,
                 const gchar * const *command_line_args,
                 gpointer             user_data)
{
  g_autofree gchar *cmdstr = g_strjoinv (" ", (gchar **)command_line_args);
  g_autoptr(GFile) file = NULL;
  Sightline *self = user_data;
  CXTranslationUnit unit;
  CXCursor cursor;
  CXIndex index;

  file = g_file_new_for_path (filename);

  if (g_hash_table_contains (self->parsed, file))
    {
      g_printerr ("Skipping %s, already parsed\n", filename);
      return;
    }

  g_hash_table_add (self->parsed, g_steal_pointer (&file));

  index = clang_createIndex (0, 0);
  unit = clang_parseTranslationUnit (index,
                                     filename,
                                     command_line_args,
                                     g_strv_length ((gchar **)command_line_args),
                                     NULL,
                                     0,
                                     CXTranslationUnit_DetailedPreprocessingRecord);
  cursor = clang_getTranslationUnitCursor (unit);
  clang_visitChildren (cursor, cursor_visitor, self);
  clang_disposeTranslationUnit (unit);
  clang_disposeIndex (index);
}

static gint
sort_by_count (gconstpointer a,
               gconstpointer b)
{
  const CallCount * const *cca = a;
  const CallCount * const *ccb = b;

  return (*ccb)->count - (*cca)->count;
}

gint
main (gint argc,
      gchar *argv[])
{
  g_autoptr(GOptionContext) context = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) sorted = NULL;
  Sightline *self;
  GHashTableIter iter;
  gpointer key, value;
  gint i;

  context = g_option_context_new (_("LOG_FILE... - Extract information about builds"));

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      return EXIT_FAILURE;
    }

  self = g_new0 (Sightline, 1);
  self->callcounts = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
  self->parsed = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, NULL);

  for (i = 1; i < argc; i++)
    {
      g_autoptr(SlLogReader) reader = NULL;
      g_autoptr(GError) error = NULL;
      const gchar *filename = argv[i];

      reader = sl_log_reader_new ();

      g_signal_connect (reader, "flags-extracted", G_CALLBACK (flags_extracted), self);

      if (!sl_log_reader_ingest (reader, filename, &error))
        {
          g_printerr ("%s\n", error->message);
          return EXIT_FAILURE;
        }
    }

  g_hash_table_iter_init (&iter, self->callcounts);

  sorted = g_ptr_array_new ();

  while (g_hash_table_iter_next (&iter, &key, &value))
    g_ptr_array_add (sorted, value);

  g_ptr_array_sort (sorted, sort_by_count);

  for (i = 0; i < sorted->len; i++)
    {
      CallCount *count = g_ptr_array_index (sorted, i);

      g_print ("%6u: %s\n", count->count, count->name);
    }

  g_hash_table_unref (self->callcounts);
  g_hash_table_unref (self->parsed);
  g_free (self);

  return EXIT_SUCCESS;
}
