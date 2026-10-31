gcc -c x.c
ld -o x x.o -lgcc -lc -lvt -lcurses -lgcc
copy /Y x \dos\qlay\sub2