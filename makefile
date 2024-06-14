BUILD_DIR := build

all: build

run: build
	./${BUILD_DIR}/main

build: 
	cmake -S . -B ${BUILD_DIR}
	cmake --build ${BUILD_DIR}

clean:
	rm -rf ${BUILD_DIR}