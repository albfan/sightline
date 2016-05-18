/* sl-make-target.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
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

#define G_LOG_DOMAIN "sl-make-target"

#include "sl-make-target.h"

G_DEFINE_BOXED_TYPE (SlMakeTarget, sl_make_target, sl_make_target_ref, sl_make_target_unref)

struct _SlMakeTarget
{
  volatile gint  ref_count;

  gchar         *subdir;
  gchar         *target;
};

void
sl_make_target_unref (SlMakeTarget *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count > 0);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    {
      g_free (self->subdir);
      g_free (self->target);
      g_slice_free (SlMakeTarget, self);
    }
}

SlMakeTarget *
sl_make_target_ref (SlMakeTarget *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count > 0, NULL);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

SlMakeTarget *
sl_make_target_new (const gchar *subdir,
                          const gchar *target)
{
  SlMakeTarget *self;

  g_assert (target);

  if (subdir != NULL && (subdir [0] == '.' || subdir [0] == '\0'))
    subdir = NULL;

  self = g_slice_new0 (SlMakeTarget);
  self->ref_count = 1;
  self->subdir = g_strdup (subdir);
  self->target = g_strdup (target);

  return self;
}

const gchar *
sl_make_target_get_subdir (SlMakeTarget *self)
{
  g_assert (self);
  return self->subdir;
}

const gchar *
sl_make_target_get_target (SlMakeTarget *self)
{
  g_assert (self);
  return self->target;
}

void
sl_make_target_set_target (SlMakeTarget *self,
                           const gchar  *target)
{
  g_assert (self);

  g_free (self->target);
  self->target = g_strdup (target);
}

guint
sl_make_target_hash (gconstpointer data)
{
  const SlMakeTarget *self = data;

  return (g_str_hash (self->subdir ?: "") ^ g_str_hash (self->target));
}

gboolean
sl_make_target_equal (gconstpointer data1,
                      gconstpointer data2)
{
  const SlMakeTarget *a = data1;
  const SlMakeTarget *b = data2;

  return ((g_strcmp0 (a->subdir, b->subdir) == 0) &&
          (g_strcmp0 (a->target, b->target) == 0));
}
