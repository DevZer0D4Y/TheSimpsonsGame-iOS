#include <rex/chrono/clock.h>
#include <rex/filesystem/devices/disc_image_device.h>
#include <rex/filesystem/entry.h>
#include <rex/filesystem/file.h>
#include <rex/logging.h>
#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

static void Put32(std::vector<uint8_t>& b, size_t at, uint32_t v) { std::memcpy(b.data()+at,&v,4); }
static std::vector<uint8_t> Image() {
  std::vector<uint8_t> b(36*2048);
  std::memcpy(b.data()+32*2048,"MICROSOFT*XBOX*MEDIA",20);
  Put32(b,32*2048+20,33); Put32(b,32*2048+24,2048);
  const size_t node=33*2048;
  Put32(b,node+4,34); Put32(b,node+8,4);
  b[node+12]=0; b[node+13]=11;
  std::memcpy(b.data()+node+14,"default.xex",11);
  std::memcpy(b.data()+34*2048,"XEX2",4);
  return b;
}
static std::filesystem::path fixtures;
static std::filesystem::path Write(const std::vector<uint8_t>& b, const char* name) {
  auto p=fixtures/name;
  std::ofstream f(p,std::ios::binary); f.write(reinterpret_cast<const char*>(b.data()),b.size());
  return p;
}
static void Reject(std::vector<uint8_t> b,const char* name) {
  rex::filesystem::DiscImageDevice disc("test",Write(b,name));
  if (disc.Initialize()) { std::cerr << "Invalid image accepted: " << name << '\n'; std::abort(); }
}
int main(int argc, char** argv) {
  fixtures = argc > 1 ? argv[1] : "runtime-test-fixtures";
  std::filesystem::create_directories(fixtures);
  auto good=Image();
  rex::filesystem::DiscImageDevice disc("test",Write(good,"valid.iso"));
  assert(disc.Initialize());
  auto* entry=disc.ResolvePath("DEFAULT.XEX"); assert(entry && entry->size()==4);
  rex::filesystem::File* file=nullptr;
  assert(entry->Open(1,&file)==0 && file);
  uint8_t data[8]={}; size_t count=0;
  assert(file->ReadSync(data,0,&count)==0 && count==4 && std::memcmp(data,"XEX2",4)==0);
  file->Destroy();
  Reject({},"empty.iso");
  Reject(std::vector<uint8_t>(7),"tiny.iso");
  auto b=good; b.resize(32*2048+10); Reject(b,"short-header.iso");
  b=good; Put32(b,32*2048+20,1000); Reject(b,"bad-root.iso");
  b=good; Put32(b,33*2048+4,1000); Reject(b,"bad-sector.iso");
  b=good; Put32(b,33*2048+8,0xffffffff); Reject(b,"bad-length.iso");
  b=good; Put32(b,32*2048+24,14); Reject(b,"bad-name.iso");
  b=good; b[33*2048+2]=0xff; b[33*2048+3]=0xff; Reject(b,"bad-node.iso");
  b=good; b[33*2048+12]=0x10; Put32(b,33*2048+4,33); Put32(b,33*2048+8,2048); Reject(b,"cycle.iso");
  // Verify full-disc partition offsets as well as a compact XISO.
  b.resize(0x02080000+good.size()); std::fill(b.begin(),b.end(),0);
  std::copy(good.begin(),good.end(),b.begin()+0x02080000);
  rex::filesystem::DiscImageDevice full("test",Write(b,"full-disc.iso"));
  assert(full.Initialize() && full.ResolvePath("default.xex"));
  auto before=rex::chrono::Clock::QueryGuestTickCount();
  rex::chrono::Clock::SetPaused(true);
  auto frozen=rex::chrono::Clock::QueryGuestTickCount();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  assert(rex::chrono::Clock::QueryGuestTickCount()==frozen && frozen>=before);
  rex::chrono::Clock::SetPaused(false);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  auto elapsed=rex::chrono::Clock::QueryGuestTickCount()-frozen;
  assert(elapsed>0 && elapsed<rex::chrono::Clock::guest_tick_frequency()/10);
  std::cout << "PASS: direct ISO/XISO reads, case-insensitive paths, full-disc offset, 9 malformed images, clock freeze/resume\n";
}
