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
#include <stdlib.h>
#include <lo/lo.h>
#include "globals.h"

static lo_server_thread server;
static void (*user_callback)();

static void handle_error(int num, const char* m, const char* path) {
        ERR("liblo error %d (%s) at %s.", num, m, path);
}

static int handle_osc(const char* path, const char* types, 
                          lo_arg **argv, int argc, 
                          void* data, void* user_data) {
        DBG("Got OSC message %s %d.", path, argv[0]->i);
        user_callback(user_data, argv[0]->i);
        return 0;
}

void setup_SLAVE(char* osc_url, void (*callback))
{
        server = lo_server_thread_new_from_url(osc_url, &handle_error);
        user_callback = callback;
        if (server == NULL) {
                ERR("Could not create server listening at %s.", osc_url);
                exit(2);
        }
}

void setup_SLAVE_handler(char* path, void* data) {
        lo_server_thread_add_method(server, path, "i", &handle_osc, data);
}


void start_SLAVE() {
        lo_server_thread_start(server);
}

void shutdown_SLAVE()
{
        lo_server_thread_stop(server);
        lo_server_thread_free(server);
}
