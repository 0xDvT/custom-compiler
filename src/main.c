#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "scanner.h"
#include "parser.h"
#include "codegen.h"

void filename_no_ext(char *path, char *out)
{
    char *name = strrchr(path, '/');
    name = name ? name + 1 : path;

    strcpy(out, name);

    char *dot = strrchr(out, '.');
    if (dot)
        *dot = '\0';
}

int main(int argc, char *argv[])
{
    char output_filename[255];
    char source_filename[258];
    char object_filename[258];

    char command_object_file[1024];
    char command_executable_file[1024];

    if (argc < 2)
    {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 1;
    }
    if (strlen(argv[1]) > 255)
    {
        fprintf(stderr, "Error: Filename too long\n");
        return 1;
    }

    filename_no_ext(argv[1], output_filename);

    snprintf(source_filename, sizeof(source_filename), "%s.s", output_filename);
    snprintf(object_filename, sizeof(object_filename), "%s.o", output_filename);

    sprintf(command_object_file, "as -o %s %s", object_filename, source_filename);
    sprintf(command_executable_file, "ld -o %s %s.o -lc -dynamic-linker /lib64/ld-linux-x86-64.so.2", output_filename, output_filename);

    FILE *file = fopen(argv[1], "r");
    if (file == NULL)
    {
        printf("Error opening file\n");
        return 1;
    }

    scan_tokens(file);
    // print_tokens();
    fclose(file);

    init_assembler_file(source_filename);
    run_parser();
    close_assembler_file();

    free_buffer_tokens();

    system(command_object_file);
    system(command_executable_file);

    return 0;
}