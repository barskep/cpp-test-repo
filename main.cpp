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
            if (word.empty()) {
                continue;
            }
            
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
        for (const string& word : words) {
            if (!word.empty()) {
                word_to_id_map_[word].insert(document_id);
                document_sizes_[document_id]++;
                document_words_counter_[document_id][word]++;
            }
        }

        for (const auto& [word, count] : document_words_counter_[document_id]) {
            document_words_tf_[document_id][word] = count / (document_sizes_[document_id] + 0.0);
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
    
    map<string, set<int>> word_to_id_map_;
    
    map<int, map<string, int>> document_words_counter_;
    map<int, map<string, double>> document_words_tf_;
    
    map<int, int> document_sizes_;

    set<string> stop_words_;

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
    
    int GetWordCountInDocument(const int& id, const string& word) const {
        if (document_words_counter_.count(id) == 0) {
            return 0;
        }
        
        if (document_words_counter_.at(id).count(word) == 0) {
            return 0;
        }
        
        return document_words_counter_.at(id).at(word);
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> result;

        set<int> minus_documents = GetMinusDocuments(query);
        const set<string>& query_words = query.GetQueryWords();
        map<string, double> words_idf = GetWordsIDFMap(query_words);
        map<int, double> relevance_map;
        double word_tf = 0;

        for (const string& query_word : query_words) {
            if (word_to_id_map_.count(query_word) == 0) {
                continue;
            }
            for (const int& id : word_to_id_map_.at(query_word)) {
                word_tf = document_words_tf_.at(id).at(query_word);
                relevance_map[id] += word_tf * words_idf[query_word];
            }
        }

        for (const auto& [id, relevance] : relevance_map) {
            if (minus_documents.count(id) > 0) {
                continue;
            }
            result.push_back({id, relevance_map[id]});
        }
        
        return result;
        
    }
    
    map<string, double> GetWordsIDFMap(const set<string>& query_words) const {
        map<string, double> result;
        
        for (const string& word : query_words) {
            result[word] = GetWordIDF(word);
        }
        
        return result;
    }
    
    double GetWordIDF(const string& word) const {
        if (word_to_id_map_.count(word) == 0) {
            return 0.0;
        }
        return log(document_count_ / (word_to_id_map_.at(word).size() + 0.0));
    }

    set<int> GetMinusDocuments(const Query& query) const {
        const set<string>& minus_words = query.GetMinusWords();
        
        set<int> minus_documents;

        for (const string& minus_word : minus_words) {
            if (word_to_id_map_.count(minus_word) == 0) {
                continue;
            }
            for (const int& id : word_to_id_map_.at(minus_word)) {
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