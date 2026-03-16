// Created by RED on 12.02.2026.
#pragma once

#include "CLI/CLI.hpp"

#include "platform/app_state.h"

class Command {
public:
    Command(const std::string_view name,
            const std::string_view description) : m_name(name), m_description(description) {
    }

    virtual ~Command() = default;

    void register_(CLI::App &app) {
        auto *sub_command = app.add_subcommand(m_name, m_description);
        customize(*sub_command);
        sub_command->callback([this] { handle(); });
    }

protected:
    virtual void customize(CLI::App &app) = 0;

    virtual void handle() = 0;

private:
    std::string m_name;
    std::string m_description;
};

class DatabaseDependantCommand : public Command {
public:
    DatabaseDependantCommand(const std::string_view &name, const std::string_view &description)
        : Command(name, description) {
    }

    ~DatabaseDependantCommand() override = default;

    void customize(CLI::App &app) override {
        app.add_option("-d,--db_path", m_db_path, "Path to hashes.db for resolving asset paths from hashes.");
    }

protected:
    std::filesystem::path m_db_path;
};

class GameCommand : public DatabaseDependantCommand {
public:
    GameCommand(const std::string_view &name, const std::string_view &description)
        : DatabaseDependantCommand(name, description) {
    }

    ~GameCommand() override = default;

protected:
    void customize(CLI::App &app) override {
        DatabaseDependantCommand::customize(app);
        app.add_option("game_root", m_game_root,
                       "Path to the root directory of the game assets (generationZero\\archives_win64).")->required();
        app.add_option("-o,--out_dir", m_export_path, "Output directory for extracted assets.");
    }

    std::filesystem::path m_game_root;
    std::filesystem::path m_export_path;
};

class ExtractCommand : public GameCommand {
public:
    ExtractCommand(const std::string_view &name, const std::string_view &description)
        : GameCommand(name, description), m_skip_textures(false), m_extract_raw(false) {
    }

    ~ExtractCommand() override = default;

protected:
    void customize(CLI::App &app) override {
        GameCommand::customize(app);
        app.add_flag("-n,--no_textures", m_skip_textures, "Don't export textures.");
        app.add_flag("-r,--raw", m_extract_raw, "Export raw data without converting to glTF.");
        app.add_option("assets", m_assets, "Paths or hashes to assets to extract.")->required()->expected(-1);
    }

    void handle() override;

private:
    bool m_skip_textures;
    bool m_extract_raw;
    std::vector<std::string> m_assets{};
};

class ExtractAnimationCommand : public GameCommand {
public:
    ExtractAnimationCommand(const std::string_view &name, const std::string_view &description)
        : GameCommand(name, description) {
    }

    ~ExtractAnimationCommand() override = default;

protected:
    void customize(CLI::App &app) override {
        GameCommand::customize(app);
        app.add_option("skeleton-path", m_skeleton_path,
                       "Path or hash to the Havok container containing the skeleton.")
                ->required();

        app.add_option("animations", m_animations, "Paths or hashes to animation containers.")
                ->required();
    }

    void handle() override;

private:
    std::string m_skeleton_path;
    std::vector<std::string> m_animations;
};

class SearchCommand : public DatabaseDependantCommand {
public:
    SearchCommand(const std::string_view &name, const std::string_view &description)
        : DatabaseDependantCommand(name, description) {
    }

    ~SearchCommand() override = default;

protected:
    void customize(CLI::App &app) override {
        DatabaseDependantCommand::customize(app);
        app.add_option("query", m_search_query, "Hash or path or pattern (% wildcards).")->required();
    }

    void handle() override;
private:
    std::string m_search_query;
};

class ConvertCommand : public GameCommand {
public:
    ConvertCommand(const std::string_view &name, const std::string_view &description)
        : GameCommand(name, description) {
    }

    ~ConvertCommand() override = default;

protected:
    void customize(CLI::App &app) override {
        GameCommand::customize(app);
        app.add_option("input", input_file, "Path to input file.")
                ->required();
        app.add_option("-v,--v_output", v_output, "Virtual output path used for hashing.")
                ->required();
    }

    void handle() override {

    }

private:
    std::string input_file;
    std::string v_output;
};
