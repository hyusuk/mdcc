#!/bin/bash

test_() {
    expected="$1"
    input="$2"
    ./mdcc "$input" > tmp.s
    gcc -arch x86_64 -o tmp tmp.s
    ./tmp
    actual="$?"
    if [ "$actual" == "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

test_ 1 "int main() { return 1;}"
test_ 3 "int main() {return 1 + 2;}"
test_ 6 "int main() { return 3 * 2;}"
test_ 2 "int main() {return 4 / 2;}"
test_ 14 "int main() { return 3*2 + 2*4;}"
test_ 6 "int main() {return (1+2) * (5-3);}"
test_ 3 "int main() { 4 / 2; return 2 + 1;}"
test_ 3 "int main() { int a = 3; return a;}"
test_ 7 "int main() { int a = 1 + 2 * 3; return a;}"
test_ 2 "int main() { int a = 1; return a + 1;}"
test_ 1 "int z() { return 1; }  int main() { return z(); }"
test_ 3 "int z() { return 1; } int main() { return z() * 2 + 1; }"
test_ 2 "int main() { int a = 1; int b = 2; return b; }"
test_ 3 "int main() { int test1 = 1; int test2 = 2; int test3 = 3; int test4 = 4; int test5 = 5; return test3; }"
test_ 48 "int main() { return '0'; }"
test_ 97 "int main() { int a = 'a'; return a;}"
test_ 2 "int main() { int a = 1; a = 2; return a;}"
test_ 1 "int main() { int a = 1; int *b = &a; return *b; }"
test_ 2 "int main() { int a = 1; a *= 2; return a; }"
test_ 1 "int main() { int a = 2; a /= 2; return a; }"
test_ 2 "int main() { int a = 1; a += 1; return a; }"
test_ 1 "int main() { int a = 2; a -= 1; return a; }"
test_ 2 "int main() { int a = 1; { a = 2; } return a;}"
test_ 1 "int main() { int a = 1; { int a = 2; } return a;}"
test_ 1 "int z(int a) { return a; } int main() { return z(1); }"
test_ 44 "int sum(int a, int b, int c, int d, int e, int f) { return a*b + c*d + e*f; } int main() { return sum(1, 2, 3, 4, 5, 6); }"
test_ 123 "int main() { return 123;}"
test_ 1 "int main() { if (1) { return 1; } return 0;}"
test_ 0 "int main() { if (0) { return 1; } return 0;}"
test_ 2 "int main() { if (0) { return 1;} else { return 2;} return 3;}"
test_ 2 "int main() { int a = 1; if (a-1) { return 1; } else if (a) { return 2; } else { return 3; }}"
test_ 0 "int main() { int a = 1; int b = 1; if (a == b) { return 0; } else { return 1; }}"
test_ 1 "int main() { int a = 1; int b = 2; if (a == b) { return 0; } else { return 1; }}"
test_ 1 "int main() { int a = 1; int b = 1; if (a != b) { return 0; } else { return 1; }}"
test_ 0 "int main() { int a = 1; int b = 2; if (a != b) { return 0; } else { return 1; }}"
test_ 1 "int main() { int a; return 1; }"
test_ 1 "int main() { int a[1]; return 1; }"
test_ 1 "int main() { int a[2]; a[1] = 1; return a[1]; }"
test_ 3 "int main() { int a[2]; a[0] = 1; a[1] = 2; return a[0]+a[1]; }"
test_ 6 "int main() { int a[2][2]; a[0][0] = 1; a[0][1] = 2; a[1][1] = 3; return a[0][0] + a[0][1] + a[1][1]; }"
test_ 6 "int main() { int a[2][2][1]; a[0][0][0] = 1; a[0][1][0] = 2; a[1][0][0] = 3; return a[0][0][0] + a[0][1][0] + a[1][0][0]; }"
test_ 1 "int f(int a[1]) { return a[0]; return 0; } int main() {int a[1]; a[0] = 1; return f(a); }"
test_ 1 "int f(int a[2]) { return a[1]; return 0; } int main() {int a[2]; a[1] = 1; return f(a); }"
test_ 1 "int f(int a[2][2]) { return a[1][0]; return 0; } int main() {int a[2][2]; a[1][0] = 1; return f(a); }"
test_ 2 "int f(int *a) { *a = 2; return 0; } int main() { int a; a = 1; f(&a); return a;}"
test_ 2 "int main() { int a; int b; for (a = 0; a != 3; a = a + 1) {b = a;} return b; }"
test_ 1 "int main() { int a[1]; int i = 0; for (i = 0; i != 1; i = i + 1) { a[i] = 1; } return a[0]; }"
test_ 2 "int main() { int a[2][2]; int i; int j; for (i = 0; i != 2; i = i + 1) { for (j = 0; j != 2; j = j + 1) { a[i][j] = i + j; }}  return a[1][1]; }"
echo OK
