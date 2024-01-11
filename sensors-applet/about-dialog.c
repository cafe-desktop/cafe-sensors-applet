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

#include <glib.h>
#include <glib/gi18n.h>
#include <ctk/ctk.h>
#include "about-dialog.h"

void about_dialog_open(SensorsApplet *sensors_applet) {
    const gchar *authors[] = {
        "Alex Murray <murray.alex@gmail.com>",
        "Pablo Barciela <scow@riseup.net>",
        "Stefano Karapetsas <stefano@karapetsas.com>",
        NULL
    };

    /* Construct the about dialog */
    ctk_show_about_dialog(NULL,
                  "icon-name", "cafe-sensors-applet",
                  "program-name", PACKAGE_NAME,
                  "version", PACKAGE_VERSION,
                  "copyright", _("Copyright \xc2\xa9 2005-2009 Alex Murray\n"
                                 "Copyright \xc2\xa9 2011 Stefano Karapetsas\n"
                                 "Copyright \xc2\xa9 2012-2020 MATE developers\n"
                                 "Copyright \xc2\xa9 2023-2024 Pablo Barciela"),
                  "authors", authors,
                  "documenters", authors,
                  /* To translator: Put your name here to show up in the About dialog as the translator */
                  "translator-credits", _("translator-credits"),
                  "logo-icon-name", SENSORS_APPLET_ICON,
                  "website", "https://cafe-desktop.org",
                  NULL);

}
