// Ahead-of-time shader compiler for the native GPU renderer.
//
// The emulated backend translates Xenos microcode to SPIR-V at runtime, on
// demand, and caches the result. A native renderer must not do that: it needs
// its shaders as ordinary SPIR-V modules it can build pipelines from at load
// time, with no guest microcode and no translator in the shipping path.
//
// This takes the microcode dumped by the `dump_shaders` cvar and emits one
// .spv per (shader, specialization) pair, plus a manifest the native renderer
// loads at startup.
//
// Feasibility was measured before writing this: a full play session
// instantiated 93 vertex + 60 pixel shaders across 156 pipeline variants --
// about 1.0 variants per shader. That is small enough to convert exhaustively
// offline, which is what makes a native renderer practical for this title.

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <set>
#include <tuple>
#include <string>
#include <vector>

#include <rex/graphics/pipeline/shader/shader.h>
#include <rex/graphics/pipeline/shader/spirv_translator.h>
#include <rex/graphics/xenos.h>
#include <rex/string/buffer.h>

namespace fs = std::filesystem;
namespace xenos = rex::graphics::xenos;
using rex::graphics::Shader;
using rex::graphics::SpirvShaderTranslator;

namespace {

std::vector<uint32_t> ReadUcode(const fs::path& path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) {
    return {};
  }
  auto size = size_t(f.tellg());
  f.seekg(0);
  std::vector<uint32_t> dwords(size / sizeof(uint32_t));
  f.read(reinterpret_cast<char*>(dwords.data()), std::streamsize(dwords.size() * 4));
  return dwords;
}

// The dumps are named shader_<hash>.ucode.bin.{vert,frag}.
bool ParseName(const fs::path& p, uint64_t* hash_out, xenos::ShaderType* type_out) {
  std::string name = p.filename().string();
  if (name.rfind("shader_", 0) != 0) {
    return false;
  }
  if (name.find(".ucode.bin.") == std::string::npos) {
    return false;
  }
  *type_out = name.ends_with(".vert") ? xenos::ShaderType::kVertex : xenos::ShaderType::kPixel;
  *hash_out = std::strtoull(name.substr(7, 16).c_str(), nullptr, 16);
  return true;
}

// (hash,type) -> the modifications the runtime actually used.
using Inventory = std::map<std::pair<uint64_t, int>, std::vector<uint64_t>>;

// A vertex/pixel pair the runtime built a pipeline from. The native renderer
// needs these to know which combinations to create pipelines for up front,
// which is the whole point of converting shaders ahead of time.
struct PipelinePair {
  uint64_t vs_hash, vs_modification, ps_hash, ps_modification;
  bool operator<(const PipelinePair& o) const {
    return std::tie(vs_hash, vs_modification, ps_hash, ps_modification) <
           std::tie(o.vs_hash, o.vs_modification, o.ps_hash, o.ps_modification);
  }
};

Inventory LoadInventory(const fs::path& csv, std::set<PipelinePair>* pipelines) {
  Inventory inv;
  std::ifstream f(csv);
  std::string line;
  std::getline(f, line);  // header
  while (std::getline(f, line)) {
    // vs_hash,vs_modification,ps_hash,ps_modification (hex, no prefix)
    uint64_t v[4]{};
    size_t pos = 0;
    bool ok = true;
    for (int i = 0; i < 4; ++i) {
      size_t comma = line.find(',', pos);
      std::string tok = line.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
      if (tok.empty()) { ok = false; break; }
      v[i] = std::strtoull(tok.c_str(), nullptr, 16);
      if (comma == std::string::npos) break;
      pos = comma + 1;
    }
    if (!ok) continue;
    auto add = [&inv](uint64_t h, int t, uint64_t m) {
      if (!h) return;
      auto& list = inv[{h, t}];
      for (uint64_t x : list) if (x == m) return;
      list.push_back(m);
    };
    add(v[0], 0, v[1]);
    add(v[2], 1, v[3]);
    if (pipelines && v[0] && v[2]) {
      pipelines->insert(PipelinePair{v[0], v[1], v[2], v[3]});
    }
  }
  return inv;
}

std::vector<uint64_t> InventoryMods(const Inventory& inv, uint64_t hash, xenos::ShaderType type) {
  auto it = inv.find({hash, type == xenos::ShaderType::kVertex ? 0 : 1});
  return it == inv.end() ? std::vector<uint64_t>{} : it->second;
}

const char* FetchDimensionName(xenos::FetchOpDimension dimension) {
  switch (dimension) {
    case xenos::FetchOpDimension::k1D:
      return "1d";
    case xenos::FetchOpDimension::k2D:
      return "2d";
    case xenos::FetchOpDimension::k3DOrStacked:
      return "3d_or_stacked";
    case xenos::FetchOpDimension::kCube:
      return "cube";
    default:
      return "unknown";
  }
}

// The vertex layout the guest asks for, in the form a host vertex input state
// is built from: one buffer per fetch constant, attributes at dword offsets.
void WriteVertexBindings(std::ostream& out, const Shader& shader) {
  out << "      \"vertex_bindings\": [";
  bool first_binding = true;
  for (const Shader::VertexBinding& binding : shader.vertex_bindings()) {
    out << (first_binding ? "\n" : ",\n");
    first_binding = false;
    out << "        {\"fetch_constant\": " << binding.fetch_constant
        << ", \"stride_words\": " << binding.stride_words << ", \"attributes\": [";
    bool first_attribute = true;
    for (const Shader::VertexBinding::Attribute& attribute : binding.attributes) {
      const auto& fetch = attribute.fetch_instr.attributes;
      out << (first_attribute ? "" : ", ");
      first_attribute = false;
      out << "{\"offset_words\": " << fetch.offset
          << ", \"format\": " << uint32_t(fetch.data_format)
          << ", \"exp_adjust\": " << fetch.exp_adjust
          << ", \"signed\": " << (fetch.is_signed ? "true" : "false")
          << ", \"integer\": " << (fetch.is_integer ? "true" : "false") << "}";
    }
    out << "]}";
  }
  out << (first_binding ? "" : "\n      ") << "],\n";
}

void WriteTextureBindings(std::ostream& out, const Shader& shader) {
  out << "      \"texture_bindings\": [";
  bool first = true;
  for (const Shader::TextureBinding& binding : shader.texture_bindings()) {
    out << (first ? "\n" : ",\n");
    first = false;
    out << "        {\"binding_index\": " << binding.binding_index
        << ", \"fetch_constant\": " << binding.fetch_constant << ", \"dimension\": \""
        << FetchDimensionName(binding.fetch_instr.dimension) << "\"}";
  }
  out << (first ? "" : "\n      ") << "]\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr,
                 "usage: aot_shaders <dump-dir> <out-dir> [shader-inventory.csv]\n"
                 "  <dump-dir>        directory written by the dump_shaders cvar\n"
                 "  <out-dir>         where .spv modules and manifest.json are written\n"
                 "  [shader-inventory] capture CSV naming the specializations and the\n"
                 "                    vertex/pixel pairs the runtime actually used\n");
    return 2;
  }
  fs::path dump_dir = argv[1];
  fs::path out_dir = argv[2];
  std::error_code ec;
  fs::create_directories(out_dir, ec);

  Inventory inventory;
  std::set<PipelinePair> pipelines;
  if (argc >= 4) {
    inventory = LoadInventory(argv[3], &pipelines);
    std::printf("inventory: %zu (shader,type) entries, %zu pipeline pairs\n", inventory.size(),
                pipelines.size());
  }

  // Match the runtime's configuration. The runtime builds Features from the
  // live VkDevice; Features(true) is the same thing for a modern driver --
  // SPIR-V 1.5 and every optional capability -- which is what RADV reports on
  // Van Gogh. Features(false) pins SPIR-V 1.0 with everything off, and the
  // translator then bails out on constructs it would otherwise emit fine.
  //
  // edram_fragment_shader_interlock is false because the host render target
  // path is what we ship (render_target_path_vulkan = "").
  SpirvShaderTranslator translator(SpirvShaderTranslator::Features(true), false, false, false);

  std::ofstream manifest(out_dir / "manifest.json");
  manifest << "{\n  \"shaders\": [\n";
  bool first_entry = true;

  size_t translated = 0, failed = 0, skipped = 0;
  for (const auto& entry : fs::directory_iterator(dump_dir, ec)) {
    uint64_t hash = 0;
    xenos::ShaderType type{};
    if (!entry.is_regular_file() || !ParseName(entry.path(), &hash, &type)) {
      continue;
    }
    std::vector<uint32_t> ucode = ReadUcode(entry.path());
    if (ucode.empty()) {
      ++skipped;
      continue;
    }

    // The dumps hold ucode_data(), which is already host endian -- the
    // constructor defaults to swapping from big endian, and double swapping
    // silently produces microcode that analyzes to nothing at all.
    Shader shader(type, hash, ucode.data(), ucode.size(), std::endian::native);
    rex::string::StringBuffer disasm;
    shader.AnalyzeUcode(disasm);

    // Write our disassembly next to the modules so it can be diffed against
    // the one the runtime dumped. That diff is the check that the microcode
    // reached the analyzer intact -- if it doesn't match, nothing downstream
    // means anything, however valid the SPIR-V looks.
    char asm_name[64];
    std::snprintf(asm_name, sizeof(asm_name), "%016lX.%s.asm", hash,
                  type == xenos::ShaderType::kVertex ? "vert" : "frag");
    std::ofstream asm_out(out_dir / asm_name);
    asm_out.write(disasm.buffer(), std::streamsize(disasm.length()));

    // Prefer the specializations the runtime actually instantiated, captured
    // in the shader inventory during a play session -- those are the variants
    // that need to exist for pipelines to be built without a translator.
    // Shaders the capture never reached fall back to a default modification,
    // which is enough to prove they convert but may not be the variant a draw
    // eventually asks for; widening the capture is what closes that gap.
    std::vector<uint64_t> mods = InventoryMods(inventory, hash, type);
    if (mods.empty()) {
      mods.push_back(type == xenos::ShaderType::kVertex
                         ? translator.GetDefaultVertexShaderModification(
                               shader.GetDynamicAddressableRegisterCount(63))
                         : translator.GetDefaultPixelShaderModification(
                               shader.GetDynamicAddressableRegisterCount(63)));
    }
    for (uint64_t modification : mods) {
      Shader::Translation* translation = shader.GetOrCreateTranslation(modification);
      const char* why = nullptr;
      if (!translation) {
        why = "GetOrCreateTranslation returned null";
      } else if (!translator.TranslateAnalyzedShader(*translation)) {
        why = "TranslateAnalyzedShader failed";
      } else if (translation->translated_binary().empty()) {
        why = "empty binary";
      }
      if (why) {
        std::fprintf(stderr, "FAIL %016lX %s mod=%016lX: %s\n", hash,
                     type == xenos::ShaderType::kVertex ? "vs" : "ps", modification, why);
        ++failed;
        continue;
      }
      const std::vector<uint8_t>& spirv = translation->translated_binary();

      char fname[64];
      std::snprintf(fname, sizeof(fname), "%016lX_%s_%016lX.spv", hash,
                    type == xenos::ShaderType::kVertex ? "vs" : "ps", modification);
      std::ofstream out(out_dir / fname, std::ios::binary);
      out.write(reinterpret_cast<const char*>(spirv.data()), std::streamsize(spirv.size()));

      char hash_text[24], modification_text[24];
      std::snprintf(hash_text, sizeof(hash_text), "%016lX", hash);
      std::snprintf(modification_text, sizeof(modification_text), "%016lX", modification);

      manifest << (first_entry ? "" : ",\n") << "    {\n";
      first_entry = false;
      manifest << "      \"hash\": \"" << hash_text << "\",\n"
               << "      \"type\": \"" << (type == xenos::ShaderType::kVertex ? "vs" : "ps")
               << "\",\n"
               << "      \"modification\": \"" << modification_text << "\",\n"
               << "      \"spirv\": \"" << fname << "\",\n"
               << "      \"spirv_bytes\": " << spirv.size() << ",\n"
               << "      \"register_count\": " << shader.register_static_address_bound() << ",\n"
               << "      \"dynamic_register_addressing\": "
               << (shader.uses_register_dynamic_addressing() ? "true" : "false") << ",\n"
               << "      \"kills_pixels\": " << (shader.kills_pixels() ? "true" : "false") << ",\n"
               << "      \"writes_depth\": " << (shader.writes_depth() ? "true" : "false") << ",\n"
               << "      \"writes_color_targets\": " << shader.writes_color_targets() << ",\n";
      WriteVertexBindings(manifest, shader);
      WriteTextureBindings(manifest, shader);
      manifest << "    }";
      ++translated;
    }
  }

  manifest << "\n  ],\n  \"pipelines\": [\n";
  bool first_pipeline = true;
  for (const PipelinePair& pipeline : pipelines) {
    char vs_hash[24], vs_modification[24], ps_hash[24], ps_modification[24];
    std::snprintf(vs_hash, sizeof(vs_hash), "%016lX", pipeline.vs_hash);
    std::snprintf(vs_modification, sizeof(vs_modification), "%016lX", pipeline.vs_modification);
    std::snprintf(ps_hash, sizeof(ps_hash), "%016lX", pipeline.ps_hash);
    std::snprintf(ps_modification, sizeof(ps_modification), "%016lX", pipeline.ps_modification);
    manifest << (first_pipeline ? "" : ",\n") << "    {\"vs\": \"" << vs_hash
             << "\", \"vs_modification\": \"" << vs_modification << "\", \"ps\": \"" << ps_hash
             << "\", \"ps_modification\": \"" << ps_modification << "\"}";
    first_pipeline = false;
  }
  manifest << "\n  ]\n}\n";

  std::printf("AOT shader compile: %zu translated, %zu failed, %zu skipped -> %s\n", translated,
              failed, skipped, out_dir.string().c_str());
  return failed ? 1 : 0;
}
