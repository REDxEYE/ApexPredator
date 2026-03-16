// Created by RED on 11.03.2026.
#include "apex/adf/adf.h"
#include "apex/adf/adf_base_type.h"

#include <memory>

#include "apex/adf/adf_support_types.h"


std::unique_ptr<ADF::BaseType> ADF::ADFFile::read_instance(const uint32 index) {
    throw std::runtime_error("Not implemented");
}

std::unique_ptr<ADF::BaseType> Deferred::read(IO::File &buffer) {
    throw std::runtime_error("Not implemented");
}
