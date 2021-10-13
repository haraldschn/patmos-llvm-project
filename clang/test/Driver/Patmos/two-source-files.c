// RUN: %clang --target=patmos %s %S/helpers/helper-function.c -o %t
// RUN: llvm-objdump -d %t | FileCheck %s
// END.
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tests that the correct libs are linked by default
//
///////////////////////////////////////////////////////////////////////////////////////////////////
extern int helper_source_function(int);

// CHECK-DAG: <main>:
int main() { 
	return helper_source_function(4);
}
// CHECK-DAG: <helper_source_function>: