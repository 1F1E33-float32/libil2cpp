#include "il2cpp-config.h"
#include "TimerQueue.h"
#include "vm/Runtime.h"
#if IL2CPP_TARGET_JAVASCRIPT
#include <emscripten/eventloop.h>
#endif

namespace il2cpp
{
namespace icalls
{
namespace System
{
namespace Private
{
namespace CoreLib
{
namespace System
{
namespace Threading
{
#if IL2CPP_TARGET_JAVASCRIPT

#if IL2CPP_SUPPORT_THREADS
#error This implementation assume a single thread
#endif

    typedef void (*TimerCallback)();
    // this points to System.Threading.TimerQueue.TimerHandler C# method
    static TimerCallback gsTimerHandler;
    static int gsTimeoutId;

    static void ExecuteTimer(void*)
    {
        gsTimeoutId = 0;
        if (vm::Runtime::IsShuttingDown())
            return;
        gsTimerHandler(); // TimerQueue.TimerHandler, which reschedules itself if more timers are due
    }

#endif

    void TimerQueue::MainThreadScheduleTimer(void* callback, int32_t shortestDueTimeMs)
    {
#if IL2CPP_TARGET_JAVASCRIPT
        IL2CPP_ASSERT(callback);
        if (callback == nullptr)
            return;
        gsTimerHandler = reinterpret_cast<TimerCallback>(callback);
        if (gsTimeoutId != 0)
            emscripten_clear_timeout(gsTimeoutId);
        gsTimeoutId = emscripten_set_timeout(ExecuteTimer, shortestDueTimeMs, nullptr);
#else
        IL2CPP_NOT_IMPLEMENTED_ICALL(TimerQueue::MainThreadScheduleTimer);
#endif
    }
} // namespace Threading
} // namespace System
} // namespace CoreLib
} // namespace Private
} // namespace System
} // namespace icalls
} // namespace il2cpp
