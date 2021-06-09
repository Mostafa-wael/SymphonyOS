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
# call it like this "make test algo=5 q=10", I have added some initial values to them. So "make test" is valid too
algo=1
q=5
mem=1
test:
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc process_generator.c -o process_generator.out
	gcc test_generator.c -o test_generator.out 
	./test_generator.out 
	./process_generator.out processes.txt $(algo) $(mem) $(q) # pass the algorithm and the quanta
