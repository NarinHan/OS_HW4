#include <stdio.h>

int main() {
    FILE *file1, *file2;
    int byte1, byte2;
    int position = 0;
    int differenceCount = 0;

    // Open the files
    file1 = fopen("./testfile/homework2.pdf", "rb");
    file2 = fopen("./testfile/homework2copied.pdf", "rb");

    // Check if files were opened successfully
    if (file1 == NULL || file2 == NULL) {
        printf("Failed to open the files.\n");
        return 1;
    }

    /* while (1) 
    {
        byte1 = fgetc(file1);
        printf("%d", byte1);
        if (byte1 == EOF) break;
    } */

    // Read and compare bytes until the end of files or a difference is found
    while (1) {
        byte1 = fgetc(file1);
        byte2 = fgetc(file2);

        // Compare bytes
        if (byte1 != byte2) {
            printf("Files differ at position %d.\n", position);
            differenceCount++;
        }

        // Check for end of files
        if (byte1 == EOF || byte2 == EOF) {
            break;
        }

        position++;
    }

    // Check if files have different lengths
    if (differenceCount == 0 && byte1 != byte2) {
        printf("Files have different lengths.\n");
    }
    else 
    {
        printf("Files are identical!\n");
    }

    // Close the files
    fclose(file1);
    fclose(file2); 

    return 0;
}
