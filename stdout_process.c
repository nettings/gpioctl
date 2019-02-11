#include "stdout_process.h"
#include <stdio.h>
#include <string.h>
#include "globals.h"

void setup_STDOUT_format(control_t* c) {
        char* tok_gpi;
        char* tok_val;
        
        tok_gpi = strstr(c->param1, "%gpi%");
        tok_val = strstr(c->param1, "%val%");
        if (tok_gpi == NULL) {
                ERR("No %%gpi%% token found.");
        }
        if (tok_val == NULL) {
                ERR("No %%val%% token found.");
        }
        
}
      