#pragma once

#include "rhi/include/RHIContext.h"

#if _DEBUG
#define BEGIN_EVENT(eventname) \
    do { if (auto* _dev = engine::getRHIDevice()) { std::string _s(eventname); _dev->pushDebugGroup(_s.c_str()); } } while(0)
#define END_EVENT() \
    do { if (auto* _dev = engine::getRHIDevice()) _dev->popDebugGroup(); } while(0)
#else
#define BEGIN_EVENT(eventname)
#define END_EVENT()
#endif
