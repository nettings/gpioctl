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

#include "stdout_process.h"
#include <stdio.h>
#include <string.h>
#include "globals.h"

void update_stdout(control_t * c)
{
        fprintf(stdout,"%03d\t%05d\n", c->pin1, c->value);
        fflush(stdout);
}


char *setup_STDOUT_format(char *c)
{
	char *tok_gpi;
	char *tok_val;

	tok_gpi = strstr(c, "%gpi%");
	tok_val = strstr(c, "%val%");
	if (tok_gpi == NULL) {
		ERR("No %%gpi%% token found.");
	}
	if (tok_val == NULL) {
		ERR("No %%val%% token found.");
	}

	return NULL;

}
