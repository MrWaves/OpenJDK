/*
 * Copyright (c) 2003, 2016, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 Red Hat, Inc.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "asm/assembler.hpp"
#include "interpreter/bytecodeHistogram.hpp"
#include "interpreter/cppInterpreter.hpp"
#include "interpreter/interpreter.hpp"
#include "interpreter/interpreterGenerator.hpp"
#include "interpreter/interpreterRuntime.hpp"
#include "oops/arrayOop.hpp"
#include "oops/methodDataOop.hpp"
#include "oops/methodOop.hpp"
#include "oops/oop.inline.hpp"
#include "prims/jvmtiExport.hpp"
#include "prims/jvmtiThreadState.hpp"
#include "runtime/arguments.hpp"
#include "runtime/deoptimization.hpp"
#include "runtime/frame.inline.hpp"
#include "runtime/interfaceSupport.hpp"
#include "runtime/sharedRuntime.hpp"
#include "runtime/stubRoutines.hpp"
#include "runtime/synchronizer.hpp"
#include "runtime/timer.hpp"
#include "runtime/vframeArray.hpp"
#include "stack_zero.inline.hpp"
#include "utilities/debug.hpp"
#ifdef SHARK
#include "shark/shark_globals.hpp"
#endif

#ifdef CC_INTERP

#define fixup_after_potential_safepoint()       \
  method = istate->method()

#define CALL_VM_NOCHECK_NOFIX(func)             \
  thread->set_last_Java_frame();                \
  func;                                         \
  thread->reset_last_Java_frame();

#define CALL_VM_NOCHECK(func)                   \
  CALL_VM_NOCHECK_NOFIX(func)                   \
  fixup_after_potential_safepoint()

int CppInterpreter::normal_entry(methodOop method, intptr_t UNUSED, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;

  // Allocate and initialize our frame.
  InterpreterFrame *frame = InterpreterFrame::build(method, CHECK_0);
  thread->push_zero_frame(frame);

  // Execute those bytecodes!
  main_loop(0, THREAD);

  // No deoptimized frames on the stack
  return 0;
}

intptr_t narrow(BasicType type, intptr_t result) {
  // mask integer result to narrower return type.
  switch (type) {
    case T_BOOLEAN:
      return result&1;
    case T_BYTE:
      return (intptr_t)(jbyte)result;
    case T_CHAR:
      return (intptr_t)(uintptr_t)(jchar)result;
    case T_SHORT:
      return (intptr_t)(jshort)result;
    case T_OBJECT:  // nothing to do fall through
    case T_ARRAY:
    case T_LONG:
    case T_INT:
    case T_FLOAT:
    case T_DOUBLE:
    case T_VOID:
      return result;
    default  : ShouldNotReachHere();
  }
}


void CppInterpreter::main_loop(int recurse, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();

  // If we are entering from a deopt we may need to call
  // ourself a few times in order to get to our frame.
  if (recurse)
    main_loop(recurse - 1, THREAD);

  InterpreterFrame *frame = thread->top_zero_frame()->as_interpreter_frame();
  interpreterState istate = frame->interpreter_state();
  methodOop method = istate->method();

  intptr_t *result = NULL;
  int result_slots = 0;

  while (true) {
    // We can set up the frame anchor with everything we want at
    // this point as we are thread_in_Java and no safepoints can
    // occur until we go to vm mode.  We do have to clear flags
    // on return from vm but that is it.
    thread->set_last_Java_frame();

    // Call the interpreter
    if (JvmtiExport::can_post_interpreter_events())
      BytecodeInterpreter::runWithChecks(istate);
    else
      BytecodeInterpreter::run(istate);
    fixup_after_potential_safepoint();

    // Clear the frame anchor
    thread->reset_last_Java_frame();

    // Examine the message from the interpreter to decide what to do
    if (istate->msg() == BytecodeInterpreter::call_method) {
      methodOop callee = istate->callee();

      // Trim back the stack to put the parameters at the top
      stack->set_sp(istate->stack() + 1);

      // Make the call
      Interpreter::invoke_method(callee, istate->callee_entry_point(), THREAD);
      fixup_after_potential_safepoint();

      // Convert the result
      istate->set_stack(stack->sp() - 1);

      // Restore the stack
      stack->set_sp(istate->stack_limit() + 1);

      // Resume the interpreter
      istate->set_msg(BytecodeInterpreter::method_resume);
    }
    else if (istate->msg() == BytecodeInterpreter::more_monitors) {
      int monitor_words = frame::interpreter_frame_monitor_size();

      // Allocate the space
      stack->overflow_check(monitor_words, THREAD);
      if (HAS_PENDING_EXCEPTION)
        break;
      stack->alloc(monitor_words * wordSize);

      // Move the expression stack contents
      for (intptr_t *p = istate->stack() + 1; p < istate->stack_base(); p++)
        *(p - monitor_words) = *p;

      // Move the expression stack pointers
      istate->set_stack_limit(istate->stack_limit() - monitor_words);
      istate->set_stack(istate->stack() - monitor_words);
      istate->set_stack_base(istate->stack_base() - monitor_words);

      // Zero the new monitor so the interpreter can find it.
      ((BasicObjectLock *) istate->stack_base())->set_obj(NULL);

      // Resume the interpreter
      istate->set_msg(BytecodeInterpreter::got_monitors);
    }
    else if (istate->msg() == BytecodeInterpreter::return_from_method) {
      // Copy the result into the caller's frame
      result_slots = type2size[result_type_of(method)];
      assert(result_slots >= 0 && result_slots <= 2, "what?");
      result = istate->stack() + result_slots;
      break;
    }
    else if (istate->msg() == BytecodeInterpreter::throwing_exception) {
      assert(HAS_PENDING_EXCEPTION, "should do");
      break;
    }
    else if (istate->msg() == BytecodeInterpreter::do_osr) {
      // Unwind the current frame
      thread->pop_zero_frame();

      // Remove any extension of the previous frame
      int extra_locals = method->max_locals() - method->size_of_parameters();
      stack->set_sp(stack->sp() + extra_locals);

      // Jump into the OSR method
      Interpreter::invoke_osr(
        method, istate->osr_entry(), istate->osr_buf(), THREAD);
      return;
    }
    else if (istate->msg() == BytecodeInterpreter::call_method_handle) {
      oop method_handle = istate->callee();

      // Trim back the stack to put the parameters at the top
      stack->set_sp(istate->stack() + 1);

      // Make the call
      process_method_handle(method_handle, THREAD);
      fixup_after_potential_safepoint();

      // Convert the result
      istate->set_stack(stack->sp() - 1);

      // Restore the stack
      stack->set_sp(istate->stack_limit() + 1);

      // Resume the interpreter
      istate->set_msg(BytecodeInterpreter::method_resume);
    }
    else {
      ShouldNotReachHere();
    }
  }

  // Unwind the current frame
  thread->pop_zero_frame();

  // Pop our local variables
  stack->set_sp(stack->sp() + method->max_locals());

  // Push our result
  for (int i = 0; i < result_slots; i++) {
    // Adjust result to smaller
    union {
      intptr_t res;
      jint res_jint;
    };
    res = result[-i];
    if (result_slots == 1) {
      BasicType t = result_type_of(method);
      if (is_subword_type(t)) {
        res_jint = (jint)narrow(t, res_jint);
      }
    }
    stack->push(res);
  }
}

int CppInterpreter::native_entry(methodOop method, intptr_t UNUSED, TRAPS) {
  // Make sure method is native and not abstract
  assert(method->is_native() && !method->is_abstract(), "should be");

  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();

  // Allocate and initialize our frame
  InterpreterFrame *frame = InterpreterFrame::build(method, CHECK_0);
  thread->push_zero_frame(frame);
  interpreterState istate = frame->interpreter_state();
  intptr_t *locals = istate->locals();

  // Update the invocation counter
  if ((UseCompiler || CountCompiledCalls) && !method->is_synchronized()) {
    InvocationCounter *counter = method->invocation_counter();
    counter->increment();
    if (counter->reached_InvocationLimit()) {
      CALL_VM_NOCHECK(
        InterpreterRuntime::frequency_counter_overflow(thread, NULL));
      if (HAS_PENDING_EXCEPTION)
        goto unwind_and_return;
    }
  }

  // Lock if necessary
  BasicObjectLock *monitor;
  monitor = NULL;
  if (method->is_synchronized()) {
    monitor = (BasicObjectLock*) istate->stack_base();
    oop lockee = monitor->obj();
    markOop disp = lockee->mark()->set_unlocked();

    monitor->lock()->set_displaced_header(disp);
    if (Atomic::cmpxchg_ptr(monitor, lockee->mark_addr(), disp) != disp) {
      if (thread->is_lock_owned((address) disp->clear_lock_bits())) {
        monitor->lock()->set_displaced_header(NULL);
      }
      else {
        CALL_VM_NOCHECK(InterpreterRuntime::monitorenter(thread, monitor));
        if (HAS_PENDING_EXCEPTION)
          goto unwind_and_return;
      }
    }
  }

  // Get the signature handler
  InterpreterRuntime::SignatureHandler *handler; {
    address handlerAddr = method->signature_handler();
    if (handlerAddr == NULL) {
      CALL_VM_NOCHECK(InterpreterRuntime::prepare_native_call(thread, method));
      if (HAS_PENDING_EXCEPTION)
        goto unlock_unwind_and_return;

      handlerAddr = method->signature_handler();
      assert(handlerAddr != NULL, "eh?");
    }
    if (handlerAddr == (address) InterpreterRuntime::slow_signature_handler) {
      CALL_VM_NOCHECK(handlerAddr =
        InterpreterRuntime::slow_signature_handler(thread, method, NULL,NULL));
      if (HAS_PENDING_EXCEPTION)
        goto unlock_unwind_and_return;
    }
    handler = \
      InterpreterRuntime::SignatureHandler::from_handlerAddr(handlerAddr);
  }

  // Get the native function entry point
  address function;
  function = method->native_function();
  assert(function != NULL, "should be set if signature handler is");

  // Build the argument list
  stack->overflow_check(handler->argument_count() * 2, THREAD);
  if (HAS_PENDING_EXCEPTION)
    goto unlock_unwind_and_return;

  void **arguments;
  void *mirror; {
    arguments =
      (void **) stack->alloc(handler->argument_count() * sizeof(void **));
    void **dst = arguments;

    void *env = thread->jni_environment();
    *(dst++) = &env;

    if (method->is_static()) {
      istate->set_oop_temp(
        method->constants()->pool_holder()->java_mirror());
      mirror = istate->oop_temp_addr();
      *(dst++) = &mirror;
    }

    intptr_t *src = locals;
    for (int i = dst - arguments; i < handler->argument_count(); i++) {
      ffi_type *type = handler->argument_type(i);
      if (type == &ffi_type_pointer) {
        if (*src) {
          stack->push((intptr_t) src);
          *(dst++) = stack->sp();
        }
        else {
          *(dst++) = src;
        }
        src--;
      }
      else if (type->size == 4) {
        *(dst++) = src--;
      }
      else if (type->size == 8) {
        src--;
        *(dst++) = src--;
      }
      else {
        ShouldNotReachHere();
      }
    }
  }

  // Set up the Java frame anchor
  thread->set_last_Java_frame();

  // Change the thread state to _thread_in_native
  ThreadStateTransition::transition_from_java(thread, _thread_in_native);

  // Make the call
  intptr_t result[4 - LogBytesPerWord];
  ffi_call(handler->cif(), (void (*)()) function, result, arguments);

  // Change the thread state back to _thread_in_Java.
  // ThreadStateTransition::transition_from_native() cannot be used
  // here because it does not check for asynchronous exceptions.
  // We have to manage the transition ourself.
  thread->set_thread_state(_thread_in_native_trans);

  // Make sure new state is visible in the GC thread
  if (os::is_MP()) {
    if (UseMembar) {
      OrderAccess::fence();
    }
    else {
      InterfaceSupport::serialize_memory(thread);
    }
  }

  // Handle safepoint operations, pending suspend requests,
  // and pending asynchronous exceptions.
  if (SafepointSynchronize::do_call_back() ||
      thread->has_special_condition_for_native_trans()) {
    JavaThread::check_special_condition_for_native_trans(thread);
    CHECK_UNHANDLED_OOPS_ONLY(thread->clear_unhandled_oops());
  }

  // Finally we can change the thread state to _thread_in_Java.
  thread->set_thread_state(_thread_in_Java);
  fixup_after_potential_safepoint();

  // Clear the frame anchor
  thread->reset_last_Java_frame();

  // If the result was an oop then unbox it and store it in
  // oop_temp where the garbage collector can see it before
  // we release the handle it might be protected by.
  if (handler->result_type() == &ffi_type_pointer) {
    if (result[0])
      istate->set_oop_temp(*(oop *) result[0]);
    else
      istate->set_oop_temp(NULL);
  }

  // Reset handle block
  thread->active_handles()->clear();

 unlock_unwind_and_return:

  // Unlock if necessary
  if (monitor) {
    BasicLock *lock = monitor->lock();
    markOop header = lock->displaced_header();
    oop rcvr = monitor->obj();
    monitor->set_obj(NULL);

    if (header != NULL) {
      if (Atomic::cmpxchg_ptr(header, rcvr->mark_addr(), lock) != lock) {
        monitor->set_obj(rcvr); {
          HandleMark hm(thread);
          CALL_VM_NOCHECK(InterpreterRuntime::monitorexit(thread, monitor));
        }
      }
    }
  }

 unwind_and_return:

  // Unwind the current activation
  thread->pop_zero_frame();

  // Pop our parameters
  stack->set_sp(stack->sp() + method->size_of_parameters());

  // Push our result
  if (!HAS_PENDING_EXCEPTION) {
    BasicType type = result_type_of(method);
    stack->set_sp(stack->sp() - type2size[type]);

    switch (type) {
    case T_VOID:
      break;

    case T_BOOLEAN:
#ifndef VM_LITTLE_ENDIAN
      result[0] <<= (BitsPerWord - BitsPerByte);
#endif
      SET_LOCALS_INT(*(jboolean *) result != 0, 0);
      break;

    case T_CHAR:
#ifndef VM_LITTLE_ENDIAN
      result[0] <<= (BitsPerWord - BitsPerShort);
#endif
      SET_LOCALS_INT(*(jchar *) result, 0);
      break;

    case T_BYTE:
#ifndef VM_LITTLE_ENDIAN
      result[0] <<= (BitsPerWord - BitsPerByte);
#endif
      SET_LOCALS_INT(*(jbyte *) result, 0);
      break;

    case T_SHORT:
#ifndef VM_LITTLE_ENDIAN
      result[0] <<= (BitsPerWord - BitsPerShort);
#endif
      SET_LOCALS_INT(*(jshort *) result, 0);
      break;

    case T_INT:
#ifndef VM_LITTLE_ENDIAN
      result[0] <<= (BitsPerWord - BitsPerInt);
#endif
      SET_LOCALS_INT(*(jint *) result, 0);
      break;

    case T_LONG:
      SET_LOCALS_LONG(*(jlong *) result, 0);
      break;

    case T_FLOAT:
      SET_LOCALS_FLOAT(*(jfloat *) result, 0);
      break;

    case T_DOUBLE:
      SET_LOCALS_DOUBLE(*(jdouble *) result, 0);
      break;

    case T_OBJECT:
    case T_ARRAY:
      SET_LOCALS_OBJECT(istate->oop_temp(), 0);
      break;

    default:
      ShouldNotReachHere();
    }
  }

  // No deoptimized frames on the stack
  return 0;
}

int CppInterpreter::accessor_entry(methodOop method, intptr_t UNUSED, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();
  intptr_t *locals = stack->sp();

  // Drop into the slow path if we need a safepoint check
  if (SafepointSynchronize::do_call_back()) {
    return normal_entry(method, 0, THREAD);
  }

  // Load the object pointer and drop into the slow path
  // if we have a NullPointerException
  oop object = LOCALS_OBJECT(0);
  if (object == NULL) {
    return normal_entry(method, 0, THREAD);
  }

  // Read the field index from the bytecode, which looks like this:
  //  0:  aload_0
  //  1:  getfield
  //  2:    index
  //  3:    index
  //  4:  ireturn/areturn
  // NB this is not raw bytecode: index is in machine order
  u1 *code = method->code_base();
  assert(code[0] == Bytecodes::_aload_0 &&
         code[1] == Bytecodes::_getfield &&
         (code[4] == Bytecodes::_ireturn ||
          code[4] == Bytecodes::_areturn), "should do");
  u2 index = Bytes::get_native_u2(&code[2]);

  // Get the entry from the constant pool cache, and drop into
  // the slow path if it has not been resolved
  constantPoolCacheOop cache = method->constants()->cache();
  ConstantPoolCacheEntry* entry = cache->entry_at(index);
  if (!entry->is_resolved(Bytecodes::_getfield)) {
    return normal_entry(method, 0, THREAD);
  }

  // Get the result and push it onto the stack
  switch (entry->flag_state()) {
  case ltos:
  case dtos:
    stack->overflow_check(1, CHECK_0);
    stack->alloc(wordSize);
    break;
  }
  if (entry->is_volatile()) {
    switch (entry->flag_state()) {
    case ctos:
      SET_LOCALS_INT(object->char_field_acquire(entry->f2()), 0);
      break;

    case btos:
    case ztos:
      SET_LOCALS_INT(object->byte_field_acquire(entry->f2()), 0);
      break;

    case stos:
      SET_LOCALS_INT(object->short_field_acquire(entry->f2()), 0);
      break;

    case itos:
      SET_LOCALS_INT(object->int_field_acquire(entry->f2()), 0);
      break;

    case ltos:
      SET_LOCALS_LONG(object->long_field_acquire(entry->f2()), 0);
      break;

    case ftos:
      SET_LOCALS_FLOAT(object->float_field_acquire(entry->f2()), 0);
      break;

    case dtos:
      SET_LOCALS_DOUBLE(object->double_field_acquire(entry->f2()), 0);
      break;

    case atos:
      SET_LOCALS_OBJECT(object->obj_field_acquire(entry->f2()), 0);
      break;

    default:
      ShouldNotReachHere();
    }
  }
  else {
    switch (entry->flag_state()) {
    case ctos:
      SET_LOCALS_INT(object->char_field(entry->f2()), 0);
      break;

    case btos:
    case ztos:
      SET_LOCALS_INT(object->byte_field(entry->f2()), 0);
      break;

    case stos:
      SET_LOCALS_INT(object->short_field(entry->f2()), 0);
      break;

    case itos:
      SET_LOCALS_INT(object->int_field(entry->f2()), 0);
      break;

    case ltos:
      SET_LOCALS_LONG(object->long_field(entry->f2()), 0);
      break;

    case ftos:
      SET_LOCALS_FLOAT(object->float_field(entry->f2()), 0);
      break;

    case dtos:
      SET_LOCALS_DOUBLE(object->double_field(entry->f2()), 0);
      break;

    case atos:
      SET_LOCALS_OBJECT(object->obj_field(entry->f2()), 0);
      break;

    default:
      ShouldNotReachHere();
    }
  }

  // No deoptimized frames on the stack
  return 0;
}

int CppInterpreter::empty_entry(methodOop method, intptr_t UNUSED, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();

  // Drop into the slow path if we need a safepoint check
  if (SafepointSynchronize::do_call_back()) {
    return normal_entry(method, 0, THREAD);
  }

  // Pop our parameters
  stack->set_sp(stack->sp() + method->size_of_parameters());

  // No deoptimized frames on the stack
  return 0;
}

int CppInterpreter::method_handle_entry(methodOop method,
                                        intptr_t UNUSED, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();
  int argument_slots = method->size_of_parameters();
  int result_slots = type2size[result_type_of(method)];
  intptr_t *vmslots = stack->sp();
  intptr_t *unwind_sp = vmslots + argument_slots;

  // Find the MethodType
  address p = (address) method;
  for (jint* pc = method->method_type_offsets_chain(); (*pc) != -1; pc++) {
    p = *(address*)(p + (*pc));
  }
  oop method_type = (oop) p;

  // The MethodHandle is in the slot after the arguments
  oop form = java_lang_invoke_MethodType::form(method_type);
  int num_vmslots = java_lang_invoke_MethodTypeForm::vmslots(form);
  assert(argument_slots == num_vmslots + 1, "should be");
  oop method_handle = VMSLOTS_OBJECT(num_vmslots);

  // InvokeGeneric requires some extra shuffling
  oop mhtype = java_lang_invoke_MethodHandle::type(method_handle);
  bool is_exact = mhtype == method_type;
  if (!is_exact) {
    if (method->intrinsic_id() == vmIntrinsics::_invokeExact) {
      CALL_VM_NOCHECK_NOFIX(
        SharedRuntime::throw_WrongMethodTypeException(
          thread, method_type, mhtype));
      // NB all oops trashed!
      assert(HAS_PENDING_EXCEPTION, "should do");
      stack->set_sp(unwind_sp);
      return 0;
    }
    assert(method->intrinsic_id() == vmIntrinsics::_invokeGeneric, "should be");

    // Load up an adapter from the calling type
    // NB the x86 code for this (in methodHandles_x86.cpp, search for
    // "genericInvoker") is really really odd.  I'm hoping it's trying
    // to accomodate odd VM/class library combinations I can ignore.
    oop adapter = java_lang_invoke_MethodTypeForm::genericInvoker(form);
    if (adapter == NULL) {
      CALL_VM_NOCHECK_NOFIX(
        SharedRuntime::throw_WrongMethodTypeException(
          thread, method_type, mhtype));
      // NB all oops trashed!
      assert(HAS_PENDING_EXCEPTION, "should do");
      stack->set_sp(unwind_sp);
      return 0;
    }

    // Adapters are shared among form-families of method-type.  The
    // type being called is passed as a trusted first argument so that
    // the adapter knows the actual types of its arguments and return
    // values.
    insert_vmslots(num_vmslots + 1, 1, THREAD);
    if (HAS_PENDING_EXCEPTION) {
      // NB all oops trashed!
      stack->set_sp(unwind_sp);
      return 0;
    }

    vmslots = stack->sp();
    num_vmslots++;
    SET_VMSLOTS_OBJECT(method_type, num_vmslots);

    method_handle = adapter;
  }

  // Start processing
  process_method_handle(method_handle, THREAD);
  if (HAS_PENDING_EXCEPTION)
    result_slots = 0;

  // If this is an invokeExact then the eventual callee will not
  // have unwound the method handle argument so we have to do it.
  // If a result is being returned the it will be above the method
  // handle argument we're unwinding.
  if (is_exact) {
    intptr_t result[2];
    for (int i = 0; i < result_slots; i++)
      result[i] = stack->pop();
    stack->pop();
    for (int i = result_slots - 1; i >= 0; i--)
      stack->push(result[i]);
  }

  // Check
  assert(stack->sp() == unwind_sp - result_slots, "should be");

  // No deoptimized frames on the stack
  return 0;
}

void CppInterpreter::process_method_handle(oop method_handle, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();
  intptr_t *vmslots = stack->sp();

  bool direct_to_method = false;
  BasicType src_rtype = T_ILLEGAL;
  BasicType dst_rtype = T_ILLEGAL;

  MethodHandleEntry *entry =
    java_lang_invoke_MethodHandle::vmentry(method_handle);
  MethodHandles::EntryKind entry_kind =
    (MethodHandles::EntryKind) (((intptr_t) entry) & 0xffffffff);

  methodOop method = NULL;
  switch (entry_kind) {
  case MethodHandles::_invokestatic_mh:
    direct_to_method = true;
    break;

  case MethodHandles::_invokespecial_mh:
  case MethodHandles::_invokevirtual_mh:
  case MethodHandles::_invokeinterface_mh:
    {
      oop receiver =
        VMSLOTS_OBJECT(
          java_lang_invoke_MethodHandle::vmslots(method_handle) - 1);
      if (receiver == NULL) {
          stack->set_sp(calculate_unwind_sp(stack, method_handle));
          CALL_VM_NOCHECK_NOFIX(
            throw_exception(
              thread, vmSymbols::java_lang_NullPointerException()));
          // NB all oops trashed!
          assert(HAS_PENDING_EXCEPTION, "should do");
          return;
      }
      if (entry_kind != MethodHandles::_invokespecial_mh) {
        int index = java_lang_invoke_DirectMethodHandle::vmindex(method_handle);
        instanceKlass* rcvrKlass =
          (instanceKlass *) receiver->klass()->klass_part();
        if (entry_kind == MethodHandles::_invokevirtual_mh) {
          method = (methodOop) rcvrKlass->start_of_vtable()[index];
        }
        else {
          oop iclass = java_lang_invoke_MethodHandle::vmtarget(method_handle);
          itableOffsetEntry* ki =
            (itableOffsetEntry *) rcvrKlass->start_of_itable();
          int i, length = rcvrKlass->itable_length();
          for (i = 0; i < length; i++, ki++ ) {
            if (ki->interface_klass() == iclass)
              break;
          }
          if (i == length) {
            stack->set_sp(calculate_unwind_sp(stack, method_handle));
            CALL_VM_NOCHECK_NOFIX(
              throw_exception(
                thread, vmSymbols::java_lang_IncompatibleClassChangeError()));
            // NB all oops trashed!
            assert(HAS_PENDING_EXCEPTION, "should do");
            return;
          }
          itableMethodEntry* im = ki->first_method_entry(receiver->klass());
          method = im[index].method();
          if (method == NULL) {
            stack->set_sp(calculate_unwind_sp(stack, method_handle));
            CALL_VM_NOCHECK_NOFIX(
              throw_exception(
                thread, vmSymbols::java_lang_AbstractMethodError()));
            // NB all oops trashed!
            assert(HAS_PENDING_EXCEPTION, "should do");
            return;
          }
        }
      }
    }
    direct_to_method = true;
    break;

  case MethodHandles::_bound_ref_direct_mh:
  case MethodHandles::_bound_int_direct_mh:
  case MethodHandles::_bound_long_direct_mh:
    direct_to_method = true;
    // fall through
  case MethodHandles::_bound_ref_mh:
  case MethodHandles::_bound_int_mh:
  case MethodHandles::_bound_long_mh:
    {
      BasicType arg_type = MethodHandles::ek_bound_mh_arg_type(entry_kind);
      int arg_mask = 0;
      int arg_slots = type2size[arg_type];;

      int arg_slot =
        java_lang_invoke_BoundMethodHandle::vmargslot(method_handle);

      // Create the new slot(s)
      intptr_t *unwind_sp = calculate_unwind_sp(stack, method_handle);
      insert_vmslots(arg_slot, arg_slots, THREAD);
      if (HAS_PENDING_EXCEPTION) {
        // all oops trashed
        stack->set_sp(unwind_sp);
        return;
      }
      vmslots = stack->sp();

      // Store bound argument into new stack slot
      oop arg = java_lang_invoke_BoundMethodHandle::argument(method_handle);
      if (arg_type == T_OBJECT) {
        assert(arg_slots == 1, "should be");
        SET_VMSLOTS_OBJECT(arg, arg_slot);
      }
      else {
        jvalue arg_value;
        arg_type = java_lang_boxing_object::get_value(arg, &arg_value);
        switch (arg_type) {
        case T_BOOLEAN:
          SET_VMSLOTS_INT(arg_value.z, arg_slot);
          break;
        case T_CHAR:
          SET_VMSLOTS_INT(arg_value.c, arg_slot);
          break;
        case T_BYTE:
          SET_VMSLOTS_INT(arg_value.b, arg_slot);
          break;
        case T_SHORT:
          SET_VMSLOTS_INT(arg_value.s, arg_slot);
          break;
        case T_INT:
          SET_VMSLOTS_INT(arg_value.i, arg_slot);
          break;
        case T_FLOAT:
          SET_VMSLOTS_FLOAT(arg_value.f, arg_slot);
          break;
        case T_LONG:
          SET_VMSLOTS_LONG(arg_value.j, arg_slot + 1);
          break;
        case T_DOUBLE:
          SET_VMSLOTS_DOUBLE(arg_value.d, arg_slot + 1);
          break;
        default:
          tty->print_cr("unhandled type %s", type2name(arg_type));
          ShouldNotReachHere();
        }
      }
    }
    break;

  case MethodHandles::_adapter_retype_only:
  case MethodHandles::_adapter_retype_raw:
    src_rtype = result_type_of_handle(
      java_lang_invoke_MethodHandle::vmtarget(method_handle));
    dst_rtype = result_type_of_handle(method_handle);
    break;

  case MethodHandles::_adapter_check_cast:
    {
      int arg_slot =
        java_lang_invoke_AdapterMethodHandle::vmargslot(method_handle);
      oop arg = VMSLOTS_OBJECT(arg_slot);
      if (arg != NULL) {
        klassOop objKlassOop = arg->klass();
        klassOop klassOf = java_lang_Class::as_klassOop(
          java_lang_invoke_AdapterMethodHandle::argument(method_handle));

        if (objKlassOop != klassOf &&
            !objKlassOop->klass_part()->is_subtype_of(klassOf)) {
          ResourceMark rm(THREAD);
          const char* objName = Klass::cast(objKlassOop)->external_name();
          const char* klassName = Klass::cast(klassOf)->external_name();
          char* message = SharedRuntime::generate_class_cast_message(
            objName, klassName);

          stack->set_sp(calculate_unwind_sp(stack, method_handle));
          CALL_VM_NOCHECK_NOFIX(
            throw_exception(
              thread, vmSymbols::java_lang_ClassCastException(), message));
          // NB all oops trashed!
          assert(HAS_PENDING_EXCEPTION, "should do");
          return;
        }
      }
    }
    break;

  case MethodHandles::_adapter_dup_args:
    {
      int arg_slot =
        java_lang_invoke_AdapterMethodHandle::vmargslot(method_handle);
      int conv =
        java_lang_invoke_AdapterMethodHandle::conversion(method_handle);
      int num_slots = -MethodHandles::adapter_conversion_stack_move(conv);
      assert(num_slots > 0, "should be");

      // Create the new slot(s)
      intptr_t *unwind_sp = calculate_unwind_sp(stack, method_handle);
      stack->overflow_check(num_slots, THREAD);
      if (HAS_PENDING_EXCEPTION) {
        // all oops trashed
        stack->set_sp(unwind_sp);
        return;
      }

      // Duplicate the arguments
      for (int i = num_slots - 1; i >= 0; i--)
        stack->push(*VMSLOTS_SLOT(arg_slot + i));

      vmslots = stack->sp(); // unused, but let the compiler figure that out
    }
    break;

  case MethodHandles::_adapter_drop_args:
    {
      int arg_slot =
        java_lang_invoke_AdapterMethodHandle::vmargslot(method_handle);
      int conv =
        java_lang_invoke_AdapterMethodHandle::conversion(method_handle);
      int num_slots = MethodHandles::adapter_conversion_stack_move(conv);
      assert(num_slots > 0, "should be");

      remove_vmslots(arg_slot, num_slots, THREAD); // doesn't trap
      vmslots = stack->sp(); // unused, but let the compiler figure that out
    }
    break;

  case MethodHandles::_adapter_opt_swap_1:
  case MethodHandles::_adapter_opt_swap_2:
  case MethodHandles::_adapter_opt_rot_1_up:
  case MethodHandles::_adapter_opt_rot_1_down:
  case MethodHandles::_adapter_opt_rot_2_up:
  case MethodHandles::_adapter_opt_rot_2_down:
    {
      int arg1 =
        java_lang_invoke_AdapterMethodHandle::vmargslot(method_handle);
      int conv =
        java_lang_invoke_AdapterMethodHandle::conversion(method_handle);
      int arg2 = MethodHandles::adapter_conversion_vminfo(conv);

      int swap_slots = MethodHandles::ek_adapter_opt_swap_slots(entry_kind);
      int rotate = MethodHandles::ek_adapter_opt_swap_mode(entry_kind);
      int swap_bytes = swap_slots * Interpreter::stackElementSize;
      swap_slots = swap_bytes >> LogBytesPerWord;

      intptr_t tmp;
      switch (rotate) {
      case 0: // swap
        for (int i = 0; i < swap_slots; i++) {
          tmp = *VMSLOTS_SLOT(arg1 + i);
          SET_VMSLOTS_SLOT(VMSLOTS_SLOT(arg2 + i), arg1 + i);
          SET_VMSLOTS_SLOT(&tmp, arg2 + i);
        }
        break;

      case 1: // up
        assert(arg1 - swap_slots > arg2, "should be");

        tmp = *VMSLOTS_SLOT(arg1);
        for (int i = arg1 - swap_slots; i >= arg2; i--)
          SET_VMSLOTS_SLOT(VMSLOTS_SLOT(i), i + swap_slots);
        SET_VMSLOTS_SLOT(&tmp, arg2);

        break;

      case -1: // down
        assert(arg2 - swap_slots > arg1, "should be");

        tmp = *VMSLOTS_SLOT(arg1);
        for (int i = arg1 + swap_slots; i <= arg2; i++)
          SET_VMSLOTS_SLOT(VMSLOTS_SLOT(i), i - swap_slots);
        SET_VMSLOTS_SLOT(&tmp, arg2);
        break;

      default:
        ShouldNotReachHere();
      }
    }
    break;

  case MethodHandles::_adapter_opt_i2l:
    {
      int arg_slot =
        java_lang_invoke_AdapterMethodHandle::vmargslot(method_handle);
      int arg = VMSLOTS_INT(arg_slot);
      intptr_t *unwind_sp = calculate_unwind_sp(stack, method_handle);
      insert_vmslots(arg_slot, 1, THREAD);
      if (HAS_PENDING_EXCEPTION) {
        // all oops trashed
        stack->set_sp(unwind_sp);
        return;
      }
      vmslots = stack->sp();
      arg_slot++;
      SET_VMSLOTS_LONG(arg, arg_slot);
    }
    break;

  case MethodHandles::_adapter_opt_unboxi:
  case MethodHandles::_adapter_opt_unboxl:
    {
      int arg_slot =
        java_lang_invoke_AdapterMethodHandle::vmargslot(method_handle);
      oop arg = VMSLOTS_OBJECT(arg_slot);
      jvalue arg_value;
      if (arg == NULL) {
        // queue a nullpointer exception for the caller
        stack->set_sp(calculate_unwind_sp(stack, method_handle));
        CALL_VM_NOCHECK_NOFIX(
          throw_exception(
            thread, vmSymbols::java_lang_NullPointerException()));
        // NB all oops trashed!
        assert(HAS_PENDING_EXCEPTION, "should do");
        return;
      }
      BasicType arg_type = java_lang_boxing_object::get_value(arg, &arg_value);
      if (arg_type == T_LONG || arg_type == T_DOUBLE) {
        intptr_t *unwind_sp = calculate_unwind_sp(stack, method_handle);
        insert_vmslots(arg_slot, 1, THREAD);
        if (HAS_PENDING_EXCEPTION) {
          // all oops trashed
          stack->set_sp(unwind_sp);
          return;
        }
        vmslots = stack->sp();
        arg_slot++;
      }
      switch (arg_type) {
      case T_BOOLEAN:
        SET_VMSLOTS_INT(arg_value.z, arg_slot);
        break;
      case T_CHAR:
        SET_VMSLOTS_INT(arg_value.c, arg_slot);
        break;
      case T_BYTE:
        SET_VMSLOTS_INT(arg_value.b, arg_slot);
        break;
      case T_SHORT:
        SET_VMSLOTS_INT(arg_value.s, arg_slot);
        break;
      case T_INT:
        SET_VMSLOTS_INT(arg_value.i, arg_slot);
        break;
      case T_FLOAT:
        SET_VMSLOTS_FLOAT(arg_value.f, arg_slot);
        break;
      case T_LONG:
        SET_VMSLOTS_LONG(arg_value.j, arg_slot);
        break;
      case T_DOUBLE:
        SET_VMSLOTS_DOUBLE(arg_value.d, arg_slot);
        break;
      default:
        tty->print_cr("unhandled type %s", type2name(arg_type));
        ShouldNotReachHere();
      }
    }
    break;

  default:
    tty->print_cr("unhandled entry_kind %s",
                  MethodHandles::entry_name(entry_kind));
    ShouldNotReachHere();
  }

  // Continue along the chain
  if (direct_to_method) {
    if (method == NULL) {
      method =
        (methodOop) java_lang_invoke_MethodHandle::vmtarget(method_handle);
    }
    address entry_point = method->from_interpreted_entry();
    Interpreter::invoke_method(method, entry_point, THREAD);
  }
  else {
    process_method_handle(
      java_lang_invoke_MethodHandle::vmtarget(method_handle), THREAD);
  }
  // NB all oops now trashed

  // Adapt the result type, if necessary
  if (src_rtype != dst_rtype && !HAS_PENDING_EXCEPTION) {
    switch (dst_rtype) {
    case T_VOID:
      for (int i = 0; i < type2size[src_rtype]; i++)
        stack->pop();
      return;

    case T_INT:
      switch (src_rtype) {
      case T_VOID:
        stack->overflow_check(1, CHECK);
        stack->push(0);
        return;

      case T_BOOLEAN:
      case T_CHAR:
      case T_BYTE:
      case T_SHORT:
        return;
      }
      // INT results sometimes need narrowing
    case T_BOOLEAN:
    case T_CHAR:
    case T_BYTE:
    case T_SHORT:
      switch (src_rtype) {
      case T_INT:
        return;
      }
    }

    tty->print_cr("unhandled conversion:");
    tty->print_cr("src_rtype = %s", type2name(src_rtype));
    tty->print_cr("dst_rtype = %s", type2name(dst_rtype));
    ShouldNotReachHere();
  }
}

// The new slots will be inserted before slot insert_before.
// Slots < insert_before will have the same slot number after the insert.
// Slots >= insert_before will become old_slot + num_slots.
void CppInterpreter::insert_vmslots(int insert_before, int num_slots, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();

  // Allocate the space
  stack->overflow_check(num_slots, CHECK);
  stack->alloc(num_slots * wordSize);
  intptr_t *vmslots = stack->sp();

  // Shuffle everything up
  for (int i = 0; i < insert_before; i++)
    SET_VMSLOTS_SLOT(VMSLOTS_SLOT(i + num_slots), i);
}

void CppInterpreter::remove_vmslots(int first_slot, int num_slots, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();
  intptr_t *vmslots = stack->sp();

  // Move everything down
  for (int i = first_slot - 1; i >= 0; i--)
    SET_VMSLOTS_SLOT(VMSLOTS_SLOT(i), i + num_slots);

  // Deallocate the space
  stack->set_sp(stack->sp() + num_slots);
}

BasicType CppInterpreter::result_type_of_handle(oop method_handle) {
  oop method_type = java_lang_invoke_MethodHandle::type(method_handle);
  oop return_type = java_lang_invoke_MethodType::rtype(method_type);
  return java_lang_Class::as_BasicType(return_type, (klassOop *) NULL);
}

intptr_t* CppInterpreter::calculate_unwind_sp(ZeroStack* stack,
                                              oop method_handle) {
  oop method_type = java_lang_invoke_MethodHandle::type(method_handle);
  oop form = java_lang_invoke_MethodType::form(method_type);
  int argument_slots = java_lang_invoke_MethodTypeForm::vmslots(form);

  return stack->sp() + argument_slots;
}

IRT_ENTRY(void, CppInterpreter::throw_exception(JavaThread* thread,
                                                Symbol*     name,
                                                char*       message))
  THROW_MSG(name, message);
IRT_END

InterpreterFrame *InterpreterFrame::build(const methodOop method, TRAPS) {
  JavaThread *thread = (JavaThread *) THREAD;
  ZeroStack *stack = thread->zero_stack();

  // Calculate the size of the frame we'll build, including
  // any adjustments to the caller's frame that we'll make.
  int extra_locals  = 0;
  int monitor_words = 0;
  int stack_words   = 0;

  if (!method->is_native()) {
    extra_locals = method->max_locals() - method->size_of_parameters();
    stack_words  = method->max_stack();
  }
  if (method->is_synchronized()) {
    monitor_words = frame::interpreter_frame_monitor_size();
  }
  stack->overflow_check(
    extra_locals + header_words + monitor_words + stack_words, CHECK_NULL);

  // Adjust the caller's stack frame to accomodate any additional
  // local variables we have contiguously with our parameters.
  for (int i = 0; i < extra_locals; i++)
    stack->push(0);

  intptr_t *locals;
  if (method->is_native())
    locals = stack->sp() + (method->size_of_parameters() - 1);
  else
    locals = stack->sp() + (method->max_locals() - 1);

  stack->push(0); // next_frame, filled in later
  intptr_t *fp = stack->sp();
  assert(fp - stack->sp() == next_frame_off, "should be");

  stack->push(INTERPRETER_FRAME);
  assert(fp - stack->sp() == frame_type_off, "should be");

  interpreterState istate =
    (interpreterState) stack->alloc(sizeof(BytecodeInterpreter));
  assert(fp - stack->sp() == istate_off, "should be");

  istate->set_locals(locals);
  istate->set_method(method);
  istate->set_self_link(istate);
  istate->set_prev_link(NULL);
  istate->set_thread(thread);
  istate->set_bcp(method->is_native() ? NULL : method->code_base());
  istate->set_constants(method->constants()->cache());
  istate->set_msg(BytecodeInterpreter::method_entry);
  istate->set_oop_temp(NULL);
  istate->set_mdx(NULL);
  istate->set_callee(NULL);

  istate->set_monitor_base((BasicObjectLock *) stack->sp());
  if (method->is_synchronized()) {
    BasicObjectLock *monitor =
      (BasicObjectLock *) stack->alloc(monitor_words * wordSize);
    oop object;
    if (method->is_static())
      object = method->constants()->pool_holder()->java_mirror();
    else
      object = (oop) locals[0];
    monitor->set_obj(object);
  }

  istate->set_stack_base(stack->sp());
  istate->set_stack(stack->sp() - 1);
  if (stack_words)
    stack->alloc(stack_words * wordSize);
  istate->set_stack_limit(stack->sp() - 1);

  return (InterpreterFrame *) fp;
}

int AbstractInterpreter::BasicType_as_index(BasicType type) {
  int i = 0;
  switch (type) {
    case T_BOOLEAN: i = 0; break;
    case T_CHAR   : i = 1; break;
    case T_BYTE   : i = 2; break;
    case T_SHORT  : i = 3; break;
    case T_INT    : i = 4; break;
    case T_LONG   : i = 5; break;
    case T_VOID   : i = 6; break;
    case T_FLOAT  : i = 7; break;
    case T_DOUBLE : i = 8; break;
    case T_OBJECT : i = 9; break;
    case T_ARRAY  : i = 9; break;
    default       : ShouldNotReachHere();
  }
  assert(0 <= i && i < AbstractInterpreter::number_of_result_handlers,
         "index out of bounds");
  return i;
}

BasicType CppInterpreter::result_type_of(methodOop method) {
  // Get method->_constMethod->_result_type
  u1 *p = ((unsigned char *)method->constMethod()
           + in_bytes(constMethodOopDesc::result_type_offset()));
  BasicType t = (BasicType)*p;
  return t;
}

address InterpreterGenerator::generate_empty_entry() {
  if (!UseFastEmptyMethods)
    return NULL;

  return generate_entry((address) CppInterpreter::empty_entry);
}

address InterpreterGenerator::generate_accessor_entry() {
  if (!UseFastAccessorMethods)
    return NULL;

  return generate_entry((address) CppInterpreter::accessor_entry);
}

address InterpreterGenerator::generate_Reference_get_entry(void) {
#ifndef SERIALGC
  if (UseG1GC) {
    // We need to generate have a routine that generates code to:
    //   * load the value in the referent field
    //   * passes that value to the pre-barrier.
    //
    // In the case of G1 this will record the value of the
    // referent in an SATB buffer if marking is active.
    // This will cause concurrent marking to mark the referent
    // field as live.
    Unimplemented();
  }
#endif // SERIALGC

  // If G1 is not enabled then attempt to go through the accessor entry point
  // Reference.get is an accessor
  return generate_accessor_entry();
}

address InterpreterGenerator::generate_native_entry(bool synchronized) {
  assert(synchronized == false, "should be");

  return generate_entry((address) CppInterpreter::native_entry);
}

address InterpreterGenerator::generate_normal_entry(bool synchronized) {
  assert(synchronized == false, "should be");

  return generate_entry((address) CppInterpreter::normal_entry);
}

address AbstractInterpreterGenerator::generate_method_entry(
    AbstractInterpreter::MethodKind kind) {
  address entry_point = NULL;

  switch (kind) {
  case Interpreter::zerolocals:
  case Interpreter::zerolocals_synchronized:
    break;

  case Interpreter::native:
    entry_point = ((InterpreterGenerator*) this)->generate_native_entry(false);
    break;

  case Interpreter::native_synchronized:
    entry_point = ((InterpreterGenerator*) this)->generate_native_entry(false);
    break;

  case Interpreter::empty:
    entry_point = ((InterpreterGenerator*) this)->generate_empty_entry();
    break;

  case Interpreter::accessor:
    entry_point = ((InterpreterGenerator*) this)->generate_accessor_entry();
    break;

  case Interpreter::abstract:
    entry_point = ((InterpreterGenerator*) this)->generate_abstract_entry();
    break;

  case Interpreter::method_handle:
    entry_point = ((InterpreterGenerator*) this)->generate_method_handle_entry();
    break;

  case Interpreter::java_lang_math_sin:
  case Interpreter::java_lang_math_cos:
  case Interpreter::java_lang_math_tan:
  case Interpreter::java_lang_math_abs:
  case Interpreter::java_lang_math_log:
  case Interpreter::java_lang_math_log10:
  case Interpreter::java_lang_math_sqrt:
    entry_point = ((InterpreterGenerator*) this)->generate_math_entry(kind);
    break;

  case Interpreter::java_lang_ref_reference_get:
    entry_point = ((InterpreterGenerator*)this)->generate_Reference_get_entry();
    break;

  default:
    ShouldNotReachHere();
  }

  if (entry_point == NULL)
    entry_point = ((InterpreterGenerator*) this)->generate_normal_entry(false);

  return entry_point;
}

InterpreterGenerator::InterpreterGenerator(StubQueue* code)
 : CppInterpreterGenerator(code) {
   generate_all();
}

// Deoptimization helpers

InterpreterFrame *InterpreterFrame::build(int size, TRAPS) {
  ZeroStack *stack = ((JavaThread *) THREAD)->zero_stack();

  int size_in_words = size >> LogBytesPerWord;
  assert(size_in_words * wordSize == size, "unaligned");
  assert(size_in_words >= header_words, "too small");
  stack->overflow_check(size_in_words, CHECK_NULL);

  stack->push(0); // next_frame, filled in later
  intptr_t *fp = stack->sp();
  assert(fp - stack->sp() == next_frame_off, "should be");

  stack->push(INTERPRETER_FRAME);
  assert(fp - stack->sp() == frame_type_off, "should be");

  interpreterState istate =
    (interpreterState) stack->alloc(sizeof(BytecodeInterpreter));
  assert(fp - stack->sp() == istate_off, "should be");
  istate->set_self_link(NULL); // mark invalid

  stack->alloc((size_in_words - header_words) * wordSize);

  return (InterpreterFrame *) fp;
}

int AbstractInterpreter::layout_activation(methodOop method,
                                           int       tempcount,
                                           int       popframe_extra_args,
                                           int       moncount,
                                           int       caller_actual_parameters,
                                           int       callee_param_count,
                                           int       callee_locals,
                                           frame*    caller,
                                           frame*    interpreter_frame,
                                           bool      is_top_frame) {
  assert(popframe_extra_args == 0, "what to do?");
  assert(!is_top_frame || (!callee_locals && !callee_param_count),
         "top frame should have no caller");

  // This code must exactly match what InterpreterFrame::build
  // does (the full InterpreterFrame::build, that is, not the
  // one that creates empty frames for the deoptimizer).
  //
  // If interpreter_frame is not NULL then it will be filled in.
  // It's size is determined by a previous call to this method,
  // so it should be correct.
  //
  // Note that tempcount is the current size of the expression
  // stack.  For top most frames we will allocate a full sized
  // expression stack and not the trimmed version that non-top
  // frames have.

  int header_words        = InterpreterFrame::header_words;
  int monitor_words       = moncount * frame::interpreter_frame_monitor_size();
  int stack_words         = is_top_frame ? method->max_stack() : tempcount;
  int callee_extra_locals = callee_locals - callee_param_count;

  if (interpreter_frame) {
    intptr_t *locals        = interpreter_frame->fp() + method->max_locals();
    interpreterState istate = interpreter_frame->get_interpreterState();
    intptr_t *monitor_base  = (intptr_t*) istate;
    intptr_t *stack_base    = monitor_base - monitor_words;
    intptr_t *stack         = stack_base - tempcount - 1;

    BytecodeInterpreter::layout_interpreterState(istate,
                                                 caller,
                                                 NULL,
                                                 method,
                                                 locals,
                                                 stack,
                                                 stack_base,
                                                 monitor_base,
                                                 NULL,
                                                 is_top_frame);
  }
  return header_words + monitor_words + stack_words + callee_extra_locals;
}

void BytecodeInterpreter::layout_interpreterState(interpreterState istate,
                                                  frame*    caller,
                                                  frame*    current,
                                                  methodOop method,
                                                  intptr_t* locals,
                                                  intptr_t* stack,
                                                  intptr_t* stack_base,
                                                  intptr_t* monitor_base,
                                                  intptr_t* frame_bottom,
                                                  bool      is_top_frame) {
  istate->set_locals(locals);
  istate->set_method(method);
  istate->set_self_link(istate);
  istate->set_prev_link(NULL);
  // thread will be set by a hacky repurposing of frame::patch_pc()
  // bcp will be set by vframeArrayElement::unpack_on_stack()
  istate->set_constants(method->constants()->cache());
  istate->set_msg(BytecodeInterpreter::method_resume);
  istate->set_bcp_advance(0);
  istate->set_oop_temp(NULL);
  istate->set_mdx(NULL);
  if (caller->is_interpreted_frame()) {
    interpreterState prev = caller->get_interpreterState();
    prev->set_callee(method);
    if (*prev->bcp() == Bytecodes::_invokeinterface)
      prev->set_bcp_advance(5);
    else
      prev->set_bcp_advance(3);
  }
  istate->set_callee(NULL);
  istate->set_monitor_base((BasicObjectLock *) monitor_base);
  istate->set_stack_base(stack_base);
  istate->set_stack(stack);
  istate->set_stack_limit(stack_base - method->max_stack() - 1);
}

address CppInterpreter::return_entry(TosState state, int length) {
  ShouldNotCallThis();
}

address CppInterpreter::deopt_entry(TosState state, int length) {
  return NULL;
}

// Helper for (runtime) stack overflow checks

int AbstractInterpreter::size_top_interpreter_activation(methodOop method) {
  return 0;
}

// Helper for figuring out if frames are interpreter frames

bool CppInterpreter::contains(address pc) {
#ifdef PRODUCT
  ShouldNotCallThis();
#else
  return false; // make frame::print_value_on work
#endif // !PRODUCT
}

// Result handlers and convertors

address CppInterpreterGenerator::generate_result_handler_for(
    BasicType type) {
  assembler()->advance(1);
  return ShouldNotCallThisStub();
}

address CppInterpreterGenerator::generate_tosca_to_stack_converter(
    BasicType type) {
  assembler()->advance(1);
  return ShouldNotCallThisStub();
}

address CppInterpreterGenerator::generate_stack_to_stack_converter(
    BasicType type) {
  assembler()->advance(1);
  return ShouldNotCallThisStub();
}

address CppInterpreterGenerator::generate_stack_to_native_abi_converter(
    BasicType type) {
  assembler()->advance(1);
  return ShouldNotCallThisStub();
}

#endif // CC_INTERP
