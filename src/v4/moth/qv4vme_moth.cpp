#include "qv4vme_moth_p.h"
#include "qv4instr_moth_p.h"
#include "qmljs_value.h"
#include "debugging.h"

#include <iostream>

#include "qv4alloca_p.h"

#ifdef DO_TRACE_INSTR
#  define TRACE_INSTR(I) fprintf(stderr, "executing a %s\n", #I);
#  define TRACE(n, str, ...) { char buf[4096]; snprintf(buf, 4096, str, __VA_ARGS__); fprintf(stderr, "    %s : %s\n", #n, buf); }
#else
#  define TRACE_INSTR(I)
#  define TRACE(n, str, ...)
#endif // DO_TRACE_INSTR

using namespace QQmlJS;
using namespace QQmlJS::Moth;

class FunctionState: public Debugging::FunctionState
{
public:
    FunctionState(QQmlJS::VM::ExecutionContext *context, const uchar **code)
        : Debugging::FunctionState(context)
        , stack(0)
        , stackSize(0)
        , code(code)
    {}

    virtual VM::Value *temp(unsigned idx) { return stack + idx; }

    void setStack(VM::Value *stack, unsigned stackSize)
    { this->stack = stack; this->stackSize = stackSize; }

private:
    VM::Value *stack;
    unsigned stackSize;
    const uchar **code;
};

#define MOTH_BEGIN_INSTR_COMMON(I) { \
    const InstrMeta<(int)Instr::I>::DataType &instr = InstrMeta<(int)Instr::I>::data(*genericInstr); \
    code += InstrMeta<(int)Instr::I>::Size; \
    Q_UNUSED(instr); \
    TRACE_INSTR(I)

#ifdef MOTH_THREADED_INTERPRETER

#  define MOTH_BEGIN_INSTR(I) op_##I: \
    MOTH_BEGIN_INSTR_COMMON(I)

#  define MOTH_NEXT_INSTR(I) { \
    genericInstr = reinterpret_cast<const Instr *>(code); \
    goto *genericInstr->common.code; \
    }

#  define MOTH_END_INSTR(I) } \
    genericInstr = reinterpret_cast<const Instr *>(code); \
    goto *genericInstr->common.code; \

#else

#  define MOTH_BEGIN_INSTR(I) \
    case Instr::I: \
    MOTH_BEGIN_INSTR_COMMON(I)

#  define MOTH_NEXT_INSTR(I) { \
    break; \
    }

#  define MOTH_END_INSTR(I) } \
    break;

#endif

static inline VM::Value *getValueRef(QQmlJS::VM::ExecutionContext *context,
                                     VM::Value* stack,
                                     const Instr::Param &param
#if !defined(QT_NO_DEBUG)
                                     , unsigned stackSize
#endif
                                     )
{
#ifdef DO_TRACE_INSTR
    if (param.isValue()) {
        fprintf(stderr, "    value\n");
    } else if (param.isArgument()) {
        fprintf(stderr, "    argument %d\n", param.index);
    } else if (param.isLocal()) {
        fprintf(stderr, "    local %d\n", param.index);
    } else if (param.isTemp()) {
        fprintf(stderr, "    temp %d\n", param.index);
    } else {
        Q_ASSERT(!"INVALID");
    }
#endif // DO_TRACE_INSTR

    if (param.isValue()) {
        return const_cast<VM::Value *>(&param.value);
    } else if (param.isArgument()) {
        const unsigned arg = param.index;
        Q_ASSERT(arg >= 0);
        Q_ASSERT((unsigned) arg < context->argumentCount);
        Q_ASSERT(context->arguments);
        return context->arguments + arg;
    } else if (param.isLocal()) {
        const unsigned index = param.index;
        Q_ASSERT(index >= 0);
        Q_ASSERT(index < context->variableCount());
        Q_ASSERT(context->locals);
        return context->locals + index;
    } else if (param.isTemp()) {
        Q_ASSERT(param.index < stackSize);
        return stack + param.index;
    } else {
        Q_UNIMPLEMENTED();
        return 0;
    }
}

#if defined(QT_NO_DEBUG)
# define VALUE(param) *getValueRef(context, stack, param)
# define VALUEPTR(param) getValueRef(context, stack, param)
#else
# define VALUE(param) *getValueRef(context, stack, param, stackSize)
# define VALUEPTR(param) getValueRef(context, stack, param, stackSize)
#endif

VM::Value VME::operator()(QQmlJS::VM::ExecutionContext *context, const uchar *code
#ifdef MOTH_THREADED_INTERPRETER
        , void ***storeJumpTable
#endif
        )
{
#ifdef DO_TRACE_INSTR
    qDebug("Starting VME with context=%p and code=%p", context, code);
#endif // DO_TRACE_INSTR

#ifdef MOTH_THREADED_INTERPRETER
    if (storeJumpTable) {
#define MOTH_INSTR_ADDR(I, FMT) &&op_##I,
        static void *jumpTable[] = {
            FOR_EACH_MOTH_INSTR(MOTH_INSTR_ADDR)
        };
#undef MOTH_INSTR_ADDR
        *storeJumpTable = jumpTable;
        return VM::Value::undefinedValue();
    }
#endif

    VM::Value *stack = 0;
    unsigned stackSize = 0;
    FunctionState state(context, &code);

#ifdef MOTH_THREADED_INTERPRETER
    const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
    goto *genericInstr->common.code;
#else
    for (;;) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
        switch (genericInstr->common.instructionType) {
#endif

    MOTH_BEGIN_INSTR(MoveTemp)
        VALUE(instr.result) = VALUE(instr.source);
    MOTH_END_INSTR(MoveTemp)

    MOTH_BEGIN_INSTR(LoadValue)
//        TRACE(value, "%s", instr.value.toString(context)->toQString().toUtf8().constData());
        VALUE(instr.result) = VALUE(instr.value);
    MOTH_END_INSTR(LoadValue)

    MOTH_BEGIN_INSTR(LoadClosure)
        VALUE(instr.result) = __qmljs_init_closure(instr.value, context);
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        TRACE(inline, "property name = %s", instr.name->toQString().toUtf8().constData());
        __qmljs_get_activation_property(context, VALUEPTR(instr.result), instr.name);
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(StoreName)
        TRACE(inline, "property name = %s", instr.name->toQString().toUtf8().constData());
        __qmljs_set_activation_property(context, instr.name, VALUEPTR(instr.source));
    MOTH_END_INSTR(StoreName)

    MOTH_BEGIN_INSTR(LoadElement)
        VALUE(instr.result) = __qmljs_get_element(context, VALUE(instr.base), VALUE(instr.index));
    MOTH_END_INSTR(LoadElement)

    MOTH_BEGIN_INSTR(StoreElement)
        __qmljs_set_element(context, VALUE(instr.base), VALUE(instr.index), VALUE(instr.source));
    MOTH_END_INSTR(StoreElement)

    MOTH_BEGIN_INSTR(LoadProperty)
        __qmljs_get_property(context, VALUEPTR(instr.result), VALUEPTR(instr.base), instr.name);
    MOTH_END_INSTR(LoadProperty)

    MOTH_BEGIN_INSTR(StoreProperty)
        __qmljs_set_property(context, VALUEPTR(instr.base), instr.name, VALUEPTR(instr.source));
    MOTH_END_INSTR(StoreProperty)

    MOTH_BEGIN_INSTR(Push)
        TRACE(inline, "stack size: %u", instr.value);
        stackSize = instr.value;
        stack = static_cast<VM::Value *>(alloca(stackSize * sizeof(VM::Value)));
        state.setStack(stack, stackSize);
    MOTH_END_INSTR(Push)

    MOTH_BEGIN_INSTR(CallValue)
#ifdef DO_TRACE_INSTR
        if (Debugging::Debugger *debugger = context->engine->debugger) {
            if (VM::FunctionObject *o = (VALUE(instr.dest)).asFunctionObject()) {
                if (Debugging::FunctionDebugInfo *info = debugger->debugInfo(o)) {
                    QString n = debugger->name(o);
                    std::cerr << "*** Call to \"" << (n.isNull() ? "<no name>" : qPrintable(n)) << "\" defined @" << info->startLine << ":" << info->startColumn << std::endl;
                }
            }
        }
#endif // DO_TRACE_INSTR
        Q_ASSERT(instr.args + instr.argc < stackSize);
        VM::Value *args = stack + instr.args;
        VALUE(instr.result) = __qmljs_call_value(context, VM::Value::undefinedValue(), VALUE(instr.dest), args, instr.argc);
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        TRACE(property name, "%s, args=%u, argc=%u", qPrintable(instr.name->toQString()), instr.args, instr.argc);
        Q_ASSERT(instr.args + instr.argc < stackSize);
        VM::Value *args = stack + instr.args;
        VALUE(instr.result) = __qmljs_call_property(context, VALUE(instr.base), instr.name, args, instr.argc);
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallElement)
        Q_ASSERT(instr.args + instr.argc < stackSize);
        VM::Value *args = stack + instr.args;
        VALUE(instr.result) = __qmljs_call_element(context, VALUE(instr.base), VALUE(instr.index), args, instr.argc);
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallActivationProperty)
        Q_ASSERT(instr.args + instr.argc < stackSize);
        VM::Value *args = stack + instr.args;
        __qmljs_call_activation_property(context, VALUEPTR(instr.result), instr.name, args, instr.argc);
    MOTH_END_INSTR(CallActivationProperty)

    MOTH_BEGIN_INSTR(CallBuiltinThrow)
        __qmljs_builtin_throw(VALUE(instr.arg), context);
    MOTH_END_INSTR(CallBuiltinThrow)

    MOTH_BEGIN_INSTR(CallBuiltinCreateExceptionHandler)
        void *buf = __qmljs_create_exception_handler(context);
        // The result is the only value we need from the instr to
        // continue execution when an exception is caught.
        VM::Value *result = getValueRef(context, stack, instr.result
#if !defined(QT_NO_DEBUG)
                                        , stackSize
#endif
                                        );
        VM::ExecutionContext *oldContext = context;
        int didThrow = setjmp(* static_cast<jmp_buf *>(buf));
        context = oldContext;
        // Two ways to come here: after a create, or after a throw.
        if (didThrow)
            // At this point, the interpreter state can be anything but
            // valid, so first restore the state.
            restoreState(context, result, code);
        else
            // Save the state and any variables we need when catching an
            // exception, so we can restore the state at that point.
            saveState(context, result, code);
        *result = VM::Value::fromInt32(didThrow);
    MOTH_END_INSTR(CallBuiltinCreateExceptionHandler)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteExceptionHandler)
        __qmljs_delete_exception_handler(context);
    MOTH_END_INSTR(CallBuiltinDeleteExceptionHandler)

    MOTH_BEGIN_INSTR(CallBuiltinGetException)
        VALUE(instr.result) = __qmljs_get_exception(context);
    MOTH_END_INSTR(CallBuiltinGetException)

    MOTH_BEGIN_INSTR(CallBuiltinPushScope)
        context = __qmljs_builtin_push_with_scope(VALUE(instr.arg), context);
    MOTH_END_INSTR(CallBuiltinPushScope)

    MOTH_BEGIN_INSTR(CallBuiltinPushCatchScope)
         context = __qmljs_builtin_push_catch_scope(instr.varName, context);
    MOTH_END_INSTR(CallBuiltinPushCatchScope)

    MOTH_BEGIN_INSTR(CallBuiltinPopScope)
        context = __qmljs_builtin_pop_scope(context);
    MOTH_END_INSTR(CallBuiltinPopScope)

    MOTH_BEGIN_INSTR(CallBuiltinForeachIteratorObject)
        VALUE(instr.result) = __qmljs_foreach_iterator_object(VALUE(instr.arg), context);
    MOTH_END_INSTR(CallBuiltinForeachIteratorObject)

    MOTH_BEGIN_INSTR(CallBuiltinForeachNextPropertyName)
        VALUE(instr.result) = __qmljs_foreach_next_property_name(VALUE(instr.arg));
    MOTH_END_INSTR(CallBuiltinForeachNextPropertyName)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteMember)
        VALUE(instr.result) = __qmljs_delete_member(context, VALUE(instr.base), instr.member);
    MOTH_END_INSTR(CallBuiltinDeleteMember)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteSubscript)
        VALUE(instr.result) = __qmljs_delete_subscript(context, VALUE(instr.base), VALUE(instr.index));
    MOTH_END_INSTR(CallBuiltinDeleteSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteName)
        VALUE(instr.result) = __qmljs_delete_name(context, instr.name);
    MOTH_END_INSTR(CallBuiltinDeleteName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofMember)
        VALUE(instr.result) = __qmljs_builtin_typeof_member(VALUE(instr.base), instr.member, context);
    MOTH_END_INSTR(CallBuiltinTypeofMember)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofSubscript)
        VALUE(instr.result) = __qmljs_builtin_typeof_element(VALUE(instr.base), VALUE(instr.index), context);
    MOTH_END_INSTR(CallBuiltinTypeofSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofName)
        VALUE(instr.result) = __qmljs_builtin_typeof_name(instr.name, context);
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofValue)
        VALUE(instr.result) = __qmljs_builtin_typeof(VALUE(instr.value), context);
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinPostIncMember)
        VALUE(instr.result) = __qmljs_builtin_post_increment_member(VALUE(instr.base), instr.member, context);
    MOTH_END_INSTR(CallBuiltinTypeofMember)

    MOTH_BEGIN_INSTR(CallBuiltinPostIncSubscript)
        VALUE(instr.result) = __qmljs_builtin_post_increment_element(VALUE(instr.base), VALUE(instr.index), context);
    MOTH_END_INSTR(CallBuiltinTypeofSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinPostIncName)
        VALUE(instr.result) = __qmljs_builtin_post_increment_name(instr.name, context);
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinPostIncValue)
        VALUE(instr.result) = __qmljs_builtin_post_increment(VALUEPTR(instr.value), context);
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinPostDecMember)
        VALUE(instr.result) = __qmljs_builtin_post_decrement_member(VALUE(instr.base), instr.member, context);
    MOTH_END_INSTR(CallBuiltinTypeofMember)

    MOTH_BEGIN_INSTR(CallBuiltinPostDecSubscript)
        VALUE(instr.result) = __qmljs_builtin_post_decrement_element(VALUE(instr.base), VALUE(instr.index), context);
    MOTH_END_INSTR(CallBuiltinTypeofSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinPostDecName)
        VALUE(instr.result) = __qmljs_builtin_post_decrement_name(instr.name, context);
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinPostDecValue)
        VALUE(instr.result) = __qmljs_builtin_post_decrement(VALUEPTR(instr.value), context);
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinDeclareVar)
        __qmljs_builtin_declare_var(context, instr.isDeletable, instr.varName);
    MOTH_END_INSTR(CallBuiltinDeclareVar)

    MOTH_BEGIN_INSTR(CallBuiltinDefineGetterSetter)
        __qmljs_builtin_define_getter_setter(VALUE(instr.object), instr.name, VALUE(instr.getter), VALUE(instr.setter), context);
    MOTH_END_INSTR(CallBuiltinDefineGetterSetter)

    MOTH_BEGIN_INSTR(CallBuiltinDefineProperty)
        __qmljs_builtin_define_property(VALUE(instr.object), instr.name, VALUE(instr.value), context);
    MOTH_END_INSTR(CallBuiltinDefineProperty)

    MOTH_BEGIN_INSTR(CallBuiltinDefineArrayProperty)
        __qmljs_builtin_define_array_property(VALUE(instr.object), instr.index, VALUE(instr.value), context);
    MOTH_END_INSTR(CallBuiltinDefineArrayProperty)

    MOTH_BEGIN_INSTR(CreateValue)
        Q_ASSERT(instr.args + instr.argc < stackSize);
        VM::Value *args = stack + instr.args;
        VALUE(instr.result) = __qmljs_construct_value(context, VALUE(instr.func), args, instr.argc);
    MOTH_END_INSTR(CreateValue)

    MOTH_BEGIN_INSTR(CreateProperty)
        Q_ASSERT(instr.args + instr.argc < stackSize);
        VM::Value *args = stack + instr.args;
        VALUE(instr.result) = __qmljs_construct_property(context, VALUE(instr.base), instr.name, args, instr.argc);
    MOTH_END_INSTR(CreateProperty)

    MOTH_BEGIN_INSTR(CreateActivationProperty)
        TRACE(inline, "property name = %s, args = %d, argc = %d", instr.name->toQString().toUtf8().constData(), instr.args, instr.argc);
        Q_ASSERT(instr.args + instr.argc < stackSize);
        VM::Value *args = stack + instr.args;
        __qmljs_construct_activation_property(context, VALUEPTR(instr.result), instr.name, args, instr.argc);
    MOTH_END_INSTR(CreateActivationProperty)

    MOTH_BEGIN_INSTR(Jump)
        code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(CJump)
        uint cond = __qmljs_to_boolean(VALUE(instr.condition), context);
        TRACE(condition, "%s", cond ? "TRUE" : "FALSE");
        if (cond)
            code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(CJump)

    MOTH_BEGIN_INSTR(Unop)
        VALUE(instr.result) = instr.alu(VALUE(instr.source), context);
    MOTH_END_INSTR(Unop)

    MOTH_BEGIN_INSTR(Binop)
        instr.alu(context, VALUEPTR(instr.result), VALUEPTR(instr.lhs), VALUEPTR(instr.rhs));
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(Ret)
        VM::Value &result = VALUE(instr.result);
//        TRACE(Ret, "returning value %s", result.toString(context)->toQString().toUtf8().constData());
        return result;
    MOTH_END_INSTR(Ret)

    MOTH_BEGIN_INSTR(LoadThis)
        VALUE(instr.result) = __qmljs_get_thisObject(context);
    MOTH_END_INSTR(LoadThis)

    MOTH_BEGIN_INSTR(InplaceElementOp)
        instr.alu(context,
                  VALUEPTR(instr.base),
                  VALUEPTR(instr.index),
                  VALUEPTR(instr.source));
    MOTH_END_INSTR(InplaceElementOp)

    MOTH_BEGIN_INSTR(InplaceMemberOp)
        instr.alu(context,
                  VALUEPTR(instr.base),
                  instr.member,
                  VALUEPTR(instr.source));
    MOTH_END_INSTR(InplaceMemberOp)

    MOTH_BEGIN_INSTR(InplaceNameOp)
        TRACE(name, "%s", instr.name->toQString().toUtf8().constData());
        instr.alu(context, instr.name, VALUEPTR(instr.source));
    MOTH_END_INSTR(InplaceNameOp)

#ifdef MOTH_THREADED_INTERPRETER
    // nothing to do
#else
        default:
            qFatal("QQmlJS::Moth::VME: Internal error - unknown instruction %d", genericInstr->common.instructionType);
            break;
        }
    }
#endif

}

#ifdef MOTH_THREADED_INTERPRETER
void **VME::instructionJumpTable()
{
    static void **jumpTable = 0;
    if (!jumpTable) {
        VME dummy;
        dummy(0, 0, &jumpTable);
    }
    return jumpTable;
}
#endif

VM::Value VME::exec(VM::ExecutionContext *ctxt, const uchar *code)
{
    VME vme;
    return vme(ctxt, code);
}

void VME::restoreState(VM::ExecutionContext *context, VM::Value *&target, const uchar *&code)
{
    VM::ExecutionEngine::ExceptionHandler &handler = context->engine->unwindStack.last();
    target = handler.target;
    code = handler.code;
}

void VME::saveState(VM::ExecutionContext *context, VM::Value *target, const uchar *code)
{
    VM::ExecutionEngine::ExceptionHandler &handler = context->engine->unwindStack.last();
    handler.target = target;
    handler.code = code;
}
