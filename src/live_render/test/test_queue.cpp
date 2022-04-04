#include <chrono>
#include <iostream>
#include <thread>

#include "thirdparty/readerwriterqueue/readerwriterqueue.h"
using namespace moodycamel;
using namespace std;

bool start_1 = true;
bool start_2 = true;

void Produce(ReaderWriterQueue<int> &q) {
    using namespace std::chrono_literals;
    int i = 0;
    while (start_1) {
        //this_thread::sleep_for(1ms);
        q.enqueue(i++);
    }
}

void Consume(ReaderWriterQueue<int> &q) {
    int *i = nullptr;
    int last = -1;
    while (1) {
        i = q.peek();
        if (i) {
            q.pop();
            if (last == -1) {
                last = *i;
            } else {
                if (*i - last != 1) {
                    cout << "error" << endl;
                    std::abort();
                }
                last++;
            }
            if (*i > 1e6) {
                break;
            }
        }
    }
}

void Produce_complex(ReaderWriterQueue<pair<int, string>> &q) {
    using namespace std::chrono_literals;
    int i = 0;
    while (start_2) {
        //this_thread::sleep_for(1ms);
        string str(1000, 0);
        pair<int, string> item = make_pair(i++, move(str));
        q.enqueue(move(item));
    }
}

void Consume_complex(ReaderWriterQueue<pair<int, string>> &q) {
    pair<int, string> *i = nullptr;
    while (1) {
        i = q.peek();
        if (i) {
            q.pop();
            if (i->first > 1e6) {
                break;
            }
        }
    }
}

void test_simple() {
    ReaderWriterQueue<int> q(100);

    thread(Produce, ref(q)).detach();

    auto job_start_time = std::chrono::high_resolution_clock::now();

    Consume(q);
    start_1 = false;

    auto job_end_time = std::chrono::high_resolution_clock::now();
    double cost_time_ms =
        std::chrono::duration<double, std::milli>(job_end_time - job_start_time).count();

    cout << "simple cost: " << cost_time_ms << "ms \n";
}

void test_complex() {
    ReaderWriterQueue<pair<int, string>> q(100);

    thread(Produce_complex, ref(q)).detach();

    auto job_start_time = std::chrono::high_resolution_clock::now();

    Consume_complex(q);
    start_2 = false;

    auto job_end_time = std::chrono::high_resolution_clock::now();
    double cost_time_ms =
        std::chrono::duration<double, std::milli>(job_end_time - job_start_time).count();

    cout << "complex cost: " << cost_time_ms << "ms \n";
}

int main() {
    test_simple();
    test_complex();
    this_thread::sleep_for(5ms);
    return 0;
}