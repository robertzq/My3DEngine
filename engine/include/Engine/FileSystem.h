#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

// 引擎通用文件系统 / 持久化工具（header-only）。
// 提供可写保存目录定位 + 文本读写，供存档、配置落盘等通用能力使用。
// 不含任何游戏逻辑：存什么、怎么序列化由上层（游戏侧）决定。
namespace EngineFileSystem {
    // 可写保存目录：当前工作目录下的 save/（后续调用会自动创建）
    inline std::filesystem::path SaveDir() {
        return std::filesystem::current_path() / "save";
    }

    // 确保目录存在；返回是否成功
    inline bool EnsureDir(const std::filesystem::path& dir) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        return !ec;
    }

    // 把文本写到 SaveDir(/relPath)，路径跨子目录时自动建目录；成功返回 true
    inline bool WriteText(const std::string& relPath, const std::string& content) {
        auto dir = SaveDir();
        if (!EnsureDir(dir)) return false;
        std::ofstream f(dir / relPath, std::ios::binary);
        if (!f) return false;
        f.write(content.data(), static_cast<std::streamsize>(content.size()));
        return f.good();
    }

    // 从 SaveDir(/relPath) 读文本；文件不存在或读取失败返回 false
    inline bool ReadText(const std::string& relPath, std::string& out) {
        std::ifstream f(SaveDir() / relPath, std::ios::binary);
        if (!f) return false;
        std::ostringstream ss;
        ss << f.rdbuf();
        out = ss.str();
        return true;
    }
}