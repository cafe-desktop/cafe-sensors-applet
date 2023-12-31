/*
 * Copyright (C) 2005-2009 Alex Murray <murray.alex@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib/gi18n.h>
#include <gio/gio.h>
#include "sensors-applet-settings.h"
#include "prefs-dialog.h"
#include "sensor-config-dialog.h"

#define OLD_TEMP_SCALE 0
#define NEW_TEMP_SCALE 1


/* when a user closes the prefs-dialog we assume that applet is now
   setup, so store all values in gsettings */
void prefs_dialog_close(SensorsApplet *sensors_applet) {

    if (sensors_applet->sensors != NULL) {
        sensors_applet_settings_save_sensors(sensors_applet);
    }

    if (sensors_applet->prefs_dialog) {
        ctk_widget_destroy(CTK_WIDGET(sensors_applet->prefs_dialog->dialog));
        g_free(sensors_applet->prefs_dialog);
        sensors_applet->prefs_dialog = NULL;
    }

    if (sensors_applet->timeout_id == 0) {
        sensors_applet->timeout_id = g_timeout_add_seconds(g_settings_get_int (sensors_applet->settings, TIMEOUT) / 1000, (GSourceFunc)sensors_applet_update_active_sensors, sensors_applet);
    }

}

void prefs_dialog_response(CtkDialog *prefs_dialog,
                           gint response,
                           gpointer data) {

    SensorsApplet *sensors_applet;
    GError *error = NULL;
    gint current_page;
    gchar *uri;

    sensors_applet = (SensorsApplet *)data;

    switch (response) {
        case CTK_RESPONSE_HELP:
            g_debug("loading help in prefs");
            current_page = ctk_notebook_get_current_page(sensors_applet->prefs_dialog->notebook);
            uri = g_strdup_printf("help:cafe-sensors-applet/%s",
                          ((current_page == 0) ?
                           "sensors-applet-general-options" :
                           ((current_page == 1) ?
                        "sensors-applet-sensors" :
                        NULL)));
            ctk_show_uri_on_window(NULL, uri, ctk_get_current_event_time(), &error);
            g_free(uri);

            if (error) {
                g_debug("Could not open help document: %s ",error->message);
                g_error_free (error);
            }
            break;
        default:
            g_debug("closing prefs dialog");
            prefs_dialog_close(sensors_applet);
    }
}


static gboolean prefs_dialog_convert_low_and_high_values(CtkTreeModel *model,
                                                         CtkTreePath *path,
                                                         CtkTreeIter *iter,
                                                         TemperatureScale scales[2]) {

    SensorType sensor_type;
    gdouble low_value, high_value;

    ctk_tree_model_get(model,
                       iter,
                       SENSOR_TYPE_COLUMN, &sensor_type,
                       LOW_VALUE_COLUMN, &low_value,
                       HIGH_VALUE_COLUMN, &high_value,
                       -1);

    if (sensor_type == TEMP_SENSOR) {
        low_value = sensors_applet_convert_temperature(low_value,
                                                       scales[OLD_TEMP_SCALE],
                                                       scales[NEW_TEMP_SCALE]);

        high_value = sensors_applet_convert_temperature(high_value,
                                                        scales[OLD_TEMP_SCALE],
                                                        scales[NEW_TEMP_SCALE]);


        ctk_tree_store_set(CTK_TREE_STORE(model),
                           iter,
                           LOW_VALUE_COLUMN, low_value,
                           HIGH_VALUE_COLUMN, high_value,
                           -1);
    }
    return FALSE;
}


static void prefs_dialog_timeout_changed(CtkSpinButton *button,
                                         PrefsDialog *prefs_dialog) {

    gint value;
    value = (gint)(ctk_spin_button_get_value(button) * 1000);
    g_settings_set_int (prefs_dialog->sensors_applet->settings, TIMEOUT, value);
}

static void prefs_dialog_display_mode_changed(CtkComboBox *display_mode_combo_box,
                                              PrefsDialog *prefs_dialog) {


    int display_mode;

    display_mode = ctk_combo_box_get_active(display_mode_combo_box);

    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->layout_mode_label),
                             (display_mode != DISPLAY_ICON) &&
                             (display_mode != DISPLAY_VALUE) &&
                             (display_mode != DISPLAY_GRAPH));

    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->layout_mode_combo_box),
                             (display_mode != DISPLAY_ICON) &&
                             (display_mode != DISPLAY_VALUE) &&
                             (display_mode != DISPLAY_GRAPH));

    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->graph_size_label),
                             (display_mode == DISPLAY_GRAPH));
    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->graph_size_spinbutton),
                             (display_mode == DISPLAY_GRAPH));

    g_settings_set_int (prefs_dialog->sensors_applet->settings,
                        DISPLAY_MODE,
                        ctk_combo_box_get_active(display_mode_combo_box));

    sensors_applet_display_layout_changed(prefs_dialog->sensors_applet);
}

static void prefs_dialog_layout_mode_changed(CtkComboBox *layout_mode_combo_box,
                                             PrefsDialog *prefs_dialog) {

    g_settings_set_int (prefs_dialog->sensors_applet->settings,
                        LAYOUT_MODE,
                        ctk_combo_box_get_active(layout_mode_combo_box));

    sensors_applet_display_layout_changed(prefs_dialog->sensors_applet);
}

static void prefs_dialog_temperature_scale_changed(CtkComboBox *temperature_scale_combo_box,
                                                   PrefsDialog *prefs_dialog) {

    /* get old temp scale value */
    TemperatureScale scales[2];
    CtkTreeModel *model;

    scales[OLD_TEMP_SCALE] = (TemperatureScale) g_settings_get_int (prefs_dialog->sensors_applet->settings, TEMPERATURE_SCALE);

    scales[NEW_TEMP_SCALE] = (TemperatureScale)ctk_combo_box_get_active(temperature_scale_combo_box);

    g_settings_set_int (prefs_dialog->sensors_applet->settings,
                        TEMPERATURE_SCALE,
                        scales[NEW_TEMP_SCALE]);

    /* now go thru and convert all low and high sensor values in
     * the tree to either celcius or Fahrenheit */
    model = ctk_tree_view_get_model(prefs_dialog->view);
    ctk_tree_model_foreach(model,
                           (CtkTreeModelForeachFunc)prefs_dialog_convert_low_and_high_values,
                           scales);

    /* finally update display of active sensors */
    sensors_applet_update_active_sensors(prefs_dialog->sensors_applet);
}

// hide/show units
static void prefs_dialog_show_units_toggled (CtkCheckButton *show_units, PrefsDialog *prefs_dialog) {
    gboolean state;

    state = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (show_units));
    g_settings_set_boolean (prefs_dialog->sensors_applet->settings, HIDE_UNITS, !state);
    sensors_applet_update_active_sensors (prefs_dialog->sensors_applet);
}


#ifdef HAVE_LIBNOTIFY
static void prefs_dialog_display_notifications_toggled(CtkCheckButton *display_notifications,
                                                       PrefsDialog *prefs_dialog) {

    gboolean notify;

    notify = ctk_toggle_button_get_active(CTK_TOGGLE_BUTTON(display_notifications));
    g_settings_set_boolean(prefs_dialog->sensors_applet->settings,
                           DISPLAY_NOTIFICATIONS,
                           notify);

    if (notify) {
        sensors_applet_update_active_sensors(prefs_dialog->sensors_applet);
    } else {
        sensors_applet_notify_end_all(prefs_dialog->sensors_applet);
    }
}
#endif


static void prefs_dialog_graph_size_changed(CtkSpinButton *button,
                                            PrefsDialog *prefs_dialog) {

    gint value;
    value = (gint)(ctk_spin_button_get_value(button));
    g_settings_set_int(prefs_dialog->sensors_applet->settings, GRAPH_SIZE, value);

    /* notify change of number of samples */
    sensors_applet_graph_size_changed(prefs_dialog->sensors_applet);

}

/* callbacks for the tree of sensors */
static void prefs_dialog_sensor_toggled(CtkCellRenderer *renderer, gchar *path_str, PrefsDialog *prefs_dialog) {
    CtkTreeIter iter;
    CtkTreePath *path;

    gboolean old_value;

    path = ctk_tree_path_new_from_string(path_str);

    ctk_tree_model_get_iter(CTK_TREE_MODEL(prefs_dialog->sensors_applet->sensors), &iter, path);
    ctk_tree_model_get(CTK_TREE_MODEL(prefs_dialog->sensors_applet->sensors),
                       &iter,
                       ENABLE_COLUMN, &old_value,
                       -1);

    if (old_value) {
        sensors_applet_sensor_disabled(prefs_dialog->sensors_applet, path);
    } else {
        sensors_applet_sensor_enabled(prefs_dialog->sensors_applet, path);
    }

    ctk_tree_store_set(prefs_dialog->sensors_applet->sensors, &iter,
                       ENABLE_COLUMN, !old_value,
                       -1);

    ctk_tree_path_free(path);
}

static void prefs_dialog_sensor_name_changed(CtkCellRenderer *renderer, gchar *path_str, gchar *new_text, PrefsDialog *prefs_dialog) {
    CtkTreeIter iter;
    CtkTreePath *path = ctk_tree_path_new_from_string(path_str);

    ctk_tree_model_get_iter(CTK_TREE_MODEL(prefs_dialog->sensors_applet->sensors), &iter, path);

    ctk_tree_store_set(prefs_dialog->sensors_applet->sensors, &iter, LABEL_COLUMN, new_text, -1);

    sensors_applet_update_sensor(prefs_dialog->sensors_applet, path);
    ctk_tree_path_free(path);
}

static void prefs_dialog_row_activated(CtkTreeView *view, CtkTreePath *path, CtkTreeViewColumn *column, PrefsDialog *prefs_dialog) {
    /* only bring up dialog this if is a sensor - ie has no
     * children */
    CtkTreeIter iter;
    CtkTreeModel *model;

    model = ctk_tree_view_get_model(view);
    /* make sure can set iter first */
    if (ctk_tree_model_get_iter(model, &iter, path) && !ctk_tree_model_iter_has_child(model, &iter)) {
        sensor_config_dialog_create(prefs_dialog->sensors_applet);
    }
}

static void prefs_dialog_sensor_up_button_clicked(CtkButton *button, PrefsDialog *prefs_dialog) {
    CtkTreeModel *model;
    CtkTreeIter iter;
    CtkTreePath *path;

    if (ctk_tree_selection_get_selected(prefs_dialog->sensors_applet->selection, &model, &iter)) {
        /* if has no prev node set up button insentive */
        path = ctk_tree_model_get_path(model, &iter);
        if (ctk_tree_path_prev(path)) {

            CtkTreeIter prev_iter;
            /* check is a valid node in out model */
            if (ctk_tree_model_get_iter(model, &prev_iter, path)) {

                ctk_tree_store_move_before(CTK_TREE_STORE(model),
                                           &iter,
                                           &prev_iter);
                g_signal_emit_by_name(prefs_dialog->sensors_applet->selection, "changed");
                sensors_applet_reorder_sensors(prefs_dialog->sensors_applet);

            }
        }

        ctk_tree_path_free(path);

    }
}

static void prefs_dialog_sensor_down_button_clicked(CtkButton *button, PrefsDialog *prefs_dialog) {
    CtkTreeModel *model;
    CtkTreeIter iter;
    CtkTreeIter iter_next;

    if (ctk_tree_selection_get_selected(prefs_dialog->sensors_applet->selection, &model, &iter)) {
        iter_next = iter;
        /* if has no next node set down button insentive */
        if (ctk_tree_model_iter_next(model, &iter_next)) {

                ctk_tree_store_move_after(CTK_TREE_STORE(model),
                                          &iter,
                                          &iter_next);
                g_signal_emit_by_name(prefs_dialog->sensors_applet->selection, "changed");
                sensors_applet_reorder_sensors(prefs_dialog->sensors_applet);

        }
    }
}

static void prefs_dialog_sensor_config_button_clicked(CtkButton *button, PrefsDialog *prefs_dialog) {
    sensor_config_dialog_create(prefs_dialog->sensors_applet);
}

/* if a sensor is selected, make config sure button is able to be
 * clicked and also set the sensitivities properly for the up and down
 * buttons */
static void prefs_dialog_selection_changed(CtkTreeSelection *selection,
                                           PrefsDialog *prefs_dialog) {

    CtkTreeIter iter;
    CtkTreePath *path;
    CtkTreeModel *model;
    /* if there is a selection with no children make config button sensitive */
    if (ctk_tree_selection_get_selected(selection, &model, &iter)) {
        if (!ctk_tree_model_iter_has_child(model, &iter)) {
            ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_config_button), TRUE);
        } else {
            /* otherwise make insensitive */
            ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_config_button), FALSE);
        }

        /* if has no prev node set up button insentive */
        path = ctk_tree_model_get_path(model, &iter);
        if (ctk_tree_path_prev(path)) {
            CtkTreeIter prev_iter;
            /* check is a valid node in out model */
            if (ctk_tree_model_get_iter(model, &prev_iter, path)) {
                ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_up_button), TRUE);
            } else {
                ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_up_button), FALSE);
            }
        }  else {
            ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_up_button), FALSE);
        }

        ctk_tree_path_free(path);

        /* if has no next node set down button insentive */
        if (ctk_tree_model_iter_next(model, &iter)) {
            ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_down_button), TRUE);
        } else {
            ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_down_button), FALSE);
        }

    } else {
        /* otherwise make all insensitive */
        ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_config_button), FALSE);
        ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_up_button), FALSE);
        ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_down_button), FALSE);

    }
}

void prefs_dialog_open(SensorsApplet *sensors_applet) {
    gchar *header_text;
    PrefsDialog *prefs_dialog;
    DisplayMode display_mode;
    CtkWidget *content_area;

    g_assert(sensors_applet->prefs_dialog == NULL);

    /* while prefs dialog is open, stop the updating of sensors so
     * we don't get any race conditions due to concurrent updates
     * of the labels, values and icons linked lists etc. */
    if (sensors_applet->timeout_id != 0) {
        if (g_source_remove(sensors_applet->timeout_id)) {
            sensors_applet->timeout_id = 0;
        }
    }

    sensors_applet->prefs_dialog = g_new0(PrefsDialog, 1);
    prefs_dialog = sensors_applet->prefs_dialog;

    prefs_dialog->sensors_applet = sensors_applet;

    prefs_dialog->dialog = CTK_DIALOG(ctk_dialog_new_with_buttons(_("Sensors Applet Preferences"),
                                                                  NULL,
                                                                  0,
                                                                  "ctk-help",
                                                                  CTK_RESPONSE_HELP,
                                                                  "ctk-close",
                                                                  CTK_RESPONSE_CLOSE,
                                                                  NULL));


    ctk_window_set_icon_name(CTK_WINDOW(prefs_dialog->dialog), "cafe-sensors-applet");

    g_object_set(prefs_dialog->dialog,
                 "border-width", 12,
                 "default-width", 480,
                 "default-height", 350,
                 NULL);

    content_area = ctk_dialog_get_content_area (prefs_dialog->dialog);
    ctk_box_set_homogeneous(CTK_BOX(content_area), FALSE);

    ctk_box_set_spacing(CTK_BOX(content_area), 5);

    g_signal_connect(prefs_dialog->dialog,
                     "response", G_CALLBACK(prefs_dialog_response),
                     sensors_applet);

    g_signal_connect_swapped(prefs_dialog->dialog,
                             "delete-event", G_CALLBACK(prefs_dialog_close),
                             sensors_applet);

    g_signal_connect_swapped(prefs_dialog->dialog,
                             "destroy", G_CALLBACK(prefs_dialog_close),
                             sensors_applet);

    /* if no SensorsList's have been created, this is because
       we haven't been able to access any sensors */
    if (sensors_applet->sensors == NULL) {
        CtkWidget *label;
        CtkWidget *content_area;
        label = ctk_label_new(_("No sensors found!"));
        content_area = ctk_dialog_get_content_area (prefs_dialog->dialog);
        ctk_box_pack_start (CTK_BOX(content_area), label, FALSE, FALSE, 0);
        return;
    }

    header_text = g_markup_printf_escaped("<b>%s</b>", _("Display"));
    prefs_dialog->display_header = g_object_new(CTK_TYPE_LABEL,
                                                "use-markup", TRUE,
                                                "label", header_text,
                                                "xalign", 0.0,
                                                NULL);
    g_free(header_text);

    prefs_dialog->display_mode_combo_box = CTK_COMBO_BOX_TEXT(ctk_combo_box_text_new());
    /*expand the whole column */
    ctk_widget_set_hexpand(CTK_WIDGET(prefs_dialog->display_mode_combo_box), TRUE);

    ctk_combo_box_text_append_text(CTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("label with value"));
    ctk_combo_box_text_append_text(CTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("icon with value"));
    ctk_combo_box_text_append_text(CTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("value only"));
    ctk_combo_box_text_append_text(CTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("icon only"));
    ctk_combo_box_text_append_text(CTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("graph only"));

    display_mode = g_settings_get_int(sensors_applet->settings, DISPLAY_MODE);
    ctk_combo_box_set_active(CTK_COMBO_BOX(prefs_dialog->display_mode_combo_box), display_mode);

    g_signal_connect(prefs_dialog->display_mode_combo_box,
                     "changed",
                     G_CALLBACK(prefs_dialog_display_mode_changed),
                     prefs_dialog);

    /* use spaces in label to indent */
    prefs_dialog->display_mode_label = g_object_new(CTK_TYPE_LABEL,
                                                    "use-underline", TRUE,
                                                    "label", _("_Display sensors in panel as"),
                                                    "mnemonic-widget", prefs_dialog->display_mode_combo_box,
                                                    "xalign", 0.0,
                                                    NULL);

    prefs_dialog->layout_mode_combo_box = CTK_COMBO_BOX_TEXT(ctk_combo_box_text_new());

    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->layout_mode_combo_box),
                             (display_mode != DISPLAY_ICON) &&
                             (display_mode != DISPLAY_VALUE) &&
                             (display_mode != DISPLAY_GRAPH));

    ctk_combo_box_text_append_text(prefs_dialog->layout_mode_combo_box, _("beside labels / icons"));
    ctk_combo_box_text_append_text(prefs_dialog->layout_mode_combo_box, _("below labels / icons"));

    ctk_combo_box_set_active(
            CTK_COMBO_BOX(prefs_dialog->layout_mode_combo_box),
            g_settings_get_int(sensors_applet->settings, LAYOUT_MODE));

    g_signal_connect(prefs_dialog->layout_mode_combo_box,
                     "changed",
                     G_CALLBACK(prefs_dialog_layout_mode_changed),
                     prefs_dialog);

    prefs_dialog->layout_mode_label = g_object_new(CTK_TYPE_LABEL,
                                                   "use-underline", TRUE,
                                                   "label", _("Preferred _position of sensor values"),
                                                   "mnemonic-widget", prefs_dialog->layout_mode_combo_box,
                                                   "xalign", 0.0,
                                                   NULL);

    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->layout_mode_label),
                             (display_mode != DISPLAY_ICON) &&
                             (display_mode != DISPLAY_VALUE) &&
                             (display_mode != DISPLAY_GRAPH));

    prefs_dialog->temperature_scale_combo_box = CTK_COMBO_BOX_TEXT(ctk_combo_box_text_new());

    ctk_combo_box_text_append_text(prefs_dialog->temperature_scale_combo_box, _("Kelvin"));
    ctk_combo_box_text_append_text(prefs_dialog->temperature_scale_combo_box, _("Celsius"));
    ctk_combo_box_text_append_text(prefs_dialog->temperature_scale_combo_box, _("Fahrenheit"));

    ctk_combo_box_set_active(
            CTK_COMBO_BOX(prefs_dialog->temperature_scale_combo_box),
            g_settings_get_int(sensors_applet->settings, TEMPERATURE_SCALE));

    g_signal_connect(prefs_dialog->temperature_scale_combo_box,
                     "changed",
                     G_CALLBACK(prefs_dialog_temperature_scale_changed),
                     prefs_dialog);

    prefs_dialog->temperature_scale_label = g_object_new(CTK_TYPE_LABEL,
                                                         "use-underline", TRUE,
                                                         "label", _("_Temperature scale"),
                                                         "mnemonic-widget", prefs_dialog->temperature_scale_combo_box,
                                                         "xalign", 0.0,
                                                         NULL);

    prefs_dialog->graph_size_adjust = g_object_new(CTK_TYPE_ADJUSTMENT,
                                                   "value", (gdouble)g_settings_get_int(sensors_applet->settings, GRAPH_SIZE),
                                                   "lower", 1.0,
                                                   "upper", 100.0,
                                                   "step-increment", 1.0,
                                                   "page-increment", 10.0,
                                                   "page-size", 0.0,
                                                   NULL);

    prefs_dialog->graph_size_spinbutton = g_object_new(CTK_TYPE_SPIN_BUTTON,
                                                       "adjustment", prefs_dialog->graph_size_adjust,
                                                       "climb-rate", 1.0,
                                                       "digits", 0,
                                                       "value", (gdouble)g_settings_get_int(sensors_applet->settings, GRAPH_SIZE),
                                                       "width-chars", 4,
                                                       NULL);

    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->graph_size_spinbutton), (display_mode == DISPLAY_GRAPH));

    prefs_dialog->graph_size_label = g_object_new(CTK_TYPE_LABEL,
                                                  "use-underline", TRUE,
                                                  "label", _("Graph _size (pixels)"),
                                                  "mnemonic-widget", prefs_dialog->graph_size_spinbutton,
                                                  "xalign", 0.0,
                                                  NULL);

    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->graph_size_label), (display_mode == DISPLAY_GRAPH));

    g_signal_connect(prefs_dialog->graph_size_spinbutton, "value-changed",
                     G_CALLBACK(prefs_dialog_graph_size_changed),
                     prefs_dialog);

    // hide/show units
    prefs_dialog->show_units = ctk_check_button_new_with_label (_("Show _units"));
    ctk_button_set_use_underline (CTK_BUTTON (prefs_dialog->show_units), TRUE);
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (prefs_dialog->show_units),
                        !g_settings_get_boolean (sensors_applet->settings, HIDE_UNITS));
    g_signal_connect (prefs_dialog->show_units, "toggled",
                        G_CALLBACK (prefs_dialog_show_units_toggled),
                        prefs_dialog);

    header_text = g_markup_printf_escaped("<b>%s</b>", _("Update"));
    prefs_dialog->update_header = g_object_new(CTK_TYPE_LABEL,
                                               "use-markup", TRUE,
                                               "label", header_text,
                                               "xalign", 0.0,
                                               NULL);
    g_free(header_text);

    prefs_dialog->timeout_adjust = g_object_new(CTK_TYPE_ADJUSTMENT,
                                                "value", 2.0,
                                                "lower", 1.5,
                                                "upper", 10.0,
                                                "step-increment", 0.5,
                                                "page-increment", 1.0,
                                                "page-size", 0.0,
                                                NULL);

    prefs_dialog->timeout_spinbutton = g_object_new(CTK_TYPE_SPIN_BUTTON,
                                                    "adjustment", prefs_dialog->timeout_adjust,
                                                    "climb-rate", 0.5,
                                                    "digits", 1,
                                                    "value", (gdouble) g_settings_get_int(sensors_applet->settings, TIMEOUT) / 1000.0,
                                                    "width-chars", 4,
                                                    NULL);

    prefs_dialog->timeout_label = g_object_new(CTK_TYPE_LABEL,
                                               "use-underline", TRUE,
                                               "label", _("Update _interval (secs)"),
                                               "mnemonic-widget", prefs_dialog->timeout_spinbutton,
                                               "xalign", 0.0,
                                               NULL);

    g_signal_connect(prefs_dialog->timeout_spinbutton, "value-changed",
                     G_CALLBACK(prefs_dialog_timeout_changed),
                     prefs_dialog);

#ifdef HAVE_LIBNOTIFY
    header_text = g_markup_printf_escaped("<b>%s</b>", _("Notifications"));
    prefs_dialog->notifications_header = g_object_new(CTK_TYPE_LABEL,
                                                      "use-markup", TRUE,
                                                      "label", header_text,
                                                      "xalign", 0.0,
                                                      NULL);
    g_free(header_text);

    prefs_dialog->display_notifications = g_object_new(CTK_TYPE_CHECK_BUTTON,
                                                       "use-underline", TRUE,
                                                       "label", _("Display _notifications"),
                                                       NULL);
    ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(prefs_dialog->display_notifications),
                                 g_settings_get_boolean(sensors_applet->settings,
                                                        DISPLAY_NOTIFICATIONS));
    g_signal_connect(prefs_dialog->display_notifications,
                     "toggled",
                     G_CALLBACK(prefs_dialog_display_notifications_toggled),
                     prefs_dialog);
#endif

    /* SIZE AND LAYOUT */
    /* keep all widgets same size */
    prefs_dialog->size_group = ctk_size_group_new(CTK_SIZE_GROUP_HORIZONTAL);

    ctk_size_group_add_widget(prefs_dialog->size_group,
                              CTK_WIDGET(prefs_dialog->display_mode_combo_box));

    ctk_size_group_add_widget(prefs_dialog->size_group,
                              CTK_WIDGET(prefs_dialog->layout_mode_combo_box));

    ctk_size_group_add_widget(prefs_dialog->size_group,
                              CTK_WIDGET(prefs_dialog->temperature_scale_combo_box));

    ctk_size_group_add_widget(prefs_dialog->size_group,
                              CTK_WIDGET(prefs_dialog->timeout_spinbutton));

    g_object_unref(prefs_dialog->size_group);

    prefs_dialog->globals_grid = g_object_new(CTK_TYPE_GRID,
                                               "row-homogeneous", FALSE,
                                               "column-homogeneous", FALSE,
                                               "row-spacing", 6,
                                               "column-spacing", 12,
                                               NULL);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->display_header),
                     0, 0, 2, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->display_mode_label),
                     1, 1, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->display_mode_combo_box),
                     2, 1, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->layout_mode_label),
                     1, 2, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->layout_mode_combo_box),
                     2, 2, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->graph_size_label),
                     1, 3, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->graph_size_spinbutton),
                     2, 3, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->temperature_scale_label),
                     1, 4, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->temperature_scale_combo_box),
                     2, 4, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->show_units),
                     1, 5, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->update_header),
                     0, 6, 2, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->timeout_label),
                     1, 7, 1, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->timeout_spinbutton),
                     2, 7, 1, 1);

#ifdef HAVE_LIBNOTIFY
    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->notifications_header),
                     0, 8, 2, 1);

    ctk_grid_attach(prefs_dialog->globals_grid,
                     CTK_WIDGET(prefs_dialog->display_notifications),
                     1, 9, 1, 1);
#endif

    ctk_widget_set_valign(CTK_WIDGET(prefs_dialog->globals_grid), CTK_ALIGN_START);
    ctk_widget_set_margin_start(CTK_WIDGET(prefs_dialog->globals_grid), 12);
    ctk_widget_set_margin_end(CTK_WIDGET(prefs_dialog->globals_grid), 12);
    ctk_widget_set_margin_top(CTK_WIDGET(prefs_dialog->globals_grid), 12);
    ctk_widget_set_margin_bottom(CTK_WIDGET(prefs_dialog->globals_grid), 12);

    prefs_dialog->view = g_object_new(CTK_TYPE_TREE_VIEW,
                                      "model", CTK_TREE_MODEL(sensors_applet->sensors),
                                      "rules-hint", TRUE,
                                      "reorderable", FALSE,
                                      "enable-search", TRUE,
                                      "search-column", LABEL_COLUMN,
                                      NULL);

    /* get double clicks on rows - do same as configure sensor
     * button clicks */
    g_signal_connect(prefs_dialog->view, "row-activated",
                     G_CALLBACK(prefs_dialog_row_activated),
                     prefs_dialog);

    prefs_dialog->id_renderer = ctk_cell_renderer_text_new();
    prefs_dialog->label_renderer = ctk_cell_renderer_text_new();
    g_object_set(prefs_dialog->label_renderer,
                 "editable", TRUE,
                 NULL);

    g_signal_connect(prefs_dialog->label_renderer, "edited",
                     G_CALLBACK(prefs_dialog_sensor_name_changed),
                     prefs_dialog);

    prefs_dialog->enable_renderer = ctk_cell_renderer_toggle_new();
    g_signal_connect(prefs_dialog->enable_renderer, "toggled",
                     G_CALLBACK(prefs_dialog_sensor_toggled),
                     prefs_dialog);
    prefs_dialog->icon_renderer = ctk_cell_renderer_pixbuf_new();

    prefs_dialog->id_column = ctk_tree_view_column_new_with_attributes(_("Sensor"),
                                                                       prefs_dialog->id_renderer,
                                                                       "text", ID_COLUMN,
                                                                       NULL);

    ctk_tree_view_column_set_min_width(prefs_dialog->id_column, 90);

    prefs_dialog->label_column = ctk_tree_view_column_new_with_attributes(_("Label"),
                                                                          prefs_dialog->label_renderer,
                                                                          "text", LABEL_COLUMN,
                                                                          "visible", VISIBLE_COLUMN,
                                                                          NULL);

    ctk_tree_view_column_set_min_width(prefs_dialog->label_column, 100);

    /* create the tooltip */
    ctk_widget_set_tooltip_text(CTK_WIDGET(prefs_dialog->view),
                                _("Labels can be edited directly by clicking on them."));
    prefs_dialog->enable_column = ctk_tree_view_column_new_with_attributes(_("Enabled"),
                                                                           prefs_dialog->enable_renderer,
                                                                           "active", ENABLE_COLUMN,
                                                                           "visible", VISIBLE_COLUMN,
                                                                           NULL);


    ctk_tree_view_column_set_min_width(prefs_dialog->enable_column, 90);

    prefs_dialog->icon_column = ctk_tree_view_column_new_with_attributes(_("Icon"),
                                                                         prefs_dialog->icon_renderer,
                                                                         "pixbuf", ICON_PIXBUF_COLUMN,
                                                                         "visible", VISIBLE_COLUMN,
                                                                         NULL);

    ctk_tree_view_append_column(prefs_dialog->view, prefs_dialog->id_column);
    ctk_tree_view_append_column(prefs_dialog->view, prefs_dialog->icon_column);
    ctk_tree_view_append_column(prefs_dialog->view, prefs_dialog->label_column);
    ctk_tree_view_append_column(prefs_dialog->view, prefs_dialog->enable_column);

    ctk_tree_view_columns_autosize(prefs_dialog->view);

    prefs_dialog->scrolled_window = g_object_new(CTK_TYPE_SCROLLED_WINDOW,
                                                 "hadjustment", NULL,
                                                 "height-request", 200,
                                                 "hscrollbar-policy", CTK_POLICY_NEVER,
                                                 "vadjustment",NULL,
                                                 "vscrollbar-policy", CTK_POLICY_AUTOMATIC,
                                                 NULL);

    ctk_scrolled_window_set_shadow_type(CTK_SCROLLED_WINDOW(prefs_dialog->scrolled_window), CTK_SHADOW_IN);

    ctk_container_add(CTK_CONTAINER(prefs_dialog->scrolled_window), CTK_WIDGET(prefs_dialog->view));

    /* CtkTree Selection */
    sensors_applet->selection = ctk_tree_view_get_selection(prefs_dialog->view);
    /* allow user to only select one row at a time at most */
    ctk_tree_selection_set_mode(sensors_applet->selection, CTK_SELECTION_SINGLE);
    /* when selection is changed, make sure sensor_config button is activated */

    /* Create buttons for user to interact with sensors tree */
    prefs_dialog->sensor_up_button = CTK_BUTTON(ctk_button_new_with_mnemonic(_("_Up")));
    ctk_button_set_image(prefs_dialog->sensor_up_button, ctk_image_new_from_icon_name("go-up", CTK_ICON_SIZE_BUTTON));
    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_up_button), FALSE);

    g_signal_connect(prefs_dialog->sensor_up_button, "clicked",
                     G_CALLBACK(prefs_dialog_sensor_up_button_clicked),
                     prefs_dialog);

    prefs_dialog->sensor_down_button = CTK_BUTTON(ctk_button_new_with_mnemonic(_("_Down")));
    ctk_button_set_image(prefs_dialog->sensor_down_button, ctk_image_new_from_icon_name("go-down", CTK_ICON_SIZE_BUTTON));
    ctk_widget_set_sensitive(CTK_WIDGET(prefs_dialog->sensor_down_button), FALSE);

    g_signal_connect(prefs_dialog->sensor_down_button, "clicked",
                     G_CALLBACK(prefs_dialog_sensor_down_button_clicked),
                     prefs_dialog);

    prefs_dialog->buttons_box = CTK_BUTTON_BOX(ctk_button_box_new(CTK_ORIENTATION_VERTICAL));

    ctk_button_box_set_layout(CTK_BUTTON_BOX(prefs_dialog->buttons_box), CTK_BUTTONBOX_SPREAD);

    ctk_box_pack_start(CTK_BOX(prefs_dialog->buttons_box), CTK_WIDGET(prefs_dialog->sensor_up_button), FALSE, FALSE, 0);

    ctk_box_pack_start(CTK_BOX(prefs_dialog->buttons_box), CTK_WIDGET(prefs_dialog->sensor_down_button), FALSE, FALSE, 0);

    prefs_dialog->sensors_hbox = g_object_new(CTK_TYPE_BOX,
                                              "orientation", CTK_ORIENTATION_HORIZONTAL,
                                              "border-width", 5,
                                              "homogeneous", FALSE,
                                              "spacing", 5,
                                              NULL);

    ctk_box_pack_start(prefs_dialog->sensors_hbox,
                       CTK_WIDGET(prefs_dialog->scrolled_window),
                       TRUE, TRUE, 0); /* make sure window takes up most of room */

    ctk_box_pack_start(prefs_dialog->sensors_hbox,
                       CTK_WIDGET(prefs_dialog->buttons_box),
                       FALSE, FALSE, 0);

    /* Sensor Config button */
    /* initially make button insensitive until user selects a row
       from the sensors tree */
    prefs_dialog->sensor_config_button = CTK_BUTTON(ctk_button_new_with_mnemonic(_("_Properties")));
    ctk_button_set_image(prefs_dialog->sensor_config_button, ctk_image_new_from_icon_name("document-properties", CTK_ICON_SIZE_BUTTON));
    g_object_set(prefs_dialog->sensor_config_button,
                 "sensitive", FALSE,
                 NULL);

    g_signal_connect(sensors_applet->selection,
                     "changed",
                     G_CALLBACK(prefs_dialog_selection_changed),
                     prefs_dialog);

    /* pass selection to signal handler so we can give user a
       sensors_applet->prefs_dialog with the selected rows alarm
       value and enable */
    g_signal_connect(prefs_dialog->sensor_config_button, "clicked",
                     G_CALLBACK(prefs_dialog_sensor_config_button_clicked),
                     prefs_dialog);

    prefs_dialog->sensor_config_hbox = g_object_new(CTK_TYPE_BOX,
                                                    "orientation", CTK_ORIENTATION_HORIZONTAL,
                                                    "border-width", 5,
                                                    "homogeneous", FALSE,
                                                    "spacing", 0,
                                                    NULL);
    ctk_box_pack_end(prefs_dialog->sensor_config_hbox,
                     CTK_WIDGET(prefs_dialog->sensor_config_button),
                     FALSE, FALSE, 0);

    /* pack sensors_vbox */
    prefs_dialog->sensors_vbox = g_object_new(CTK_TYPE_BOX,
                                              "orientation", CTK_ORIENTATION_VERTICAL,
                                              "border-width", 5,
                                              "homogeneous", FALSE,
                                              "spacing", 0,
                                              NULL);

    ctk_box_pack_start(prefs_dialog->sensors_vbox,
                       CTK_WIDGET(prefs_dialog->sensors_hbox),
                       TRUE, TRUE, 0);
    ctk_box_pack_start(prefs_dialog->sensors_vbox,
                       CTK_WIDGET(prefs_dialog->sensor_config_hbox),
                       FALSE, FALSE, 0);

    prefs_dialog->notebook = g_object_new(CTK_TYPE_NOTEBOOK, NULL);

    ctk_notebook_append_page(prefs_dialog->notebook,
                             CTK_WIDGET(prefs_dialog->globals_grid),
                             ctk_label_new(_("General Options")));

    ctk_notebook_append_page(prefs_dialog->notebook,
                             CTK_WIDGET(prefs_dialog->sensors_vbox),
                             ctk_label_new(_("Sensors")));

    /* pack notebook into prefs_dialog */
    content_area = ctk_dialog_get_content_area (prefs_dialog->dialog);
    ctk_box_pack_start (CTK_BOX(content_area), CTK_WIDGET(prefs_dialog->notebook), TRUE, TRUE, 0);


    ctk_widget_show_all(CTK_WIDGET(prefs_dialog->dialog));
}
