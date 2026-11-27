#pragma once

#if !IL2CPP_THREADS_STD && IL2CPP_THREADS_PTHREAD

#include <pthread.h>
#include <vector>
#include <atomic>

#include "os/Generic/ThreadAPCQueue.h"
#include "os/Generic/WaitObject.h"
#include "os/ErrorCodes.h"
#include "os/Mutex.h"
#include "os/Event.h"
#include "os/Thread.h"
#include "os/WaitStatus.h"
#include "utils/NonCopyable.h"
#include "Cpp/CappedSemaphore.h"
#include "Cpp/Atomic.h"

#if defined(IL2CPP_ENABLE_PLATFORM_THREAD_AFFINTY)
struct cpu_set_t;
int pthread_attr_setaffinity_np(pthread_attr_t *attr, size_t cpusetsize, const cpu_set_t *cpuset);
#endif

#if defined(IL2CPP_ENABLE_PLATFORM_THREAD_RENAME)
int pthread_setname_np(pthread_t handle, const char *name);
#endif

#if !defined(IL2CPP_DEFAULT_STACK_SIZE)
#define IL2CPP_DEFAULT_STACK_SIZE ( 1 * 1024 * 1024)            // default .NET stacksize is 1mb
#endif

namespace il2cpp
{
namespace os
{
/// POSIX threads implementation. Supports APCs and interruptible waits.
    class ThreadImpl : public ThreadAPCQueue, public il2cpp::utils::NonCopyable
    {
    public:

        ThreadImpl();
        ~ThreadImpl();

        static void AllocateStaticData();
        static void FreeStaticData();


        uint64_t Id();
        ErrorCode Run(Thread::StartFunc func, void* arg, int64_t affinityMask);
        void SetName(const char* name);
        void SetPriority(ThreadPriority priority);
        ThreadPriority GetPriority();
        void SetStackSize(size_t newsize);

        static int GetMaxStackSize();

        static void Sleep(uint32_t milliseconds, bool interruptible);
        static uint64_t CurrentThreadId();
        static ThreadImpl* GetCurrentThread();
        static ThreadImpl* CreateForCurrentThread();

        static bool YieldInternal();

#if IL2CPP_HAS_NATIVE_THREAD_CLEANUP
        static void SetNativeThreadCleanup(Thread::ThreadCleanupFunc cleanupFunction);
        static void RegisterCurrentThreadForCleanup(void* arg);
        static void UnregisterCurrentThreadForCleanup();
#endif

    private:

        std::atomic<pthread_t> m_Handle;

        /// Start data.
        Thread::StartFunc m_StartFunc;
        void* m_StartArg;

        size_t m_StackSize; // size of stack (can not be adjusted after thread creation)

        static void* ThreadStartWrapper(void* arg);
    };
}
}

#endif
