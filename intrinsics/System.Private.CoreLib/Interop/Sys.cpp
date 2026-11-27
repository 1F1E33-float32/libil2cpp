#pragma once
#include "il2cpp-config.h"
#include "codegen/il2cpp-codegen-metadata.h"

#include "os/Mutex.h"
#include "os/Semaphore.h"
#include "vm/Coverage.h"

namespace il2cpp
{
namespace intrinsics
{
namespace System_Private_CoreLib
{
namespace Interop
{
    struct LowLevelMonitor
    {
        LowLevelMonitor() : mutex(false), semaphore(0, 1) {}
        os::Mutex mutex;
        os::Semaphore semaphore;
    };

    // These intrinsics replace the class libraries' P/Invokes, which never run, so they record the calls the P/Invokes would have.
    static void RecordSystemNativePInvoke(const char* entryPoint)
    {
#if IL2CPP_ENABLE_COVERAGE
        vm::Coverage::RecordPInvoke("System.Private.CoreLib", "libSystem.Native", entryPoint);
#endif
    }

    intptr_t il2cpp_intrinsic_sys_low_level_monitor_create()
    {
        RecordSystemNativePInvoke("SystemNative_LowLevelMonitor_Create");

        LowLevelMonitor* llmon = new LowLevelMonitor();
        return reinterpret_cast<intptr_t>(llmon);
    }

    void il2cpp_intrinsic_sys_low_level_monitor_destroy(intptr_t monitor)
    {
        RecordSystemNativePInvoke("SystemNative_LowLevelMonitor_Destroy");

        LowLevelMonitor* llmon = reinterpret_cast<LowLevelMonitor*>(monitor);
        delete llmon;
    }

    void il2cpp_intrinsic_sys_low_level_monitor_wait(intptr_t monitor)
    {
        RecordSystemNativePInvoke("SystemNative_LowLevelMonitor_Wait");

        LowLevelMonitor* llmon = reinterpret_cast<LowLevelMonitor*>(monitor);

        os::AutoUnlock unlock(&llmon->mutex, true);
        llmon->semaphore.Wait(true);
    }

    bool il2cpp_intrinsic_sys_low_level_monitor_timed_wait(intptr_t monitor, int32_t timeoutMilliseconds)
    {
        RecordSystemNativePInvoke("SystemNative_LowLevelMonitor_TimedWait");

        LowLevelMonitor* llmon = reinterpret_cast<LowLevelMonitor*>(monitor);

        os::AutoUnlock unlock(&llmon->mutex, true);
        return llmon->semaphore.Wait(timeoutMilliseconds, true) == kWaitStatusSuccess;
    }

    void il2cpp_intrinsic_sys_low_level_monitor_acquire(intptr_t monitor)
    {
        RecordSystemNativePInvoke("SystemNative_LowLevelMonitor_Acquire");

        LowLevelMonitor* llmon = reinterpret_cast<LowLevelMonitor*>(monitor);
        llmon->mutex.Lock(true);
    }

    void il2cpp_intrinsic_sys_low_level_monitor_release(intptr_t monitor)
    {
        RecordSystemNativePInvoke("SystemNative_LowLevelMonitor_Release");

        LowLevelMonitor* llmon = reinterpret_cast<LowLevelMonitor*>(monitor);
        llmon->mutex.Unlock();
    }

    void il2cpp_intrinsic_sys_low_level_monitor_signal_release(intptr_t monitor)
    {
        RecordSystemNativePInvoke("SystemNative_LowLevelMonitor_Signal_Release");

        LowLevelMonitor* llmon = reinterpret_cast<LowLevelMonitor*>(monitor);
        llmon->semaphore.Post();
        llmon->mutex.Unlock();
    }
} // namespace Interop
} // namespace System_Private_CoreLib
} // namespace intrinsics
} // namespace il2cpp
