#include <ruby.h>
#include "Pointer.h"
#include "Types.h"

ffi_type*
rb_FFI_NativeTypeToFFI(NativeType type)
{
    switch (type) {
        case VOID:
            return &ffi_type_void;
        case INT8:
            return &ffi_type_sint8;
        case UINT8:
            return &ffi_type_uint8;
        case INT16:
            return &ffi_type_sint16;
        case UINT16:
            return &ffi_type_uint16;
        case INT32:
            return &ffi_type_sint32;
        case UINT32:
            return &ffi_type_uint32;
        case INT64:
            return &ffi_type_sint64;
        case UINT64:
            return &ffi_type_uint64;
        case FLOAT32:
            return &ffi_type_float;
        case FLOAT64:
            return &ffi_type_double;
        case STRING:
        case RBXSTRING:
        case POINTER:
        case BUFFER_IN:
        case BUFFER_OUT:
        case BUFFER_INOUT:
            return &ffi_type_pointer;
        default:
            return NULL;
    }
}

VALUE
rb_FFI_NativeValueToRuby(NativeType type, const void* ptr)
{
    switch (type) {
        case VOID:
            return Qnil;        
        case INT8:
            return INT2NUM(*(char *) ptr);
        case INT16:
            return INT2NUM(*(short *) ptr);
        case INT32:
            return INT2NUM(*(int *) ptr);
        case UINT8:
            return UINT2NUM(*(unsigned char *) ptr);
        case UINT16:
            return UINT2NUM(*(unsigned short *) ptr);
        case UINT32:
            return UINT2NUM(*(unsigned int *) ptr);
        case INT64:
            return LL2NUM(*(signed long long *) ptr);
        case UINT64:
            return ULL2NUM(*(unsigned long long *) ptr);
        case FLOAT32:
            return rb_float_new(*(float *) ptr);
        case FLOAT64:
            return rb_float_new(*(double *) ptr);
        case STRING:
            return rb_tainted_str_new2(*(char **) ptr);
        case POINTER:
            return rb_FFI_Pointer_new(*(void **) ptr);
        default:
            rb_raise(rb_eRuntimeError, "Unknown type: %d", type);
    }
}