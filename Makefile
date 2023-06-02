all: compare_thread.c
      gcc -pthread -o output_file compare_thread.c

clean:
      rm -rf output_file
