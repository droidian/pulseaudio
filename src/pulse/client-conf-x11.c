/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-13071
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <xcb/xcb.h>

#include <pulse/xmalloc.h>

#include <pulsecore/i18n.h>
#include <pulsecore/x11prop.h>
#include <pulsecore/log.h>
#include <pulsecore/core-util.h>
#include <pulsecore/macro.h>

#include "client-conf-x11.h"

int pa_client_conf_from_x11(pa_client_conf *c, const char *dname) {
    xcb_connection_t *xcb = NULL;
    int ret = -1, screen = 0;
    char t[1024];

    pa_assert(c);

    if (!dname && !(dname = getenv("DISPLAY")))
        goto finish;

    if (*dname == 0)
        goto finish;

    if (!(xcb = xcb_connect(dname, NULL))) {
        pa_log(_("xcb_connect() failed"));
        goto finish;
    }

    if (xcb_connection_has_error(xcb)) {
        pa_log(_("xcb_connection_has_error() returned true"));
        goto finish;
    }

    if (pa_x11_get_prop(xcb, screen, "PULSE_SERVER", t, sizeof(t))) {
        bool disable_autospawn = true;

        pa_xfree(c->default_server);
        c->default_server = pa_xstrdup(t);

        if (pa_x11_get_prop(xcb, screen, "PULSE_SESSION_ID", t, sizeof(t))) {
            char *id;

            if ((id = pa_session_id())) {
                if (pa_streq(t, id))
                    disable_autospawn = false;
                pa_xfree(id);
            }
        }

        if (disable_autospawn)
            c->autospawn = false;
    }

    if (pa_x11_get_prop(xcb, screen, "PULSE_SINK", t, sizeof(t))) {
        pa_xfree(c->default_sink);
        c->default_sink = pa_xstrdup(t);
    }

    if (pa_x11_get_prop(xcb, screen, "PULSE_SOURCE", t, sizeof(t))) {
        pa_xfree(c->default_source);
        c->default_source = pa_xstrdup(t);
    }

    if (pa_x11_get_prop(xcb, screen, "PULSE_COOKIE", t, sizeof(t))) {
        if (pa_client_conf_load_cookie_from_hex(c, t) < 0) {
            pa_log(_("Failed to parse cookie data"));
            goto finish;
        }
    }

    ret = 0;

finish:
    if (xcb)
        xcb_disconnect(xcb);

    return ret;

}