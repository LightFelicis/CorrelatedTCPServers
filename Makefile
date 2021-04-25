CXXFLAGS+=-g -Wall -std=c++17
LDLIBS+=-lstdc++fs
FILES = src/server/connectionHandler.h src/utils/httpParsers.h

serwer: assertions.o pathUtils.o
	g++ ${CXXFLAGS} ${LDLIBS} -o serwer src/server/server.cpp ${FILES} pathUtils.o  assertions.o

assertions.o: src/utils/serverAssertions.cpp src/utils/serverAssertions.h
	g++ ${CXXFLAGS} ${LDLIBS} -c src/utils/serverAssertions.cpp -o assertions.o

pathUtils.o: src/utils/pathUtils.h src/utils/pathUtils.cpp
	g++ ${CXXFLAGS} ${LDLIBS} -c src/utils/pathUtils.cpp -o pathUtils.o

clean:
	rm pathUtils.o && rm assertions.o && rm serwer



