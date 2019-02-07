#ifndef PARSE_CMDLINE_H
#define PARSE_CMDLINE_H

#define EXIT_USAGE -1
#define EXIT_CLEAN 0
#define EXIT_ERR -2
void usage();
int parse_cmdline(int argc, char *argv[]);

#endif

