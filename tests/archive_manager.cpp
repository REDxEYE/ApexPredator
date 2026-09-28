#include "redscore/platform/archive_manager.h"

#include <stdexcept>
#include <unordered_set>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

class TestArchive final : public Archive<u64> {
    u64 m_key;
    std::shared_ptr<bool> m_alive;
public:
    TestArchive(u64 key, std::shared_ptr<bool> alive) : m_key(key), m_alive(std::move(alive)) {
        *m_alive = true;
    }
    ~TestArchive() override { *m_alive = false; }
    bool has(const u64 &key) override { return key == m_key; }
    std::unique_ptr<IO::File> get(const u64 &key) override {
        return has(key) ? std::make_unique<IO::MemoryFile>(std::vector<uint8>{42}) : nullptr;
    }
    std::string_view name() const override { return "test"; }
    const u64 &key() const override { return m_key; }
    bool foreach_file(const std::function<bool(const ArchiveEntry &)> &callback) override {
        const auto alive = m_alive;
        const bool result = callback({m_key, 1});
        require(*alive, "archive destroyed during its callback");
        return result;
    }
};

class TestManager final : public ArchiveManager<u64> {
    std::pair<bool, u64> load_child_archive(const u64 &) override { return {false, 0}; }
public:
    std::shared_ptr<bool> add(u64 key, bool dynamic = false) {
        auto alive = std::make_shared<bool>(false);
        mount(std::make_unique<TestArchive>(key, alive));
        if (dynamic) {
            touch_dynamic_mount(key);
            evict_dynamic_mounts();
        }
        return alive;
    }
    size_t dynamic_count() const { return m_dynamic_mount_set.size(); }
    void touch(u64 key) { touch_dynamic_mount(key); }
};

void permanent_archives_survive() {
    TestManager manager;
    for (u64 key = 1; key <= 40; ++key) manager.add(key);
    std::unordered_set<u64> visited;
    manager.foreach_file([&](const auto &entry) {
        require(visited.insert(entry.key).second, "archive visited twice");
        // Force rehashing and eviction from inside a collector-style callback.
        for (u64 key = 100; key < 300; ++key) manager.add(key);
        for (u64 key = 1000; key < 1040; ++key) manager.add(key, true);
        return true;
    });
    require(visited.size() == 40, "traversal must visit exactly its starting snapshot");
    for (u64 key = 1; key <= 40; ++key) require(manager.is_mounted(key), "permanent mount evicted");
    require(manager.dynamic_count() == 32, "dynamic mount limit not enforced");
}

void pinned_archives_survive(bool stop, bool fail) {
    TestManager manager;
    for (u64 key = 1; key <= 32; ++key) manager.add(key, true);
    size_t calls = 0;
    try {
        const bool completed = manager.foreach_file([&](const auto &) {
            if (++calls == 1) {
                for (u64 key = 100; key < 140; ++key) manager.add(key, true);
                for (u64 key = 1; key <= 32; ++key)
                    require(manager.is_mounted(key), "pinned archive evicted");
                require(manager.is_mounted(139), "newly loaded child evicted before use");
                size_t nested = 0;
                manager.foreach_file([&](const auto &) { ++nested; return true; });
                require(nested == 64, "nested traversal lost pinned or cached archives");
            }
            if (fail) throw std::logic_error("callback failed");
            return !stop;
        });
        require(!fail && completed == !stop, "incorrect traversal return value");
    } catch (const std::logic_error &) {
        require(fail, "unexpected callback exception");
    }
    require(calls == (stop || fail ? 1 : 32), "wrong callback count");
    // Pins must be released on completion, early stop, and exceptions.
    manager.add(200, true);
    require(manager.is_mounted(200) && manager.dynamic_count() == 32, "traversal leaked pins");
}

void unmount_keeps_active_archive_alive() {
    TestManager manager;
    const auto alive = manager.add(1, true);
    manager.foreach_file([&](const auto &) {
        manager.unmount(1);
        require(*alive && !manager.is_mounted(1), "unmount destroyed active archive");
        return true;
    });
    require(!*alive, "archive leaked after traversal");
}

void lru_eviction() {
    TestManager manager;
    for (u64 key = 1; key <= 32; ++key) manager.add(key, true);
    manager.touch(1);
    manager.add(33, true);
    require(manager.is_mounted(1) && !manager.is_mounted(2) && manager.is_mounted(33), "incorrect LRU eviction");
}
}

int main() {
    permanent_archives_survive();
    pinned_archives_survive(false, false);
    pinned_archives_survive(true, false);
    pinned_archives_survive(false, true);
    unmount_keeps_active_archive_alive();
    lru_eviction();
}
