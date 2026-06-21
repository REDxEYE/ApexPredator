//
// Created by red_eye on 4/27/26.
//

#pragma once
#include "apex/adf/adf_base_type.h"
struct BufferDescriptor;

void read_data(BufferDescriptor &desc, IO::File &buffer);

struct BufferDescriptor : ADF::BaseType {
    u32 offset{};
    u32 size{};
    u32 unk8{};
    u32 unk12{};

    IO::Buffer data{};

    void read(IO::File &buffer) override {
        offset = buffer.read_pod<u32>();
        size = buffer.read_pod<u32>();
        unk8 = buffer.read_pod<u32>();
        unk12 = buffer.read_pod<u32>();

        read_data(*this, buffer);
    }
};

inline void read_data(BufferDescriptor &desc, IO::File &buffer) {
    const auto orig_offset = buffer.get_position();
    buffer.set_position(desc.offset, std::ios::beg);
    desc.data.resize(desc.size);
    buffer.read_exact(desc.data.as_span());
    buffer.set_position(orig_offset, std::ios::beg);
}


namespace ADF {
    struct SDisableAtDistance : BufferDescriptor {
        void print(std::ostream &out) const override {
            throw std::runtime_error("SDisableAtDistance is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["type"] = "SDisableAtDistance";
            obj["offset"] = offset;
            obj["size"] = size;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            obj["data"] = data.as_span();
            return obj;
        }
    };

    static std::unique_ptr<SDisableAtDistance> SDisableAtDistance_new_instance() {
        return std::make_unique<SDisableAtDistance>();
    }

    inline TypeInfo SDisableAtDistance_TI = {
        .new_instance = SDisableAtDistance_new_instance,
        .hash = 0xBC425335,
        .name = "CustomDeferred"
    };

    struct SMachineDamagePropagation : BufferDescriptor {
        void print(std::ostream &out) const override {
            throw std::runtime_error("SMachineDamagePropagation is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["type"] = "SMachineDamagePropagation";
            obj["offset"] = offset;
            obj["size"] = size;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            obj["data"] = data.as_span();
            return obj;
        }
    };

    static std::unique_ptr<SMachineDamagePropagation> SMachineDamagePropagation_new_instance() {
        return std::make_unique<SMachineDamagePropagation>();
    }

    inline TypeInfo SMachineDamagePropagation_TI = {
        .new_instance = SMachineDamagePropagation_new_instance,
        .hash = 0xBC425335,
        .name = "SMachineDamagePropagation"
    };

    struct Custom_C0393B66 : BaseType {
        f32 unk0{};
        f32 unk4{};
        f32 unk8{};
        f32 unk12{};


        void read(IO::File &buffer) override {
            unk0 = buffer.read_pod<f32>();
            unk4 = buffer.read_pod<f32>();
            unk8 = buffer.read_pod<f32>();
            unk12 = buffer.read_pod<f32>();
        }

        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_C0393B66 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["unk0"] = unk0;
            obj["unk4"] = unk4;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            GLog_Warning("Unknown type {}", nlohmann::to_string(obj));
            return obj;
        }
    };

    static std::unique_ptr<Custom_C0393B66> Custom_C0393B66_new_instance() {
        return std::make_unique<Custom_C0393B66>();
    }

    inline TypeInfo Custom_C0393B66_TI = {
        .new_instance = Custom_C0393B66_new_instance,
        .hash = 0xC0393B66,
        .name = "Custom_C0393B66"
    };

    struct Custom_B634CDE0 : BaseType {
        f32 unk0{};
        f32 unk4{};
        f32 unk8{};
        f32 unk12{};

        IO::Buffer data;

        void read(IO::File &buffer) override {
            unk0 = buffer.read_pod<f32>();
            unk4 = buffer.read_pod<f32>();
            unk8 = buffer.read_pod<f32>();
            unk12 = buffer.read_pod<f32>();
        }

        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_B634CDE0 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["unk0"] = unk0;
            obj["unk4"] = unk4;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            GLog_Warning("Unknown type {}", nlohmann::to_string(obj));
            return obj;
        }
    };

    static std::unique_ptr<Custom_B634CDE0> Custom_B634CDE0_new_instance() {
        return std::make_unique<Custom_B634CDE0>();
    }

    inline TypeInfo Custom_B634CDE0_TI = {
        .new_instance = Custom_B634CDE0_new_instance,
        .hash = 0xB634CDE0,
        .name = "Custom_B634CDE0"
    };

    struct Custom_D43D6303 : BufferDescriptor {
        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_D43D6303 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["type"] = "Custom_D43D6303";
            obj["offset"] = offset;
            obj["size"] = size;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            obj["data"] = data.as_span();
            return obj;
        }
    };

    static std::unique_ptr<Custom_D43D6303> Custom_D43D6303_new_instance() {
        return std::make_unique<Custom_D43D6303>();
    }

    inline TypeInfo Custom_D43D6303_TI = {
        .new_instance = Custom_D43D6303_new_instance,
        .hash = 0xD43D6303,
        .name = "Custom_D43D6303"
    };

    struct Custom_95FA4010 : BufferDescriptor {
        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_95FA4010 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["type"] = "Custom_95FA4010";
            obj["offset"] = offset;
            obj["size"] = size;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            obj["data"] = data.as_span();
            return obj;
        }
    };

    static std::unique_ptr<Custom_95FA4010> Custom_95FA4010_new_instance() {
        return std::make_unique<Custom_95FA4010>();
    }

    inline TypeInfo Custom_95FA4010_TI = {
        .new_instance = Custom_95FA4010_new_instance,
        .hash = 0x95FA4010,
        .name = "Custom_95FA4010"
    };

    struct Custom_C67C0D75 : BaseType {
        f32 unk0{};
        f32 unk4{};
        f32 unk8{};
        f32 unk12{};

        IO::Buffer data;

        void read(IO::File &buffer) override {
            unk0 = buffer.read_pod<f32>();
            unk4 = buffer.read_pod<f32>();
            unk8 = buffer.read_pod<f32>();
            unk12 = buffer.read_pod<f32>();
        }

        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_C67C0D75 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["unk0"] = unk0;
            obj["unk4"] = unk4;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            GLog_Warning("Unknown type {}", nlohmann::to_string(obj));
            return obj;
        }
    };

    static std::unique_ptr<Custom_C67C0D75> Custom_C67C0D75_new_instance() {
        return std::make_unique<Custom_C67C0D75>();
    }

    inline TypeInfo Custom_C67C0D75_TI = {
        .new_instance = Custom_C67C0D75_new_instance,
        .hash = 0xC67C0D75,
        .name = "Custom_C67C0D75"
    };

    struct Custom_10DA01FA : BaseType {
        f32 unk0{};
        f32 unk4{};
        f32 unk8{};
        f32 unk12{};

        IO::Buffer data;

        void read(IO::File &buffer) override {
            unk0 = buffer.read_pod<f32>();
            unk4 = buffer.read_pod<f32>();
            unk8 = buffer.read_pod<f32>();
            unk12 = buffer.read_pod<f32>();
        }

        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_10DA01FA is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["unk0"] = unk0;
            obj["unk4"] = unk4;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            GLog_Warning("Unknown type {}", nlohmann::to_string(obj));
            return obj;
        }
    };

    static std::unique_ptr<Custom_10DA01FA> Custom_10DA01FA_new_instance() {
        return std::make_unique<Custom_10DA01FA>();
    }

    inline TypeInfo Custom_10DA01FA_TI = {
        .new_instance = Custom_10DA01FA_new_instance,
        .hash = 0x10DA01FA,
        .name = "Custom_10DA01FA"
    };

    struct Custom_6B4752BB : BaseType {
        u32 unk0{};
        f32 unk4{};
        f32 unk8{};
        f32 unk12{};

        IO::Buffer data;

        void read(IO::File &buffer) override {
            unk0 = buffer.read_pod<u32>();
            unk4 = buffer.read_pod<f32>();
            unk8 = buffer.read_pod<f32>();
            unk12 = buffer.read_pod<f32>();
        }

        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_6B4752BB is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["unk0"] = unk0;
            obj["unk4"] = unk4;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            GLog_Warning("Unknown type {}", nlohmann::to_string(obj));
            return obj;
        }
    };

    static std::unique_ptr<Custom_6B4752BB> Custom_6B4752BB_new_instance() {
        return std::make_unique<Custom_6B4752BB>();
    }

    inline TypeInfo Custom_6B4752BB_TI = {
        .new_instance = Custom_6B4752BB_new_instance,
        .hash = 0x6B4752BB,
        .name = "Custom_6B4752BB"
    };

    struct Custom_3CDE22B1 : BaseType {
        u32 unk0{};
        u32 unk4{};
        u32 unk8{};
        u32 unk12{};

        IO::Buffer data;

        void read(IO::File &buffer) override {
            unk0 = buffer.read_pod<u32>();
            unk4 = buffer.read_pod<u32>();
            unk8 = buffer.read_pod<u32>();
            unk12 = buffer.read_pod<u32>();
        }

        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_3CDE22B1 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["unk0"] = unk0;
            obj["unk4"] = unk4;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            GLog_Warning("Unknown type {}", nlohmann::to_string(obj));
            return obj;
        }
    };

    static std::unique_ptr<Custom_3CDE22B1> Custom_3CDE22B1_new_instance() {
        return std::make_unique<Custom_3CDE22B1>();
    }

    inline TypeInfo Custom_3CDE22B1_TI = {
        .new_instance = Custom_3CDE22B1_new_instance,
        .hash = 0x3CDE22B1,
        .name = "Custom_3CDE22B1"
    };

    struct Custom_904F4B40 : BufferDescriptor {
        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_904F4B40 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["type"] = "Custom_904F4B40";
            obj["offset"] = offset;
            obj["size"] = size;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            obj["data"] = data.as_span();
            return obj;
        }
    };

    static std::unique_ptr<Custom_904F4B40> Custom_904F4B40_new_instance() {
        return std::make_unique<Custom_904F4B40>();
    }

    inline TypeInfo Custom_904F4B40_TI = {
        .new_instance = Custom_904F4B40_new_instance,
        .hash = 0x904F4B40,
        .name = "Custom_904F4B40"
    };

    struct Custom_B8C94E57 : BufferDescriptor {
        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_B8C94E57 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["type"] = "Custom_B8C94E57";
            obj["offset"] = offset;
            obj["size"] = size;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            obj["data"] = data.as_span();
            return obj;
        }
    };

    static std::unique_ptr<Custom_B8C94E57> Custom_B8C94E57_new_instance() {
        return std::make_unique<Custom_B8C94E57>();
    }

    inline TypeInfo Custom_B8C94E57_TI = {
        .new_instance = Custom_B8C94E57_new_instance,
        .hash = 0xB8C94E57,
        .name = "Custom_B8C94E57"
    };

    struct Custom_0F494862 : BaseType {
        u32 unk0{};
        u32 unk4{};
        f32 unk8{};
        f32 unk12{};

        IO::Buffer data;

        void read(IO::File &buffer) override {
            unk0 = buffer.read_pod<u32>();
            unk4 = buffer.read_pod<u32>();
            unk8 = buffer.read_pod<f32>();
            unk12 = buffer.read_pod<f32>();
        }

        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_0F494862 is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["unk0"] = unk0;
            obj["unk4"] = unk4;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            GLog_Warning("Unknown type {}", nlohmann::to_string(obj));
            return obj;
        }
    };

    static std::unique_ptr<Custom_0F494862> Custom_0F494862_new_instance() {
        return std::make_unique<Custom_0F494862>();
    }

    inline TypeInfo Custom_0F494862_TI = {
        .new_instance = Custom_0F494862_new_instance,
        .hash = 0x0F494862,
        .name = "Custom_0F494862"
    };

    struct Custom_275FA35B : BufferDescriptor {
        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_275FA35B is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["type"] = "Custom_275FA35B";
            obj["offset"] = offset;
            obj["size"] = size;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            obj["data"] = data.as_span();
            return obj;
        }
    };

    static std::unique_ptr<Custom_275FA35B> Custom_275FA35B_new_instance() {
        return std::make_unique<Custom_275FA35B>();
    }

    inline TypeInfo Custom_275FA35B_TI = {
        .new_instance = Custom_275FA35B_new_instance,
        .hash = 0x275FA35B,
        .name = "Custom_275FA35B"
    };

    struct Custom_5EDD698F : BaseType {
        u32 unk0{};
        u32 unk4{};
        u32 unk8{};
        u32 unk12{};

        IO::Buffer data;

        void read(IO::File &buffer) override {
            unk0 = buffer.read_pod<u32>();
            unk4 = buffer.read_pod<u32>();
            unk8 = buffer.read_pod<u32>();
            unk12 = buffer.read_pod<u32>();
        }

        void print(std::ostream &out) const override {
            throw std::runtime_error("Custom_5EDD698F is not supported yet");
        }

        [[nodiscard]] nlohmann::json to_json() const override {
            nlohmann::json obj;
            obj["unk0"] = unk0;
            obj["unk4"] = unk4;
            obj["unk8"] = unk8;
            obj["unk12"] = unk12;
            GLog_Warning("Unknown type {}", nlohmann::to_string(obj));
            return obj;
        }
    };

    static std::unique_ptr<Custom_5EDD698F> Custom_5EDD698F_new_instance() {
        return std::make_unique<Custom_5EDD698F>();
    }

    inline TypeInfo Custom_5EDD698F_TI = {
        .new_instance = Custom_5EDD698F_new_instance,
        .hash = 0x5EDD698F,
        .name = "Custom_5EDD698F"
    };

    static void register_custom_types() {
        adf_type_info[0xBC425335] = &SDisableAtDistance_TI;
        adf_type_info[0x90A5413E] = &SMachineDamagePropagation_TI;
        adf_type_info[0xC0393B66] = &Custom_C0393B66_TI;
        adf_type_info[0xB634CDE0] = &Custom_B634CDE0_TI;
        adf_type_info[0xD43D6303] = &Custom_D43D6303_TI;
        adf_type_info[0x95FA4010] = &Custom_95FA4010_TI;
        adf_type_info[0xC67C0D75] = &Custom_C67C0D75_TI;
        adf_type_info[0x10DA01FA] = &Custom_10DA01FA_TI;
        adf_type_info[0x6B4752BB] = &Custom_6B4752BB_TI;
        adf_type_info[0x3CDE22B1] = &Custom_3CDE22B1_TI;
        adf_type_info[0xB8C94E57] = &Custom_B8C94E57_TI;
        adf_type_info[0x904F4B40] = &Custom_904F4B40_TI;
        adf_type_info[0x0F494862] = &Custom_0F494862_TI;
        adf_type_info[0x275FA35B] = &Custom_275FA35B_TI;
        adf_type_info[0x5EDD698F] = &Custom_5EDD698F_TI;
    }
}
