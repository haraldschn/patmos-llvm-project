; RUN: EXEC_ARGS="0=0 1=1 2=2 6=6"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset with a lenght of 513 without needing standard library memset
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main(i32 %set_to)  {
entry:
  %ptr = alloca i8, i32 513
  %set_to_i8 = trunc i32 %set_to to i8
  call void @llvm.memset.p0i8.i32(i8* %ptr, i8 %set_to_i8, i32 513, i1 false)
  br label %for.cond
  
for.cond:
  %i = phi i32 [513, %entry], [%i.decd, %for.body]
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
!1 = !{!"llvm.loop.bound", i32 513, i32 513}