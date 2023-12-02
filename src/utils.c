#include "utils.h"

char* set_str_status(int si_code) {
    switch (si_code) {
    case CLD_CONTINUED:
        return "continued";
    case CLD_DUMPED:
        return "dumped";
    case CLD_EXITED:
        return "exited";
    case CLD_KILLED:
        return "killed";
    case CLD_STOPPED:
        return "stopped";
    case CLD_TRAPPED:
        return "trapped";
    }
    return "unknown";
}