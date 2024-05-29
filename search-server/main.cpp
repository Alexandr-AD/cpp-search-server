#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        map<int, double> words_freq;
        const double word_TF = 1./words.size();
        for(const string& word: words)
        {
            word_to_document_freqs_[word][document_id] += word_TF;
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;
    int document_count_ = 0;

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

    struct Query {
      set<string> plus_words;
      set<string> minus_words;
    };
    
    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if(word[0] == '-')
            {
                string stripped = word.substr(1);
                if(!IsStopWord(stripped))
                {
                    query_words.minus_words.insert(stripped);
                }
            }
            else
                query_words.plus_words.insert(word);
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;
        double idf = 0;
        for (const auto& word : query_words.plus_words) {
            if(!word_to_document_freqs_.count(word))
            {
                continue;
            }

            idf = CalculateIDF(word);

            for (auto& [id, freq]: word_to_document_freqs_.at(word)) {
                document_to_relevance[id] += freq*idf;
            }
            idf = 0;
        }
        for (const auto& word : query_words.minus_words) {
            if(word_to_document_freqs_.count(word))
            {
                 for (auto& [id, freq]: word_to_document_freqs_.at(word)){
                    document_to_relevance.erase(id);
                }
            }
        }

        for(const auto& [id, rel] : document_to_relevance)
        {
            matched_documents.push_back({id, rel});
        }

        return matched_documents;
    }

    // Большое спасибо за развёрнутый комментарий (:

    // Вынес в отдельный метод
    double CalculateIDF(const string& word) const {
        
        double idf = 0;

        for(const auto& doc: word_to_document_freqs_.at(word)) {
            if(doc.second == 0)
                continue;
            else
                idf += 1;
        }
            
        idf = log(document_count_ / idf); // Локальную idf для этой функции можно было бы оставить int и возвращать результат логарифма,
        return idf;                       // но я сомневался в результате деления. Мог получиться целочисленный, поэтому оставил так
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}