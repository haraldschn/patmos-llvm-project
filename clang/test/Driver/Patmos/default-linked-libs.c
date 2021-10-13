// RUN: %clang --target=patmos %s -o %t
// RUN: llvm-objdump -d %t | FileCheck %s
// END.
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tests that the correct libs are linked by default
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// Declare the functions in the mock libs
extern int patmos_mock_crt0_function(int);
extern int patmos_mock_crtbegin_function(int);
extern int patmos_mock_crtend_function(int);
extern int patmos_mock_libc_function(int);
extern int patmos_mock_libm_function(int);
extern int patmos_mock_libpatmos_function(int);
extern int patmos_mock_librt_function(int);

// CHECK-DAG: <patmos_mock_crt0_function>:
// CHECK-DAG: <patmos_mock_crtbegin_function>:
// CHECK-DAG: <patmos_mock_crtend_function>:
// CHECK-DAG: <patmos_mock_librt_function>:
// CHECK-DAG: <patmos_mock_libc_function>:
// CHECK-DAG: <patmos_mock_libm_function>:
// CHECK-DAG: <patmos_mock_libpatmos_function>:

// CHECK-DAG: <main>:
int main() { 
	return 
		patmos_mock_crt0_function(
		patmos_mock_crtbegin_function(
		patmos_mock_crtend_function(
		patmos_mock_librt_function(
		patmos_mock_libpatmos_function(
		patmos_mock_libc_function(
		patmos_mock_libm_function(
		0)))))));
}