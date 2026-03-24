/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * Helper functions for manipulation structures.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"
#include "util/u_memory.h"

#include "lp_bld_const.h"
#include "lp_bld_debug.h"
#include "lp_bld_struct.h"


LLVMValueRef
lp_build_struct_get_ptr2(struct gallivm_state *gallivm,
                        LLVMTypeRef ptr_type,
                        LLVMValueRef ptr,
                        unsigned member,
                        const char *name)
{
   LLVMValueRef indices[2];
   LLVMValueRef member_ptr;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   indices[0] = lp_build_const_int32(gallivm, 0);
   indices[1] = lp_build_const_int32(gallivm, member);
   member_ptr = LLVMBuildGEP2(gallivm->builder, ptr_type, ptr, indices, ARRAY_SIZE(indices), "");
   lp_build_name(member_ptr, "%s.%s_ptr", LLVMGetValueName(ptr), name);
   return member_ptr;
}

LLVMValueRef
lp_build_struct_get_ptr(struct gallivm_state *gallivm,
                        LLVMValueRef ptr,
                        unsigned member,
                        const char *name)
{
   LLVMTypeRef ptr_type = lp_llvm_pointee_type(ptr);
   assert(ptr_type);
   assert(LLVMGetTypeKind(ptr_type) == LLVMStructTypeKind);
   return lp_build_struct_get_ptr2(gallivm, ptr_type, ptr, member, name);
}

LLVMValueRef
lp_build_struct_get2(struct gallivm_state *gallivm,
                    LLVMTypeRef ptr_type,
                    LLVMValueRef ptr,
                    unsigned member,
                    const char *name)
{
   LLVMValueRef member_ptr;
   LLVMValueRef res;
   member_ptr = lp_build_struct_get_ptr2(gallivm, ptr_type, ptr, member, name);
   res = LLVMBuildLoad2(gallivm->builder, LLVMStructGetTypeAtIndex(ptr_type, member), member_ptr, "");
   lp_build_name(res, "%s.%s", LLVMGetValueName(ptr), name);
   return res;
}

LLVMValueRef
lp_build_struct_get(struct gallivm_state *gallivm,
                    LLVMValueRef ptr,
                    unsigned member,
                    const char *name)
{
   LLVMTypeRef ptr_type = lp_llvm_pointee_type(ptr);
   assert(ptr_type);
   assert(LLVMGetTypeKind(ptr_type) == LLVMStructTypeKind);
   return lp_build_struct_get2(gallivm, ptr_type, ptr, member, name);
}

LLVMValueRef
lp_build_array_get_ptr2(struct gallivm_state *gallivm,
                        LLVMTypeRef array_type,
                        LLVMValueRef ptr,
                        LLVMValueRef index)
{
   LLVMValueRef indices[2];
   LLVMValueRef element_ptr;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   indices[0] = lp_build_const_int32(gallivm, 0);
   indices[1] = index;
   element_ptr = LLVMBuildGEP2(gallivm->builder, array_type, ptr, indices, ARRAY_SIZE(indices), "");
#ifdef DEBUG
   lp_build_name(element_ptr, "&%s[%s]",
                 LLVMGetValueName(ptr), LLVMGetValueName(index));
#endif
   return element_ptr;
}

LLVMValueRef
lp_build_array_get2(struct gallivm_state *gallivm,
                    LLVMTypeRef array_type,
                    LLVMValueRef ptr,
                    LLVMValueRef index)
{
   LLVMValueRef element_ptr;
   LLVMValueRef res;
   element_ptr = lp_build_array_get_ptr2(gallivm, array_type, ptr, index);
   res = LLVMBuildLoad2(gallivm->builder, LLVMGetElementType(array_type), element_ptr, "");
#ifdef DEBUG
   lp_build_name(res, "%s[%s]", LLVMGetValueName(ptr), LLVMGetValueName(index));
#endif
   return res;
}

LLVMValueRef
lp_build_array_get_ptr(struct gallivm_state *gallivm,
                       LLVMValueRef ptr,
                       LLVMValueRef index)
{
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   LLVMTypeRef array_type = lp_llvm_pointee_type(ptr);
   assert(array_type);
   assert(LLVMGetTypeKind(array_type) == LLVMArrayTypeKind);
   return lp_build_array_get_ptr2(gallivm, array_type, ptr, index);
}


LLVMValueRef
lp_build_array_get(struct gallivm_state *gallivm,
                   LLVMValueRef ptr,
                   LLVMValueRef index)
{
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   LLVMTypeRef array_type = lp_llvm_pointee_type(ptr);
   assert(array_type);
   assert(LLVMGetTypeKind(array_type) == LLVMArrayTypeKind);
   return lp_build_array_get2(gallivm, array_type, ptr, index);
}


void
lp_build_array_set(struct gallivm_state *gallivm,
                   LLVMValueRef ptr,
                   LLVMValueRef index,
                   LLVMValueRef value)
{
   LLVMValueRef element_ptr;
   LLVMTypeRef array_type = lp_llvm_pointee_type(ptr);
   assert(array_type);
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   assert(LLVMGetTypeKind(array_type) == LLVMArrayTypeKind);
   element_ptr = lp_build_array_get_ptr2(gallivm, array_type, ptr, index);
   LLVMBuildStore(gallivm->builder, value, element_ptr);
}


LLVMValueRef
lp_build_pointer_get_unaligned2(LLVMBuilderRef builder,
                               LLVMTypeRef ptr_type,
                               LLVMValueRef ptr,
                               LLVMValueRef index,
                               unsigned alignment)
{
   LLVMValueRef element_ptr;
   LLVMValueRef res;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   element_ptr = LLVMBuildGEP2(builder, ptr_type, ptr, &index, 1, "");
   res = LLVMBuildLoad2(builder, ptr_type, element_ptr, "");
   if (alignment)
      LLVMSetAlignment(res, alignment);
#ifdef DEBUG
   lp_build_name(res, "%s[%s]", LLVMGetValueName(ptr), LLVMGetValueName(index));
#endif
   return res;
}

LLVMValueRef
lp_build_pointer_get2(LLVMBuilderRef builder,
                     LLVMTypeRef ptr_type,
                     LLVMValueRef ptr,
                     LLVMValueRef index)
{
   return lp_build_pointer_get_unaligned2(builder, ptr_type, ptr, index, 0);
}

LLVMValueRef
lp_build_pointer_get(LLVMBuilderRef builder,
                     LLVMValueRef ptr,
                     LLVMValueRef index)
{
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   LLVMTypeRef ptr_type = lp_llvm_pointee_type(ptr);
   assert(ptr_type);
   return lp_build_pointer_get2(builder, ptr_type, ptr, index);
}

LLVMValueRef
lp_build_pointer_get_unaligned(LLVMBuilderRef builder,
                               LLVMValueRef ptr,
                               LLVMValueRef index,
                               unsigned alignment)
{
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   LLVMTypeRef ptr_type = lp_llvm_pointee_type(ptr);
   assert(ptr_type);
   return lp_build_pointer_get_unaligned2(builder, ptr_type, ptr, index, alignment);
}

void
lp_build_pointer_set2(LLVMBuilderRef builder,
                      LLVMTypeRef elem_type,
                      LLVMValueRef ptr,
                      LLVMValueRef index,
                      LLVMValueRef value)
{
   LLVMValueRef element_ptr;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   assert(elem_type);
   element_ptr = LLVMBuildGEP2(builder, elem_type, ptr, &index, 1, "");
   LLVMBuildStore(builder, value, element_ptr);
}


void
lp_build_pointer_set(LLVMBuilderRef builder,
                     LLVMValueRef ptr,
                     LLVMValueRef index,
                     LLVMValueRef value)
{
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   LLVMTypeRef elem_type = lp_llvm_pointee_type(ptr);
   assert(elem_type);
   lp_build_pointer_set2(builder, elem_type, ptr, index, value);
}


void
lp_build_pointer_set_unaligned2(LLVMBuilderRef builder,
                                LLVMTypeRef elem_type,
                                LLVMValueRef ptr,
                                LLVMValueRef index,
                                LLVMValueRef value,
                                unsigned alignment)
{
   LLVMValueRef element_ptr;
   LLVMValueRef instr;
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   assert(elem_type);
   element_ptr = LLVMBuildGEP2(builder, elem_type, ptr, &index, 1, "");
   instr = LLVMBuildStore(builder, value, element_ptr);
   LLVMSetAlignment(instr, alignment);
}


void
lp_build_pointer_set_unaligned(LLVMBuilderRef builder,
                               LLVMValueRef ptr,
                               LLVMValueRef index,
                               LLVMValueRef value,
                               unsigned alignment)
{
   assert(LLVMGetTypeKind(LLVMTypeOf(ptr)) == LLVMPointerTypeKind);
   LLVMTypeRef elem_type = lp_llvm_pointee_type(ptr);
   assert(elem_type);
   lp_build_pointer_set_unaligned2(builder, elem_type, ptr, index, value,
                                   alignment);
}
