#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <rex/graphics/vulkan/persistent_cache.h>

int main(int argc, char** argv) {
  assert(argc == 2);
  const std::filesystem::path root = argv[1];
  std::filesystem::create_directories(root);
  // Exercise both cache streams, including append-after-read and a torn tail.
  for (const char* name : {"45410809.xsh", "45410809.fbo.vk.xpso"}) {
    const auto path = root / name;
    const std::array<uint32_t, 3> header{0x53504558, 0, 0x01000000};
    FILE* file = rex::graphics::vulkan::OpenPersistentCacheFile(path);
    assert(file);
    assert(std::fwrite(header.data(), sizeof(header), 1, file) == 1);
    std::fclose(file);
    for (uint32_t run = 0; run < 32; ++run) {
      file = rex::graphics::vulkan::OpenPersistentCacheFile(path);
      assert(file && rex::filesystem::Tell(file) == 0);
      std::array<uint32_t, 3> loaded{};
      assert(std::fread(loaded.data(), sizeof(loaded), 1, file) == 1);
      assert(loaded == header);
      for (uint32_t record = 0; record < run; ++record) {
        uint32_t loaded_record = UINT32_MAX;
        assert(std::fread(&loaded_record, sizeof(loaded_record), 1, file) == 1);
        assert(loaded_record == record);
      }
      // A read seek must not cause writes to overwrite the cached header.
      assert(rex::filesystem::Seek(file, 0, SEEK_SET));
      assert(std::fwrite(&run, sizeof(run), 1, file) == 1);
      std::fclose(file);
      assert(std::filesystem::file_size(path) == sizeof(header) + (run + 1) * 4);
    }
    const auto valid_size = std::filesystem::file_size(path);
    file = rex::graphics::vulkan::OpenPersistentCacheFile(path);
    assert(file);
    const uint8_t torn = 0xFF;
    assert(std::fwrite(&torn, 1, 1, file) == 1);
    std::fclose(file);
    file = rex::graphics::vulkan::OpenPersistentCacheFile(path);
    assert(file && rex::filesystem::TruncateStdioFile(file, valid_size));
    std::fclose(file);
    file = rex::graphics::vulkan::OpenPersistentCacheFile(path);
    std::array<uint32_t, 3> loaded{};
    assert(file && std::fread(loaded.data(), sizeof(loaded), 1, file) == 1 && loaded == header);
    std::fclose(file);
  }
  assert(!rex::graphics::vulkan::OpenPersistentCacheFile(root / "missing" / "cache"));
  std::cout << "Persistent shader/pipeline cache: 64 reopen/appends and torn-tail recovery passed\n";
}
