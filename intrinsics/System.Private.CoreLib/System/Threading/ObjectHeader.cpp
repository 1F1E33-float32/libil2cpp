#include "ObjectHeader.h"
#include "vm/Monitor.h"
#include "vm/Object.h"

namespace il2cpp
{
namespace intrinsics
{
namespace System_Private_CoreLib
{
namespace System
{
namespace Threading
{
    bool il2cpp_intrinsic_object_header_has_owner(RuntimeObject* obj)
    {
        return il2cpp::vm::Monitor::IsAcquired(obj);
    }

    bool il2cpp_intrinsic_object_header_try_get_hash_code(RuntimeObject* obj, int32_t* hash)
    {
        // Mono caches the hash code in the object header because its collector moves objects. il2cpp's
        // collectors never do, so the address derived hash is always available and always the same, and
        // the managed implementation - which reads mono's lock word - would always report "no hash
        // assigned". Object::GetHash never returns 0 for a non-null object, which callers such as
        // ConditionalWeakTable.Container.FindEntry rely on: they read 0 as "this key has never been
        // hashed, so it cannot be in the table".
        *hash = obj != NULL ? il2cpp::vm::Object::GetHash(obj) : 0;
        return true;
    }

    bool il2cpp_intrinsic_object_header_is_entered(RuntimeObject* obj)
    {
        return il2cpp::vm::Monitor::IsOwnedByCurrentThread(obj);
    }
} // namespace Threading
} // namespace System
} // namespace System_Private_CoreLib
} // namespace intrinsics
} // namespace il2cpp
