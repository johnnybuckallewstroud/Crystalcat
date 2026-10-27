CC = gcc 
CFLAGS =  -g -std=c99 -Wall -D_DEFAULT_SOURCE `pkg-config --cflags gtk+-3.0 webkit2gtk-4.0 sqlite3 gtksourceview-3.0`
LDLIBS = `pkg-config --libs gtk+-3.0 webkit2gtk-4.0 sqlite3 gtksourceview-3.0` -lm

TARGET = crystalcat
SRCS = crystalcat.c
OBJS = $(SRCS:.c=.o)

# Installation paths
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

# Primary target
all: $(TARGET)

# Detect package manager and install compile dependencies
deps:
	@if command -v apt-get > /dev/null; then \
		sudo apt-get update && sudo apt-get install -y \
			libgtk-3-dev libwebkit2gtk-4.0-dev libsqlite3-dev libgtksourceview-3.0-dev pkg-config gcc; \
	elif command -v dnf > /dev/null; then \
		sudo dnf install -y \
			gtk3-devel webkit2gtk3-devel sqlite-devel gtksourceview3-devel pkgconf-pkg-config gcc; \
	elif command -v pacman > /dev/null; then \
		sudo pacman -S --needed --noconfirm \
			gtk3 webkit2gtk sqlite gtksourceview3 pkgconf gcc; \
	else \
		echo "Unsupported package manager. Please install dependencies manually."; exit 1; \
	fi

# Rule to link the final executable (spaces between variables fixed)
$(TARGET):$(OBJS)
	$(CC)  $(OBJS) -o $(TARGET) $(LDLIBS)

# Rule to compile C source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o$@

# Install dependencies, compile binary, and install to system PATH
install-deps: deps all install

# Copy built executable to system directory
install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)

# Remove executable from system directory
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)$(OBJS)

dev: clean all run

debug: all
	gdb ./$(TARGET)

valgrind: all
	valgrind --leak-check=full --track-origins=yes ./$(TARGET)

.PHONY: all run debug valgrind clean dev deps install-deps install uninstall
