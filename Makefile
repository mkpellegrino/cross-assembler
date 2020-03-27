CPPFLAGS=-arch x86_64 -m64
DEBUG=-g -DDEBUG
OPT=-O2
OPT_SIZE=-O3 -Os -fno-threadsafe-statics -fno-exceptions -ffunction-sections -fdata-sections -fno-rtti -flto -fvisibility-inlines-hidden

all: clean cross

cross:  cross.cpp
	g++ cross.cpp $(CPPFLAGS) $(OPT_SIZE) -o cross
	g++ cross.cpp $(DEBUG) $(CPPFLAGS) -o cross-debug
	strip -no_uuid -A -u -S -X -x cross

clean:
	rm -f ./cross
	rm -f ./cross-debug
	rm -f *.*~
	rm -f *~
