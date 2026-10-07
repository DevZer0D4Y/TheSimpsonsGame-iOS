#include <rex/graphics/vulkan/upload_policy.h>
#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
using rex::graphics::vulkan::CpuUploadPath;
using rex::graphics::vulkan::SelectCpuUploadPath;

// Models a command buffer with a submission prelude and interleaved draw/copy
// commands. The real renderer uses the same path selector; draws must retain
// the CPU version available when their commands were recorded.
struct Submission {
  std::string gpu = "previous", prelude;
  bool has_prelude = false, touched = false;
  std::vector<std::function<void()>> commands;
  std::vector<std::string> draws;
  void Upload(std::string cpu, bool directly_writable = false, bool enabled = true) {
    auto path = SelectCpuUploadPath(has_prelude, touched, enabled, directly_writable);
    if (path == CpuUploadPath::kDeferred) {
      has_prelude = true;
      prelude = std::move(cpu);
    } else if (path == CpuUploadPath::kDirect) {
      gpu = std::move(cpu);
    } else {
      touched = true;
      commands.push_back([this, snapshot = std::move(cpu)] { gpu = snapshot; });
    }
  }
  void Draw() {
    touched = true;
    commands.push_back([this] { draws.push_back(gpu); });
  }
  void Execute() {
    if (has_prelude) gpu = prelude;
    for (auto& cmd : commands) cmd();
  }
};
int main() {
  {
    Submission s;
    s.Upload("EA"); s.Draw();
    s.Upload("copyright"); s.Draw();
    s.Upload("trademark"); s.Draw(); s.Execute();
    assert((s.draws == std::vector<std::string>{"EA", "copyright", "trademark"}));
  }
  {
    Submission s;
    s.Upload("unused draft"); s.Upload("EA"); s.Draw(); s.Execute();
    assert((s.draws == std::vector<std::string>{"EA"}));
  }
  {
    Submission s;
    s.Upload("EA"); s.Draw();
    // The mapped page still contains 'previous', but the queued draw will read
    // 'EA'. Matching the mapped bytes cannot authorize changing its snapshot.
    s.Upload("previous", true); s.Draw(); s.Execute();
    assert((s.draws == std::vector<std::string>{"EA", "previous"}));
  }
  {
    Submission s;
    s.Upload("EA", false, false); s.Draw();
    s.Upload("copyright", false, false); s.Draw(); s.Execute();
    assert((s.draws == std::vector<std::string>{"EA", "copyright"}));
  }
  for (int i = 0; i < 1000; ++i) {
    Submission s;
    std::vector<std::string> expected;
    for (int j = 0; j < 12; ++j) {
      auto text = std::to_string(i) + ":glyph:" + std::to_string(j);
      s.Upload(text); s.Draw(); expected.push_back(text);
    }
    s.Execute(); assert(s.draws == expected);
  }
  std::cout << "PASS: ordered GPU draws retain each CPU upload version (12,000 draws)\n";
}
