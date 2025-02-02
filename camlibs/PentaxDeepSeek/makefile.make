CC = gcc
CFLAGS = -Wall -g
SOURCES = \
    pentax_camera_init.c \
    pentax_bulb_mode.c \
    pentax_mirror_up.c \
    pentax_file_manipulation.c \
    lens_lookup.c \
    exif_parser.c

OBJECTS = $(SOURCES:.c=.o)

TARGET = libghoto2.so
LIBS = -shared -lgphoto2 -lgphoto2_port -lexif

all: $(TARGET)
$(TARGET): $(OBJECTS) $(CFLAGS) $(INCLUDES) $(LIBS)

%.o: %.c:
    $(CC) $(CFLAGS) -c $$< $(INCLUDES)

clean:
    rm -f $(OBJECTS) $(TARGET)
