CC ?= cc
EMCC ?= emcc
PYTHON ?= python3
PORT ?= 8000
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -O2
CPPFLAGS ?= -I.
LDLIBS ?= -lm
WEB_BUILD_DIR := build/web/site

ifeq ($(origin NODE), undefined)
ifneq ($(EMSDK_NODE),)
NODE := $(EMSDK_NODE)
else
NODE := node
endif
endif

.PHONY: all example test web web-test web-serve clean

all: example

build/example: examples/example.c cbounce.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)

example test: build/example
	./build/example

$(WEB_BUILD_DIR)/cbounce_demo.mjs: examples/web/demo.c cbounce.h Makefile
	mkdir -p $(WEB_BUILD_DIR)
	$(EMCC) $(CPPFLAGS) -std=c99 -O2 --no-entry $< -o $@ \
		-sMODULARIZE=1 \
		-sEXPORT_ES6=1 \
		-sEXPORT_NAME=createCbounceDemo \
		-sENVIRONMENT=web,node \
		-sALLOW_MEMORY_GROWTH=1 \
		-sINITIAL_MEMORY=33554432 \
		-sNO_EXIT_RUNTIME=1 \
		-sFILESYSTEM=0 \
		-sEXPORTED_RUNTIME_METHODS='["HEAPF32"]' \
		-sEXPORTED_FUNCTIONS='["_demo_reset","_demo_step","_demo_move_player","_demo_shoot_projectile","_demo_render_count","_demo_render_stride","_demo_render_data","_demo_player_x","_demo_player_y","_demo_player_z","_demo_active_projectiles","_demo_total_targets","_demo_toppled_targets","_demo_body_count"]'

$(WEB_BUILD_DIR)/cbounce_demo.wasm: $(WEB_BUILD_DIR)/cbounce_demo.mjs
	@test -f $@ || $(MAKE) -B $(WEB_BUILD_DIR)/cbounce_demo.mjs

$(WEB_BUILD_DIR)/index.html: examples/web/index.html
	mkdir -p $(WEB_BUILD_DIR)
	cp $< $@

$(WEB_BUILD_DIR)/main.js: examples/web/main.js
	mkdir -p $(WEB_BUILD_DIR)
	cp $< $@

$(WEB_BUILD_DIR)/.nojekyll:
	mkdir -p $(WEB_BUILD_DIR)
	touch $@

web: $(WEB_BUILD_DIR)/index.html $(WEB_BUILD_DIR)/main.js $(WEB_BUILD_DIR)/cbounce_demo.mjs $(WEB_BUILD_DIR)/cbounce_demo.wasm $(WEB_BUILD_DIR)/.nojekyll

web-test: web
	$(NODE) examples/web/smoke_test.mjs

web-serve: web
	$(PYTHON) -m http.server $(PORT) --directory $(WEB_BUILD_DIR)

clean:
	rm -rf build
