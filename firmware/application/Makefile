# Compiler and flags
CC = gcc
CFLAGS = -g -fcommon -Wall -Wextra -O2 -I. -Ilib -Ihandler -Iui -I/usr/include -I/usr/include/dbus-1.0 -I/usr/lib/aarch64-linux-gnu/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/aarch64-linux-gnu/glib-2.0/include -march=armv8-a -Wno-unused-parameter -Wno-unused-variable -Wno-unknown-pragmas -Wno-format-truncation -Wno-stringop-truncation
LDFLAGS = -lbluetooth -ldbus-1 -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lpigpio -lpthread -lrt -lm

# Source files
SRC = \
    main.c \
    handler.c \
    handler/handler_bt.c \
    handler/handler_ibus.c \
    handler/handler_common.c \
    lib/bt/bt_common.c \
    lib/bt.c \
    lib/bt/bt_rpi4.c \
    lib/char_queue.c \
    lib/config.c \
    lib/event.c \
    lib/ibus.c \
    lib/ibus_pretty.c \
    lib/locale.c \
    lib/log.c \
    lib/shutdown.c \
    lib/timer.c \
    lib/utils.c \
    ui/bmbt.c \
#    ui/cd53.c \
#    ui/cli.c \
    ui/mid.c \
    ui/menu/menu_singleline.c

# Header files
HEADERS = \
    handler.h \
    mappings.h \
    sysconfig.h \
    handler/handler_bt.h \
    handler/handler_ibus.h \
    handler/handler_common.h \
    lib/bt/bt_common.h \
    lib/bt.h \
    lib/bt_rpi4.h \
    lib/char_queue.h \
    lib/config.h \
    lib/event.h \
    lib/ibus.h \
    lib/ibus_pretty.h \
    lib/locale.h \
    lib/log.h \
    lib/shutdown.h \
    lib/timer.h \
    lib/utils.h \
    ui/menu/menu_singleline.h \
    ui/bmbt.h \
#    ui/cd53.h \
#    ui/cli.h \
    ui/mid.h

# Output binary
OUTPUT = bluebus

# Build rules
all: $(OUTPUT)

$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OUTPUT) *.o

.PHONY: all clean
