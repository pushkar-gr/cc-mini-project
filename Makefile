CXX      = g++
CXXFLAGS = -Wall -std=c++17 $(shell pkg-config --cflags fuse)
LDFLAGS  = $(shell pkg-config --libs fuse)

SRCS    = src/main.cpp src/path.cpp src/cow.cpp src/whiteout.cpp
TARGET  = bin/unionfs

.PHONY: all clean

all: bin $(TARGET)

bin:
	mkdir -p bin

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf bin
