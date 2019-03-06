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

#include "osc_process.h"
#include <string.h>
#include <lo/lo.h>
#include <errno.h>
#include "globals.h"

int setup_OSC()
{
        DBG("Setting up OSC.");
        return 0;
}

int shutdown_OSC()
{
        DBG("Shutting down OSC.");
        return 0;
}

int update_OSC(control_t * c)
{
	int e;
	DBG("Updating OSC message queue: '%s %d' -> %s", 
	    (char*)c->param2, c->value, (char *)c->param1);
	lo_address addr = lo_address_new_from_url((char *)c->param1);
	if (addr == NULL) {
		ERR("Could not create OSC address from URL '%s'.",
		    (char *)c->param1);
                return -EINVAL;
	}
	e = lo_send(addr, (char *)c->param2, "i", (char *)c->value);
	lo_address_free(addr);
	if (e == -1) {
	        ERR("Could not send OSC message '%s %d'.", 
	            (char *)c->param2, c->value);
                return -ECOMM;
	}
	return 0;
}
