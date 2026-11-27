#pragma once

#include "utils/NonCopyable.h"

#include <stddef.h>
#include <stdint.h>

namespace il2cpp
{
namespace gc
{
    // An epoch identifies one GC lifetime, from GarbageCollector::Initialize to UninitializeGC. Code that may
    // still call into the GC after shutdown has stopped waiting for it, e.g. a thread exiting late, records the
    // epoch and makes those calls only while that GC is still the live one.
    class GCEpoch
    {
    public:
        // Fixed memory that records the epoch it was allocated in. Only for attached managed threads, which
        // shutdown stops before End().
        static void* AllocateFixed(size_t size, void* descr, uint32_t& epoch);
        // Frees the block if its GC is still alive, and returns whether it did; otherwise the block is left alone.
        static bool FreeFixed(void* addr, uint32_t epoch);

        // Keeps the GC that `epoch` names from being uninitialized while the scope is alive.
        // GC calls made inside the scope are safe when IsCurrent() is true. Keep it short and never wait
        // in it: shutdown waits for the scope before it tears the GC down.
        class Scope : public il2cpp::utils::NonCopyable
        {
        public:
            explicit Scope(uint32_t epoch);
            ~Scope();
            bool IsCurrent() const;

        private:
            uint32_t m_Epoch;
        };

        // Runtime calls these around the GC's lifetime: Begin right after GarbageCollector::Initialize,
        // End right before GarbageCollector::UninitializeGC.
        static void Begin();
        static void End();
    };
} /* gc */
} /* il2cpp */
