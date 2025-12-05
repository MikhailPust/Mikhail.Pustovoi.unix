#include <iostream>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/sha.h>
 
namespace fs = std::filesystem;
 
std::string make_hash(const std::string &filepath)
{
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) return "";
 
    SHA_CTX sha;
    if (!SHA1_Init(&sha)) {
        close(fd);
        return "";
    }
 
    const size_t CHUNK = 8192;
    unsigned char buf[CHUNK];
    ssize_t r;
 
    while ((r = read(fd, buf, CHUNK)) > 0) {
        if (!SHA1_Update(&sha, buf, (size_t)r)) {
            close(fd);
            return "";
        }
    }
 
    if (r < 0) {
        close(fd);
        return "";
    }
 
    unsigned char raw[SHA_DIGEST_LENGTH];
    if (!SHA1_Final(raw, &sha)) {
        close(fd);
        return "";
    }
 
    close(fd);
 
    char hex[SHA_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        std::snprintf(hex + i * 2, 3, "%02x", raw[i]);
    }
    hex[SHA_DIGEST_LENGTH * 2] = '\0';
 
    return std::string(hex);
}
 
int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <каталог>\n";
        return 1;
    }
 
    std::string root = argv[1];
    if (!fs::exists(root) || !fs::is_directory(root)) {
        std::cerr << "Некорректный каталог\n";
        return 1;
    }
 
    std::unordered_map<std::string, std::string> seen_hashes;
 
    int processed_count = 0;
    int hardlinks_made = 0;
 
    try {
        for (const auto &item : fs::recursive_directory_iterator(root)) {
            if (!item.is_regular_file()) continue;
 
            std::string file = item.path().string();
            std::string h = make_hash(file);
            if (h.empty()) continue;
 
            processed_count++;
 
            auto found = seen_hashes.find(h);
            if (found == seen_hashes.end()) {
                seen_hashes[h] = file;
            } else {
                const std::string &orig = found->second;
 
                struct stat a{}, b{};
                if (stat(orig.c_str(), &a) != 0 || stat(file.c_str(), &b) != 0)
                    continue;
 
                if (a.st_dev == b.st_dev && a.st_ino == b.st_ino)
                    continue;
 
                if (a.st_dev != b.st_dev)
                    continue;
 
                if (unlink(file.c_str()) != 0)
                    continue;
 
                if (link(orig.c_str(), file.c_str()) != 0)
                    continue;
 
                hardlinks_made++;
            }
        }
    }
    catch (const fs::filesystem_error &e) {
        std::cerr << "Ошибка обхода: " << e.what() << "\n";
        return 1;
    }
 
    std::cout << "Обработано файлов: " << processed_count << "\n";
    std::cout << "Создано жёстких ссылок: " << hardlinks_made << "\n";
 
    return 0;
}
