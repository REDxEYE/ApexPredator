// Created by RED on 12.10.2025.

#ifndef APEXPREDATOR_HAVOK_CODEGEN_H
#define APEXPREDATOR_HAVOK_CODEGEN_H

#include "havok_types.h"

void Havok_TypeLibrary_generate_code(Havok_TypeLibrary *lib, const String *namespace, FILE *header_output,
                                const String *header_relative_path, FILE *impl_output);

#endif //APEXPREDATOR_HAVOK_CODEGEN_H
