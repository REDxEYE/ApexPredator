//
// Created by red_eye on 10/9/26.
//

#pragma once
#include "platform/app_state.h"
#include "redscore/platform/model/model.hpp"

namespace Apex::RBMdl {
    class RenderBlock {
    public:
        RenderBlock() = default;

        virtual ~RenderBlock() = default;

        virtual void to_primitive(ApexArchiveManager &manager, VM::SceneBuilder &builder, VM::Primitive &primitive) = 0;
    };
}
