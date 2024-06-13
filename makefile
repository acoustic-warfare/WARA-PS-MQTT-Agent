BUILD_DIR := build

all: build

build: 
	cmake -S . -B ${BUILD_DIR}
	cmake --build ${BUILD_DIR}

clean:
	rm -rf ${BUILD_DIR}