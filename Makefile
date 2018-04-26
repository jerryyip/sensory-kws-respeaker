main:
	g++ main -o src/main.cc -Iinclude/ -Llib/ -lsnsr

clean:
	rm *.o main