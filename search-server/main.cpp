#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <stdexcept>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <optional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

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

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
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
    Document() : id(0), relevance(0.0), rating(0)
    {}
    Document(int p_id, double p_relevance, int p_rating) : id(p_id), relevance(p_relevance), rating(p_rating)
    {}
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    SearchServer() = default;
    explicit SearchServer(const string& stop_string) {
        auto tmp = SplitIntoWords(stop_string);
        for(const string& word: tmp) {
            if(!IsValidWord(word)) {
                throw invalid_argument("The stop word contains forbidden characters");
            }
        }
        stop_words_ = set(tmp.begin(), tmp.end());
    }

    template<typename T>
    explicit SearchServer(const T& t) {
        for(const string& word: t) {
            if(!IsValidWord(word)) {
                throw invalid_argument("The stop word contains forbidden characters");
            }
            stop_words_.insert(word);
        }
    }

     int GetDocumentId(int index) const {
        if(index < 0 || static_cast<size_t>(index) >= index_to_id_.size()) {
            // return INVALID_DOCUMENT_ID;
            throw out_of_range("Document index out of range");
        }
        return index_to_id_.at(index);
    }

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
    //------------проверка на правильность id----------
        if(document_id < 0 || documents_.count(document_id)) {
            throw invalid_argument("The document id < 0 or already exists");
            // return false;
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
    //------------проверка на спецсимволы--------------
        for(const string& word: words) {
            if(!IsValidWord(word)) {
                throw invalid_argument("The text of the document contains the following characters");
                // return false;
            }
        }

        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        if(index_to_id_.empty()) {
            index_to_id_[0] = document_id;
        } else {
            index_to_id_[index_to_id_.size()] = document_id;
            }
        // return true;
    }
    template <typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate pred) const {
        // const Query query = ParseQuery(raw_query);
        Query query;
        if(!ParseQuery(raw_query, query)) {
            throw invalid_argument("The query contains the following characters");
        }
        auto matched_documents = FindAllDocuments(query, pred);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                     return lhs.rating > rhs.rating;
                 }
                return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        // result = matched_documents;

        return matched_documents;
    }
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status] (int document_id, DocumentStatus status_, int rating) {
            return status_ == status;
        });
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [] (int document_id, DocumentStatus status_, int rating) {
            return status_ == DocumentStatus::ACTUAL;
        });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        // const Query query = ParseQuery(raw_query);
        Query query;
        if( !ParseQuery(raw_query, query) ) {
            throw invalid_argument("The query contains the following characters");
        }
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    map<int, int> index_to_id_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);;

        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
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

    bool ParseQuery(const string& text, Query& query) const {
        // Query query;
        for (const string& word : SplitIntoWords(text)) {
            if(!IsValidWord(word)) {  //если запрещённые символы
                return false;
            }
            for(int i = 0; static_cast<size_t>(i) < word.size(); ++i) {
                if(static_cast<int>(word.size()) == 1 && word[i] == '-') {  //если всё слово "-"
                    return false;
                }
                if(word[i] == '-') {
                    if( static_cast<size_t>(i) != (word.size()-1) ) { 
                        if( word[i+1] == '-') {   //если два минуса подряд
                            return false;
                        }
                    }
                }
            }
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return true;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate pred) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto &[document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (pred(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto &[document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto &[document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
};

// ==================== для примера =========================
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    // SearchServer search_server("и в на"s);
    // // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // // о неиспользуемом результате его вызова
    // (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    // if (!search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2})) {
    //     cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
    // }
    // if (!search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2})) {
    //     cout << "Документ не был добавлен, так как его id отрицательный"s << endl;
    // }
    // if (!search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2})) {
    //     cout << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
    // }
    // if (const auto documents = search_server.FindTopDocuments("--пушистый"s)) {
    //     for (const Document& document : *documents) {
    //         PrintDocument(document);
    //     }
    // } else {
    //     cout << "Ошибка в поисковом запросе"s << endl;
    // }
}