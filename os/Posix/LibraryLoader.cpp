#include "il2cpp-config.h"

#if IL2CPP_TARGET_POSIX && !IL2CPP_USE_PLATFORM_SPECIFIC_LIBRARYLOADER

#include <dlfcn.h>
#include <string>
#include <set>
#if IL2CPP_TARGET_LINUX
#include <unistd.h>
#include <gnu/lib-names.h>
#endif

#include "il2cpp-runtime-metadata.h"
#include "os/LibraryLoader.h"
#include "os/Path.h"
#include "utils/PathUtils.h"
#include "utils/StringUtils.h"

namespace il2cpp
{
namespace os
{
    struct LibraryNamePrefixAndSuffix
    {
        LibraryNamePrefixAndSuffix(const char* prefix_, const char* suffix_)
        {
            prefix = std::string(prefix_);
            suffix = std::string(suffix_);
        }

        std::string prefix;
        std::string suffix;
    };

    static LibraryNamePrefixAndSuffix LibraryNamePrefixAndSuffixVariations[8] =
    {
        LibraryNamePrefixAndSuffix("", ".so"),
        LibraryNamePrefixAndSuffix("", ".dll"),
        LibraryNamePrefixAndSuffix("", ".dylib"),
        LibraryNamePrefixAndSuffix("", ".bundle"),
        LibraryNamePrefixAndSuffix("lib", ".so"),
        LibraryNamePrefixAndSuffix("lib", ".dll"),
        LibraryNamePrefixAndSuffix("lib", ".dylib"),
        LibraryNamePrefixAndSuffix("lib", ".bundle")
    };

    const HardcodedPInvokeDependencyLibrary* LibraryLoader::HardcodedPInvokeDependencies = NULL;
    const size_t LibraryLoader::HardcodedPInvokeDependenciesCount = 0;

    static Baselib_DynamicLibrary_Handle CheckLibraryVariations(const std::string& name, std::string& detailedError)
    {
        // The prefix belongs on the file name, not on any directory in front of it
        const size_t fileNameStart = name.find_last_of('/') + 1;
        const std::string directory = name.substr(0, fileNameStart);
        const std::string fileName = name.substr(fileNameStart);

        const int numberOfVariations = sizeof(LibraryNamePrefixAndSuffixVariations) / sizeof(LibraryNamePrefixAndSuffixVariations[0]);
        for (int i = 0; i < numberOfVariations; ++i)
        {
            std::string libraryName = directory + LibraryNamePrefixAndSuffixVariations[i].prefix + fileName + LibraryNamePrefixAndSuffixVariations[i].suffix;
            auto handle = LibraryLoader::TryOpeningLibrary(libraryName.c_str(), detailedError);
            if (handle != Baselib_DynamicLibrary_Handle_Invalid)
                return handle;
        }

        return Baselib_DynamicLibrary_Handle_Invalid;
    }

    static Baselib_DynamicLibrary_Handle ProbeForLibraryImpl(const std::string& libraryName, std::string& detailedError)
    {
        auto handle = Baselib_DynamicLibrary_Handle_Invalid;

#if IL2CPP_TARGET_LINUX
        // Workaround the fact that on Linux, libc is actually named libc.so.6 instead of libc.so.
        // mscorlib P/Invokes into plain libc, so we need this for those P/Invokes to succeed
        if (strncasecmp(libraryName.c_str(), "libc", libraryName.size()) == 0)
            handle = LibraryLoader::TryOpeningLibrary(LIBC_SO, detailedError);
#endif

        if (handle == Baselib_DynamicLibrary_Handle_Invalid)
            handle = LibraryLoader::TryOpeningLibrary(libraryName.c_str(), detailedError);

        if (handle == Baselib_DynamicLibrary_Handle_Invalid)
            handle = CheckLibraryVariations(libraryName.c_str(), detailedError);

        if (handle == Baselib_DynamicLibrary_Handle_Invalid)
        {
            const size_t lengthWithoutDotDll = libraryName.size() - 4;
            if (strncmp(libraryName.c_str() + lengthWithoutDotDll, ".dll", 4) == 0)
            {
                char* nativeDynamicLibraryWithoutExtension = static_cast<char*>(alloca((lengthWithoutDotDll + 1) * sizeof(char)));
                memcpy(nativeDynamicLibraryWithoutExtension, libraryName.c_str(), lengthWithoutDotDll);
                nativeDynamicLibraryWithoutExtension[lengthWithoutDotDll] = 0;

                handle = CheckLibraryVariations(nativeDynamicLibraryWithoutExtension, detailedError);
            }
        }

        return handle;
    }

    Baselib_DynamicLibrary_Handle LibraryLoader::ProbeForLibrary(const Il2CppNativeChar* libraryName, const size_t libraryNameLength, std::string& detailedError)
    {
        std::string libraryNameStr = utils::StringUtils::NativeStringToUtf8(libraryName, libraryNameLength);

        std::string applicationDir = os::Path::GetApplicationFolder();
        std::string libraryPath = utils::PathUtils::Combine(applicationDir, libraryNameStr);

        // Absolute paths are tried as-is first so the real path and dlopen failure lead the error.
        // The application directory is still probed afterwards because Combine turns "/foo" into
        // "<appDir>//foo", and existing code may rely on that resolving inside the application directory.
        if (libraryNameStr[0] == '/')
        {
            Baselib_DynamicLibrary_Handle handle = ProbeForLibraryImpl(libraryNameStr, detailedError);
            if (handle == Baselib_DynamicLibrary_Handle_Invalid)
                handle = ProbeForLibraryImpl(libraryPath, detailedError);
            return handle;
        }

        // Probe the application directory first - this matches the expected P/Invoke behavior from Win32
        Baselib_DynamicLibrary_Handle handle = ProbeForLibraryImpl(libraryPath, detailedError);

        // Otherwise use the OS's search paths
        if (handle == Baselib_DynamicLibrary_Handle_Invalid)
            handle = ProbeForLibraryImpl(libraryNameStr, detailedError);

        return handle;
    }

    Baselib_DynamicLibrary_Handle LibraryLoader::OpenProgramHandle(Baselib_ErrorState& errorState, bool& /*needsClosing*/)
    {
#if IL2CPP_TARGET_ANDROID && MONO_NET_BCL
        Dl_info info;
        if (dladdr(reinterpret_cast<void*>(&LibraryLoader::OpenProgramHandle), &info) && info.dli_fname)
        {
            void* handle = dlopen(info.dli_fname, RTLD_NOLOAD | RTLD_NOW);
            if (handle)
                return Baselib_DynamicLibrary_FromNativeHandle(reinterpret_cast<uint64_t>(handle), Baselib_DynamicLibrary_PosixDlopen, &errorState);
        }
#endif
        return Baselib_DynamicLibrary_OpenProgramHandle(&errorState);
    }
}
}

#endif
