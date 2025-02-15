#ifndef STREAM_OBJECT_H_
#define STREAM_OBJECT_H_


typedef struct stream_obj_struct* StreamObject;

StreamObject CreateStreamObject();
void StartStreamReading(StreamObject stream_obj);
void FreeStreamObject(StreamObject stream_obj);

#endif
