#pragma once

#include <stdint.h>
#include <vector>
#include "il2cpp-config.h"
#include "il2cpp-metadata.h"

namespace il2cpp
{
namespace vm
{
    typedef std::vector<Il2CppStackFrameInfo> StackFrames;

    class BorrowedStackFrames
    {
    public:
        // Takes over the borrow held by other, which is left holding nothing.
        BorrowedStackFrames(BorrowedStackFrames&& other) noexcept
            : m_Frames(std::move(other.m_Frames))
            , m_OwnsFrames(other.m_OwnsFrames)
        {
            other.m_OwnsFrames = false;
        }

        ~BorrowedStackFrames();

        StackFrames Copy() const
        {
            return m_Frames;
        }

        decltype(auto)begin() const
        {
            return m_Frames.begin();
        }

        decltype(auto)end() const
        {
            return m_Frames.end();
        }

        decltype(auto)rbegin() const
        {
            return m_Frames.rbegin();
        }

        decltype(auto)rend() const
        {
            return m_Frames.rend();
        }

        decltype(auto)size() const
        {
            return m_Frames.size();
        }

        decltype(auto)operator[](size_t index) const
        {
            IL2CPP_ASSERT(index < m_Frames.size());
            return m_Frames[index];
        }

    private:
        friend class CachedStackImplementation;
        friend class MethodStack;

        explicit BorrowedStackFrames(std::nullptr_t) :
            m_OwnsFrames(false)
        {
        }

        explicit BorrowedStackFrames(StackFrames&& frames)
            : m_Frames(std::move(frames))
            , m_OwnsFrames(true)
        {
        }

        BorrowedStackFrames(const BorrowedStackFrames&) = delete;
        BorrowedStackFrames& operator=(const BorrowedStackFrames&) = delete;

        StackFrames m_Frames;
        bool m_OwnsFrames;
    };

    class LIBIL2CPP_CODEGEN_API StackTrace
    {
    public:
        static void InitializeStackTracesForCurrentThread();
        static void CleanupStackTracesForCurrentThread();

        // Current thread functions
        static BorrowedStackFrames GetStackFrames();

        // Reuses the previous walk when this call is the continuation of one, which is how the
        // System.Diagnostics.StackFrame icall avoids one walk per frame as it builds a managed stack
        // trace.
        static BorrowedStackFrames GetCachedStackFrames(int32_t depth);

        static bool GetStackFrameAt(int32_t depth, Il2CppStackFrameInfo& frame);
        static void WalkFrameStack(Il2CppFrameWalkFunc callback, void* context);

        static size_t GetStackDepth();
        inline static bool GetTopStackFrame(Il2CppStackFrameInfo& frame) { return GetStackFrameAt(0, frame); }

        static void PushFrame(Il2CppStackFrameInfo& frame);
        static void PopFrame();

        // Remote thread functions
        static bool GetThreadStackFrameAt(Il2CppThread* thread, int32_t depth, Il2CppStackFrameInfo& frame);
        static void WalkThreadFrameStack(Il2CppThread* thread, Il2CppFrameWalkFunc callback, void* context);
        static int32_t GetThreadStackDepth(Il2CppThread* thread);
        static bool GetThreadTopStackFrame(Il2CppThread* thread, Il2CppStackFrameInfo& frame);
    };
}
}
