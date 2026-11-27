#include "il2cpp-config.h"
#include "gc/GCEpoch.h"
#include "gc/GarbageCollector.h"
#include "os/Mutex.h"

#include "Baselib.h"
#include "Cpp/ReentrantLock.h"

namespace il2cpp
{
namespace gc
{
    static baselib::ReentrantLock s_GCEpochMutex;
    // Only ever goes up, so a block allocated in one lifetime never matches a later one.
    // Odd while a GC is alive: Begin and End each bump it once.
    static uint32_t s_GCEpoch = 0;

    void* GCEpoch::AllocateFixed(size_t size, void* descr, uint32_t& epoch)
    {
        // Callers are attached managed threads, including the finalizer thread, and shutdown stops all of them
        // before End(), so the epoch can't change here. No lock: it would be held through a possible collection.
        epoch = s_GCEpoch;
        IL2CPP_ASSERT((epoch & 1) == 1);
        return GarbageCollector::AllocateFixed(size, descr);
    }

    bool GCEpoch::FreeFixed(void* addr, uint32_t epoch)
    {
        Scope scope(epoch);
        if (!scope.IsCurrent())
            return false;

        GarbageCollector::FreeFixed(addr);
        return true;
    }

    GCEpoch::Scope::Scope(uint32_t epoch)
        : m_Epoch(epoch)
    {
        s_GCEpochMutex.Acquire();
    }

    GCEpoch::Scope::~Scope()
    {
        s_GCEpochMutex.Release();
    }

    bool GCEpoch::Scope::IsCurrent() const
    {
        // An even epoch was read between End and the GC teardown, and stays current until the next Begin
        return (m_Epoch & 1) == 1 && m_Epoch == s_GCEpoch;
    }

    void GCEpoch::Begin()
    {
        os::FastAutoLock lock(&s_GCEpochMutex);
        IL2CPP_ASSERT((s_GCEpoch & 1) == 0);
        ++s_GCEpoch;
    }

    void GCEpoch::End()
    {
        os::FastAutoLock lock(&s_GCEpochMutex);
        IL2CPP_ASSERT((s_GCEpoch & 1) == 1);
        ++s_GCEpoch;
    }
} /* gc */
} /* il2cpp */
