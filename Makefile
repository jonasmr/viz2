
LDFLAGS=-framework OpenGL -framework CoreAudio `/usr/local/bin/sdl-config --static-libs`
CFLAGS=`/usr/local/bin/sdl-config --cflags` -mmacosx-version-min="10.7" -Iglew/glew-1.9.0/include -DGLEW_STATIC -Wno-c++11-extensions
CFLAGS+=-g -O0 -Wno-invalid-offsetof

CPP_SOURCES = main.cpp math.cpp text.cpp
C_SOURCES = stb_image.c glew.c

CC=clang
CPP=clang++
LD=clang++
CPP_OBJS = $(patsubst %.cpp,%.o,$(CPP_SOURCES))
C_OBJS = $(patsubst %.c,%.o,$(C_SOURCES))


all: bsp


bsp: $(C_OBJS) $(CPP_OBJS)
	$(LD) -o bsp $(C_OBJS) $(CPP_OBJS) $(LDFLAGS)

-include .depend

.cpp.o: 
	$(CPP) -c $< $(CFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm *.o bsp

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CC) -MM $(CFLAGS) $^ > .depend