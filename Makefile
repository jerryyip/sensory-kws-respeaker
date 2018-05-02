# main:
# 	g++ src/main.cc -o main -Iinclude/ -Llib/ -lsnsr -std=c++11 -fpermissive -lasound -lpthread -g

# clean:
# 	rm *.o main


main: sensory.o
	g++ src/main_readfromstream.cc -o main sensory.o -Iinclude/ -Llib/ -lsnsr -std=c++11 -fpermissive -lasound -lpthread -I/usr/local/include/respeaker -lrespeaker -lsndfile -g

sensory.o:
	g++ -c src/sensory.cc -Iinclude/ -Llib/ -lsnsr -std=c++11 -fpermissive -lasound -lpthread -g

clean:
	rm *.o main
