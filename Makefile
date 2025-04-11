CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O3  # Cambiado de c++11 a c++14
LDFLAGS = -lrt

SRCS = main.cpp image_processor.cpp buddy_allocator.cpp stb_wrapper.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = image_processor

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean