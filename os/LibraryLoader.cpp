#include "LibraryLoader.h"
#include "utils/StringUtils.h"
#include "utils/Exception.h"

#include "os/Mutex.h"

namespace il2cpp
{
namespace os
{
    static Il2CppSetFindPlugInCallback s_FindPluginCallback = NULL;

    typedef std::vector<std::pair<std::basic_string<Il2CppNativeChar>, Baselib_DynamicLibrary_Handle> > DllCacheContainer;
    typedef DllCacheContainer::const_iterator DllCacheIterator;
    static DllCacheContainer s_DllCache; // If a library does not need to be closed - do not add it to the cache.
    static baselib::ReentrantLock s_DllCacheMutex;

    static inline Il2CppNativeChar AsciiToLower(Il2CppNativeChar c)
    {
        if (c >= 'A' && c <= 'Z')
            return c - 'A' + 'a';

        return c;
    }

    static bool DoesNativeDynamicLibraryNameMatch(const il2cpp::utils::StringView<Il2CppNativeChar>& desiredLibraryName, const Il2CppNativeChar* hardcodedLibraryName)
    {
        size_t desiredLibraryNameLength = desiredLibraryName.Length();
        for (size_t i = 0; i < desiredLibraryNameLength; i++)
        {
            Il2CppNativeChar desiredCharacter = AsciiToLower(desiredLibraryName[i]);
            Il2CppNativeChar hardcodedCharacter = hardcodedLibraryName[i];

            // Assume hardcodedLibraryName consists of only lower case ascii characters
            IL2CPP_ASSERT(hardcodedCharacter < 128 && (hardcodedCharacter<'A' || hardcodedCharacter> 'Z'));

            if (desiredCharacter != hardcodedCharacter)
            {
                // If we've reached end of our hardcoded dll name, it can still match if we've
                // reached end of desiredLibraryName file name and only the extension is left
                return hardcodedCharacter == 0 &&
                    i + 4 == desiredLibraryNameLength &&
                    desiredLibraryName[i] == '.' &&
                    AsciiToLower(desiredLibraryName[i + 1]) == 'd' &&
                    AsciiToLower(desiredLibraryName[i + 2]) == 'l' &&
                    AsciiToLower(desiredLibraryName[i + 3]) == 'l';
            }
            else if (hardcodedCharacter == 0)
            {
                // We've reached the end of hardcoded library name
                // It's a match if we're at the end of desired library name too
                return i + 1 == desiredLibraryNameLength;
            }
            else if (i == desiredLibraryNameLength - 1)
            {
                // We've reached the end of desired library name
                // It's a match if we're at the end of hardcoded library name too
                return hardcodedLibraryName[i + 1] == 0;
            }
        }

        // We've reached the end of desired library name,
        // but not the end of hardcoded library name.
        // It is not a match.
        return false;
    }

    static const std::basic_string<Il2CppNativeChar>* FindLibraryNameFromHandle(Baselib_DynamicLibrary_Handle nativeDynamicLibraryHandle)
    {
        os::FastAutoLock lock(&s_DllCacheMutex);

        for (DllCacheIterator it = s_DllCache.begin(); it != s_DllCache.end(); it++)
        {
            if (it->second == nativeDynamicLibraryHandle)
                return &it->first;
        }

        return NULL;
    }

    Il2CppMethodPointer LibraryLoader::GetHardcodedPInvokeDependencyFunctionPointer(Baselib_DynamicLibrary_Handle nativeDynamicLibraryHandle, const il2cpp::utils::StringView<char>& entryPoint, Il2CppCharSet charSet)
    {
        const std::basic_string<Il2CppNativeChar>* libraryName = FindLibraryNameFromHandle(nativeDynamicLibraryHandle);
        if (libraryName == NULL)
            return NULL;
        return GetHardcodedPInvokeDependencyFunctionPointer(*libraryName, entryPoint, charSet);
    }

    Il2CppMethodPointer LibraryLoader::GetHardcodedPInvokeDependencyFunctionPointer(const il2cpp::utils::StringView<Il2CppNativeChar>& nativeDynamicLibrary, const il2cpp::utils::StringView<char>& entryPoint, Il2CppCharSet charSet)
    {
        // We don't support, nor do we need to Ansi functions.  That would break forwarding method names to Unicode MoveFileEx -> MoveFileExW
        if (HardcodedPInvokeDependencies == NULL || charSet == CHARSET_ANSI)
            return NULL;

        for (size_t i = 0; i < HardcodedPInvokeDependenciesCount; i++)
        {
            const HardcodedPInvokeDependencyLibrary& library = HardcodedPInvokeDependencies[i];
            if (DoesNativeDynamicLibraryNameMatch(nativeDynamicLibrary, library.libraryName))
            {
                size_t functionCount = library.functionCount;
                for (size_t j = 0; j < functionCount; j++)
                {
                    const HardcodedPInvokeDependencyFunction function = library.functions[j];
                    if (EntryNameMatches(il2cpp::utils::StringView<char>(function.functionName, function.functionNameLen), entryPoint))
                        return function.functionPointer;
                }

                // We assume that kHardcodedPInvokeDependencies will not contain duplicates
                return NULL;
            }
        }

        return NULL;
    }

    Baselib_DynamicLibrary_Handle LibraryLoader::LoadDynamicLibrary(const utils::StringView<Il2CppNativeChar> nativeDynamicLibrary, std::string& detailedError)
    {
        StringViewAsNullTerminatedStringOf(Il2CppNativeChar, nativeDynamicLibrary, libraryName);
        if (s_FindPluginCallback)
            libraryName = s_FindPluginCallback(libraryName);
        auto libraryNameLength = utils::StringUtils::StrLen(libraryName);

        {
            os::FastAutoLock lock(&s_DllCacheMutex);
            for (DllCacheIterator it = s_DllCache.begin(); it != s_DllCache.end(); it++)
                if (it->first.compare(0, std::string::npos, libraryName, libraryNameLength) == 0)
                    return it->second;
        }

        bool needsClosing = true;

        auto handle = Baselib_DynamicLibrary_Handle_Invalid;
        if (libraryName == nullptr || libraryNameLength == 0)
        {
            auto errorState = Baselib_ErrorState_Create();
            handle = OpenProgramHandle(errorState, needsClosing);
            // Disabling it for emscripten builds as they seem to be quite code sensitive
#if (!defined(__EMSCRIPTEN__))
            if (Baselib_ErrorState_ErrorRaised(&errorState))
            {
                if (!detailedError.empty())
                    detailedError += " ";
                detailedError += "Unable to open program handle because of '";
                detailedError += utils::Exception::FormatBaselibErrorState(errorState);
                detailedError += "'.";
            }
#endif
        }
        else
            handle = ProbeForLibrary(libraryName, libraryNameLength, detailedError);

        if ((handle != Baselib_DynamicLibrary_Handle_Invalid) && needsClosing)
        {
            os::FastAutoLock lock(&s_DllCacheMutex);
            s_DllCache.push_back(std::make_pair(libraryName, handle));
        }

        return handle;
    }

    template<typename T>
    static std::make_unsigned_t<T> MakeUnsigned(T value)
    {
        using unsigned_type = std::make_unsigned_t<T>;
        return static_cast<unsigned_type>(value);
    }

    template<typename T>
    static constexpr size_t MaxUnsignedDigitCount()
    {
        constexpr auto maxValue = std::numeric_limits<std::make_unsigned_t<T> >::max();
        static_assert(maxValue != 0, "Expected max value of an unsigned type to not be 0");

        size_t digitCount = 0;
        auto value = maxValue;

        while (value > 0)
        {
            value /= 10;
            digitCount++;
        }

        return digitCount;
    }

    Il2CppMethodPointer LibraryLoader::GetFunctionPointer(Baselib_DynamicLibrary_Handle handle, const PInvokeArguments& pinvokeArgs, std::string& detailedError)
    {
        if (handle == Baselib_DynamicLibrary_Handle_Invalid)
            return NULL;

        StringViewAsNullTerminatedStringOf(char, pinvokeArgs.entryPoint, entryPoint);

        // If it's not cdecl, also try decorating the name with @<parameterSize>
        // THIS ONLY APPLIES TO 32-bit x86!
#if defined(_M_IX86)
        const bool decorateWithParameterSize = pinvokeArgs.callingConvention != IL2CPP_CALL_C;
#else
        const bool decorateWithParameterSize = false;
#endif

        // If there's 'no mangle' flag set, just return directly what GetFunctionPointer returns, unless it needs decorating
        if (pinvokeArgs.isNoMangle && !decorateWithParameterSize)
            return GetFunctionPointer(handle, entryPoint, detailedError);

        // Room after the name for the 'W' or 'A' suffix, '@' and max number of digits for parameter size
        constexpr size_t kBufferOverhead = 2 + MaxUnsignedDigitCount<decltype(pinvokeArgs.parameterSize)>();
        Il2CppMethodPointer func = nullptr;
        size_t originalFuncNameEndPos = strlen(entryPoint) + 1; // We index the string from '1', because we might have to prepend an underscore in case of stdcall mangling
        std::string functionName;

        functionName.resize(originalFuncNameEndPos + kBufferOverhead);
        functionName[0] = '_';
        memcpy(&functionName[1], entryPoint, originalFuncNameEndPos); // Note: this copies the null terminator from `entryPoint`, which looks weird but is fine
        memset(&functionName[1] + originalFuncNameEndPos, 0, kBufferOverhead);

        const auto decorateNameWithParameterSize = [functionNamePtr = &functionName[0], bufferLength = functionName.size() + 1 /* std::string has space for null terminator after the string */, parameterSize = MakeUnsigned(pinvokeArgs.parameterSize)](size_t decorationOffset)
            {
                // We don't expect parameterSize to ever be negative and we don't allocate space for a minus
                // However, in case it is in some really bad circumstance, we don't want to overflow the buffer
                // so we print it as unsigned
                snprintf(functionNamePtr + decorationOffset, bufferLength - decorationOffset, "@%u", parameterSize);
            };

        // If there's no 'dont mangle' flag set, 'W' function takes priority over original name, but 'A' function does not (yes, really)
        if (!pinvokeArgs.isNoMangle && pinvokeArgs.charSet == CHARSET_UNICODE)
        {
            functionName[originalFuncNameEndPos] = 'W';
            if ((func = GetFunctionPointer(handle, functionName.c_str() + 1, detailedError)))
                return func;

            if (decorateWithParameterSize)
            {
                decorateNameWithParameterSize(originalFuncNameEndPos + 1);
                if ((func = GetFunctionPointer(handle, functionName.c_str(), detailedError)))
                    return func;
            }
        }

        // Original name
        if ((func = GetFunctionPointer(handle, entryPoint, detailedError)))
            return func;

        if (decorateWithParameterSize)
        {
            decorateNameWithParameterSize(originalFuncNameEndPos);
            if ((func = GetFunctionPointer(handle, functionName.c_str(), detailedError)))
                return func;
        }

        if (!pinvokeArgs.isNoMangle && pinvokeArgs.charSet != CHARSET_UNICODE)
        {
            functionName[originalFuncNameEndPos] = 'A';
            functionName[originalFuncNameEndPos + 1] = 0; // The decoration above may have overwritten the zero character here
            if ((func = GetFunctionPointer(handle, functionName.c_str() + 1, detailedError)))
                return func;

            if (decorateWithParameterSize)
            {
                decorateNameWithParameterSize(originalFuncNameEndPos + 1);
                if ((func = GetFunctionPointer(handle, functionName.c_str(), detailedError)))
                    return func;
            }
        }

        return nullptr;
    }

    Il2CppMethodPointer LibraryLoader::GetFunctionPointer(Baselib_DynamicLibrary_Handle handle, const char* functionName, std::string& detailedError)
    {
        auto errorState = Baselib_ErrorState_Create();
        if (handle == Baselib_DynamicLibrary_Handle_Invalid)
            return NULL;
        auto func = reinterpret_cast<Il2CppMethodPointer>(Baselib_DynamicLibrary_GetFunction(handle, functionName, &errorState));
#if (!defined(__EMSCRIPTEN__))
        if (Baselib_ErrorState_ErrorRaised(&errorState))
        {
            if (!detailedError.empty())
                detailedError += " ";
            detailedError += "Unable to get function '";
            detailedError += functionName;
            detailedError += "' because of '";
            detailedError += utils::Exception::FormatBaselibErrorState(errorState);
            detailedError += "'.";
        }
#else
        NO_UNUSED_WARNING(detailedError);
#endif
        return func;
    }

    void LibraryLoader::CleanupLoadedLibraries()
    {
        // We assume that presence of the library in s_DllCache is a valid reason to be able to close it
        for (DllCacheIterator it = s_DllCache.begin(); it != s_DllCache.end(); it++)
        {
            // If libc is a "loaded library", it is a special case, and closing it will cause dlclose
            // on some Posix platforms to return an error (I'm looking at you, iOS 11). This really is
            // not an error, but Baselib_DynamicLibrary_Close will correctly assert when dlclose
            // returns an error. To avoid this assert, let's skip closing libc.
            if (utils::StringUtils::NativeStringToUtf8(it->first.c_str()) != "libc")
                Baselib_DynamicLibrary_Close(it->second);
        }
        s_DllCache.clear();
    }

    bool LibraryLoader::CloseLoadedLibrary(Baselib_DynamicLibrary_Handle handle)
    {
        if (handle == Baselib_DynamicLibrary_Handle_Invalid)
            return false;

        os::FastAutoLock lock(&s_DllCacheMutex);
        // We assume that presence of the library in s_DllCache is a valid reason to be able to close it
        for (DllCacheIterator it = s_DllCache.begin(); it != s_DllCache.end(); it++)
        {
            if (it->second == handle)
            {
                Baselib_DynamicLibrary_Close(it->second);
                s_DllCache.erase(it);
                return true;
            }
        }
        return false;
    }

    void LibraryLoader::SetFindPluginCallback(Il2CppSetFindPlugInCallback method)
    {
        s_FindPluginCallback = method;
    }

    Baselib_DynamicLibrary_Handle LibraryLoader::TryOpeningLibrary(const Il2CppNativeChar* libraryName, std::string& detailedError)
    {
#if (defined(__EMSCRIPTEN__))
        // WASM_SUPPORT.md/no-dynamic-linking: We do not use Emscripten/WebAssembly dynamic linking support, due to its various limitations in usefulness.
        NO_UNUSED_WARNING(libraryName);
        NO_UNUSED_WARNING(detailedError);
        return Baselib_DynamicLibrary_Handle_Invalid;
#else
        auto errorState = Baselib_ErrorState_Create();
        auto handle = Baselib_DynamicLibrary_Open(utils::StringUtils::NativeStringToBaselib(libraryName), &errorState);

        if (Baselib_ErrorState_ErrorRaised(&errorState))
        {
            if (!detailedError.empty())
                detailedError += " ";
            detailedError += "Unable to load dynamic library '";
            detailedError += utils::StringUtils::NativeStringToUtf8(libraryName);
            detailedError += "' because of '";
            detailedError += utils::Exception::FormatBaselibErrorState(errorState);
            detailedError += "'.";
        }
        return handle;
#endif
    }
} /* namespace vm */
} /* namespace il2cpp */
