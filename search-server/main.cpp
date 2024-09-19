#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text) {
    vector<string> words;
    string word;
    for (const char c: text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id), relevance(relevance), rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template<typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer &strings) {
    set<string> non_empty_strings;
    for (const string &str: strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    static bool IsValidWord(const string &word) {
        return none_of(word.begin(), word.end(),
                       [](const char c) { return c >= '\0' && c < ' '; });
    }

    template<typename StringContainer>
    static bool IsValidWords(const StringContainer &words) {
        return all_of(words.begin(), words.end(), IsValidWord);
    }

    template<typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        if (!IsValidWords(stop_words_)) {
            throw invalid_argument("Стоп слово содержит спец символ");
        }
    }

    explicit SearchServer(const string &stop_words_text)
            : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    int GetDocumentId(const int index) {
        if (index >= 0 && index < static_cast<int>(documents_id_.size())) {
            return documents_id_[index];
        } else {
            throw out_of_range("Incorrect index");
        }
    }

    bool IsValidDoc(const int document_id, const vector<string> &words) {
        return document_id >= 0 &&
               find(documents_id_.begin(), documents_id_.end(), document_id) == documents_id_.end() &&
               all_of(words.begin(), words.end(), [](const string &word) { return IsValidWord(word); });

    }

    void AddDocument(int document_id, const string &document, DocumentStatus status,
                     const vector<int> &ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        if (!IsValidDoc(document_id, words)) {
            throw invalid_argument("Incorrect document");
        }
        const double inv_word_count = 1.0 / words.size();
        for (const string &word: words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        documents_id_.push_back(document_id);
    }

    template<typename DocumentPredicate>
    [[nodiscard]] vector<Document>
    FindTopDocuments(const string &raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);

        if (!IsValidQuery(query)) {
            throw invalid_argument("Not a valid argument");
        }
        vector<Document> matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;

    }

    [[nodiscard]] vector<Document>
    FindTopDocuments(const string &raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query,
                                [status](int document_id, DocumentStatus document_status, int rating) {
                                    return document_status == status;
                                });
    }

    [[nodiscard]] vector<Document> FindTopDocuments(const string &raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    [[nodiscard]] int GetDocumentCount() const {
        return documents_.size();
    }

    [[nodiscard]] tuple<vector<string>, DocumentStatus>
    MatchDocument(const string &raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        if (!IsValidQuery(query)) {
            throw invalid_argument("Not a valid argument");
        }
        vector<string> matched_words;
        for (const string &word: query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string &word: query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return tuple<vector<string>, DocumentStatus>{matched_words, documents_.at(document_id).status};

    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> documents_id_;

    [[nodiscard]] bool IsStopWord(const string &word) const {
        return stop_words_.count(word) > 0;
    }

    [[nodiscard]] vector<string> SplitIntoWordsNoStop(const string &text) const {
        vector<string> words;
        for (const string &word: SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int> &ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating: ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

//
    [[nodiscard]] QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    [[nodiscard]] static bool IsValidQuery(const Query &query) {
        //Predicator for finding words in a query with errors
        auto Predictor = [](const string &word) {
            if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
                return true;
            } else {
                return false;
            }
        };
        if (none_of(query.plus_words.begin(), query.plus_words.end(), Predictor) &&
            none_of(query.minus_words.begin(), query.minus_words.end(), Predictor)) {
            return true;
        } else {
            return false;
        }


    }

    [[nodiscard]] Query ParseQuery(const string &text) const {
        Query query;
        for (const string &word: SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    [[nodiscard]] double ComputeWordInverseDocumentFreq(const string &word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename DocumentPredicate>
    [[nodiscard]] vector<Document> FindAllDocuments(const Query &query,
                                                    DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string &word: query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto &[document_id, term_freq]: word_to_document_freqs_.at(word)) {
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string &word: query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto &[document_id, _]: word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto &[document_id, relevance]: document_to_relevance) {
            matched_documents.push_back(
                    {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

// ==================== test-case =========================


void PrintDocument(const Document &document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    SearchServer search_server("и на по с"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,
                              DocumentStatus::ACTUAL, {8, -3});
    search_server.GetDocumentId(0);
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,
                              DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
                              DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,
                              DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document &document: search_server.FindTopDocuments(
            "пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document &document: search_server.FindTopDocuments(
            "пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document &document: search_server.FindTopDocuments(
            "пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status,
               int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}