#include "stdout_process.h"
#include <stdio.h>
#include <string.h>
#include "globals.h"

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
