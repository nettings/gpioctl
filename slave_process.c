/*
  gpioctl

  Copyright (C) 2019 Jörn Nettingsmeier

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#include "slave_process.h"
#include <string.h>
#include <lo/lo.h>
#include <errno.h>
#include "globals.h"

static lo_server_thread server;
static void (*user_callback)();

static void handle_error(int num, const char* m, const char* path) {
        ERR("liblo error %d (%s) at %s.", num, m, path);
}

static int handle_usermsg(const char* path, const char* types, 
                          lo_arg **argv, int argc, 
                          lo_message msg, void* user_data) {
        DBG("User handler got OSC message %s %d.", path, argv[0]->i);
        user_callback(user_data, argv[0]->i);
        // the same message can be handled by
        // several registered handlers (think
        // multiple channels reacting to the 
        // same master)
        return 1;
}

static int handle_all(const char* path, const char* types, 
                          lo_arg **argv, int argc, 
                          lo_message msg, void* user_data) {
        DBG("Generic handler got OSC message %s %d and ate it.", path, argv[0]->i);
        // now we mark it as dealt with.
        return 0;
}

int setup_SLAVE(char* osc_url, void (*callback))
{
        DBG("Setting up SLAVE.");
        server = lo_server_thread_new_from_url(osc_url, &handle_error);
        user_callback = callback;
        if (server == NULL) {
                ERR("Could not create OSC server at %s.", osc_url);
                return -ENOANO;
        }
//        lo_server_thread_add_method(server, NULL, NULL, handle_all, NULL);
        return 0;
}

int setup_SLAVE_handler(char* path, void* data) {
        DBG("Setting up SLAVE handler for '%s'.", path);
        lo_method m;
        m = lo_server_thread_add_method(server, path, "i", &handle_usermsg, data);
        if (m == NULL) {
                ERR("Could not add OSC path handler at '%s'.", path);
                return -ENOANO;
        }
        return 0;
}


int start_SLAVE() {
        DBG("Starting SLAVE.");
        int e;
        e = lo_server_thread_start(server);
        if (e < 0) {
                ERR("Could not start OSC server thread (error no. %d).", e);
                return e;
        }
        return 0;
}

int shutdown_SLAVE()
{
        DBG("Shutting down SLAVE.");
        lo_server_thread_stop(server);
        lo_server_thread_free(server);
        return 0;
}
