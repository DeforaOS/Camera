/* $Id$ */
/* This file is part of DeforaOS desktop camera */



#ifndef CAMERA_CAMERA_H
# define CAMERA_CAMERA_H


/* public */
/* types */
typedef struct _Camera Camera;


/* functions */
Camera * camera_new(char const * device);
void camera_delete(Camera * camera);

#endif /* !CAMERA_CAMERA_H */
