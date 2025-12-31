#ifndef ENCRYPTEDMESSENGER_BASE64_H
#define ENCRYPTEDMESSENGER_BASE64_H

#include <string>
#include <vector>

namespace base64 {

    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    inline std::string encode(const uint8_t* data, size_t len) {
        std::string out;
        out.reserve((len * 4 + 2) / 3);

        int val = 0;
        int valb = -6;

        for (size_t i = 0; i < len; i++) {
            uint8_t c = data[i];
            val = (val << 8) + c;
            valb += 8;

            while (valb >= 0) {
                out.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }

        if (valb > -6) {
            out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }
        while (out.size() % 4) {
            out.push_back('=');
        }

        return out;
    }

    inline std::string encode(const std::vector<uint8_t>& data) {
        return encode(data.data(), data.size());
    }

    inline std::string encode(const std::string& data) {
        return encode(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    inline std::vector<uint8_t> decode(const std::string& s) {
        static std::vector<int> T = []{
            std::vector<int> t(256, -1);
            for (int i = 0; i < 64; i++) t[(unsigned char)chars[i]] = i;
            return t;
        }();

        std::vector<uint8_t> out;
        out.reserve(s.size() * 3 / 4);

        int val = 0;
        int valb = -8;

        for (unsigned char c : s) {
            if (c == '=' ) break; // padding = end
            if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue; // ignore whitespace

            int d = T[c];
            if (d == -1) {
                // invalid character
                break;
            }

            val = (val << 6) + d;
            valb += 6;

            if (valb >= 0) {
                out.push_back(uint8_t((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    inline std::string bytesToString(const std::vector<uint8_t>& v) {
        return std::string(reinterpret_cast<const char*>(v.data()), v.size());
    }

}

#endif //ENCRYPTEDMESSENGER_BASE64_H