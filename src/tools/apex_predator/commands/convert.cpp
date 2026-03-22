#include "apex/avtx.h"
#include "platform/app_state.h"
#include "platform/cli_parser.h"
// Created by RED on 22.02.2026.

void convert_handler(const ApexAppState *app_state, const CliResult *cli_res) {
    std::string output_dir = {};
    cli_get_string(cli_res, "out_dir", &output_dir);
    String input_path = {};
    cli_get_string(cli_res, "input", &input_path);
    String virtual_path = {};
    cli_get_string(cli_res, "v_output", &virtual_path);

    AVTXTexture_from_png(&input_path, &virtual_path, &output_dir);

    String_free(&output_dir);
    String_free(&input_path);
    String_free(&virtual_path);
}
