// RUN: %clang --target=patmos %s %S/helpers/helper-function.c -o %t 
// RUN: llvm-objdump -d %t | FileCheck %s
// END.
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tests that if multiple source files are compiled together,
// all of their items are present in the final binary, even when not used.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// CHECK-DAG: <main>:
int main() { 
	return 6;
}

// CHECK-DAG: <unused_function>:
int unused_function()  {
	return 4;
}
// CHECK-DAG: <helper_source_function>: