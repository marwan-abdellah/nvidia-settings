/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
 * of the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *           Free Software Foundation, Inc.
 *           59 Temple Place - Suite 330
 *           Boston, MA 02111-1307, USA
 *
 */

#ifndef __CTK_UTILS_H__
#define __CTK_UTILS_H__

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkconfig.h"

G_BEGIN_DECLS

gchar* create_gpu_name_string(NvCtrlAttributeHandle *gpu_handle);


GtkWidget *add_table_row_with_help_text(GtkWidget *table,
                                        CtkConfig *ctk_config,
                                        const char *help,
                                        const gint row,
                                        const gint col,
                                        // 0 = left, 1 = right
                                        const gfloat name_xalign,
                                        // 0 = top, 1 = bottom
                                        const gfloat name_yalign,
                                        const gchar *name,
                                        const gfloat value_xalign,
                                        const gfloat value_yalign,
                                        const gchar *value);

GtkWidget *add_table_row(GtkWidget *, const gint,
                         const gfloat, const gfloat, const gchar *,
                         const gfloat, const gfloat, const gchar *);

GtkWidget * ctk_get_parent_window(GtkWidget *child);

void ctk_display_error_msg(GtkWidget *parent, gchar *msg);

void ctk_display_warning_msg(GtkWidget *parent, gchar *msg);

void ctk_empty_container(GtkWidget *);

void update_display_enabled_flag(NvCtrlAttributeHandle *handle,
                                 gboolean *display_enabled,
                                 unsigned int display_device_mask);

G_END_DECLS

#endif /* __CTK_UTILS_H__ */
