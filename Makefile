CC=gcc
LD=gcc

OPTLIBS=-lv4l2
CFLAGS=-g -O2 -Wall -Wextra -Isrc -rdynamic -DNDEBUG $(OPTFLAGS)
LIBS=-ldl $(OPTLIBS)
PREFIX?=/usr/local
LIBNAME=v4l2cam

SRC_DIR=./src
OBJ_DIR=./objs
BUILD_DIR=./build
TESTS_DIR=./tests

SOURCES=$(wildcard $(SRC_DIR)/**/*.c $(SRC_DIR)/*.c)
OBJECTS=$(SOURCES:%.c=%.o)

TEST_SRC=$(wildcard $(TESTS_DIR)/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

TARGET=$(BUILD_DIR)/lib$(LIBNAME).a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

# The Target Build
all: $(TARGET) $(SO_TARGET) tests

target: build $(TARGET) $(SO_TARGET)

$(TARGET): CFLAGS += -fPIC
$(TARGET): $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@

$(SO_TARGET): $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

test: $(TARGET)
	$(CC) $(CFLAGS) $(TESTS_DIR)/main.c -o $@ -L$(BUILD_DIR) -l$(LIBNAME)

build:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BUILD_DIR)
	mkdir -p $(TESTS_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR) || true
	rm $(OBJECTS) || true
	rm $(TESTS) || true
	rm test || true