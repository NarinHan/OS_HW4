all: compare_thread.c
      gcc -pthread -o output_file findeq.c
      

clean:
      rm -rf output_file
