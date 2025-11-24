
all : test
test: ../my_vm.h
	gcc test.c -L../ -lmy_vm -o test
	gcc multi_test.c -L../ -lmy_vm -o mtest -lpthread 

#modified test
	gcc functions_test.c -L../ -lmy_vm -o ftest -lpthread

clean:
	rm -rf test mtest ftest
