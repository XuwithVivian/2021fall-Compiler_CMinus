; ModuleID = 'while.c'
source_filename = "while.c"

; target的开始
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"
; target的结束

; 全局main函数的定义
; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
    %1 = alloca i32  ; int a
    %2 = alloca i32  ; int i
    store i32 10, i32* %1  ; a = 10
    store i32 0, i32* %2   ; i = 0
    br label %3

3:
    %4 = load i32, i32* %2
    ; while(i < 10)
    %5 = icmp slt i32 %4, 10
    br i1 %5, label %6, label %11

6:
    %7 = load i32, i32* %2
    %8 = add i32 %7, 1  ; i = i + 1
    %9 = load i32, i32* %1
    %10 = add i32 %8, %9  ; a = a + i
    store i32 %8, i32* %2
    store i32 %10, i32* %1
    br label %3

11:
    %12 = load i32, i32* %1
    ret i32 %12  ; return a
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}