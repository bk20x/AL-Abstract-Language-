#include "al_fileutils.h"

String* read_entire_file(const char* filename) {
    #define CHUNK_SIZE 1024

    FILE* f = fopen(filename, fmRead);
    if (!f) return nullptr;

    String* result = str_of_cap(1028);
    if (!result) {
        fclose(f);
        return nullptr;
    }

    char    buf[CHUNK_SIZE];
    size_t  bytes_read;
    while ((bytes_read = fread(buf, 1, CHUNK_SIZE, f)) != 0) {
        str_append_bytes_unsafe(result, buf, bytes_read);
    }

    fclose(f);
    return result;
}

String* read_entire_file_s(String* filename) {
    #define CHUNK_SIZE 1024
    constexpr char NUL = '\0';
    str_append_bytes_unsafe(filename, &NUL, 1);
    FILE* f = fopen(filename->chars, fmRead);
    if (!f) return nullptr;

    String* result = str_of_cap(1028);
    if (!result) {
        fclose(f);
        return nullptr;
    }

    char    buf[CHUNK_SIZE];
    size_t  bytes_read;
    while ((bytes_read = fread(buf, 1, CHUNK_SIZE, f)) != 0) {
        str_append_bytes_unsafe(result, buf, bytes_read);
    }

    fclose(f);
    return result;
}


Vector* read_entire_file_lines(const char* restrict filename) {
    String* file_content = read_entire_file(filename);
    if (!file_content) return nullptr;

    Vector* lines = vec_new_of_cap(64);
    if (!lines) {
        str_free(file_content);
        return nullptr;
    }

    #define data file_content->chars
    #define len  file_content->len
    size_t start = 0;

    for (size_t i = 0; i <= len; i++) {
        if (i == len || data[i] == '\n' || data[i] == '\r') {
            const size_t line_len = i - start;

            if (line_len > 0 || i < len) {
                String* line = str_of_cap(line_len);
                str_append_bytes_unsafe(line, &data[start], line_len);

                if (line) {
                    vec_append(lines, line);
                }
            }

            // for windows bs \r\n
            if (data[i] == '\r' && (i + 1) < len && data[i + 1] == '\n') {
                i++;
            }
            start = i + 1;
        }
    }
    str_free(file_content);
    return lines;
}
