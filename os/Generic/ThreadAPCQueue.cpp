#include "il2cpp-config.h"

#if IL2CPP_THREADS_GENERIC_WAIT_OBJECT

#include "ThreadAPCQueue.h"
#include "os/Thread.h"
#if IL2CPP_THREADS_WIN32
#include "os/Win32/ThreadImpl.h"
#elif IL2CPP_THREADS_PTHREAD
#include "os/Posix/ThreadImpl.h"
#else
#include "os/ThreadImpl.h"
#endif

namespace il2cpp
{
namespace os
{
    bool ThreadAPCQueue::IsOwnedByCurrentThread() const
    {
        // Threads that IL2CPP does not know about have no ThreadImpl to compare against.
        if (!Thread::HasCurrentThread())
            return true;

        return static_cast<const ThreadAPCQueue*>(ThreadImpl::GetCurrentThread()) == this;
    }
}
}

#endif // IL2CPP_THREADS_GENERIC_WAIT_OBJECT
