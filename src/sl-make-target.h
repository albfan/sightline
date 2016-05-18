/* sl-make-target.h
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

#ifndef SL_MAKE_TARGET_H
#define SL_MAKE_TARGET_H

#include <glib-object.h>

G_BEGIN_DECLS

#define SL_TYPE_MAKE_TARGET (sl_make_target_get_type())

typedef struct _SlMakeTarget SlMakeTarget;

GType         sl_make_target_get_type   (void);
SlMakeTarget *sl_make_target_new        (const gchar   *subdir,
                                         const gchar   *target);
SlMakeTarget *sl_make_target_ref        (SlMakeTarget  *self);
void          sl_make_target_unref      (SlMakeTarget  *self);
const gchar  *sl_make_target_get_target (SlMakeTarget  *self);
const gchar  *sl_make_target_get_subdir (SlMakeTarget  *self);
void          sl_make_target_set_target (SlMakeTarget  *self,
                                         const gchar   *target);
guint         sl_make_target_hash       (gconstpointer  data);
gboolean      sl_make_target_equal      (gconstpointer  data1,
                                         gconstpointer  data2);

G_END_DECLS

#endif /* SL_MAKE_TARGET_H */
