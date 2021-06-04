build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt *.o

all: clean build

run:
	./process_generator.out processes.txt 1

testGen:
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc process_generator.c -o process_generator.out
	gcc test_generator.c -o test_generator.out 
	./test_generator.out 
	./process_generator.out processes.txt 1
