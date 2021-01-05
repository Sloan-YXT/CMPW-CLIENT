client:dht11.o
	g++ $^ -o client -lwiringPi -pthread -g
%.o:%.cpp
	g++ -c $< -o $@  -std=c++14 -g
clean:
	rm *.o client
#g++ faceDetect.cpp -o faceDetct  `pkg-config opencv4 --cflags --libs`