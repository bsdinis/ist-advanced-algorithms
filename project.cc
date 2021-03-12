/***
 * project.cc
 *
 * string matching problems
 */

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>

template <typename... Ts>
void KILL(char const * head, Ts... tail) {
    std::fprintf(stderr, "[ERROR] %s:%d | ", __FILE__, __LINE__);  // NOLINT
    std::fprintf(stderr, head, tail...);                           // NOLINT
    std::fputc('\n', stderr);
    std::exit(-1);
}

void KILL(char const * head) {
    std::fprintf(stderr, "[ERROR] %s:%d | %s\n", __FILE__, __LINE__, // NOLINT
                 head);  // NOLINT
}

#ifndef NDEBUG
template <typename... Ts>
void WARN(char const * head, Ts... tail) {
    std::fprintf(stderr, "[WARN]  %s:%d | ", __FILE__, __LINE__);  // NOLINT
    std::fprintf(stderr, head, tail...);                           // NOLINT
    std::fputc('\n', stderr);
}

void WARN(char const * head) {
    std::fprintf(stderr, "[WARN]  %s:%d | %s\n", __FILE__, __LINE__, // NOLINT
                 head);  // NOLINT
}

template <typename... Ts>
void LOG(char const * head, Ts... tail) {
    std::fprintf(stderr, "[LOG]   %s:%d | ", __FILE__, __LINE__);  // NOLINT
    std::fprintf(stderr, head, tail...);                           // NOLINT
    std::fputc('\n', stderr);
}

void LOG(char const * head) {
    std::fprintf(stderr, "[LOG]   %s:%d | %s\n", __FILE__, __LINE__, // NOLINT
                 head);  // NOLINT
}
#endif           // NDEBUG

enum class dna { A , C , T , G  };

inline size_t dna_to_int(dna const &d) {
    switch (d) {
        case dna::A: return 0;
        case dna::C: return 1;
        case dna::T: return 2;
        case dna::G: return 3;
    }
}

inline dna char_to_dna(int const & ch) {
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

template <typename T>
class span {
    private:
        T const * m_ptr;
        size_t m_sz;

    public:
        span(T const * ptr, size_t sz) : m_ptr(ptr), m_sz(sz) { }

        T const & operator[] (size_t pos) const {
#ifndef NDEBUG
            if (pos >= m_sz) {
                KILL("illegal access to position %zu @ %p: size is %zu", pos, m_ptr, m_sz);
            }
#endif // NDEBUG

            return m_ptr[pos]; // NOLINT
        }
};

class pattern;
class pattern {
    private:
    std::vector<dna> m_pat;
    mutable std::vector<size_t> m_prefix;
    mutable std::vector<size_t> m_good_sufix;

    pattern reverse() const {
        pattern reverse;
        reverse.m_pat.reserve(len());
        std::copy(std::crbegin(m_pat), std::crend(m_pat), std::back_inserter(reverse.m_pat));
        return reverse;
    }

    public:
    pattern() = default;
    explicit pattern(FILE* stream) {
        int ch = std::fgetc(stream);
        while (ch != '\n') {
            m_pat.push_back(char_to_dna(ch));
            ch = std::fgetc(stream);
        }
    }

    size_t len() const { return m_pat.size(); }

    dna operator[] (size_t pos) const {
        return m_pat[pos];
    }


    span<size_t> prefix() const {
        if (m_prefix.empty()) {
            m_prefix = std::vector<size_t>(m_pat.size());
            m_prefix[0] = 0;
            size_t k = 0;

            for (size_t q = 1; q < m_pat.size(); ++q) {
                while (k > 0 && m_pat[k] != m_pat[q]) {
                    k = m_prefix[k - 1];
                }

                if (m_pat[k] == m_pat[q]) {
                    ++k;
                }

                m_prefix[q] = k;
            }
        }

        return {m_prefix.data(), m_prefix.size()};
    }

    std::array<size_t, 4> last_occurence() const {
        std::array<size_t, 4> lambda = {0, 0, 0, 0};

        for (size_t i = 0; i < len(); i++) {
            lambda[dna_to_int(m_pat[i])] = i; // NOLINT
        }

        return lambda;
    }

    span<size_t> good_sufix() const {
        if (m_good_sufix.empty()) {
            auto pref = this->prefix();
            auto rev = this->reverse();
            auto reverse_pref = rev.prefix();
            m_good_sufix = std::vector<size_t>(len() + 1);
            std::fill(std::begin(m_good_sufix), std::end(m_good_sufix), len() - pref[len() - 1]);
            for (size_t l = 0; l < len(); ++l) {
                size_t j = len() - 1 - reverse_pref[l];
                m_good_sufix[j] = std::min(m_good_sufix[j], l - reverse_pref[l]);
            }
        }

        return {m_good_sufix.data(), m_good_sufix.size()};
    }
};

class text {
   private:
    std::vector<dna> m_text;

    size_t len() const { return m_text.size(); }

    void claim_success(size_t pos) {
        std::fprintf(stdout, "%zu ", pos);  // NOLINT
    }

    // returns first position in pattern which does not match
    // returns pat.len() if it is equal
    //
    size_t compare_pattern(pattern const & pat, size_t pos) const {
        for (size_t i = pat.len(); i > 0; --i) {
            if (m_text[pos + i - 1] != pat[i - 1]) {
                return i - 1;
            }
        }

        return pat.len();
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
            if (compare_pattern(pat, i) == pat.len()) {
                claim_success(i);
            }
        }

        std::fputc('\n', stdout);
    }

    void kmp(pattern const& pat) {
        auto prefix = pat.prefix();
        size_t q = 0;
        size_t comparisons = 0;

        // the guard should be (i + (pat.len - 1) < this->len())
        //
        for (size_t i = 0; i  < this->len(); ++i) {
            // this is just `while (q > 0 && pat[q] != m_text[i])`
            // the unfolding is done to accurately count comparisons
            //
            while (q > 0) {
                comparisons++;
                if (pat[q] == m_text[i]) {
                    break;
                }
                q = prefix[q];
            }

            comparisons++;
            if (pat[q] == m_text[i]) {
                q++;
            }

            if (q == pat.len()) {
                claim_success(i + 1 - pat.len() );
                q = prefix[q - 1];
            }
        }

        std::fprintf(stdout, "\n%zu \n", comparisons); // NOLINT
    }

    void bm(pattern const& pat) {
        size_t comparisons = 0;

        auto lambda = pat.last_occurence();
        auto gamma = pat.good_sufix();
        for (size_t i = 0; i + (pat.len() - 1) < this->len(); ) {
            auto comp = compare_pattern(pat, i);
            comparisons += (comp == pat.len() ? comp : (pat.len() - comp));
            if (comp == pat.len()) {
                claim_success(i);
                i += gamma[pat.len()];
            } else {
                i += std::max(gamma[comp], comp - lambda[dna_to_int(m_text[i + comp])]);
            }
        }

        std::fprintf(stdout, "\n%zu \n", comparisons); // NOLINT
    }
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
