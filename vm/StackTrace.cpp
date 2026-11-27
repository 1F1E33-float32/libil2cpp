#include "il2cpp-config.h"
#include "StackTrace.h"
#include "il2cpp-object-internals.h"
#include "os/Event.h"
#include "os/StackTrace.h"
#include "os/Thread.h"
#include "os/ThreadLocalValue.h"
#include "os/Image.h"
#include "vm/Method.h"
#include "vm/Thread.h"
#include "vm/Type.h"
#include "vm-utils/Debugger.h"
#include "vm-utils/NativeSymbol.h"
#include "vm-utils/DebugSymbolReader.h"
#include "vm-utils/Debugger.h"

#include <limits>
#include <cstdio>
#include <type_traits>

namespace il2cpp
{
namespace vm
{
#if IL2CPP_ENABLE_STACKTRACES

    class CachedInfo
    {
        const void* m_StackPointer;
        int32_t m_Depth;
        bool m_IsBorrowed;

    public:
        CachedInfo()
        {
            Invalidate();
        }

        bool Borrow(int32_t depth)
        {
            auto stackPointer = os::StackTrace::GetStackPointer();
            bool canReuse = CheckCondition(depth, stackPointer);

            m_Depth = depth;
            m_StackPointer = stackPointer;
            m_IsBorrowed = true;

            return canReuse;
        }

        void RecordBorrowedReturn()
        {
            if (!m_IsBorrowed)
            {
                // Someone is returning a buffer that wasn't from the cached path
                // That means cached path is no longer valid, and we must thus invalidate it
                Invalidate();
            }
            else
            {
                m_IsBorrowed = false;
            }
        }

        void Invalidate()
        {
            m_Depth = std::numeric_limits<int32_t>::max();
            m_StackPointer = nullptr;
            m_IsBorrowed = false;
        }

    private:
        bool CheckCondition(int32_t depth, const void* stackPointer) const
        {
            // We can use cached value if stack pointer is the same and not NULL, and 'depth' has been incremented since previous call
            return m_StackPointer != nullptr && stackPointer == m_StackPointer && depth - 1 == m_Depth && !m_IsBorrowed;
        }
    };

#if IL2CPP_ENABLE_STACKTRACE_SENTRIES
    // StacktraceSentryMethodStack
    class StackImplementation
    {
    public:
        inline void GetStackFrames(StackFrames& stackFrames) const
        {
            stackFrames.assign(m_Stack.begin(), m_Stack.end());
        }

        inline bool GetStackFrameAt(int32_t depth, Il2CppStackFrameInfo& frame) const
        {
            if (static_cast<int>(m_Stack.size()) + depth < 1)
                return false;

            frame = m_Stack[m_Stack.size() - 1 + depth];
            return true;
        }

        inline size_t GetStackDepth() const
        {
            return m_Stack.size();
        }

        inline void PushFrame(Il2CppStackFrameInfo& frame)
        {
            m_Stack.push_back(frame);
        }

        inline void PopFrame()
        {
            m_Stack.pop_back();
        }

    private:
        StackFrames m_Stack;
    };

#endif // IL2CPP_ENABLE_STACKTRACE_SENTRIES

#if IL2CPP_ENABLE_NATIVE_STACKTRACES

#if IL2CPP_MONO_DEBUGGER
    // DebuggerMethodStack
    class StackImplementation
    {
    public:
        inline static void GetStackFrames(StackFrames& stackFrames)
        {
            stackFrames.clear();
            utils::Debugger::GetStackFrames(&stackFrames);
        }

        inline static bool GetStackFrameAt(int32_t depth, Il2CppStackFrameInfo& frame)
        {
            return utils::Debugger::GetStackFrameAt(depth, frame);
        }

        inline static size_t GetStackDepth()
        {
            return utils::Debugger::GetStackDepth();
        }

        inline static void PushFrame(Il2CppStackFrameInfo& frame)
        {
        }

        inline static void PopFrame()
        {
        }
    };
#else
    // NativeMethodStack
    class StackImplementation
    {
    public:
        static bool GetStackFramesCallback(Il2CppMethodPointer frame, void* context)
        {
            const MethodInfo* method = il2cpp::utils::NativeSymbol::GetMethodFromNativeSymbol(frame);
            StackFrames* stackFrames = static_cast<StackFrames*>(context);

            if (method != NULL)
            {
                bool frames_added = il2cpp::utils::DebugSymbolReader::AddStackFrames(reinterpret_cast<void*>(frame), stackFrames);

                if (!frames_added)
                {
                    Il2CppStackFrameInfo frameInfo = { 0 };
                    frameInfo.method = method;
                    frameInfo.raw_ip = reinterpret_cast<uintptr_t>(frame) - reinterpret_cast<uintptr_t>(os::Image::GetImageBase());
                    stackFrames->push_back(frameInfo);
                }
            }

            return true;
        }

        struct GetStackFrameAtContext
        {
            int32_t currentDepth;
            Il2CppStackFrameInfo frameInfo;
        };

        static bool GetStackFrameAtCallback(Il2CppMethodPointer frame, void* context)
        {
            const MethodInfo* method = il2cpp::utils::NativeSymbol::GetMethodFromNativeSymbol(frame);
            GetStackFrameAtContext* ctx = static_cast<GetStackFrameAtContext*>(context);

            if (method != NULL)
            {
                auto depth = ctx->currentDepth;
                bool frameFound = il2cpp::utils::DebugSymbolReader::GetStackFrameAt(reinterpret_cast<void*>(frame), depth, ctx->frameInfo);
                if (frameFound)
                    return false;

                // depth wasn't incremented, so GetStackFrameAt didn't run out of frames: it failed to locate the first one
                if (depth == ctx->currentDepth)
                {
                    if (ctx->currentDepth == 0)
                    {
                        ctx->frameInfo = Il2CppStackFrameInfo {};
                        ctx->frameInfo.method = method;
                        ctx->frameInfo.raw_ip = reinterpret_cast<uintptr_t>(frame) - reinterpret_cast<uintptr_t>(os::Image::GetImageBase());
                        return false;
                    }

                    ctx->currentDepth++;
                }
                else
                {
                    ctx->currentDepth = depth;
                }
            }

            return true;
        }

    public:
        inline static void GetStackFrames(StackFrames& stackFrames)
        {
            stackFrames.clear();
            os::StackTrace::WalkStack(&StackImplementation::GetStackFramesCallback, &stackFrames, os::StackTrace::kFirstCalledToLastCalled);
        }

        inline static bool GetStackFrameAt(int32_t depth, Il2CppStackFrameInfo& frame)
        {
            GetStackFrameAtContext context = { depth, {} };

            os::StackTrace::WalkStack(&StackImplementation::GetStackFrameAtCallback, &context, os::StackTrace::kLastCalledToFirstCalled);

            if (context.frameInfo.method != NULL)
            {
                frame = context.frameInfo;
                return true;
            }

            return false;
        }

        inline static size_t GetStackDepth()
        {
            GetStackFrameAtContext context = { std::numeric_limits<int32_t>::min(), {} };
            os::StackTrace::WalkStack(&StackImplementation::GetStackFrameAtCallback, &context, os::StackTrace::kLastCalledToFirstCalled);

            IL2CPP_ASSERT(context.currentDepth < 0);
            return static_cast<size_t>(context.currentDepth - std::numeric_limits<int32_t>::min());
        }

        inline static void PushFrame(Il2CppStackFrameInfo& frame)
        {
        }

        inline static void PopFrame()
        {
        }
    };
#endif // IL2CPP_MONO_DEBUGGER

#endif // IL2CPP_ENABLE_NATIVE_STACKTRACES

    class CachedStackImplementation : public StackImplementation
    {
    public:
        CachedStackImplementation()
            : m_MaxPoolCapacity(0)
        {
        }

        inline BorrowedStackFrames GetStackFrames()
        {
            auto stackFrames = GetStackFramesRaw();
            m_CachedInfo.Invalidate();
            StackImplementation::GetStackFrames(stackFrames);
            return BorrowedStackFrames(std::move(stackFrames));
        }

        // Avoiding calling GetStackFrames() method for the same stack trace with incremented 'depth' value
        inline BorrowedStackFrames GetCachedStackFrames(int32_t depth)
        {
            auto stackFrames = GetStackFramesRaw();
            if (!m_CachedInfo.Borrow(depth))
                StackImplementation::GetStackFrames(stackFrames);

            return BorrowedStackFrames(std::move(stackFrames));
        }

        inline void ReturnBorrowedStackFrames(StackFrames&& frames)
        {
            m_CachedInfo.RecordBorrowedReturn();
            m_StackFramePool.push_back(std::move(frames));
        }

    private:
        inline StackFrames GetStackFramesRaw()
        {
            StackFrames frames;
            if (!m_StackFramePool.empty())
            {
                frames = std::move(m_StackFramePool.back());
                m_StackFramePool.pop_back();
            }
            else
            {
                // Reserve space for return now, so that we couldn't ever
                // throw from BorrowedStackFrames destructor
                m_StackFramePool.reserve(++m_MaxPoolCapacity);
                frames.reserve(64);
            }

            return frames;
        }

    private:
        std::vector<StackFrames> m_StackFramePool;
        size_t m_MaxPoolCapacity;
        CachedInfo m_CachedInfo;
    };

#if IL2CPP_ENABLE_NATIVE_STACKTRACES
    static_assert(std::is_empty<StackImplementation>::value, "Native stacktrace implementation class was expected to be empty");
    static_assert(sizeof(std::vector<StackFrames>) + sizeof(CachedInfo) + sizeof(size_t) == sizeof(CachedStackImplementation), "Empty base class optimization failed");
#endif

    class MethodStack
    {
    private:
        os::ThreadLocalValue s_StackImplementation;

        inline CachedStackImplementation* GetStackImplementation()
        {
            CachedStackImplementation* stack = nullptr;

            os::ErrorCode result = s_StackImplementation.GetValue(reinterpret_cast<void**>(&stack));
            Assert(result == os::kErrorCodeSuccess);

            return stack;
        }

    public:
        inline void InitializeForCurrentThread()
        {
            if (GetStackImplementation() != nullptr)
                return;

            os::ErrorCode result = s_StackImplementation.SetValue(new CachedStackImplementation());
            Assert(result == os::kErrorCodeSuccess);
        }

        inline void CleanupForCurrentThread()
        {
            auto stack = GetStackImplementation();
            if (stack == nullptr)
                return;

            delete stack;

            os::ErrorCode result = s_StackImplementation.SetValue(nullptr);
            Assert(result == os::kErrorCodeSuccess);
        }

        inline BorrowedStackFrames GetStackFrames()
        {
            auto stack = GetStackImplementation();
            if (stack == nullptr)
                return BorrowedStackFrames(nullptr);

            return stack->GetStackFrames();
        }

        inline BorrowedStackFrames GetCachedStackFrames(int32_t depth)
        {
            auto stack = GetStackImplementation();
            if (stack == nullptr)
            {
                // This should really never happen, as cached stack frames are
                // only called from icalls and by the time, the thread should
                // be fully initialized
                IL2CPP_ASSERT(false);
                return BorrowedStackFrames(nullptr);
            }

            return stack->GetCachedStackFrames(depth);
        }

        inline void ReturnBorrowedStackFrames(StackFrames&& frames)
        {
            auto stack = GetStackImplementation();
            if (stack == nullptr)
                return;

            stack->ReturnBorrowedStackFrames(std::move(frames));
        }

        inline bool GetStackFrameAt(int32_t depth, Il2CppStackFrameInfo& frame)
        {
            auto stack = GetStackImplementation();
            if (stack == nullptr)
                return false;

            return stack->GetStackFrameAt(depth, frame);
        }

        inline size_t GetStackDepth()
        {
            auto stack = GetStackImplementation();
            if (stack == nullptr)
                return 0;

            return stack->GetStackDepth();
        }

        inline void PushFrame(Il2CppStackFrameInfo& frame)
        {
            auto stack = GetStackImplementation();
            IL2CPP_ASSERT(stack != nullptr);
            stack->PushFrame(frame);
        }

        inline void PopFrame()
        {
            auto stack = GetStackImplementation();
            IL2CPP_ASSERT(stack != nullptr);
            stack->PopFrame();
        }
    };

#else

    class MethodStack
    {
    public:
        inline void InitializeForCurrentThread()
        {
        }

        inline void CleanupForCurrentThread()
        {
        }

        inline BorrowedStackFrames GetStackFrames()
        {
            return BorrowedStackFrames(nullptr);
        }

        inline BorrowedStackFrames GetCachedStackFrames(int32_t depth)
        {
            return BorrowedStackFrames(nullptr);
        }

        inline void ReturnBorrowedStackFrames(StackFrames&& frames)
        {
        }

        inline bool GetStackFrameAt(int32_t depth, Il2CppStackFrameInfo& frame)
        {
            return false;
        }

        inline size_t GetStackDepth()
        {
            return 0;
        }

        inline void PushFrame(Il2CppStackFrameInfo& frame)
        {
        }

        inline void PopFrame()
        {
        }
    };

#endif // IL2CPP_ENABLE_STACKTRACES

    static MethodStack s_MethodStack;

// Current thread functions

    void StackTrace::InitializeStackTracesForCurrentThread()
    {
        s_MethodStack.InitializeForCurrentThread();
    }

    void StackTrace::CleanupStackTracesForCurrentThread()
    {
        s_MethodStack.CleanupForCurrentThread();
    }

    BorrowedStackFrames StackTrace::GetStackFrames()
    {
        return s_MethodStack.GetStackFrames();
    }

    BorrowedStackFrames StackTrace::GetCachedStackFrames(int32_t depth)
    {
        return s_MethodStack.GetCachedStackFrames(depth);
    }

    BorrowedStackFrames::~BorrowedStackFrames()
    {
        if (m_OwnsFrames)
            s_MethodStack.ReturnBorrowedStackFrames(std::move(m_Frames));
    }

    bool StackTrace::GetStackFrameAt(int32_t depth, Il2CppStackFrameInfo& frame)
    {
        Assert(depth <= 0 && "Frame depth must be 0 or less");
        return s_MethodStack.GetStackFrameAt(depth, frame);
    }

    void StackTrace::WalkFrameStack(Il2CppFrameWalkFunc callback, void* context)
    {
        const auto frames = GetStackFrames();

        for (const auto& frame : frames)
            callback(&frame, context);
    }

    size_t StackTrace::GetStackDepth()
    {
        return s_MethodStack.GetStackDepth();
    }

    void StackTrace::PushFrame(Il2CppStackFrameInfo& frame)
    {
        s_MethodStack.PushFrame(frame);
    }

    void StackTrace::PopFrame()
    {
        s_MethodStack.PopFrame();
    }

// Remote thread functions

    struct GetThreadFrameAtContext
    {
        il2cpp::os::Event apcDoneEvent;
        int32_t depth;
        Il2CppStackFrameInfo* frame;
        bool hasResult;
    };

    struct WalkThreadFrameStackContext
    {
        il2cpp::os::Event apcDoneEvent;
        Il2CppFrameWalkFunc callback;
        void* userContext;
    };

    struct GetThreadStackDepthContext
    {
        il2cpp::os::Event apcDoneEvent;
        int32_t stackDepth;
    };

    struct GetThreadTopFrameContext
    {
        il2cpp::os::Event apcDoneEvent;
        Il2CppStackFrameInfo* frame;
        bool hasResult;
    };

    static void STDCALL GetThreadFrameAtCallback(void* context)
    {
        GetThreadFrameAtContext* ctx = static_cast<GetThreadFrameAtContext*>(context);

        ctx->hasResult = StackTrace::GetStackFrameAt(ctx->depth, *ctx->frame);
        ctx->apcDoneEvent.Set();
    }

    bool StackTrace::GetThreadStackFrameAt(Il2CppThread* thread, int32_t depth, Il2CppStackFrameInfo& frame)
    {
#if IL2CPP_ENABLE_STACKTRACES
        GetThreadFrameAtContext apcContext;

        apcContext.depth = depth;
        apcContext.frame = &frame;

#if MONO_NET_BCL
        thread->handle->QueueUserAPC(GetThreadFrameAtCallback, &apcContext);
#else
        thread->GetInternalThread()->handle->QueueUserAPC(GetThreadFrameAtCallback, &apcContext);
#endif
        apcContext.apcDoneEvent.Wait();

        return apcContext.hasResult;
#else
        return false;
#endif
    }

    static void STDCALL WalkThreadFrameStackCallback(void* context)
    {
        WalkThreadFrameStackContext* ctx = static_cast<WalkThreadFrameStackContext*>(context);

        StackTrace::WalkFrameStack(ctx->callback, ctx->userContext);
        ctx->apcDoneEvent.Set();
    }

    void StackTrace::WalkThreadFrameStack(Il2CppThread* thread, Il2CppFrameWalkFunc callback, void* context)
    {
#if IL2CPP_ENABLE_STACKTRACES
        WalkThreadFrameStackContext apcContext;

        apcContext.callback = callback;
        apcContext.userContext = context;

#if MONO_NET_BCL
        thread->handle->QueueUserAPC(WalkThreadFrameStackCallback, &apcContext);
#else
        thread->GetInternalThread()->handle->QueueUserAPC(WalkThreadFrameStackCallback, &apcContext);
#endif
        apcContext.apcDoneEvent.Wait();
#endif
    }

    static void STDCALL GetThreadStackDepthCallback(void* context)
    {
        GetThreadStackDepthContext* ctx = static_cast<GetThreadStackDepthContext*>(context);

        ctx->stackDepth = static_cast<int32_t>(StackTrace::GetStackDepth());
        ctx->apcDoneEvent.Set();
    }

    int32_t StackTrace::GetThreadStackDepth(Il2CppThread* thread)
    {
#if IL2CPP_ENABLE_STACKTRACES
        GetThreadStackDepthContext apcContext;

#if MONO_NET_BCL
        thread->handle->QueueUserAPC(GetThreadStackDepthCallback, &apcContext);
#else
        thread->GetInternalThread()->handle->QueueUserAPC(GetThreadStackDepthCallback, &apcContext);
#endif
        apcContext.apcDoneEvent.Wait();

        return apcContext.stackDepth;
#else
        return 0;
#endif
    }

    static void STDCALL GetThreadTopFrameCallback(void* context)
    {
        GetThreadTopFrameContext* ctx = static_cast<GetThreadTopFrameContext*>(context);

        ctx->hasResult = StackTrace::GetTopStackFrame(*ctx->frame);
        ctx->apcDoneEvent.Set();
    }

    bool StackTrace::GetThreadTopStackFrame(Il2CppThread* thread, Il2CppStackFrameInfo& frame)
    {
#if IL2CPP_ENABLE_STACKTRACES
        GetThreadTopFrameContext apcContext;
        apcContext.frame = &frame;

#if MONO_NET_BCL
        thread->handle->QueueUserAPC(GetThreadTopFrameCallback, &apcContext);
#else
        thread->GetInternalThread()->handle->QueueUserAPC(GetThreadTopFrameCallback, &apcContext);
#endif
        apcContext.apcDoneEvent.Wait();

        return apcContext.hasResult;
#else
        return false;
#endif
    }
}
}
