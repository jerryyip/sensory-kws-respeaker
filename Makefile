main:
	g++ src/main.cc -o main -Iinclude/ -Llib/ -lsnsr -std=c++11 -fpermissive -lasound -lpthread -g

clean:
	rm *.o main
