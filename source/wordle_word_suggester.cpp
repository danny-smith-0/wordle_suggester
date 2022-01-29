#include <wordle_word_suggester.h>

#include <algorithm> // std::unique
#include <iostream>  // std::cout
#include <iomanip>   // std::setw
#include <fstream>   // std::ifstream (and ofstream)
#include <sstream>   // std::stringstream

#include <set>

using namespace wordle;


template <typename T>
void sort_and_remove_non_unique_elements(T* vector_or_string)
{
    std::sort(vector_or_string->begin(), vector_or_string->end());
    auto itr_last = std::unique(vector_or_string->begin(), vector_or_string->end());
    vector_or_string->erase(itr_last, vector_or_string->end());
}

void WordSuggester::load_words()
{
    std::string file_path = "../include/valid_answers.txt";
    std::ifstream file_stream1 (file_path);
    std::string line;
    while (std::getline(file_stream1, line))
        _valid_answers_orig.push_back(line);

    file_path = "../include/valid_guesses.txt";
    std::ifstream file_stream2 (file_path);
    while (std::getline(file_stream2, line))
        _valid_guesses_orig.push_back(line);

    _valid_answers_trimmed = _valid_answers_orig;
    _valid_guesses_trimmed = _valid_guesses_orig;
}

size_t WordSuggester::remove_words_with_letter(char letter)
{
    if (_excluded_letters.find(letter) == std::string::npos)
        _excluded_letters += letter;

    for (auto itr = _valid_answers_trimmed.begin(); itr != _valid_answers_trimmed.end(); )
    {
        if (itr->find(letter) != std::string::npos)
            itr = _valid_answers_trimmed.erase(itr);
        else
            ++itr;
    }

    for (auto itr = _valid_guesses_trimmed.begin(); itr != _valid_guesses_trimmed.end(); )
    {
        if (itr->find(letter) != std::string::npos)
            itr = _valid_guesses_trimmed.erase(itr);
        else
            ++itr;
    }

    return _valid_answers_trimmed.size();
}

size_t WordSuggester::remove_words_with_letter_index(char letter, size_t index)
{
    for (auto itr = _valid_answers_trimmed.begin(); itr != _valid_answers_trimmed.end(); )
    {
        if (itr->find(letter) != std::string::npos && itr->at(index) == letter)
            itr = _valid_answers_trimmed.erase(itr);
        else
            ++itr;
    }

    // for (auto itr = _valid_guesses_trimmed.begin(); itr != _valid_guesses_trimmed.end(); )
    // {
    //     if (itr->find(letter) != std::string::npos && itr->at(index) == letter)
    //         itr = _valid_guesses_trimmed.erase(itr);
    //     else
    //         ++itr;
    // }
    return _valid_answers_trimmed.size();
}

void WordSuggester::black_letter(char letter)
{
    this->remove_words_with_letter(letter);
}


void WordSuggester::green_letter(char letter, size_t correct_index)
{
    this->remove_words_without_letter_index(letter, correct_index);
}

void WordSuggester::yellow_letter(char letter, size_t wrong_index)
{
    this->remove_words_without_letter(letter);

    this->remove_words_with_letter_index(letter, wrong_index);
}

size_t WordSuggester::remove_words_without_letter(char required_letter)
{
    if (_required_letters.find(required_letter) == std::string::npos)
        _required_letters += required_letter;

    for (auto itr = _valid_answers_trimmed.begin(); itr != _valid_answers_trimmed.end(); )
    {
        if (itr->find(required_letter) == std::string::npos)
            itr = _valid_answers_trimmed.erase(itr);
        else
            ++itr;
    }

    // for (auto itr = _valid_guesses_trimmed.begin(); itr != _valid_guesses_trimmed.end(); )
    // {
    //     if (itr->find(required_letter) == std::string::npos)
    //         itr = _valid_guesses_trimmed.erase(itr);
    //     else
    //         ++itr;
    // }
    return _valid_answers_trimmed.size();
}

size_t WordSuggester::remove_words_without_letter_index(char required_letter, size_t index)
{
    if (_required_letters.find(required_letter) == std::string::npos)
        _required_letters += required_letter;

    for (auto itr = _valid_answers_trimmed.begin(); itr != _valid_answers_trimmed.end(); )
    {
        if (itr->find(required_letter) == std::string::npos || itr->at(index) != required_letter)
            itr = _valid_answers_trimmed.erase(itr);
        else
            ++itr;
    }

    // for (auto itr = _valid_guesses_trimmed.begin(); itr != _valid_guesses_trimmed.end(); )
    // {
    //     if (itr->find(required_letter) == std::string::npos || itr->at(index) != required_letter)
    //         itr = _valid_guesses_trimmed.erase(itr);
    //     else
    //         ++itr;
    // }

    return _valid_answers_trimmed.size();
}


void WordSuggester::print_words(int words_per_row, std::vector<std::string> words)
{
    // Default to printing the valid answers
    if (words.empty())
        words = this->_valid_answers_trimmed;

    int count = 1;
    for (auto word : words)
    {
        std::cout << word << " ";
        if (++count > words_per_row)
        {
            std::cout << "\n";
            count = 1;
        }
    }
    std::cout << "\n";
}

void WordSuggester::subtract_required_letters(std::vector<std::string> const& words, std::string required_letters, std::string* unspecified_letters, std::vector<std::string>* unspecified_letters_by_word)
{
    for (auto word : words)
    {
        std::string reqs = required_letters;  // Make a copy so we can edit
        std::string remaining_letters;
        for (auto mychar : word)
        {
            if (reqs.find(mychar) != std::string::npos)
                reqs.erase(reqs.find(mychar), 1);
            else
                remaining_letters += mychar;
        }
        if (unspecified_letters)
            *unspecified_letters += remaining_letters;
        if (unspecified_letters_by_word && !remaining_letters.empty())
            unspecified_letters_by_word->push_back(remaining_letters);
    }
}

void WordSuggester::score_words_by_letter_scores(std::vector<std::string> const& unspecified_letters_by_word, std::vector<std::string> const& original_words, std::map<char, size_t> const& letter_count)
{
    if (this->_valid_answers_trimmed.size() == 1)
    {
        std::cout << "Final answer: " << this->_valid_answers_trimmed[0] << std::endl;
        return;
    }

    // std::map<std::string, size_t> scoring_words;
    std::map<size_t, std::string> scoring_words;
    for (size_t ii = 0; ii != unspecified_letters_by_word.size(); ++ii)
    {
        // Don't score a word for the same letter multiple times.
        auto one_words_unspecifieds = unspecified_letters_by_word[ii];  // Make a copy so we can edit
        sort_and_remove_non_unique_elements(&one_words_unspecifieds);

        size_t score = 0;
        for (auto mychar : one_words_unspecifieds)
        {
            if (letter_count.find(mychar) == letter_count.end())
                continue;
            score += letter_count.at(mychar);
        }

        // scoring_words[this->_valid_answers_trimmed[ii]] = score;
        scoring_words[score] += original_words[ii];
    }

    // Print out the scored by score
    size_t count = 0;
    size_t word_print_cutoff = 25;
    for (auto ritr = scoring_words.rbegin(); ritr != scoring_words.rend(); ++ritr)
    {
        std::string words = ritr->second;
        while (count < word_print_cutoff && !words.empty())
        {
            std::string word (words.begin(), words.begin() + 5);
            words.erase(words.begin(), words.begin() + 5);
            std::cout << word << " : " << ritr->first << "\n";
            ++count;
        }
    }
    if ( count >= word_print_cutoff)
        std::cout << " Printed only " << word_print_cutoff << " of " << original_words.size() << " words\n";
    std::cout << "\n";
}

void WordSuggester::suggest()
{
    if (this->_valid_answers_trimmed.size() == 1)
    {
        std::cout << "\nFinal answer: " << this->_valid_answers_trimmed[0] << "\n\n";
        return;
    }

    if (this->_valid_answers_trimmed.size() == 2)
    {
        std::cout << "\nFlip a coin!\n" << this->_valid_answers_trimmed[0] << "\n" << this->_valid_answers_trimmed[1] << "\n\n";
        return;
    }

    // Get all the letters in the words that weren't required, grouped all together and by word
    std::string unspecified_letters = "";
    std::vector<std::string> unspecified_letters_by_word;
    std::vector<std::string> unspecified_letters_by_word__guesses;

    subtract_required_letters(this->_valid_answers_trimmed, this->_required_letters, &unspecified_letters, &unspecified_letters_by_word);
    subtract_required_letters(this->_valid_guesses_orig,    this->_required_letters,                    0, &unspecified_letters_by_word__guesses);

    // Count (score) the unspecified letters
    std::sort(unspecified_letters.begin(), unspecified_letters.end());
    std::map<char, size_t> letter_count;
    if (!unspecified_letters.empty())
    {
        letter_count[unspecified_letters[0]] = 1;
        for (auto itr = unspecified_letters.begin(); itr != unspecified_letters.end() - 1; ++itr)
        {
            if (*(itr + 1) == *itr)
                ++letter_count[*itr];
            else
                letter_count[*(itr + 1)] = 1;
        }
    }
    // Remove non-unique letters
    sort_and_remove_non_unique_elements(&unspecified_letters);

    // // Print the letter counts
    // std::cout << "Counts of the unspecified letters in the remaining words (alphabetical & by count)\n ";
    // for (auto letter : unspecified_letters)
    //     std::cout << "  " << letter << " ";
    // std::cout << "\n";
    // for (auto letter : unspecified_letters)
    //     std::cout << std::fixed << std::setw(4) << letter_count[letter];
    // std::cout << "\n\n";

    // Backwards letter count, then print in order
    std::map<size_t, std::string> backwards_letter_count;
    for (auto [letter, score] : letter_count)
        backwards_letter_count[score] += letter;

    std::stringstream ss;
    std::cout << "Scoring the remaining words by the count of their unique unspecified letters\n ";
    for (auto ritr = backwards_letter_count.rbegin(); ritr != backwards_letter_count.rend(); ++ritr)
    {
        std::string letters = ritr->second;
        while (!letters.empty())
        {
            std::cout << "  " << letters[0] << " ";
            letters.erase(0, 1);
            ss << std::fixed << std::setw(4) << ritr->first;
        }
    }
    std::cout << "\n" << ss.str() << "\n\n";

    // Score the remaining words by popular letter - easy first pass for a score
    std::cout << "valid answers: \n";
    score_words_by_letter_scores(unspecified_letters_by_word, this->_valid_answers_trimmed, letter_count);
    std::cout << "valid guesses (invalid answers): \n";
    score_words_by_letter_scores(unspecified_letters_by_word__guesses, this->_valid_guesses_orig, letter_count);

    /*
    Idea of next pass on score - what word, if guessed could give us the most information about the remaining words?
    Which touches the most words, but also leads us closer to a final answer.
    It's tricky, because there are a lot of types of information. It'll be fun to work through

    So far I haven't been considering location of the letters after the initial removing, before this function. That will affect how to get info.
    Compare all guess words, see which have the min, max, and average number of remaining words across the guess and all remaining valid answers
    */

    this->how_many_words_remain_after_guess();
}

colors_with_answers_t WordSuggester::how_many_words_remain_after_guess(std::string guess, std::vector<std::string> words)
{

    // Get the words that match with each color result
    colors_with_answers_t words_by_results;

    for (auto word : words)
    {
        std::string color_res;
        for (size_t guess_index = 0; guess_index < guess.size(); ++guess_index)
        {
            char guess_letter = guess[guess_index];
            if (word.find(guess_letter) == std::string::npos) // not in the word
                color_res += "k";
            else if (word[guess_index] == guess_letter)  // in the exact same spot
                color_res += "g";
            else                                  // in the wrong spot
                color_res += "y";
        }
        words_by_results[color_res].push_back(word);
    }

    //average
    double average_words_remaining = static_cast<double>(words.size()) / static_cast<double>(words_by_results.size());

    return words_by_results;
}

double get_average_bucket_size(colors_with_answers_t in)
{
    double total_elements = 0;
    for (auto row : in)
        total_elements += row.second.size();
    return total_elements / static_cast<double>(in.size());
}

size_t max_bucket_size(colors_with_answers_t in)
{
    size_t max_bucket_size = 0;
    for (auto row : in)
        max_bucket_size = std::max(row.second.size(), max_bucket_size);
    return max_bucket_size;
}

// TODO make this a tuple so I can include the results
struct comparator
{
    template <typename T>
    bool operator()(T const& l, T const& r) const
    {
        if (l.second != r.second)
            return l.second < r.second;
        return l.first < r.first;
    }
};

void WordSuggester::how_many_words_remain_after_guess()
{
    std::cout  << "\n\nhow_many_words_remain_after_guess\nanswers: (out of " << this->_valid_answers_trimmed.size() << ")\n";

    // TODO refactor These next two blocks
    std::map<std::string, double> answer_average_result;
    std::map<std::string, colors_with_answers_t> answer_all_results;
    for (auto word : this->_valid_answers_trimmed)
    {
        colors_with_answers_t words_by_results = this->how_many_words_remain_after_guess(word, this->_valid_answers_trimmed);
        answer_average_result[word] = get_average_bucket_size(words_by_results);
        answer_all_results[word] = words_by_results;
    }
    std::set<std::pair<std::string, double>, comparator> answers_ordered(answer_average_result.begin(), answer_average_result.end());
    int count = 0;
    static constexpr int count_cutoff = 20;
    for (auto [word, average_words_remaining] : answers_ordered)
    {
        std::cout << "    " << word << " :: " << average_words_remaining << ", max bucket: " << max_bucket_size(answer_all_results[word]) << "\n";
        if (count++ >= count_cutoff)
            break;
    }

    std::cout  << "\n\nguesses:\n";
    std::map<std::string, double> guess_average_result;
    std::map<std::string, colors_with_answers_t> guess_all_results;
    for (auto word : this->_valid_guesses_orig)
    {
        colors_with_answers_t words_by_results = this->how_many_words_remain_after_guess(word, this->_valid_answers_trimmed);
        guess_average_result[word] = get_average_bucket_size(words_by_results);
        guess_all_results[word] = words_by_results;
    }
    std::set<std::pair<std::string, double>, comparator> guesses_ordered(guess_average_result.begin(), guess_average_result.end());
    count = 0;
    for (auto [word, average_words_remaining] : guesses_ordered)
    {
        std::cout << "    " << word << " :: " << average_words_remaining << ", max bucket: " << max_bucket_size(guess_all_results[word]) << "\n";
        if (count++ >= count_cutoff)
            break;
    }
}

int main()
{
    std::cout << "Hello Wordle!\n";
    WordSuggester word_suggester;
    std::cout << "Libraries loaded.\n";

    // word_suggester.black_letter( '');
    // word_suggester.green_letter( '', );
    // word_suggester.yellow_letter('', );


    word_suggester.suggest();

    int c = 0;
    c++;
}
