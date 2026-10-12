.PHONY: all psp test clean package smoke-build smoke
all: psp

psp:
	@command -v psp-config >/dev/null || { echo 'Run bash scripts/install-pspdev.sh, then source scripts/env.sh'; exit 1; }
	mkdir -p build/psp
	$(MAKE) -C build/psp -f ../../Makefile.psp

test:
	mkdir -p build/host
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror -pedantic -Isrc tests/test_skate.c src/skate.c -lm -o build/host/test_skate
	./build/host/test_skate
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror -pedantic -Isrc tests/test_session.c src/session.c src/records.c src/skate.c -lm -o build/host/test_session
	./build/host/test_session

smoke-build:
	@command -v psp-config >/dev/null || { echo 'Source scripts/env.sh first'; exit 1; }
	mkdir -p build/psp-smoke
	$(MAKE) -C build/psp-smoke -f ../../Makefile.psp SMOKE_TEST=1

smoke: smoke-build
	python3 scripts/smoke-test.py

package: psp
	mkdir -p dist/PSP/GAME/SidewalkSessions
	cp build/psp/EBOOT.PBP dist/PSP/GAME/SidewalkSessions/EBOOT.PBP
	python3 scripts/package.py

clean:
	rm -rf build dist
