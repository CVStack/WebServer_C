CC= gcc
APP=WebServer
CFLAGS = -D_REENTRANT 
LDFLAGS = -lpthread 
t1=WebServer_tries
t2=FileTree
SRC=$(t1).c $(t2).c

all: $(APP)
$(APP): $(objs) 
	$(CC) $(CFLAGS) -o $@ ${SRC} $^ $(LDFLAGS)
clean:
	rm -f $(APP)


