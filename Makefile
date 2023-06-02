all: findeq.c
	gcc findeq.c -pthread -o findeq

clean:
	rm -rf findeq
