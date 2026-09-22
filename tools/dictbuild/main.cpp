// Build-time helper. Turns a plain word list into the sorted, deduplicated file
// that Dictionary memory-maps at runtime.
//
//     dictbuild <input.txt> <output.dict>
//
// The output is UTF-8, one word per line, sorted by byte value, deduplicated, with a
// trailing newline. Dictionary::contains() binary searches on byte order, so the sort
// performed here and the comparison performed there have to agree exactly. That is why
// this uses std::string_view comparison, which char_traits defines in terms of
// unsigned char, the same ordering memcmp gives.
//
// Doing this once at build time is what lets the editor stop sorting 1.3 million
// strings on the UI thread every time the transcript language changes.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool readFile(const char* path, std::string& out)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        return false;

    const std::streamsize size = in.tellg();
    if (size < 0)
        return false;

    in.seekg(0, std::ios::beg);
    out.resize(static_cast<std::size_t>(size));
    if (size > 0 && !in.read(out.data(), size))
        return false;

    return true;
}

std::string_view trim(std::string_view s)
{
    const auto isBlank = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && isBlank(s.front()))
        s.remove_prefix(1);
    while (!s.empty() && isBlank(s.back()))
        s.remove_suffix(1);
    return s;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: dictbuild <input.txt> <output.dict>\n");
        return 2;
    }

    std::string data;
    if (!readFile(argv[1], data)) {
        std::fprintf(stderr, "dictbuild: cannot read %s\n", argv[1]);
        return 1;
    }

    // Strip a UTF-8 byte order mark so it cannot end up sorting ahead of every word.
    std::string_view rest(data);
    if (rest.size() >= 3 && static_cast<unsigned char>(rest[0]) == 0xEF
        && static_cast<unsigned char>(rest[1]) == 0xBB && static_cast<unsigned char>(rest[2]) == 0xBF) {
        rest.remove_prefix(3);
    }

    std::vector<std::string_view> words;
    words.reserve(rest.size() / 8 + 1);

    while (!rest.empty()) {
        const std::size_t nl = rest.find('\n');
        std::string_view line = nl == std::string_view::npos ? rest : rest.substr(0, nl);
        rest = nl == std::string_view::npos ? std::string_view{} : rest.substr(nl + 1);

        line = trim(line);
        if (!line.empty())
            words.push_back(line);
    }

    std::sort(words.begin(), words.end());
    words.erase(std::unique(words.begin(), words.end()), words.end());

    std::string out;
    std::size_t total = 0;
    for (std::string_view w : words)
        total += w.size() + 1;
    out.reserve(total);
    for (std::string_view w : words) {
        out.append(w);
        out.push_back('\n');
    }

    std::ofstream os(argv[2], std::ios::binary | std::ios::trunc);
    if (!os || !os.write(out.data(), static_cast<std::streamsize>(out.size()))) {
        std::fprintf(stderr, "dictbuild: cannot write %s\n", argv[2]);
        return 1;
    }

    std::printf("dictbuild: %s -> %s (%zu words)\n", argv[1], argv[2], words.size());
    return 0;
}
