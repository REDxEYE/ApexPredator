// Created by RED on 18.09.2025.

#ifndef APEXPREDATOR_LOOKUP3_H
#define APEXPREDATOR_LOOKUP3_H
#include "int_def.h"
#include "stdint.h"

void hashlittle2(
  const void *key,       /* the key to hash */
  size_t      length,    /* length of the key */
  uint32   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32   *pb);        /* IN: secondary initval, OUT: secondary hash */

#endif //APEXPREDATOR_LOOKUP3_H