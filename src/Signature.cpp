// Spooky Scoreboard Daemon
// Copyright (C) 2025 Greg MacKenzie
// https://spookyscoreboard.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <iostream>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <json/json.h>

#include "Signature.h"
#include "Config.h"

std::string Signature::sign(const Json::Value& payload)
{
  Json::Value copy = payload;
  copy.removeMember("signature");

  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  std::string canonical = Json::writeString(builder, copy);

  return computeHmac(canonical);
}

bool Signature::verify(const Json::Value& payload)
{
  if (!payload.isMember("signature") || !payload["signature"].isString()) {
    std::cerr << "Missing signature" << std::endl;
    return false;
  }

  Json::Value copy = payload;
  copy.removeMember("signature");

  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  std::string canonical = Json::writeString(builder, copy);

#ifdef DEBUG
  std::cout << "Computed=" << computeHmac(canonical) << std::endl;
  std::cout << "Received=" << payload["signature"].asString() << std::endl;
#endif

  return computeHmac(canonical) == payload["signature"].asString();
}

std::string Signature::computeHmac(const std::string& canonical)
{
  unsigned char hmac[EVP_MAX_MD_SIZE];
  unsigned int len = 0;

  HMAC(EVP_sha256(),
    Config::token.data(), static_cast<int>(Config::token.size()),
    reinterpret_cast<const unsigned char*>(canonical.data()), canonical.size(),
    hmac, &len);

  return base64Encode(hmac, len);
}

std::string Signature::base64Encode(const unsigned char* data, size_t len)
{
  static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((len + 2) / 3 * 4);

  for (size_t i = 0; i < len; i += 3) {
    uint32_t a = data[i];
    uint32_t b = (i + 1 < len) ? data[i + 1] : 0;
    uint32_t c = (i + 2 < len) ? data[i + 2] : 0;
    uint32_t t = (a << 16) | (b << 8) | c;

    out += table[(t >> 18) & 0x3F];
    out += table[(t >> 12) & 0x3F];
    out += (i + 1 < len) ? table[(t >> 6) & 0x3F] : '=';
    out += (i + 2 < len) ? table[t & 0x3F] : '=';
  }

  return out;
}

// vim: set ts=2 sw=2 expandtab:

