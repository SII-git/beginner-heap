# 설정
CC     = gcc
CXX    = g++
AR     = ar rcs
CFLAGS = -Wall -O2 -fPIC -Iinclude -g -fvisibility=hidden
CXXFLAGS = -Wall -O2 -fPIC -Iinclude -g -fvisibility=hidden

C_SOURCES   = src_c/heap.c
CPP_SOURCES = src_cpp/heap.cpp

C_OBJECTS   = $(C_SOURCES:.c=.o)
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)

STATIC_LIB_C   = libheapc.a
STATIC_LIB_CPP = libheapcpp.a

SHARED_LIB_C   = libheapc.so
SHARED_LIB_CPP = libheapcpp.so

.PHONY: all C CPP clean

all: C

# C 라이브러리 빌드
C: $(STATIC_LIB_C) $(SHARED_LIB_C)

$(STATIC_LIB_C): $(C_OBJECTS)
	$(AR) $(STATIC_LIB_C) $(C_OBJECTS)

$(SHARED_LIB_C): $(C_OBJECTS)
	$(CC) -shared -o $(SHARED_LIB_C) $(C_OBJECTS)

# C++ 라이브러리 빌드
CPP: $(STATIC_LIB_CPP) $(SHARED_LIB_CPP)

$(STATIC_LIB_CPP): $(CPP_OBJECTS)
	$(AR) $(STATIC_LIB_CPP) $(CPP_OBJECTS)

$(SHARED_LIB_CPP): $(CPP_OBJECTS)
	$(CXX) -shared -o $(SHARED_LIB_CPP) $(CPP_OBJECTS)


testcpp: test.cpp $(STATIC_LIB_CPP) $(SHARED_LIB_CPP)
	$(CXX) -O0 -g -Iinclude -DHEAP_CPP -o test test.cpp -L. -lheapcpp -Wl,-rpath=.


testc: test.cpp $(STATIC_LIB_C) $(SHARED_LIB_C)
	$(CXX) -O0 -g -Iinclude -DHEAP_C -o test test.cpp -L. -lheapc -Wl,-rpath=.

# 개별 소스 파일 컴파일
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 정리
clean:
	rm -f src_c/*.o src_cpp/*.o *.a *.so test
