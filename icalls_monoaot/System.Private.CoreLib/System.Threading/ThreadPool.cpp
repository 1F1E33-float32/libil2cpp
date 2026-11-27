#include "il2cpp-config.h"
#include "ThreadPool.h"
#include "vm/Runtime.h"
#include <vector>
#include <algorithm>

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
    typedef void (*BackgroundJobCallback)();

    // Single-threaded WebGL only: everything here runs on the main thread, so no locking.
    static std::vector<BackgroundJobCallback> gsJobs;

    static void ExecuteBackgroundJobs(void*)
    {
        std::vector<BackgroundJobCallback> jobs;
        jobs.swap(gsJobs); // jobs queued while running go to the next tick

        // Equivalent of Mono's is_runtime_running(). A job can still be pending when il2cpp
        // shuts down, and running managed code after that point is invalid, throwing an error.
        if (vm::Runtime::IsShuttingDown())
            return;

        for (BackgroundJobCallback cb : jobs)
            cb();
    }

    // Utilize emscripten's async scheduler instead of writing our own.
    // Adding this as an abstraction layer in case we need to do that later.
    static void ScheduleBackgroundExec()
    {
        emscripten_async_call(ExecuteBackgroundJobs, nullptr, 0); // setTimeout(..., 0)
    }

#endif

    void ThreadPool::MainThreadScheduleBackgroundJob(void* callback)
    {
#if IL2CPP_TARGET_JAVASCRIPT
        IL2CPP_ASSERT(callback);
        BackgroundJobCallback cb = reinterpret_cast<BackgroundJobCallback>(callback);
        if (gsJobs.empty())
            ScheduleBackgroundExec();
        if (std::find(gsJobs.begin(), gsJobs.end(), cb) == gsJobs.end())
            gsJobs.push_back(cb);
#else
        IL2CPP_NOT_IMPLEMENTED_ICALL(ThreadPool::MainThreadScheduleBackgroundJob);
#endif
    }
} // namespace Threading
} // namespace System
} // namespace CoreLib
} // namespace Private
} // namespace System
} // namespace icalls
} // namespace il2cpp
