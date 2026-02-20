#pragma once

#if defined(__EXCEPTIONS)
    #define RHEOSCAPE_EXCEPTIONAL_CONDITION(e) throw e();
#else
    #define RHEOSCAPE_EXCEPTIONAL_CONDITION(e) assert(e{}.what());
#endif
