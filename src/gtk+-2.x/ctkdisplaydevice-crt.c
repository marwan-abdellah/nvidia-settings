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

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkbanner.h"

#include "ctkdisplaydevice-crt.h"

#include "ctkimagesliders.h"
#include "ctkedid.h"
#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkutils.h"
#include <stdio.h>

static void ctk_display_device_crt_class_init(CtkDisplayDeviceCrtClass *);
static void ctk_display_device_crt_finalize(GObject *);

static void reset_button_clicked(GtkButton *button, gpointer user_data);

static void ctk_display_device_crt_setup(CtkDisplayDeviceCrt
                                         *ctk_display_device_crt);

static void info_update_received(GtkObject *object, gpointer arg1,
                                 gpointer user_data);

static void crt_info_setup(CtkDisplayDeviceCrt *ctk_display_device_crt);

static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data);

static const char * __refresh_rate_help = "The refresh rate displays the "
"rate at which the screen is currently refreshing the image.";

GType ctk_display_device_crt_get_type(void)
{
    static GType ctk_display_device_crt_type = 0;
    
    if (!ctk_display_device_crt_type) {
        static const GTypeInfo ctk_display_device_crt_info = {
            sizeof (CtkDisplayDeviceCrtClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_display_device_crt_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkDisplayDeviceCrt),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_display_device_crt_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkDisplayDeviceCrt", &ctk_display_device_crt_info, 0);
    }

    return ctk_display_device_crt_type;
}

static void ctk_display_device_crt_class_init(
    CtkDisplayDeviceCrtClass *ctk_display_device_crt_class
)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_display_device_crt_class;
    gobject_class->finalize = ctk_display_device_crt_finalize;
}

static void ctk_display_device_crt_finalize(
    GObject *object
)
{
    CtkDisplayDeviceCrt *ctk_display_device_crt = CTK_DISPLAY_DEVICE_CRT(object);
    g_free(ctk_display_device_crt->name);
    g_signal_handlers_disconnect_matched(ctk_display_device_crt->ctk_event,
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_display_device_crt);
}

/*
 * ctk_display_device_crt_new() - constructor for the CRT display
 * device page.
 */

GtkWidget* ctk_display_device_crt_new(NvCtrlAttributeHandle *handle,
                                      CtkConfig *ctk_config,
                                      CtkEvent *ctk_event,
                                      unsigned int display_device_mask,
                                      char *name)
{
    GObject *object;
    CtkDisplayDeviceCrt *ctk_display_device_crt;
    GtkWidget *banner;
    GtkWidget *hbox;
    GtkWidget *tmpbox;
    GtkWidget *tmphbox, *eventbox;
    GtkWidget *frame;
    GtkWidget *alignment;

    object = g_object_new(CTK_TYPE_DISPLAY_DEVICE_CRT, NULL);
    if (!object) return NULL;

    ctk_display_device_crt = CTK_DISPLAY_DEVICE_CRT(object);
    ctk_display_device_crt->handle = handle;
    ctk_display_device_crt->ctk_config = ctk_config;
    ctk_display_device_crt->ctk_event = ctk_event;
    ctk_display_device_crt->display_device_mask = display_device_mask;
    ctk_display_device_crt->name = g_strdup(name);
    ctk_display_device_crt->txt_refresh_rate = gtk_label_new("");
    gtk_box_set_spacing(GTK_BOX(object), 10);
    
    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_CRT);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    /*
     * create the reset button (which we need while creating the
     * controls in this page so that we can set the button's
     * sensitivity), though we pack it at the bottom of the page
     */

    ctk_display_device_crt->reset_button =
        gtk_button_new_with_label("Reset Hardware Defaults");

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_display_device_crt->reset_button);
    gtk_box_pack_end(GTK_BOX(object), alignment, TRUE, TRUE, 0);
    
    g_signal_connect(G_OBJECT(ctk_display_device_crt->reset_button),
                     "clicked", G_CALLBACK(reset_button_clicked),
                     (gpointer) ctk_display_device_crt);

    ctk_config_set_tooltip(ctk_config, ctk_display_device_crt->reset_button,
                           "The Reset Hardware Defaults button restores "
                           "the CRT settings to their default values.");
    
    /* pack the image sliders */
    
    ctk_display_device_crt->image_sliders =
        ctk_image_sliders_new(handle, ctk_config, ctk_event,
                              ctk_display_device_crt->reset_button,
                              display_device_mask, name);
    if (ctk_display_device_crt->image_sliders) {
        gtk_box_pack_start(GTK_BOX(object),
                           ctk_display_device_crt->image_sliders,
                           FALSE, FALSE, 0);
    }
    /* add the label Refresh Rate and its value */
        
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    tmphbox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(tmphbox), 5);
    gtk_container_add(GTK_CONTAINER(frame), tmphbox);
        
    tmpbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(tmphbox), tmpbox);
    
    /* pack the Refresh Rate Label */
    {
        typedef struct {
            GtkWidget *label;
            GtkWidget *txt;
            const gchar *tooltip;
        } TextLineInfo;

        TextLineInfo line = {
            gtk_label_new("Refresh Rate:"),
            ctk_display_device_crt->txt_refresh_rate,
            __refresh_rate_help,
        };
    
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), line.label,
                               FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), line.txt,
                               FALSE, FALSE, 5);

        /* Include tooltips */
        if (!line.tooltip) {
            gtk_box_pack_start(GTK_BOX(tmpbox), hbox, FALSE, FALSE, 0);
        } else {
            eventbox = gtk_event_box_new();
            gtk_container_add(GTK_CONTAINER(eventbox), hbox);
            ctk_config_set_tooltip(ctk_config, eventbox, line.tooltip);
            gtk_box_pack_start(GTK_BOX(tmpbox), eventbox, FALSE, FALSE, 0);
        }
    }
    /* pack the EDID button */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);
    ctk_display_device_crt->edid_box = hbox;
    
    /* show the page */

    gtk_widget_show_all(GTK_WIDGET(object));

    /* Update the GUI */

    update_display_enabled_flag(ctk_display_device_crt->handle,
                                &ctk_display_device_crt->display_enabled,
                                ctk_display_device_crt->display_device_mask);

    ctk_display_device_crt_setup(ctk_display_device_crt);
    
    /* handle enable/disable events on the display device */

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_ENABLED_DISPLAYS),
                     G_CALLBACK(enabled_displays_received),
                     (gpointer) ctk_display_device_crt);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_REFRESH_RATE),
                     G_CALLBACK(info_update_received),
                     (gpointer) ctk_display_device_crt);
    
    return GTK_WIDGET(object);
}



GtkTextBuffer *ctk_display_device_crt_create_help(GtkTextTagTable *table,
                                                  CtkDisplayDeviceCrt
                                                  *ctk_display_device_crt)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "%s Help", ctk_display_device_crt->name);
    
    ctk_help_heading(b, &i, "Refresh rate");
    ctk_help_para(b, &i, __refresh_rate_help);
        
    add_image_sliders_help
        (CTK_IMAGE_SLIDERS(ctk_display_device_crt->image_sliders), b, &i);

    if (ctk_display_device_crt->edid) {
        add_acquire_edid_help(b, &i);
    }
    
    ctk_help_reset_hardware_defaults(b, &i, ctk_display_device_crt->name);

    ctk_help_finish(b);

    return b;
}



/*
 * reset_button_clicked() -
 */

static void reset_button_clicked(GtkButton *button, gpointer user_data)
{
    CtkDisplayDeviceCrt *ctk_display_device_crt =
        CTK_DISPLAY_DEVICE_CRT(user_data);
    
    ctk_image_sliders_reset
        (CTK_IMAGE_SLIDERS(ctk_display_device_crt->image_sliders));

    gtk_widget_set_sensitive(ctk_display_device_crt->reset_button, FALSE);
    
    ctk_config_statusbar_message(ctk_display_device_crt->ctk_config,
                                 "Reset hardware defaults for %s.",
                                 ctk_display_device_crt->name);

} /* reset_button_clicked() */



/*
 * Updates the display device page to reflect the current
 * configuration of the display device.
 */
static void ctk_display_device_crt_setup(CtkDisplayDeviceCrt
                                         *ctk_display_device_crt)
{
    /* Update CRT-specific settings */
    crt_info_setup(ctk_display_device_crt);
    
    /* Update the image sliders */

    ctk_image_sliders_setup
        (CTK_IMAGE_SLIDERS(ctk_display_device_crt->image_sliders));


    /* update acquire EDID button */
    
    if (ctk_display_device_crt->edid) {
        GList *list;
            
        list = gtk_container_get_children
                (GTK_CONTAINER(ctk_display_device_crt->edid_box));
        if (list) {
            gtk_container_remove
                (GTK_CONTAINER(ctk_display_device_crt->edid_box),
                 (GtkWidget *)(list->data));
            g_list_free(list);
        }
    }

    ctk_display_device_crt->edid =
        ctk_edid_new(ctk_display_device_crt->handle,
                     ctk_display_device_crt->ctk_config,
                     ctk_display_device_crt->ctk_event,
                     ctk_display_device_crt->reset_button,
                     ctk_display_device_crt->display_device_mask,
                     ctk_display_device_crt->name);

    if (ctk_display_device_crt->edid) {
        gtk_box_pack_start(GTK_BOX(ctk_display_device_crt->edid_box),
                           ctk_display_device_crt->edid, TRUE, TRUE, 0);
    }


    /* update the reset button */

    gtk_widget_set_sensitive(ctk_display_device_crt->reset_button, FALSE);

} /* ctk_display_device_crt_setup() */

static void crt_info_setup(CtkDisplayDeviceCrt *ctk_display_device_crt)
{
    ReturnStatus ret;
    gint val;
    
    /* Refresh Rate */
    ret = NvCtrlGetDisplayAttribute(ctk_display_device_crt->handle,
                                    ctk_display_device_crt->display_device_mask,
                                    NV_CTRL_REFRESH_RATE, &val);
    if (ret == NvCtrlSuccess) {
        char str[32];
        float fvalue = ((float)(val)) / 100.0f;
        snprintf(str, 32, "%.2f Hz", fvalue);
        gtk_label_set_text
                    (GTK_LABEL(ctk_display_device_crt->txt_refresh_rate),
                     str);
    } else {
        gtk_label_set_text
                  (GTK_LABEL(ctk_display_device_crt->txt_refresh_rate),
                   "Unknown");
    }
}
/*
 * When the list of enabled displays on the GPU changes,
 * this page should disable/enable access based on whether
 * or not the display device is enabled.
 */
static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data)
{
    CtkDisplayDeviceCrt *ctk_object = CTK_DISPLAY_DEVICE_CRT(user_data);

    /* Requery display information only if display disabled */

    update_display_enabled_flag(ctk_object->handle,
                                &ctk_object->display_enabled,
                                ctk_object->display_device_mask);

    if (ctk_object->display_enabled) {
        return;
    }

    ctk_display_device_crt_setup(ctk_object);

} /* enabled_displays_received() */


/*
 * When resolution changes occur, we should update the GUI to reflect 
 * the current state.
 */

static void info_update_received(GtkObject *object, gpointer arg1,
                                 gpointer user_data)
{
    CtkDisplayDeviceCrt *ctk_object = CTK_DISPLAY_DEVICE_CRT(user_data);
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    
    /* if the event is not for this display device, return */
    if (!(event_struct->display_mask & ctk_object->display_device_mask)) {
        return;
    }
    crt_info_setup(ctk_object);
}
