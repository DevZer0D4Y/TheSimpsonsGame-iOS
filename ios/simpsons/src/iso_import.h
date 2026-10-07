#pragma once
#include <filesystem>
#include <functional>
void SimpsonsImportISO(std::function<void(std::filesystem::path)> ready);
