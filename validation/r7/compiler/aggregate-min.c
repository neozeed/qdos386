struct S { unsigned int a, b, c; };
unsigned int sum(struct S x) { return x.a + x.b + x.c; }
int main(void) { struct S x; x.a=1; x.b=2; x.c=3; return sum(x)!=6; }
