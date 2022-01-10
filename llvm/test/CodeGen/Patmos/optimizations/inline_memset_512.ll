; RUN: llc < %s | FileCheck %s
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests when using the intrinsic memset functions, setting up to 512 bytes
; is inlined instead of calling the standard library 'memset'
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; CHECK-LABEL: set_512:
define void @set_512(i8* %dest1, i8 %value1, i8* %dest2, i8 %value2)  {
entry:
  ; CHECK-NOT: memset
  call void @llvm.memset.p0i8.i32(i8* %dest1, i8 %value1, i32 512, i1 false)
  call void @llvm.memset.p0i8.i64(i8* %dest2, i8 %value2, i64 512, i1 false)
  ret void
}

declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i1)
declare void @llvm.memset.p0i8.i64(i8*, i8, i64, i1)
