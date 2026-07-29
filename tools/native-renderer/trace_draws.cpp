// Draw-list extractor for GPU frame traces - the front half of the native
// command translator.
//
// A frame trace (F10 in-game) is the complete record of one frame: every PM4
// packet, register write and memory read the GPU consumed. This walks that
// stream the same way the emulated command processor does - a register shadow
// updated by packet actions - but instead of drawing, it emits one record per
// draw with the register state that defines it. That list, joined against the
// AOT shader manifest and the pipeline inventory, is exactly the input the
// native draw path replays.
//
// Memory payloads are skipped (only their ranges are tallied); the vertex,
// index and texture data step comes once the draw list itself is proven
// against a real trace.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <rex/graphics/packet_disassembler.h>
#include <rex/graphics/registers.h>
#include <rex/graphics/trace_protocol.h>
#include <snappy.h>

namespace fs = std::filesystem;
using namespace rex::graphics;

namespace {

constexpr uint32_t kRegisterCount = 0x5004;

struct Stats {
  size_t packets = 0;
  size_t register_writes = 0;
  size_t draws = 0;
  size_t swaps = 0;
  size_t memory_reads = 0;
  uint64_t memory_read_bytes = 0;
  size_t memory_writes = 0;
  std::map<std::string, size_t> draws_by_packet;
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: trace_draws <trace-file> [draws-out.jsonl]\n");
    return 2;
  }
  std::ifstream f(argv[1], std::ios::binary | std::ios::ate);
  if (!f) {
    std::fprintf(stderr, "cannot open %s\n", argv[1]);
    return 1;
  }
  std::vector<uint8_t> data(size_t(f.tellg()));
  f.seekg(0);
  f.read(reinterpret_cast<char*>(data.data()), std::streamsize(data.size()));

  const uint8_t* p = data.data();
  const uint8_t* end = p + data.size();

  const auto* header = reinterpret_cast<const TraceHeader*>(p);
  p += sizeof(TraceHeader);
  std::printf("trace: version %u, title %08X, %zu MB\n", header->version, header->title_id,
              data.size() >> 20);

  std::vector<uint32_t> regs(kRegisterCount, 0);
  Stats stats;

  std::ofstream draws_out;
  if (argc >= 3) {
    draws_out.open(argv[2]);
  }

  // Registers of interest for a draw record: the same state the emulated
  // backend samples when it builds a pipeline and issues a draw.
  static const struct { uint32_t index; const char* name; } kDrawRegs[] = {
      {reg::SQ_PROGRAM_CNTL::register_index, "sq_program_cntl"},
      {reg::RB_MODECONTROL::register_index, "rb_modecontrol"},
      {reg::RB_SURFACE_INFO::register_index, "rb_surface_info"},
      {reg::RB_DEPTHCONTROL::register_index, "rb_depthcontrol"},
      {reg::RB_COLORCONTROL::register_index, "rb_colorcontrol"},
      {reg::RB_BLENDCONTROL::register_index, "rb_blendcontrol0"},
      {reg::RB_COLOR_INFO::register_index, "rb_color_info"},
      {reg::RB_DEPTH_INFO::register_index, "rb_depth_info"},
      {reg::PA_SU_SC_MODE_CNTL::register_index, "pa_su_sc_mode_cntl"},
      {reg::VGT_DRAW_INITIATOR::register_index, "vgt_draw_initiator"},
  };

  while (p + sizeof(uint32_t) <= end) {
    auto type = *reinterpret_cast<const TraceCommandType*>(p);
    switch (type) {
      case TraceCommandType::kPrimaryBufferStart:
        p += sizeof(PrimaryBufferStartCommand);
        break;
      case TraceCommandType::kPrimaryBufferEnd:
        p += sizeof(PrimaryBufferEndCommand);
        break;
      case TraceCommandType::kIndirectBufferStart:
        p += sizeof(IndirectBufferStartCommand);
        break;
      case TraceCommandType::kIndirectBufferEnd:
        p += sizeof(IndirectBufferEndCommand);
        break;
      case TraceCommandType::kPacketStart: {
        auto cmd = reinterpret_cast<const PacketStartCommand*>(p);
        p += sizeof(*cmd);
        const uint8_t* packet_ptr = p;
        p += cmd->count * sizeof(uint32_t);
        ++stats.packets;

        PacketInfo info;
        if (!PacketDisassembler::DisasmPacket(packet_ptr, &info)) {
          break;
        }
        for (const PacketAction& action : info.actions) {
          if (action.type == PacketAction::Type::kRegisterWrite &&
              action.register_write.index < kRegisterCount) {
            regs[action.register_write.index] = action.register_write.value;
            ++stats.register_writes;
          }
        }
        if (info.type_info && info.type_info->category == PacketCategory::kDraw) {
          ++stats.draws;
          ++stats.draws_by_packet[info.type_info->name];
          if (draws_out.is_open()) {
            draws_out << "{\"n\":" << stats.draws << ",\"packet\":\"" << info.type_info->name
                      << "\"";
            char buf[64];
            for (const auto& r : kDrawRegs) {
              std::snprintf(buf, sizeof(buf), ",\"%s\":\"%08X\"", r.name, regs[r.index]);
              draws_out << buf;
            }
            draws_out << "}\n";
          }
        }
        break;
      }
      case TraceCommandType::kPacketEnd:
        p += sizeof(PacketEndCommand);
        break;
      case TraceCommandType::kMemoryRead: {
        auto cmd = reinterpret_cast<const MemoryCommand*>(p);
        p += sizeof(*cmd) + cmd->encoded_length;
        ++stats.memory_reads;
        stats.memory_read_bytes += cmd->decoded_length;
        break;
      }
      case TraceCommandType::kMemoryWrite: {
        auto cmd = reinterpret_cast<const MemoryCommand*>(p);
        p += sizeof(*cmd) + cmd->encoded_length;
        ++stats.memory_writes;
        break;
      }
      case TraceCommandType::kEdramSnapshot: {
        auto cmd = reinterpret_cast<const EdramSnapshotCommand*>(p);
        p += sizeof(*cmd) + cmd->encoded_length;
        break;
      }
      case TraceCommandType::kEvent: {
        auto cmd = reinterpret_cast<const EventCommand*>(p);
        p += sizeof(*cmd);
        if (cmd->event_type == EventCommand::Type::kSwap) {
          ++stats.swaps;
        }
        break;
      }
      case TraceCommandType::kRegisters: {
        auto cmd = reinterpret_cast<const RegistersCommand*>(p);
        p += sizeof(*cmd);
        const char* payload = reinterpret_cast<const char*>(p);
        p += cmd->encoded_length;
        std::string decoded;
        const uint32_t* values = nullptr;
        if (cmd->encoding_format == MemoryEncodingFormat::kSnappy) {
          if (!snappy::Uncompress(payload, cmd->encoded_length, &decoded)) {
            std::fprintf(stderr, "bad snappy block in registers command\n");
            return 1;
          }
          values = reinterpret_cast<const uint32_t*>(decoded.data());
        } else {
          values = reinterpret_cast<const uint32_t*>(payload);
        }
        for (uint32_t i = 0; i < cmd->register_count; ++i) {
          uint32_t index = cmd->first_register + i;
          if (index < kRegisterCount) {
            regs[index] = values[i];
          }
        }
        break;
      }
      case TraceCommandType::kGammaRamp: {
        auto cmd = reinterpret_cast<const GammaRampCommand*>(p);
        p += sizeof(*cmd) + cmd->encoded_length;
        break;
      }
      default:
        std::fprintf(stderr, "unknown trace command %u at offset %zu - stopping\n",
                     uint32_t(type), size_t(p - data.data()));
        p = end;
        break;
    }
  }

  std::printf("packets: %zu  register writes: %zu  swaps: %zu\n", stats.packets,
              stats.register_writes, stats.swaps);
  std::printf("memory reads: %zu (%llu MB)  writes: %zu\n", stats.memory_reads,
              (unsigned long long)(stats.memory_read_bytes >> 20), stats.memory_writes);
  std::printf("draws: %zu\n", stats.draws);
  for (const auto& [name, count] : stats.draws_by_packet) {
    std::printf("  %-24s %zu\n", name.c_str(), count);
  }
  return 0;
}
