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

class Query {
public: 
    void Setup(const string& query) {
        vector<string> words = SplitIntoWords(query);
        for(const string& word : words) {            
            if (word[0] == '-') {
                minus_words_.insert(word.substr(1));
            } else {
                query_words_.insert(word);
            }
        }
    }
    
    set<string> GetQueryWords() const {
        return query_words_;
    }
    
    set<string> GetMinusWords() const {
        return minus_words_;
    }
private:
    set<string> query_words_;
    set<string> minus_words_;
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
        const double size = words.size() + 0.0;
        for (const string& word : words) {
            document_words_tf_[word][document_id] += 1 / size;
        }
        
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        Query query;
        query.Setup(raw_query);
        auto matched_documents = FindAllDocuments(query);

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
    int document_count_ = 0;
    
    map<string, map<int, double>> document_words_tf_;

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        string word;
        for (const char c : text) {
            if (c == ' ') {
                if (!word.empty()) {
                    if (!IsStopWord(word)) {
                        words.push_back(word);
                    }
                    word.clear();
                }
            } else {
                word += c;
            }
        }
        if (!word.empty() && !IsStopWord(word)) {
            words.push_back(word);
        }

        return words;
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> result;

        const set<int>& minus_documents = GetMinusDocuments(query);
        const set<string>& query_words = query.GetQueryWords();
        map<int, double> relevance_map;
        double word_idf = 0;
        double size = 0;

        for (const string& query_word : query_words) {
            if (document_words_tf_.count(query_word) == 0) {
                continue;
            }
            size = document_words_tf_.at(query_word).size() + 0.0;
            for (const auto& [id, tf] : document_words_tf_.at(query_word)) {
                word_idf = log(document_count_ / size);
                relevance_map[id] += tf * word_idf;
            }
        }

        for (const auto& [id, relevance] : relevance_map) {
            
            result.push_back({id, relevance_map[id]});
        }
        
        return result;
        
    }

    set<int> GetMinusDocuments(const Query& query) const {
        const set<string>& minus_words = query.GetMinusWords();
        
        set<int> minus_documents;

        for (const string& minus_word : minus_words) {
            if (document_words_tf_.count(minus_word) == 0) {
                continue;
            }
            for (const auto& [id, tf] : document_words_tf_.at(minus_word)) {
                minus_documents.insert(id);
            }
        }

        return minus_documents;
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