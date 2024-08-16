// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:433
//

// Закомитьте изменения и отправьте их в свой репозиторий.
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <string>
#include <map>
#include <vector>

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
	int id;
	double relevance;
};
struct QueryWords {
	set<string> words_minus;
	set<string> words;
};

class SearchServer {
public:
	void SetStopWords(const string &text) {
		for (const string &word: SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string &document) {
		const auto doc = SplitIntoWordsNoStop(document);
		documents_count_++;
		const double temp_rel_tf = 1.0 / doc.size();
		for (const auto &word: doc) {
			words_storage_[word][document_id] += temp_rel_tf;
		}
	}

	vector<Document> FindTopDocuments(const string &raw_query) const {
		const QueryWords query_words = ParseQuery(raw_query);
		//We receive a list of required documents
		auto matched_documents = FindAllDocuments(query_words);
		//Sort by descending relevance
		sort(matched_documents.begin(), matched_documents.end(),
		     [](const Document &lhs, const Document &rhs) {
			     return lhs.relevance > rhs.relevance;
		     });
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

private:
	map<string, map<int, double>> words_storage_;//Word and his map<ID documents and relevance TF>
	set<string> stop_words_;
	unsigned int documents_count_ = 0;

	bool IsStopWord(const string &word) const {
		return stop_words_.count(word) > 0;
	}

	vector<string> SplitIntoWordsNoStop(const string &text) const {
		vector<string> words;
		for (const string &word: SplitIntoWords(text)) {
			if (!IsStopWord(word)) {
				words.push_back(word);
			}
		}
		return words;
	}

	QueryWords ParseQuery(const string &text) const {
		QueryWords query_words;
		for (const string &word: SplitIntoWordsNoStop(text)) {
			//Split into minus words and plus words
			if (word[0] == '-') {
				query_words.words_minus.insert(word.substr(1));
			} else {
				query_words.words.insert(word);
			}
		}
		return query_words;
	}

	double CalculateRelevanceIDF(const int number_entry_word) const {
		return log(documents_count_ / static_cast<double>(number_entry_word));
	}

	vector<Document> FindAllDocuments(const QueryWords &query_words) const {
		map<int, double> relevance; // ID and number of occurrences of a word in documents
		double temp_rel_idf{0};
		//We go through the words of the request
		for (const auto &query_word: query_words.words) {
			//We get an iterator for the desired word
			auto required_word = words_storage_.find(query_word);
			//Checking for existence in the repository
			if (required_word != words_storage_.end()) {
				//We cache the required rel_idf so as not to calculate it
				temp_rel_idf = CalculateRelevanceIDF(required_word->second.size());
				for (auto &[id, rel_tf]: required_word->second) {
					//We go through all the ID documents that the iterator refers to
					relevance[id] += rel_tf * temp_rel_idf;
				}
			}
		}
		//Analogue of the above function
		for (const auto &query_word: query_words.words_minus) {
			auto required_document = words_storage_.find(query_word);
			if (required_document != words_storage_.end()) {
				for (auto &[id, rel_tf]: words_storage_.find(query_word)->second) {
					relevance.erase(id);
				}
			}
		}
		//Transfer map to vector
		vector<Document> document_relevance;
		document_relevance.reserve(relevance.size());
		for (const auto &[id, rel]: relevance) {
			document_relevance.push_back({id, rel});
		}
		return document_relevance;
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
	for (const auto &[document_id, relevance]: search_server.FindTopDocuments(query)) {
		cout << "{ document_id = "s << document_id << ", "
		     << "relevance = "s << relevance << " }"s << endl;
	}
}