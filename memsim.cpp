#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdlib.h>

using namespace std;

// Define algorithm types
enum Algorithm {FIFO, LRU, MFU, Optimal, NullAlgorithm};

// Declare a data type 'Page' to represent a page number and the time it arrived in memory
struct Page {
    int page_number;
    int mem_arrival_time;
    int last_use_time;
    int num_references;

    Page() {
        page_number = 0;
        mem_arrival_time = 0;
        last_use_time = -1;
        num_references = 0;
    }

    Page(int page_num, int arrival) {
        page_number = page_num;
        mem_arrival_time = arrival;
        last_use_time = -1;
        num_references = 0;
    }

    void operator= (Page p) {
        page_number = p.page_number;
        mem_arrival_time = p.mem_arrival_time;
        last_use_time = p.last_use_time;
        num_references = p.num_references;
    }
};

// Declare a 'ResultPackage' data type to store all of the results of an algorithm run
struct ResultPackage {
    Algorithm run_type;
    int total_page_faults;
    vector<float> rates;

    ResultPackage(Algorithm a) {
        run_type = a;
    }

    void set_rates(int references, int num_faults) {
        rates.push_back((float) num_faults / (float) references);
    }
};

// Declare a vector-type 'pagelist' that is used to store pages
typedef vector<Page> pagelist;

// Forward declarations
void run_algorithms(pagelist, ResultPackage*);
pagelist get_pages();
bool contains(pagelist, Page);
void update_reference_ptr(Page, pagelist*, Algorithm);
Page get_replacement(Algorithm, pagelist, pagelist);
void replace(pagelist*, Page, Page);
string lookup_algorithm(Algorithm);
void output_stats(ResultPackage *);

// Global variables
unsigned int FRAME_SIZE;
char* INPUT_FILE;
char* OUTPUT_FILE;
int INTERVAL_RATE = 2000;
int produce_csv = 0;

// Functions
int main(int argc, char* argv[]) {
    FRAME_SIZE = (unsigned int) atoi(argv[1]);
    INPUT_FILE = argv[2];
    OUTPUT_FILE = argv[3];
    pagelist pages = get_pages();

    if (argc > 4) {
        produce_csv = atoi(argv[4]);
    }

    ResultPackage results[4] = { ResultPackage(FIFO), ResultPackage(LRU),
                                 ResultPackage(MFU), ResultPackage(Optimal) };
    run_algorithms(pages, results);
    output_stats(results);

    return 0;
}

void run_algorithms(pagelist all_pages, ResultPackage* results) {
    pagelist pages = all_pages;
    int page_fault_count = 0;
    unsigned int ref_num = 0;
    int interval_progress = 1;

    for (int algorithm = FIFO; algorithm != NullAlgorithm; ++algorithm) {
        pagelist frames;
        float increment = 0.0;
        cout << "Working on algorithm: " << lookup_algorithm(static_cast<Algorithm>(algorithm)) << endl;
        if (static_cast<Algorithm>(algorithm) == Optimal) {
            cout << "    - Progress on Optimal algorithm simulation: ";
        }

        for (ref_num = 0; ref_num < pages.size(); ++ref_num) {
            Algorithm current_algorithm = static_cast<Algorithm>(algorithm);
            if (current_algorithm == Optimal) {
                float percent = ((float) ref_num / (float) pages.size()) * 100;
                if (percent >= increment) {
                    cout << percent << "% " << flush;
                    increment += 1.0;
                }
            }
            Page requested_page = pages.at(ref_num);
            if (current_algorithm == LRU) {
                requested_page.last_use_time = ref_num;
            } else if (current_algorithm == MFU) {
                ++requested_page.num_references;
                update_reference_ptr(requested_page, &pages, current_algorithm);
            }
            if (!contains(frames, requested_page)) {
                // Requested page is not in memory and a page fault will occur

                ++page_fault_count;

                if (current_algorithm == FIFO) {
                    requested_page.mem_arrival_time = ref_num;
                }

                if (frames.size() < FRAME_SIZE) {
                    // There is enough memory to allocate the requested page

                    frames.push_back(requested_page);
                } else {
                    // There is not enough memory to allocate the requested page
                    // Use a page replacement strategy

                    if (current_algorithm == Optimal) {
                        pagelist pages_down_to;
                        for (unsigned int i = ref_num + 1; i < pages.size(); ++i) {
                            pages_down_to.push_back((Page &&) pages.at(i));
                        }

                        pages = pages_down_to;
                    }

                    Page replacement = get_replacement(current_algorithm, frames, pages);
                    replace(&frames, replacement, requested_page);

                    if (current_algorithm == Optimal) {
                        pages = all_pages;
                    }
                }
            } else {
                if (current_algorithm == MFU || current_algorithm == LRU) {
                    update_reference_ptr(requested_page, &frames, current_algorithm);
                }
            }

            if (ref_num + 1 == INTERVAL_RATE * interval_progress) {
                results[algorithm].set_rates(ref_num + 1, page_fault_count);
                ++interval_progress;
            }
        }

        results[algorithm].total_page_faults = page_fault_count;
        interval_progress = 1;
        page_fault_count = 0;
        pages = all_pages;
    }
}

bool contains(pagelist v, Page p) {
    // Determines if p is in v

    bool found = false;
    for (unsigned int i = 0; i < v.size(); ++i) {
        if (v.at(i).page_number == p.page_number) {
            found = true;
            break;
        }
    }
    return found;
}

void update_reference_ptr(Page p, pagelist* l, Algorithm a) {
    bool hit = false;
    for (unsigned int i = 0; i < l->size(); ++i) {
        if (l->at(i).page_number == p.page_number) {
            hit = true;
            if (a == LRU) {
                l->at(i).last_use_time = p.last_use_time;
            } else if (a == MFU) {
                l->at(i).num_references = p.num_references;
            }
        }
    }

    if (!hit) {
        cout << "Error: update of page number " << p.page_number
             << " in frame failed while running algorithm " << lookup_algorithm(a)
             << ". No such page found in frame." << endl;
    }
}

pagelist get_pages() {
    // Populates a vector with the Pages given in the input file

    pagelist pages;
    string curr_line;
    ifstream infile(INPUT_FILE);

    if (infile.good()) {
        while (getline(infile, curr_line)) {
            Page current_page(atoi(curr_line.c_str()), 0);
            pages.push_back(current_page);
        }
    } else {
        cout << "Error opening \'" << string(INPUT_FILE) << "\'";
    }

    infile.close();

    return pages;
}

Page get_replacement(Algorithm a, pagelist f, pagelist p) {
    // Returns the page to replace, according to the page replacement algorithm

    Page replacement;

    switch (a) {
        case FIFO: {
            Page oldest_page;
            oldest_page.mem_arrival_time = numeric_limits<int>::max();
            for (unsigned int i = 0; i < f.size(); ++i) {
                int current_time = f.at(i).mem_arrival_time;
                if (current_time < oldest_page.mem_arrival_time) {
                    int page_num = f.at(i).page_number;
                    Page new_oldest_page(page_num, current_time);
                    oldest_page = new_oldest_page;
                }
            }
            replacement = oldest_page;
        }
            break;
        case LRU: {
            Page least_recently_used;
            least_recently_used.last_use_time = numeric_limits<int>::max();
            for (unsigned int i = 0; i < f.size(); ++i) {
                int current_usage = f.at(i).last_use_time;
                if (current_usage < least_recently_used.last_use_time) {
                    int page_num = f.at(i).page_number;
                    Page new_least_recently_used;
                    new_least_recently_used.page_number = page_num;
                    new_least_recently_used.last_use_time = current_usage;
                    least_recently_used = new_least_recently_used;
                }
            }
            replacement = least_recently_used;
        }
            break;
        case MFU: {
            Page most_frequently_used;
            for (unsigned int i = 0; i < f.size(); ++i) {
                int current_references = f.at(i).num_references;
                if (current_references > most_frequently_used.num_references) {
                    int page_num = f.at(i).page_number;
                    Page new_most_frequently_used;
                    new_most_frequently_used.page_number = page_num;
                    new_most_frequently_used.num_references = current_references;
                    most_frequently_used = new_most_frequently_used;
                }
            }
            replacement = most_frequently_used;
        }
            break;
        case Optimal: {
            unsigned int p_candidate_idx = 0;
            unsigned int f_candidate_idx = 0;
            for (unsigned int i = 0; i < f.size(); ++i) {
                Page candidate_page = f.at(i);
                if (contains(p, candidate_page)) {
                    for (unsigned int j = 0; j < p.size(); ++j) {
                        if (p.at(j).page_number == candidate_page.page_number) {
                            if (j > p_candidate_idx) {
                                p_candidate_idx = j;
                                f_candidate_idx = i;
                            }
                            break;
                        }
                    }
                } else {
                    f_candidate_idx = i;
                    break;
                }
            }
            replacement = f.at(f_candidate_idx);
        }
            break;
        default: break; // do nothing
    }

    return replacement;
}

void replace(pagelist* p, Page replacement, Page requested_page) {
    for (unsigned int i = 0; i < p->size(); ++i) {
        if (p->at(i).page_number == replacement.page_number) {
            p->at(i) = requested_page;
            break;
        }
    }
}

void output_stats(ResultPackage *r) {
    ofstream out, csv;
    out.open(OUTPUT_FILE, ofstream::out | ofstream::app);

    if (produce_csv == 1) {
        csv.open("result.csv", ofstream::out | ofstream::app);
    }

    out << "  " << string(60, '=') << endl
        << "    Page Replacement Algorithm Simulation (frame size = "
        << FRAME_SIZE << ")\n  " << string(60, '=') << endl
        << string(40, ' ') << "Page fault rates" << endl
        << "Algorithm  Total page faults  ";

    if (produce_csv == 1) {
        csv << FRAME_SIZE << endl << "Algorithm,Total page faults,";
    }

    unsigned long max_references = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned long current_references = r[i].rates.size() * INTERVAL_RATE;
        if (current_references > max_references) {
            max_references = current_references;
        }
    }

    int num_intervals = 0;
    for (int i = INTERVAL_RATE; i <= max_references; i += INTERVAL_RATE) {
        if ((int) max_references - i >= 0) {
            ++num_intervals;
        } else {
            break;
        }
    }

    for (int i = 1; i <= num_intervals; ++i) {
        out << i * INTERVAL_RATE << "    ";

        if (produce_csv == 1) {
            csv << i * INTERVAL_RATE << ",";
        }
    }

    out << endl << string(68, '-') << endl;

    if (produce_csv == 1) {
        csv << endl;
    }

    for (int i = 0; i < 4; ++i) {
        out << setw(7) << left << lookup_algorithm(r[i].run_type)
            << setw(13) << right << r[i].total_page_faults
            << setw(15) << right;

        if (produce_csv == 1) {
            csv << lookup_algorithm(r[i].run_type) << "," << r[i].total_page_faults << ",";
        }

        for (unsigned int j = 0; j < r[i].rates.size(); ++j) {
            out << fixed << setprecision(3) << r[i].rates.at(j) << setw(8);

            if (produce_csv == 1) {
                csv << r[i].rates.at(j) << ",";
            }
        }

        out << endl << endl;

        if (produce_csv == 1) {
            csv << endl << endl;
        }
    }

    out.close();

    if (produce_csv == 1) {
        csv.close();
    }

    cout << "\n\nJob finished successfully. Output sent to " << OUTPUT_FILE << endl;
}

string lookup_algorithm(Algorithm a) {
    string algorithm = "";
    switch(a) {
        case FIFO:    algorithm = "FIFO";          break;
        case LRU:     algorithm = "LRU";           break;
        case MFU:     algorithm = "MFU";           break;
        case Optimal: algorithm = "Optimal";       break;
        default:      algorithm = "NullAlgorithm"; break;
    }
    return algorithm;
}