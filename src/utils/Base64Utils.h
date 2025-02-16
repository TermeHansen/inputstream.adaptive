/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>

namespace UTILS
{
namespace BASE64
{

void Encode(const char* input, size_t length, std::string& output);
std::string Encode(const unsigned char* input, size_t length);
std::string Encode(const char* input, size_t length);
void Encode(const std::string& input, std::string& output);
std::string Encode(const std::string& input);
void Decode(const char* input, size_t length, std::string& output);
std::string Decode(const char* input, size_t length);
void Decode(std::string_view input, std::string& output);
std::string Decode(std::string_view input);

} // namespace BASE64
} // namespace UTILS
