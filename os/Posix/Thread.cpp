#include "il2cpp-config.h"

#if IL2CPP_THREADS_PTHREAD && !IL2CPP_TARGET_DARWIN

#include <pthread.h>
#include <tuple>

#include "os/Thread.h"

namespace il2cpp
{
namespace os
{
    bool Thread::GetCurrentThreadStackBounds(void** low, void** high)
    {
#if IL2CPP_TARGET_LINUX
        thread_local static std::tuple<void*, void*> stack_limits;

        std::tie(*low, *high) = stack_limits;
        if (*low == NULL)
        {
            pthread_attr_t attr;
            size_t stacksize = 0;
            pthread_t self = pthread_self();
            int ret = pthread_getattr_np(self, &attr);
            IL2CPP_ASSERT(ret == 0);
            if (ret != 0)
                return false;

            ret = pthread_attr_getstack(&attr, low, &stacksize);
            IL2CPP_ASSERT(ret == 0);

            *high = (void*)((uintptr_t)*low + stacksize);

            pthread_attr_destroy(&attr);

            stack_limits = std::make_tuple(*low, *high);

            return ret == 0;
        }

        return true;
#else
        return false;
#endif
    }
}
}

#endif
