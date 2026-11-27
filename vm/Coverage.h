#pragma once

#if IL2CPP_ENABLE_COVERAGE

#include <string>

namespace il2cpp
{
namespace vm
{
    // Test-only instrumentation, compiled in when il2cpp runs with --enable-coverage. The player records which parts of the runtime it uses, and the
    // integration test harness reads the records back through il2cpp_coverage_copy_records at the end of the run to report the tests' coverage.
    class Coverage
    {
    public:
        // Called by every generated P/Invoke method on each call, and by the intrinsics that replace P/Invokes, with the P/Invoke they replace.
        static void RecordPInvoke(const char* assemblyName, const char* moduleName, const char* entryPoint);

        // One line per distinct record so far, sorted, each starting with its kind, such as "PInvoke\tassembly\tmodule\tentryPoint\n".
        static std::string GetRecords();
    };
} /* namespace vm */
} /* namespace il2cpp */

#endif
