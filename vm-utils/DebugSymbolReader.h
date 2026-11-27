#pragma once

#include <cstdint>

namespace il2cpp
{
namespace utils
{
    struct SourceLocation
    {
        const char* filePath;
        uint32_t lineNumber;
    };

    class DebugSymbolReader
    {
    public:
        static bool LoadDebugSymbols();
        static bool GetSourceLocation(void* nativeInstructionPointer, SourceLocation& sourceLocation);
        static bool AddStackFrames(void* nativeInstructionPointer, std::vector<Il2CppStackFrameInfo>* stackFrames);
        static bool GetStackFrameAt(void* nativeInstructionPointer, int32_t& depth, Il2CppStackFrameInfo& outStackFrame);
        static bool DebugSymbolsAvailable();
    };
} /* namespace utils */
} /* namespace il2cpp */
