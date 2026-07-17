#ifndef ALD_FILEUTILS_H
#define ALD_FILEUTILS_H
#include "al_string.h"
#include "al_vector.h"
#define fmRead "r"

String*  read_entire_file(const char*);
Vector*  read_entire_file_lines(const char*);

#endif //ALD_FILEUTILS_H
