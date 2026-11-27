#pragma once

#if IL2CPP_THREADS_GENERIC_WAIT_OBJECT

#include <stdint.h>
#include <vector>

#include "os/Thread.h"
#include "Baselib.h"
#include "Cpp/Atomic.h"
#include "Cpp/CappedSemaphore.h"
#include "Cpp/Lock.h"

namespace il2cpp
{
namespace os
{
    class WaitObject;

    /// Per-thread state that the generic WaitObject implementations need, shared by every
    /// platform which implements its synchronization primitives with them. Individual
    /// platforms ThreadImpl implementations are expected to inherit from this class and
    /// call its constructor if they are to use the generic WaitObject implementations.
    class ThreadAPCQueue
    {
    public:
        ThreadAPCQueue()
            : m_CurrentWaitObject(NULL)
            , m_ConditionSemaphore(1)
        {
        }

        void ReleaseSemaphore() { m_ConditionSemaphore.Release(1); }
        void AcquireSemaphore() { m_ConditionSemaphore.Acquire(); }
        bool TryTimedAcquireSemaphore(uint32_t timeout) { return m_ConditionSemaphore.TryTimedAcquire(baselib::timeout_ms(timeout)); }

        /// Queue a callback to run on this thread the next time it reaches an interruptible wait.
        /// NOTE: Called from other threads.
        void QueueUserAPC(Thread::APCFunc callback, void* context)
        {
            IL2CPP_ASSERT(callback != NULL);

            // Put on queue.
            {
                m_PendingAPCsMutex.Acquire();
                m_PendingAPCs.push_back(APCRequest(callback, context));
                m_PendingAPCsMutex.Release();
            }

            // Interrupt an ongoing wait, only interrupt if we have an object waiting
            if (m_CurrentWaitObject.load())
            {
                m_ConditionSemaphore.Release(1);
            }
        }

        /// Handle any pending APCs.
        /// NOTE: Can only be called on current thread.
        void CheckForUserAPCAndHandle()
        {
            IL2CPP_ASSERT(IsOwnedByCurrentThread() && "Must be called on the owning thread!");

            m_PendingAPCsMutex.Acquire();

            while (!m_PendingAPCs.empty())
            {
                APCRequest apcRequest = m_PendingAPCs.front();

                // Remove from list. We do this before calling the function to make sure
                // the list is up to date in case the function throws.
                m_PendingAPCs.erase(m_PendingAPCs.begin());

                // Release mutex while we call the function so that we don't deadlock
                // if the function starts waiting on a thread that queues an APC.
                m_PendingAPCsMutex.Release();

                apcRequest.callback(apcRequest.context);

                m_PendingAPCsMutex.Acquire();
            }

            m_PendingAPCsMutex.Release();
        }

    protected:

        friend class WaitObject; // SetWaitObject()

        /// Set the synchronization object the thread is about to wait on.
        /// NOTE: This can only be called on the current thread.
        void SetWaitObject(WaitObject* waitObject)
        {
            IL2CPP_ASSERT(IsOwnedByCurrentThread() && "Must be called on the owning thread!");

            m_CurrentWaitObject = waitObject;
        }

    private:

        bool IsOwnedByCurrentThread() const;

        /// List of APC requests for this thread.
        struct APCRequest
        {
            Thread::APCFunc callback;
            void* context;

            APCRequest(Thread::APCFunc callback, void* context) :
                callback(callback), context(context)
            {
            }
        };

        /// The synchronization primitive that this thread is currently blocked on.
        /// (Atomic to signal intent, not because it's actually needed.)
        baselib::atomic<WaitObject*> m_CurrentWaitObject;

        baselib::Lock m_PendingAPCsMutex;
        std::vector<APCRequest> m_PendingAPCs;
        baselib::CappedSemaphore m_ConditionSemaphore;
    };
}
}

#endif // IL2CPP_THREADS_GENERIC_WAIT_OBJECT
