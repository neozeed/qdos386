\qemu.mingw64\qemu-system-i386 ^
   -drive file=kernels/qdos386-r17-vga.img,format=raw,if=ide,index=0 ^
   -drive file=examples/system-r17-mbr.img,format=raw,if=ide,index=1 ^
   -m 128 -cpu Nehalem ^
   -smp 8,sockets=1,cores=4,threads=2 ^
   -accel tcg,thread=multi