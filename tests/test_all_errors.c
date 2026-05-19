// ============================================================
// Test file for ALL AD + AT errors (32 total)
// Uncomment ONE error block at a time to test
// Re-comment after testing and move to the next
// ============================================================

// === BASE CODE (keep uncommented) ===

struct S{
	int n;
	char text[16];
};

struct S a;
struct S v[10];
int x;
int y;
char ch;
double d;
int arr[5];

void f(char text[], int i, char ch){
}

int h(int x, int y){
	return x + y;
}

// ============================================================
// AD ERRORS (6)
// ============================================================

// --- ERR01: AD: symbol redefinition (variable) ---
// int x;
// Expected: AD: symbol redefinition: x

// --- ERR02: AD: symbol redefinition (struct) ---
// struct S{ int z; };
// Expected: SD: symbol redefinition: S

// --- ERR03: AD: symbol redefinition (function) ---
// void f(){ }
// Expected: AD: symbol redefinition: f

// --- ERR04: AD: symbol redefinition (parameter) ---
// void dup(int a, int a){ }
// Expected: AD: symbol redefinition: a

// --- ERR05: AD: undefined struct ---
// struct Undefined u;
// Expected: AD: undefined struct: Undefined

// --- ERR06: AD: vector variable must have dimension ---
// int nosize[];
// Expected: AD: a vector variable must have a specified dimension

// ============================================================
// AT ERRORS - stm (6)
// ============================================================

// --- ERR07: AT: if condition must be scalar ---
// void err07(){ if(v) x=1; }
// Expected: AT: the if condition must be a scalar value

// --- ERR08: AT: while condition must be scalar ---
// void err08(){ while(v) x=1; }
// Expected: AT: the while condition must be a scalar value

// --- ERR09: AT: void function cannot return a value ---
// void err09(){ return 1; }
// Expected: AT: a void function cannot return a value

// --- ERR10: AT: non-void function must return a value ---
// int err10(){ return; }
// Expected: AT: a non-void function must return a value

// --- ERR11: AT: return value must be scalar ---
// int err11(){ return v; }
// Expected: AT: the return value must be a scalar value

// --- ERR12: AT: cannot convert return type ---
// int err12(){ return a; }
// Expected: AT: cannot convert the return expression type to the function return type

// ============================================================
// AT ERRORS - exprAssign (5)
// ============================================================

// --- ERR13: AT: assign destination must be left-value ---
// void err13(){ 3 = x; }
// Expected: AT: the assign destination must be a left-value

// --- ERR14: AT: assign destination cannot be constant ---
// void err14(){ v = v; }
// Expected: AT: the assign destination cannot be constant

// --- ERR15: AT: assign destination must be scalar ---
// (hard to trigger - array/void can't be lval easily)
// NOTE: this error is guarded by lval/ct checks first

// --- ERR16: AT: assign source must be scalar ---
// (hard to trigger - most expressions are scalar)
// NOTE: this error is guarded by other checks first

// --- ERR17: AT: assign source cannot be converted ---
// void err17(){ x = a; }
// Expected: AT: the assign source cannot be converted to destination

// ============================================================
// AT ERRORS - binary operators (6)
// ============================================================

// --- ERR18: AT: invalid operand type for || ---
// void err18(){ x = a || 1; }
// Expected: AT: invalid operand type for ||

// --- ERR19: AT: invalid operand type for && ---
// void err19(){ x = a && 1; }
// Expected: AT: invalid operand type for &&

// --- ERR20: AT: invalid operand type for == or != ---
// void err20(){ x = a == 1; }
// Expected: AT: invalid operand type for ==

// --- ERR21: AT: invalid operand type for < <= > >= ---
// void err21(){ x = a < 1; }
// Expected: AT: invalid operand type for <

// --- ERR22: AT: invalid operand type for + or - ---
// void err22(){ x = a + 1; }
// Expected: AT: invalid operand type for +

// --- ERR23: AT: invalid operand type for * or / ---
// void err23(){ x = a * 1; }
// Expected: AT: invalid operand type for *

// ============================================================
// AT ERRORS - exprCast (4)
// ============================================================

// --- ERR24: AT: cannot convert to a struct type ---
// void err24(){ x = (struct S)1; }
// Expected: AT: cannot convert to a struct type

// --- ERR25: AT: cannot convert a struct ---
// void err25(){ x = (int)a; }
// Expected: AT: cannot convert a struct

// --- ERR26: AT: array can only be converted to another array ---
// void err26(){ x = (int)v; }
// Expected: AT: an array can be converted only to another array

// --- ERR27: AT: scalar can only be converted to another scalar ---
// void err27(){ d = (double[])d; }
// Expected: AT: a scalar can be converted only to another scalar

// ============================================================
// AT ERRORS - exprUnary (1)
// ============================================================

// --- ERR28: AT: unary operator must have scalar operand ---
// void err28(){ x = -v; }
// Expected: AT: unary - must have a scalar operand

// ============================================================
// AT ERRORS - exprPostfix (4)
// ============================================================

// --- ERR29: AT: only an array can be indexed ---
// void err29(){ x = ch[0]; }
// Expected: AT: only an array can be indexed

// --- ERR30: AT: index is not convertible to int ---
// void err30(){ x = arr[a]; }
// Expected: AT: the index is not convertible to int

// --- ERR31: AT: field can only be selected from struct ---
// void err31(){ x = x.n; }
// Expected: AT: a field can only be selected from a struct

// --- ERR32: AT: structure does not have field ---
// void err32(){ x = a.z; }
// Expected: AT: the structure S does not have a field z

// ============================================================
// AT ERRORS - exprPrimary (6)
// ============================================================

// --- ERR33: AT: undefined id ---
// void err33(){ x = nedefinit; }
// Expected: AT: undefined id: nedefinit

// --- ERR34: AT: only a function can be called ---
// void err34(){ x(); }
// Expected: AT: only a function can be called

// --- ERR35: AT: a function can only be called ---
// void err35(){ x = h; }
// Expected: AT: a function can only be called

// --- ERR36: AT: too many arguments ---
// void err36(){ h(1, 2, 3); }
// Expected: AT: too many arguments in function call

// --- ERR37: AT: too few arguments ---
// void err37(){ h(1); }
// Expected: AT: too few arguments in function call

// --- ERR38: AT: cannot convert argument type ---
// void err38(){ h(a, 1); }
// Expected: AT: in call, cannot convert the argument type to the parameter type
