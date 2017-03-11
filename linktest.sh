make
filename=testcases/merge.tig

./a.out hell.tig > debug2
gcc -Wl,--wrap,getchar -m32 hell.tig.s runtime.c -o hello.out


#./a.out $filename > debug2
#gcc -Wl,--wrap,getchar -m32 ${filename}.s runtime.c -o hello.out

./hello.out
