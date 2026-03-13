/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Wrapper for LLVM header file #includes.
 */


#ifndef LP_BLD_H
#define LP_BLD_H


/**
 * @file
 * LLVM IR building helpers interfaces.
 *
 * We use LLVM-C bindings for now. They are not documented, but follow the C++
 * interfaces very closely, and appear to be complete enough for code
 * genration. See
 * http://npcontemplation.blogspot.com/2008/06/secret-of-llvm-c-bindings.html
 * for a standalone example.
 */

#include <llvm/Config/llvm-config.h>

#include <llvm-c/Core.h>  

#if LLVM_VERSION_MAJOR >= 15
static inline LLVMTypeRef
lp_llvm_elem_type_from_aggregate_and_index(LLVMTypeRef type, LLVMValueRef index)
{
   LLVMTypeKind kind = LLVMGetTypeKind(type);

   if (kind == LLVMStructTypeKind) {
      if (LLVMGetValueKind(index) == LLVMConstantIntValueKind) {
         unsigned field = (unsigned)LLVMConstIntGetZExtValue(index);
         return LLVMStructGetTypeAtIndex(type, field);
      }
      return NULL;
   }

   if (kind == LLVMArrayTypeKind || kind == LLVMVectorTypeKind)
      return LLVMGetElementType(type);

   return NULL;
}


static inline LLVMTypeRef
lp_llvm_value_type_from_memory_ref(LLVMValueRef value)
{
   switch (LLVMGetValueKind(value)) {
   case LLVMGlobalVariableValueKind:
      return LLVMGlobalGetValueType(value);
   default:
      return NULL;
   }
}

static inline LLVMTypeRef
lp_llvm_pointee_type_from_gep(LLVMValueRef gep)
{
   LLVMTypeRef type = LLVMGetGEPSourceElementType(gep);
   int num_operands = LLVMGetNumOperands(gep);

   /* Operand 0 is the base pointer and operand 1 is the first index into
    * the pointer itself. Traverse from operand 2 to walk the pointee type. */
   for (int i = 2; i < num_operands; i++) {
      LLVMTypeRef next = lp_llvm_elem_type_from_aggregate_and_index(type,
                                                                     LLVMGetOperand(gep, i));
      if (!next)
         break;
      type = next;
   }

   return type;
}

static inline LLVMTypeRef
lp_llvm_pointee_type(LLVMValueRef pointer)
{
   LLVMTypeRef pointer_type = LLVMTypeOf(pointer);
   LLVMTypeRef elem;

   if (LLVMGetTypeKind(pointer_type) == LLVMPointerTypeKind) {
      elem = LLVMGetElementType(pointer_type);
      if (elem)
         return elem;
   }

   elem = lp_llvm_value_type_from_memory_ref(pointer);
   if (elem)
      return elem;

   if (LLVMGetValueKind(pointer) == LLVMInstructionValueKind) {
      LLVMOpcode op = LLVMGetInstructionOpcode(pointer);
      if (op == LLVMAlloca)
         return LLVMGetAllocatedType(pointer);
      if (op == LLVMGetElementPtr)
         return lp_llvm_pointee_type_from_gep(pointer);
      if (op == LLVMBitCast || op == LLVMAddrSpaceCast || op == LLVMIntToPtr)
         return lp_llvm_pointee_type(LLVMGetOperand(pointer, 0));
   } else if (LLVMGetValueKind(pointer) == LLVMConstantExprValueKind) {
      LLVMOpcode op = LLVMGetConstOpcode(pointer);
      if (op == LLVMGetElementPtr)
         return lp_llvm_pointee_type_from_gep(pointer);
      if (op == LLVMBitCast || op == LLVMAddrSpaceCast || op == LLVMIntToPtr)
         return lp_llvm_pointee_type(LLVMGetOperand(pointer, 0));
   }

   return LLVMInt8TypeInContext(LLVMGetTypeContext(pointer_type));
}

static inline LLVMValueRef
lp_llvm_build_load(LLVMBuilderRef B, LLVMValueRef PointerVal, const char *Name)
{
   return LLVMBuildLoad2(B, lp_llvm_pointee_type(PointerVal), PointerVal, Name);
}

static inline LLVMTypeRef
lp_llvm_function_type(LLVMValueRef function)
{
   LLVMTypeRef type = LLVMTypeOf(function);

   if (LLVMGetTypeKind(type) == LLVMPointerTypeKind) {
      LLVMTypeRef elem = LLVMGetElementType(type);
      if (elem && LLVMGetTypeKind(elem) == LLVMFunctionTypeKind)
         return elem;
   }

   switch (LLVMGetValueKind(function)) {
   case LLVMFunctionValueKind:
   case LLVMGlobalAliasValueKind:
   case LLVMGlobalIFuncValueKind:
      return LLVMGlobalGetValueType(function);
   default:
      return LLVMGetElementType(type);
   }
}

static inline LLVMValueRef
lp_llvm_build_call(LLVMBuilderRef B, LLVMValueRef Fn,
                   LLVMValueRef *Args, unsigned NumArgs,
                   const char *Name)
{
   return LLVMBuildCall2(B, lp_llvm_function_type(Fn),
                         Fn, Args, NumArgs, Name);
}

static inline LLVMValueRef
lp_llvm_build_gep(LLVMBuilderRef B, LLVMValueRef Pointer,
                  LLVMValueRef *Indices, unsigned NumIndices,
                  const char *Name)
{
   return LLVMBuildGEP2(B, lp_llvm_pointee_type(Pointer), Pointer,
                        Indices, NumIndices, Name);
}

#define LLVMBuildLoad(B, PointerVal, Name) \
   lp_llvm_build_load((B), (PointerVal), (Name))
#define LLVMBuildCall(B, Fn, Args, NumArgs, Name) \
   lp_llvm_build_call((B), (Fn), (Args), (NumArgs), (Name))
#define LLVMBuildGEP(B, Pointer, Indices, NumIndices, Name) \
   lp_llvm_build_gep((B), (Pointer), (Indices), (NumIndices), (Name))
#endif



/**
 * Redefine these LLVM entrypoints as invalid macros to make sure we
 * don't accidentally use them.  We need to use the functions which
 * take an explicit LLVMContextRef parameter.
 */
#define LLVMInt1Type ILLEGAL_LLVM_FUNCTION
#define LLVMInt8Type ILLEGAL_LLVM_FUNCTION
#define LLVMInt16Type ILLEGAL_LLVM_FUNCTION
#define LLVMInt32Type ILLEGAL_LLVM_FUNCTION
#define LLVMInt64Type ILLEGAL_LLVM_FUNCTION
#define LLVMIntType ILLEGAL_LLVM_FUNCTION
#define LLVMFloatType ILLEGAL_LLVM_FUNCTION
#define LLVMDoubleType ILLEGAL_LLVM_FUNCTION
#define LLVMX86FP80Type ILLEGAL_LLVM_FUNCTION
#define LLVMFP128Type ILLEGAL_LLVM_FUNCTION
#define LLVMPPCFP128Type ILLEGAL_LLVM_FUNCTION
#define LLVMStructType ILLEGAL_LLVM_FUNCTION
#define LLVMVoidType ILLEGAL_LLVM_FUNCTION
#define LLVMLabelType ILLEGAL_LLVM_FUNCTION
#define LLVMOpaqueType ILLEGAL_LLVM_FUNCTION
#define LLVMUnionType ILLEGAL_LLVM_FUNCTION
#define LLVMMDString ILLEGAL_LLVM_FUNCTION
#define LLVMMDNode ILLEGAL_LLVM_FUNCTION
#define LLVMConstString ILLEGAL_LLVM_FUNCTION
#define LLVMConstStruct ILLEGAL_LLVM_FUNCTION
#define LLVMAppendBasicBlock ILLEGAL_LLVM_FUNCTION
#define LLVMInsertBasicBlock ILLEGAL_LLVM_FUNCTION
#define LLVMCreateBuilder ILLEGAL_LLVM_FUNCTION

#if LLVM_VERSION_MAJOR >= 8
#define GALLIVM_HAVE_CORO 1
#else
#define GALLIVM_HAVE_CORO 0
#endif

#endif /* LP_BLD_H */
