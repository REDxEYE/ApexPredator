#pragma once

#include "apex/hashes.h"
#include "platform/app_state.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

// The owning game selects whether its StringLookup dialogue schema has call
// fields. This template only sees the generated type at the game call site.
template<bool HasCallFields, typename StringLookup>
void export_apex_dialogue(ApexAppState &app_state, const StringLookup &lookup,
                          const uint64 path_hash, const uint32 type_hash) {
    const char *string_data = reinterpret_cast<const char *>(lookup.Text.data());
    std::unordered_map<u32, std::string> string_map;
    nlohmann::json root;
    auto &sorted_pairs = root["SortedPairs"];
    for (const auto &sorted_pair: lookup.SortedPairs) {
        string_map[sorted_pair.Hash] = std::string_view(string_data + sorted_pair.TextOffset);
        sorted_pairs.emplace_back(nlohmann::json{
            {"Hash", sorted_pair.Hash},
            {"Name", std::string_view(string_data + sorted_pair.NameOffset)},
            {"Text", std::string_view(string_data + sorted_pair.TextOffset)}
        });
    }
    auto &sorted_dialogue_lines = root["SortedDialogueLines"];
    for (const auto &line: lookup.SortedDialogueLines) {
        nlohmann::json subtitles;
        for (const auto &subtitle: line.Subtitles) {
            std::string subtitle_line = "<!!LINE NOT FOUND!!>";
            if (string_map.contains(subtitle.LineHash)) {
                subtitle_line = string_map.at(subtitle.LineHash);
            }
            subtitles.emplace_back(nlohmann::json{
                {"LineHash", subtitle.LineHash},
                {"Line", subtitle_line},
                {"Start", subtitle.Start},
                {"Duration", subtitle.Duration},
            });
        }
        nlohmann::json dialogue_line{
            {"Hash", line.Hash},
            {"Name", std::string_view(string_data + line.NameOffset)},
            {"Subtitles", subtitles},
            {"FMODEvent", line.FMODEvent},
            {"Flags", line.Flags},
        };
        if constexpr (HasCallFields) {
            dialogue_line["IncomingCall"] = line.IncomingCall.to_json();
            dialogue_line["CharacterName"] = line.IncomingCall.to_json();
        }
        sorted_dialogue_lines.emplace_back(std::move(dialogue_line));
    }

    auto path = find_asset_name(path_hash).value_or(std::format("unknown_{:08X}", path_hash));
    path += std::format("_{:08X}", type_hash);
    std::filesystem::path dump_path = app_state.export_path() / path;
    std::filesystem::create_directories(dump_path.parent_path());
    dump_path.replace_extension("json");
    std::ofstream json_out(dump_path);
    json_out << root.dump(2);
    json_out.close();
}
