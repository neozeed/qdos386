int table[3] = {11, 22, 33};
char msg[] = "DATA!";
int zbss[5];
int counter;
int get(int i) { return table[i] + zbss[i] + counter; }
int main(void) { zbss[1] = 7; counter = 4; return get(1) + msg[0]; }
