CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2
LDLIBS  := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
TARGET  := build/desktop/template
SRC     := $(wildcard *.c)

-include .env
WEB_DIR    := build/web

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS := -std=c99 -Wall -Wextra -g -O0
debug: clean $(TARGET)

web: $(SRC)
	@test -f $(RAYLIB_WEB)/libraylib.web.a || { echo "Missing $(RAYLIB_WEB)/libraylib.web.a (build raylib for web first)"; exit 1; }
	mkdir -p $(WEB_DIR)
	emcc $^ -o $(WEB_DIR)/index.html -Os -Wall -std=c99 \
		-I$(RAYLIB_WEB) -I$(RAYGUI_DIR) $(RAYLIB_WEB)/libraylib.web.a \
		-DPLATFORM_WEB -s USE_GLFW=3 -s ASYNCIFY -s ALLOW_MEMORY_GROWTH=1 \
		--shell-file $(RAYLIB_WEB)/minshell.html

serve: web
	python3 -m http.server -d $(WEB_DIR) 8000

clean:
	rm -f $(TARGET)
	rm -f $(WEB_DIR)/index.*

.PHONY: run debug web serve clean