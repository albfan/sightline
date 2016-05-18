/* sl-log-reader.h
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

#ifndef SL_LOG_READER_H
#define SL_LOG_READER_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define SL_TYPE_LOG_READER (sl_log_reader_get_type())

G_DECLARE_FINAL_TYPE (SlLogReader, sl_log_reader, SL, LOG_READER, GObject)

SlLogReader *sl_log_reader_new    (void);
gboolean     sl_log_reader_ingest (SlLogReader  *self,
                                   const gchar  *filename,
                                   GError      **error);

G_END_DECLS

#endif /* SL_LOG_READER_H */
