#include <wordle_word_suggester.h>

#include <algorithm> // std::unique
#include <iostream>  // std::cout
#include <iomanip>   // std::setw
#include <fstream>   // std::ifstream (and ofstream)
#include <map>       // std::map
#include <sstream>   // std::stringstream

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
        _valid_answers.push_back(line);

    file_path = "../include/valid_guesses.txt";
    std::ifstream file_stream2 (file_path);
    while (std::getline(file_stream2, line))
        _valid_guesses.push_back(line);
}

size_t WordSuggester::remove_words_with_letter(char letter)
{
    for (auto itr = _valid_answers.begin(); itr != _valid_answers.end(); )
    {
        if (itr->find(letter) != std::string::npos)
            itr = _valid_answers.erase(itr);
        else
            ++itr;
    }
    return _valid_answers.size();
}

size_t WordSuggester::remove_words_with_letter_position(char letter, size_t position)
{
    for (auto itr = _valid_answers.begin(); itr != _valid_answers.end(); )
    {
        if (itr->find(letter) == position)
            itr = _valid_answers.erase(itr);
        else
            ++itr;
    }
    return _valid_answers.size();
}

void WordSuggester::black_letter(char letter)
{
    this->remove_words_with_letter(letter);
}


void WordSuggester::green_letter(char letter, size_t correct_position)
{
    this->remove_words_without_letter(letter);

    // Remove words with this letter in everything position except the correct one
    if (correct_position != 0) this->remove_words_with_letter_position(letter, 0);
    if (correct_position != 1) this->remove_words_with_letter_position(letter, 1);
    if (correct_position != 2) this->remove_words_with_letter_position(letter, 2);
    if (correct_position != 3) this->remove_words_with_letter_position(letter, 3);
    if (correct_position != 4) this->remove_words_with_letter_position(letter, 4);
}

void WordSuggester::yellow_letter(char letter, size_t wrong_position)
{
    this->remove_words_without_letter(letter);

    this->remove_words_with_letter_position(letter, wrong_position);
}

size_t WordSuggester::remove_words_without_letter(char required_letter)
{
    if (_required_letters.find(required_letter) == std::string::npos)
        _required_letters += required_letter;

    for (auto itr = _valid_answers.begin(); itr != _valid_answers.end(); )
    {
        if (itr->find(required_letter) == std::string::npos)
            itr = _valid_answers.erase(itr);
        else
            ++itr;
    }
    return _valid_answers.size();
}

void WordSuggester::print_words(int words_per_row, std::vector<std::string> words)
{
    // Default to printing the valid answers
    if (words.empty())
        words = this->_valid_answers;

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

void WordSuggester::which_word_should_i_choose()
{
    // Get all the letters in the words that weren't required, grouped all together and by word
    std::string unspecified_letters = "";
    std::vector<std::string> unspecified_letters_by_word;
    for (auto word : this->_valid_answers)
    {
        std::string reqs = this->_required_letters;  // Make a copy so we can edit
        std::string remaining_letters;
        for (auto mychar : word)
        {
            if (reqs.find(mychar) != std::string::npos)
                reqs.erase(reqs.find(mychar), 1);
            else
                remaining_letters += mychar;
        }
        unspecified_letters += remaining_letters;
        unspecified_letters_by_word.push_back(remaining_letters);
    }

    // Count the unspecified letters
    std::sort(unspecified_letters.begin(), unspecified_letters.end());
    std::map<char, size_t> letter_count;
    if (!unspecified_letters.empty())
        letter_count[unspecified_letters[0]] = 1;
    for (auto itr = unspecified_letters.begin(); itr != unspecified_letters.end() - 1; ++itr)
    {
        if (*(itr + 1) == *itr)
            ++letter_count[*itr];
        else
            letter_count[*(itr + 1)] = 1;
    }

    // Remove non-unique letters
    sort_and_remove_non_unique_elements(&unspecified_letters);

    // Print the letter counts
    std::cout << "Counts of the unspecified letters in the remaining words (alphabetical & by count)\n ";
    for (auto letter : unspecified_letters)
        std::cout << "  " << letter << " ";
    std::cout << "\n";
    for (auto letter : unspecified_letters)
        std::cout << std::fixed << std::setw(4) << letter_count[letter];
    std::cout << "\n\n";

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
    // std::map<std::string, size_t> scoring_words;
    std::map<size_t, std::string> scoring_words;
    for (size_t ii = 0; ii != unspecified_letters_by_word.size(); ++ii)
    {
        // Don't score a word for the same letter multiple times.
        auto one_words_unspecifieds = unspecified_letters_by_word[ii];  // Make a copy so we can edit
        sort_and_remove_non_unique_elements(&one_words_unspecifieds);

        size_t score = 0;
        for (auto mychar : one_words_unspecifieds)
            score += letter_count[mychar];

        // scoring_words[this->_valid_answers[ii]] = score;
        scoring_words[score] += this->_valid_answers[ii];
    }

    // Print out the scored by score
    for (auto ritr = scoring_words.rbegin(); ritr != scoring_words.rend(); ++ritr)
    {
        std::string words = ritr->second;
        while (!words.empty())
        {
            std::string word (words.begin(), words.begin() + 5);
            words.erase(words.begin(), words.begin() + 5);
            std::cout << word << " : " << ritr->first << "\n";
        }
    }
    std::cout << "\n";
    /*
    Idea of next pass on score - what word, if guessed could give us the most information about the remaining words?
    Which touches the most words, but also leads us closer to a final answer.
    It's tricky, because there are a lot of types of information. It'll be fun to work through

    So far I haven't been considering location of the letters after the initial removing, before this function. That will affect how to get info.
    Compare all guess words, see which have the min, max, and average number of remaining words across the guess and all remaining valid answers
    */
}

int main()
{
    std::cout << "Hello Wordle!\n";
    WordSuggester word_suggester;
    std::cout << "Libraries loaded.\n";

    // word_suggester.black_letter( '');
    // word_suggester.green_letter( '', );
    // word_suggester.yellow_letter('', );

    word_suggester.black_letter( 'l');
    word_suggester.black_letter( 'a');
    word_suggester.black_letter( 't');
    word_suggester.yellow_letter('e', 3);
    word_suggester.yellow_letter('r', 4);

    word_suggester.black_letter( 'p');
    word_suggester.yellow_letter('r', 1);
    word_suggester.black_letter( 'o');
    word_suggester.yellow_letter('s', 3);
    word_suggester.green_letter( 'e', 4);

    word_suggester.green_letter( 's', 0);
    word_suggester.black_letter( 'u');
    word_suggester.yellow_letter('r', 2);
    word_suggester.black_letter( 'g');
    word_suggester.green_letter( 'e', 4);

    word_suggester.which_word_should_i_choose();

    // word_suggester.print_words();

    int c = 0;
    c++;
}
