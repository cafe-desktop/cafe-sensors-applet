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

#include <cafe-panel-applet.h>
#include <string.h>
#include "sensors-applet.h"

static gboolean sensors_applet_fill(MatePanelApplet *applet,
                                    const gchar *iid,
                                    gpointer data) {

    SensorsApplet *sensors_applet;
    gboolean retval = FALSE;

    if (strcmp(iid, "SensorsApplet") == 0) {
        sensors_applet = g_new0(SensorsApplet, 1);
        sensors_applet->applet = applet;
        sensors_applet_init(sensors_applet);
        retval = TRUE;
    }
    return retval;
}

CAFE_PANEL_APPLET_OUT_PROCESS_FACTORY ("SensorsAppletFactory",
                                       PANEL_TYPE_APPLET,
                                       "SensorsApplet",
                                       sensors_applet_fill,
                                       NULL);
