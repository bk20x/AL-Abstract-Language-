#ifndef ALD_AL_STREAMS_H
#define ALD_AL_STREAMS_H
#include <stdio.h>
#include "al_fileutils.h"
typedef struct IStream IStream;


typedef struct {
    FILE*  fd;
    bool   at_end;
} File_Stream;




#endif //ALD_AL_STREAMS_H
