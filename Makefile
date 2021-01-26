CC = gcc  
MAINC = main.c screen-server.c tt.c
EXEC = style
CFLAGS = `pkg-config --cflags --libs gtk+-3.0 gstreamer-1.0 check`
main: 
	$(CC)  -g $(MAINC)  -o $(EXEC) $(CFLAGS)
clean:
	rm $(EXEC) -rf
