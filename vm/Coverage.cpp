#include "il2cpp-config.h"

#if IL2CPP_ENABLE_COVERAGE

#include "vm/Coverage.h"
#include "vm/LastError.h"
#include "os/Mutex.h"

#include <set>
#include <string.h>

namespace il2cpp
{
namespace vm
{
    // Generated code and intrinsics pass string literals, so recording a P/Invoke that has already been recorded never allocates.
    struct PInvokeRecord
    {
        const char* assemblyName;
        const char* moduleName;
        const char* entryPoint;

        // Compares the strings rather than their addresses, so that the same P/Invoke recorded through different string literals is one record, and the
        // records are read back sorted.
        bool operator<(const PInvokeRecord& other) const
        {
            int order = strcmp(assemblyName, other.assemblyName);
            if (order == 0)
                order = strcmp(moduleName, other.moduleName);
            if (order == 0)
                order = strcmp(entryPoint, other.entryPoint);

            return order < 0;
        }
    };

    static baselib::ReentrantLock s_RecordsMutex;

    static std::set<PInvokeRecord> s_PInvokeRecords;

    void Coverage::RecordPInvoke(const char* assemblyName, const char* moduleName, const char* entryPoint)
    {
        // This runs between a P/Invoke and the read of its error, because on Unix Marshal.GetLastSystemError is itself a P/Invoke, to
        // SystemNative_GetErrNo. On Linux and Android, waiting for a contended lock makes a futex syscall that can set errno to EAGAIN.
        LastOsErrorPreserver preserveLastError;
        os::FastAutoLock lock(&s_RecordsMutex);
        s_PInvokeRecords.insert(PInvokeRecord{assemblyName, moduleName, entryPoint});
    }

    std::string Coverage::GetRecords()
    {
        os::FastAutoLock lock(&s_RecordsMutex);

        std::string records;
        for (const PInvokeRecord& record : s_PInvokeRecords)
        {
            records += "PInvoke\t";
            records += record.assemblyName;
            records += '\t';
            records += record.moduleName;
            records += '\t';
            records += record.entryPoint;
            records += '\n';
        }

        return records;
    }
} /* namespace vm */
} /* namespace il2cpp */

// Called by the generated integration test harness through [DllImport("__Internal")]. Copies the records into the buffer if they fit, and returns
// their size in bytes either way, so the caller can grow the buffer and call again.
IL2CPP_EXTERN_C int32_t CDECL il2cpp_coverage_copy_records(uint8_t* buffer, int32_t bufferSize)
{
    std::string records = il2cpp::vm::Coverage::GetRecords();
    if (buffer != NULL && records.size() <= static_cast<size_t>(bufferSize))
        memcpy(buffer, records.data(), records.size());

    return static_cast<int32_t>(records.size());
}

#endif
