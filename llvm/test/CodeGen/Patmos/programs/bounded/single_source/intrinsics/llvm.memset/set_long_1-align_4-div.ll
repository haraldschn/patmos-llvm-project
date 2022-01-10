; RUN: EXEC_ARGS="0=0 1=52 2=104 4=208"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without standard library 'memset' for:
; * High length
; * 1-Aligned start
; * 4-divisable length
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main(i32 %set_to)  {
entry:
  %ptr.0 = alloca i8, i32 53, align 4
  ; Increment the pointer to guarantee that it is now 1-aligned
  %ptr = getelementptr i8, i8* %ptr.0, i32 1
  %set_to_i8 = trunc i32 %set_to to i8
  call void @llvm.memset.p0i8.i32(i8* %ptr, i8 %set_to_i8, i32 52, i1 false)
  br label %for.cond
  
for.cond:
  %i = phi i32 [52, %entry], [%i.decd, %for.body]
  %dest = phi i8* [%ptr, %entry], [%dest.incd, %for.body]
  %sum = phi i8 [0, %entry], [%sum.next, %for.body]
  %stop = icmp eq i32 %i, 0
  br i1 %stop, label %for.end, label %for.body, !llvm.loop !0

for.body:
  %read = load volatile i8, i8* %dest
  %sum.next = add i8 %sum, %read
  %i.decd = sub i32 %i, 1
  %dest.incd = getelementptr i8, i8* %dest, i32 1
  br label %for.cond
  
for.end:
  %result = zext i8 %sum to i32
  ret i32 %result
}

declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i1)

!0 = !{!0, !1}
!1 = !{!"llvm.loop.bound", i32 52, i32 52}