; RUN: EXEC_ARGS="0=0 1=3 2=6 60=180"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without needing the standard library "memset" for 3 bytes
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main(i32 %set_to)  {
entry:
  %ptr = alloca i8, i32 4
  %set_to_i8 = trunc i32 %set_to to i8
  call void @llvm.memset.p0i8.i32(i8* %ptr, i8 %set_to_i8, i32 3, i1 false)
  %ptr_1 = getelementptr i8, i8* %ptr, i32 1
  %ptr_2 = getelementptr i8, i8* %ptr, i32 2
  %read_0 = load volatile i8, i8 *%ptr
  %read_1 = load volatile i8, i8 *%ptr_1
  %read_2 = load volatile i8, i8 *%ptr_2
  %add1 = add i8 %read_0, %read_1
  %add2 = add i8 %add1, %read_2
  %result = zext i8 %add2 to i32
  ret i32 %result
}

declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i1)
