/* sl-line-reader.h
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

#ifndef SL_LINE_READER_H
#define SL_LINE_READER_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _SlLineReader SlLineReader;

SlLineReader *sl_line_reader_new  (const gchar   *contents,
                                   gssize         length);
void          sl_line_reader_free (SlLineReader  *self);
const gchar  *sl_line_reader_next (SlLineReader  *self,
                                   gsize         *length);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (SlLineReader, sl_line_reader_free)

G_END_DECLS

#endif /* SL_LINE_READER_H */
