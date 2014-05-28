
TARGET =  trace_replay 
SRCS   =  trace_replay.o disk_io.o sgio.o identify.o

CFLAGS :=  -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64   
LDFLAGS := -lpthread -laio -lrt



OBJS=$(SRCS:.c=.o)


.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	@echo "Compiling $< ..."
	@$(RM) $@
	$(CC) -c -O2 -g  -D_GNU_SOURCE $(CFLAGS) -o $@ $<

$(TARGET):     $(OBJS)
	@echo "Making ./$(TARGET) ..."
	@echo "$(CC) -o $@ $(OBJS) $(LDFLAGS)"
	@$(CC) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)
	

all:    $(TARGET)

clean:
	rm -f *.o *.a *~ $(TARGET)

distclean:
	rm -f Makefile.bak *.o *.a *~ .depend $(TARGET)
install: 
	cp $(TARGET) /usr/local/bin

uninstall:

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif

