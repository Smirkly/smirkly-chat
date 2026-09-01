CLANG_FORMAT ?= clang-format

.PHONY: configure-debug build-debug test-debug configure-release build-release format

configure-debug:
	cmake --preset debug

build-debug: configure-debug
	cmake --build --preset debug --parallel

test-debug: build-debug
	ctest --preset debug

configure-release:
	cmake --preset release

build-release: configure-release
	cmake --build --preset release --parallel

format:
	find src tests/unit -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i
