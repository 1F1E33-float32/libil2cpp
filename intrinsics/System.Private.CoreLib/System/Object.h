#pragma once
#include "il2cpp-config.h"

#include "codegen/il2cpp-codegen-metadata.h"
#include "codegen/il2cpp-codegen.h"
namespace il2cpp
{
namespace intrinsics
{
namespace System_Private_CoreLib
{
namespace System
{
    /* METHOD MAPPING
        Method: System.Type System.Object::GetType()
        AssemblyName: System.Private.CoreLib
    */
    inline Type_t* il2cpp_intrinsic_object_get_type(Il2CppObject* thisPtr)
    {
        return il2cpp_codegen_class_get_type_object(thisPtr->klass);
    }

    /* METHOD MAPPING
        Method: System.Boolean System.Object::ReferenceEquals(System.Object,System.Object)
        AssemblyName: System.Private.CoreLib
    */
    inline bool il2cpp_intrinsic_object_reference_equals(const RuntimeObject* objA, const RuntimeObject* objB)
    {
        return objA == objB;
    }
} // namespace System
} // namespace System_Private_CoreLib
} // namespace intrinsics
} // namespace il2cpp
