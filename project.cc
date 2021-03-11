/***
 * project.cc
 *
 * string matching problems
 */

#include <cstdio>
#include <cstdlib>
#include <vector>

template <typename T, typename... Ts>
constexpr void KILL(T head, Ts... tail) {
    std::fprintf(stderr, "[ERROR] %s:%d | ", __FILE__, __LINE__);  // NOLINT
    std::fprintf(stderr, head, tail...);                           // NOLINT
    std::fputc('\n', stderr);
    std::exit(-1);
}

template <typename T>
constexpr void KILL(T head) {
    std::fprintf(stderr, "[ERROR] %s:%d | %s\n", __FILE__, __LINE__, // NOLINT
                 head);  // NOLINT
}

template <typename T, typename... Ts>
constexpr void WARN(T head, Ts... tail) {
    std::fprintf(stderr, "[WARN]  %s:%d | ", __FILE__, __LINE__);  // NOLINT
    std::fprintf(stderr, head, tail...);                           // NOLINT
    std::fputc('\n', stderr);
}

template <typename T>
constexpr void WARN(T head) {
    std::fprintf(stderr, "[WARN]  %s:%d | %s\n", __FILE__, __LINE__, // NOLINT
                 head);  // NOLINT
}

template <typename T, typename... Ts>
constexpr void LOG(T head, Ts... tail) {
    std::fprintf(stderr, "[LOG]   %s:%d | ", __FILE__, __LINE__);  // NOLINT
    std::fprintf(stderr, head, tail...);                           // NOLINT
    std::fputc('\n', stderr);
}

template <typename T>
constexpr void LOG(T head) {
    std::fprintf(stderr, "[LOG]   %s:%d | %s\n", __FILE__, __LINE__, // NOLINT
                 head);  // NOLINT
}

enum class dna { A, C, T, G };

dna char_to_dna(int ch) {
    switch (ch) {
        case 'A':
            return dna::A;
        case 'C':
            return dna::C;
        case 'T':
            return dna::T;
        case 'G':
            return dna::G;
        default:
            KILL("invalid DNA aminoacid: %c.", ch);
    }
    __builtin_unreachable();
}

struct pattern {
    std::vector<dna> m_pat;  // NOLINT

    pattern() = default;
    explicit pattern(FILE* stream) {
        int ch = std::fgetc(stream);
        while (ch != '\n') {
            m_pat.push_back(char_to_dna(ch));
            ch = std::fgetc(stream);
        }
    }

    size_t len() const { return m_pat.size(); }
};

class text {
   private:
    std::vector<dna> m_text;

    size_t len() const { return m_text.size(); }

    void claim_success(size_t pos) {
        std::fprintf(stdout, "%zu ", pos);  // NOLINT
    }

   public:
    text() = default;
    explicit text(FILE* stream) {
        int ch = std::fgetc(stream);
        while (ch != '\n') {
            m_text.push_back(char_to_dna(ch));
            ch = std::fgetc(stream);
        }
    }

    void naive(pattern const& pat) {
        for (size_t i = 0; i + (pat.len() - 1) < this->len(); ++i) {
            bool success = true;
            for (size_t j = 0; success && j < pat.len(); ++j) {
                success = m_text[i + j] == pat.m_pat[j];
            }

            if (success) {
                claim_success(i);
            }
        }

        std::fputc('\n', stdout);
    }

    void kmp(pattern const& pat) {}

    void bm(pattern const& pat) {}
};

int main() {
    text txt;
    pattern pat;
    int ch;
    do {
        ch = std::fgetc(stdin);
        switch (ch) {
            case 'T':
                std::fgetc(stdin);  // drop space
                txt = text(stdin);
                break;
            case 'N':
                std::fgetc(stdin);  // drop space
                pat = pattern(stdin);
                txt.naive(pat);
                break;
            case 'K':
                std::fgetc(stdin);  // drop space
                pat = pattern(stdin);
                txt.kmp(pat);
                break;
            case 'B':
                std::fgetc(stdin);  // drop space
                pat = pattern(stdin);
                txt.bm(pat);
                break;
            case 'X':
            case EOF:
                break;
            default:
                WARN("unrecognized command `%c`", ch);
                while (std::fgetc(stdin) != '\n') {
                }
                break;
        }
    } while (ch != 'X' && ch != EOF);
}
