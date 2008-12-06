#include <sys/types.h>
#include <sys/param.h>
#include <stdint.h>
#include <stdbool.h>
#include <ruby.h>
#include <st.h>
#include "rbffi.h"
#include "compat.h"
#include "AbstractMemory.h"
#include "Pointer.h"
#include "Struct.h"

typedef struct StructField {
    unsigned int type;
    unsigned int offset;
    unsigned int size;
    unsigned int align;
} StructField;

typedef struct StructLayout {
    VALUE rbFields;
    unsigned int fieldCount;
    st_table* symbolMap;
    st_table* stringMap;
} StructLayout;

typedef struct StructLayoutBuilder {
    unsigned int offset;
    st_table* fieldMap;
} StructLayoutBuilder;

typedef struct Struct {
    StructLayout* layout;
    AbstractMemory* pointer;
    VALUE rbPointer;
} Struct;

static void struct_field_mark(StructField *);
static void struct_field_free(StructField *);
static void struct_mark(Struct *);
static void struct_free(Struct *);
static void struct_layout_mark(StructLayout *);
static void struct_layout_free(StructLayout *);

static VALUE classBaseStruct = Qnil, classStructLayout = Qnil;
static VALUE classStructField = Qnil, classStructLayoutBuilder = Qnil;
static ID initializeID = 0, pointerID = 0, layoutID = 0;
static ID getID = 0, putID = 0;

static VALUE
struct_layout_builder_new(VALUE self)
{

}

static VALUE
struct_layout_builder_add(int argc, VALUE* argv, VALUE self)
{

}

static VALUE
struct_field_new(int argc, VALUE* argv, VALUE klass)
{
    VALUE offset = Qnil, info = Qnil;
    StructField* field;
    VALUE retval;
    int nargs;
    nargs = rb_scan_args(argc, argv, "11", &offset, &info);
    retval = Data_Make_Struct(klass, StructField, struct_field_mark, struct_field_free, field);
    field->offset = NUM2UINT(offset);
    rb_iv_set(retval, "@off", offset);
    rb_iv_set(retval, "@info", info);
    return retval;
}

static VALUE
struct_field_offset(VALUE self)
{
    StructField* field = (StructField *) DATA_PTR(self);
    return UINT2NUM(field->offset);
}

static void
struct_field_mark(StructField *f)
{
}
static void
struct_field_free(StructField *f)
{
    if (f != NULL) {
        xfree(f);
    }
}

#define NUM_OP(name, type, toNative, fromNative) \
static VALUE struct_field_put_##name(VALUE self, VALUE pointer, VALUE value); \
static inline void ptr_put_##name(AbstractMemory* ptr, StructField* field, VALUE value); \
static inline VALUE ptr_get_##name(AbstractMemory* ptr, StructField* field); \
static inline void \
ptr_put_##name(AbstractMemory* ptr, StructField* field, VALUE value) \
{ \
    type tmp = toNative(value); \
    memcpy(ptr->address + field->offset, &tmp, sizeof(tmp)); \
} \
static inline VALUE \
ptr_get_##name(AbstractMemory* ptr, StructField* field) \
{ \
    type tmp; \
    memcpy(&tmp, ptr->address + field->offset, sizeof(tmp)); \
    return fromNative(tmp); \
} \
static VALUE \
struct_field_put_##name(VALUE self, VALUE pointer, VALUE value) \
{ \
    ptr_put_##name((AbstractMemory *) DATA_PTR(pointer), (StructField *) DATA_PTR(self), value); \
    return self; \
} \
static VALUE struct_field_get_##name(VALUE self, VALUE pointer); \
static VALUE \
struct_field_get_##name(VALUE self, VALUE pointer) \
{ \
    return ptr_get_##name((AbstractMemory *) DATA_PTR(pointer), (StructField *) DATA_PTR(self)); \
}
NUM_OP(int8, int8_t, NUM2INT, INT2NUM);
NUM_OP(uint8, u_int8_t, NUM2UINT, UINT2NUM);
NUM_OP(int16, int16_t, NUM2INT, INT2NUM);
NUM_OP(uint16, u_int16_t, NUM2UINT, UINT2NUM);
NUM_OP(int32, int32_t, NUM2INT, INT2NUM);
NUM_OP(uint32, u_int32_t, NUM2UINT, UINT2NUM);
NUM_OP(int64, int64_t, NUM2LL, LL2NUM);
NUM_OP(uint64, u_int64_t, NUM2ULL, ULL2NUM);
NUM_OP(float32, float, NUM2DBL, rb_float_new);
NUM_OP(float64, double, NUM2DBL, rb_float_new);

static VALUE
struct_new(int argc, VALUE* argv, VALUE klass)
{
    VALUE offset = Qnil, info = Qnil;
    Struct* s;
    VALUE retval;

    retval = Data_Make_Struct(klass, Struct, struct_mark, struct_free, s);
    s->rbPointer = Qnil;
    s->layout = NULL;
    s->pointer = NULL;
    rb_funcall2(retval, initializeID, argc, argv);
    return retval;
}
static void
struct_mark(Struct *s)
{
    if (s->rbPointer != Qnil) {
        rb_gc_mark(s->rbPointer);
    }
}
static void struct_free(Struct *s)
{
    xfree(s);
}
static VALUE
struct_field(Struct* s, VALUE fieldName)
{
    StructLayout* layout = s->layout;
    VALUE rbField;
    if (layout == NULL) {
        rb_raise(rb_eRuntimeError, "layout not set for Struct");
    }
    rbField = rb_hash_aref(layout->rbFields, fieldName);
    if (rbField == Qnil) {
        VALUE str = rb_funcall2(fieldName, rb_intern("to_s"), 0, NULL);
        rb_raise(rb_eArgError, "No such field '%s'", StringValuePtr(str));
    }
    return rbField;
}

static VALUE
struct_get_field(VALUE self, VALUE fieldName)
{
    Struct* s = (Struct *) DATA_PTR(self);
    VALUE rbField = struct_field(s, fieldName);
    return rb_funcall2(rbField, getID, 1, &s->rbPointer);
}

static VALUE
struct_put_field(VALUE self, VALUE fieldName, VALUE value)
{
    Struct* s = (Struct *) DATA_PTR(self);
    VALUE rbField = struct_field(s, fieldName);
    VALUE argv[] = { s->rbPointer, value };
    return rb_funcall2(rbField, putID, 2, argv);
}

static VALUE
struct_set_pointer(VALUE self, VALUE pointer)
{
    Struct* s = (Struct *) DATA_PTR(self);
    if (!rb_obj_is_kind_of(pointer, rb_FFI_AbstractMemory_class)) {
        rb_raise(rb_eArgError, "Invalid pointer");
    }
    s->pointer = (AbstractMemory *) DATA_PTR(pointer);
    s->rbPointer = pointer;
    rb_ivar_set(self, pointerID, pointer);
    return self;
}

static VALUE
struct_set_layout(VALUE self, VALUE layout)
{
    Struct* s = (Struct *) DATA_PTR(self);
    if (rb_obj_is_kind_of(layout, classStructLayout)) {
        s->layout = (StructLayout *) DATA_PTR(layout);
    }
    rb_ivar_set(self, layoutID, layout);
    return self;
}

static VALUE
struct_layout_new(VALUE klass, VALUE fields, VALUE size)
{
    StructLayout* layout;
    VALUE retval;
    VALUE argv[] = { fields, size };
    retval = Data_Make_Struct(klass, StructLayout, struct_layout_mark, struct_layout_free, layout);
    layout->rbFields = fields;
    rb_funcall2(retval, initializeID, 2, argv);
    return retval;
}

static void
struct_layout_mark(StructLayout *layout)
{
    if (layout->rbFields != Qnil) {
        rb_gc_mark(layout->rbFields);
    }
}
static void
struct_layout_free(StructLayout *layout)
{
    xfree(layout);
}

static VALUE
struct_layout_get(VALUE self, VALUE field)
{
    StructLayout* layout = (StructLayout *) DATA_PTR(self);
    return rb_hash_aref(layout->rbFields, field);
}
void
rb_FFI_Struct_Init()
{
    VALUE moduleFFI = rb_define_module("FFI");
    VALUE klass;
    classBaseStruct = rb_define_class_under(moduleFFI, "BaseStruct", rb_cObject);
    classStructLayout = rb_define_class_under(moduleFFI, "StructLayout", rb_cObject);
    classStructLayoutBuilder = rb_define_class_under(moduleFFI, "StructLayoutBuilder", rb_cObject);
    classStructField = rb_define_class_under(classStructLayoutBuilder, "Field", rb_cObject);

    //rb_define_singleton_method(classStructLayoutBuilder, "new", builder_new, 0);
    rb_define_singleton_method(classBaseStruct, "new", struct_new, -1);
    rb_define_private_method(classBaseStruct, "pointer=", struct_set_pointer, 1);
    rb_define_attr(classBaseStruct, "pointer", 1, 0);
    rb_define_private_method(classBaseStruct, "layout=", struct_set_layout, 1);
    rb_define_attr(classBaseStruct, "layout", 1, 0);
    rb_define_method(classBaseStruct, "[]", struct_get_field, 1);
    rb_define_method(classBaseStruct, "[]=", struct_put_field, 2);
    rb_define_singleton_method(classStructField, "new", struct_field_new, -1);
    rb_define_method(classStructField, "offset", struct_field_offset, 0);
    rb_define_singleton_method(classStructLayout, "new", struct_layout_new, 2);
    rb_define_method(classStructLayout, "[]", struct_layout_get, 1);
    initializeID = rb_intern("initialize");
    pointerID = rb_intern("@pointer");
    layoutID = rb_intern("@layout");
    getID = rb_intern("get");
    putID = rb_intern("put");
#undef NUM_OP
#define NUM_OP(name, typeName, T) do { \
    typedef struct { char c; T v; } s; \
    klass = rb_define_class_under(classStructLayoutBuilder, #name, classStructField); \
    rb_define_method(klass, "put", struct_field_put_##typeName, 2); \
    rb_define_method(klass, "get", struct_field_get_##typeName, 1); \
    rb_define_const(klass, "ALIGN", INT2NUM((sizeof(s) - sizeof(T)) * 8)); \
    rb_define_const(klass, "SIZE", INT2NUM(sizeof(T)* 8)); \
    } while(0)
    
    NUM_OP(Signed8, int8, char);
    NUM_OP(Unsigned8, uint8, unsigned char);
    NUM_OP(Signed16, int16, short);
    NUM_OP(Unsigned16, uint16, unsigned short);
    NUM_OP(Signed32, int32, int);
    NUM_OP(Unsigned32, uint32, unsigned int);
    NUM_OP(Signed64, int64, long long);
    NUM_OP(Unsigned64, uint64, unsigned long long);
    NUM_OP(FloatField, float32, float);
    NUM_OP(DoubleField, float64, double);
}