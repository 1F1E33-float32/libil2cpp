#include "il2cpp-config.h"
#include "il2cpp-runtime-metadata.h"
#include "il2cpp-class-internals.h"
#include "il2cpp-object-internals.h"
#include "Parameter.h"
#include "vm-utils/BlobReader.h"
#include "vm/Class.h"
#include "vm/Object.h"
#include "vm/Method.h"

namespace il2cpp
{
namespace vm
{
    static Il2CppObject* BoxDefaultParameterValue(const Il2CppImage* image, Il2CppClass* boxedType, const Il2CppType* typeOfDefaultValue, const char* data)
    {
        Class::SetupFields(boxedType);
        IL2CPP_ASSERT(boxedType->size_inited);
        void* value = alloca(boxedType->instance_size - sizeof(Il2CppObject));
        utils::BlobReader::GetConstantValueFromBlob(image, typeOfDefaultValue->type, data, value);
        return Object::Box(boxedType, value);
    }

    Il2CppObject* Parameter::GetDefaultParameterValueObject(const MethodInfo* method, int32_t parameterPosition, bool* isExplicitlySetNullDefaultValue)
    {
        const Il2CppType* typeOfDefaultValue;
        const char* data = Method::GetParameterDefaultValue(method, parameterPosition, &typeOfDefaultValue, isExplicitlySetNullDefaultValue);
        if (data == NULL)
            return NULL;

        Il2CppClass* parameterType = Class::FromIl2CppType(method->parameters[parameterPosition]);
        if (il2cpp::vm::Class::IsValuetype(parameterType))
        {
            if (il2cpp::vm::Class::IsNullable(parameterType))
            {
                parameterType = il2cpp::vm::Class::GetNullableArgument(parameterType);
                typeOfDefaultValue = il2cpp::vm::Class::IsEnum(parameterType) ? il2cpp::vm::Class::GetEnumBaseType(parameterType) : &parameterType->byval_arg;
            }

            return BoxDefaultParameterValue(method->klass->image, parameterType, typeOfDefaultValue, data);
        }

        // A reference type parameter can have a value type default value: [DefaultParameterValue(-1)] object x gives an object
        // parameter an int, which is boxed as its own type.
        Il2CppClass* defaultValueType = Class::FromIl2CppType(typeOfDefaultValue);
        if (Class::IsValuetype(defaultValueType))
            return BoxDefaultParameterValue(method->klass->image, defaultValueType, typeOfDefaultValue, data);

        Il2CppObject* value = NULL;
        utils::BlobReader::GetConstantValueFromBlob(method->klass->image, typeOfDefaultValue->type, data, &value);
        return value;
    }
}
}
