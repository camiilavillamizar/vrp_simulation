#ifndef STATUS_REPORT_H
#define STATUS_REPORT_H

#include "game_rules.h"
#include "villager.h"
#include "map.h"

//Prints a textual report of the current simulation state
//to stdout or a log file (one line per villager + resource + building summary)
void print_status_report(int tick);

#endif
