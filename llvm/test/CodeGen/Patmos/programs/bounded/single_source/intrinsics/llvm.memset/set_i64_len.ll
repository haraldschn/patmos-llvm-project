; RUN: EXEC_ARGS="0=0 1=1 2=2 100=100"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset with an i64 length argument
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main(i32 %set_to)  {
entry:
  %ptr = alloca i8
  %set_to_i8 = trunc i32 %set_to to i8
  call void @llvm.memset.p0i8.i64(i8* %ptr, i8 %set_to_i8, i64 1, i1 false)
  %read = load volatile i8, i8 *%ptr
  %read_i32 = zext i8 %read to i32
  ret i32 %read_i32
}

declare void @llvm.memset.p0i8.i64(i8*, i8, i64, i1)
