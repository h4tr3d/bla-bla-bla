/**
 *
 * Load generator
 *
 * Uses file system to generate test input and parse it.
 * Do it in parallel using all existing CPU cores (minus 1: for logger thread).
 *
 * Idea:
 *
 * Divide complete range of possible requests (10^5) into blocks with size N, where
 *    3<=N<=10^5
 * Each block consists from N-2 BOOKings requests, where booking time is:
 *    t = t + 1
 * inital value of t is 0.
 *
 * Next request (N-1) is BOOKing request too, but time is:
 *    t = t + 86400
 * So, any ROOMS or CLIENTS request should drop previous N-2 items in data base. By design, this is
 * most slow operation in system.
 *
 * Next request (N) is CLIENTS or ROOMS request, that must cause database cleaning.
 *
 * So, using this logic, we iterate for each blocks sizes from 3 to 10^5. With small block size
 * amount of dirty data is small, but amount of clean requests too big. With block size growing
 * anount of dirty data also growing, but amount of the clean requests decreases.
 *
 */

#include <fstream>
#include <iostream>
#include <random>
#include <chrono>
#include <filesystem>
#include <thread>
#include <future>
#include <vector>
#include <fmt/format.h>

#include "hotels.h"

static constexpr size_t MAX_REQ_COUNT = 100'000;
static constexpr size_t MAX_USER_ID   = 1'000'000'000;

namespace dt = std::chrono;
namespace fs = std::filesystem;

/**
 * The locked_queue struct
 *
 * simple structure to communicate between workers and logger thread
 */
struct locked_queue
{
    std::mutex m;
    std::condition_variable cv;
    std::deque<std::tuple<size_t, uint64_t>> q;

    void push(size_t block_size, uint64_t spent_time)
    {
        std::unique_lock<std::mutex> lock{m};
        q.emplace_back(block_size, spent_time);
        lock.unlock();
        cv.notify_one();
    }

    void pop(size_t &block_size, uint64_t &spent_time)
    {
        std::unique_lock<std::mutex> lock{m};
        if (q.empty()) {
            cv.wait(lock, [this]{
                return !q.empty();
            });
        }
        block_size = std::get<0>(q.front());
        spent_time = std::get<1>(q.front());
        q.pop_front();
    }
};

/**
 * process input file
 *
 * Just a copy of the main.cpp:main() proc oriented to work with file streams instead of
 * std::cin.
 *
 * @param fn   file to process
 * @return time spent for processing, uint64_t(-1) on failure
 */
static uint64_t process(const fs::path& fn)
{
    auto start = dt::steady_clock::now();

    std::ifstream ifs(fn);
    if (!ifs)
        return -1;

    hotel_processing::context ctx;

    size_t requests_count;
    ifs >> requests_count;

    for (size_t i = 0; i < requests_count; ++i) {
        std::string request_kind;
        ifs >> request_kind;

        if (request_kind == "BOOK") {
            hotel_processing::time_t time;
            hotel_processing::client_id_t client;
            hotel_processing::room_t rooms;
            std::string hotel_name;

            ifs >> time >> hotel_name >> client >> rooms;

            ctx.book(time, hotel_name, client, rooms);

        } else {
            std::string hotel_name;
            ifs >> hotel_name;

            if (request_kind == "CLIENTS") {
                ctx.clients(hotel_name);
                //std::cout << ctx.clients(hotel_name) << '\n';
            } else if (request_kind == "ROOMS") {
                ctx.rooms(hotel_name);
                //std::cout << ctx.rooms(hotel_name) << '\n';
            }
        }

    }

    return dt::duration_cast<dt::milliseconds>(dt::steady_clock::now() - start).count();
}

/**
 * generator   A data generator.
 *
 * After data set completing, call process step.
 *
 * @param queue
 * @param block_start
 * @param block_step
 * @return
 */
int generator(locked_queue *queue, size_t block_start, size_t block_step)
{
    std::random_device rnd;
    std::default_random_engine re(rnd());
    std::uniform_int_distribution<size_t> client_id(0, MAX_USER_ID);

        const char *hotels[] = {
            "aaa",
            "bbb",
            "ccc",
            "ddd",
            };
    size_t idx = 0;

    for (size_t block_size = block_start; block_size <= MAX_REQ_COUNT; block_size += block_step) {
        auto fn_formatted = fs::path(fmt::format("INPUT_BLK{:06d}", block_size));
        auto fn = fs::path(fmt::format("INPUT_BLK_TMP{}", block_start));
        auto blocks = MAX_REQ_COUNT / block_size;
        auto total = blocks * block_size;
        size_t book_time = 0;

        std::ofstream ofs{fn};
        if (!ofs)
            return 1;

        ofs << total << '\n';
        for (size_t i = 0; i < blocks; ++i) {
            for (size_t j = 0; j < block_size - 2; ++j) {
                book_time++;
                auto user_id = client_id(re);
                auto hotel = hotels[idx++ % std::size(hotels)];

                ofs << "BOOK " << book_time << " " << hotel << " " << user_id << " 10\n";
            }
            book_time += 86400;
            auto user_id = client_id(re);
            auto hotel = hotels[idx++ % std::size(hotels)];
            ofs << "BOOK " << book_time << " " << hotel << " " << user_id << " 10\n";

            if (i % 2) {
                ofs << "CLIENTS " << hotel << '\n';
            } else {
                ofs << "ROOMS " << hotel << '\n';
            }
        }
        ofs.close();

        auto spent_time = process(fn);
        queue->push(block_size, spent_time);
        if (spent_time >= 350) {
            fs::copy_file(fn, fn_formatted, fs::copy_options::overwrite_existing);
        }
    }

    return 0;
}

int main()
{
    locked_queue queue;
    std::vector<std::future<int>> workers;
    auto pool_size = std::thread::hardware_concurrency() - 1;

    workers.reserve(pool_size);

    // Synchronize logging in one place
    auto logger = std::thread([&queue]() {
        uint64_t max_spent_time = 0;
        while (true) {
            size_t block_size;
            uint64_t spent_time;

            queue.pop(block_size, spent_time);
            if (!block_size)
                break;

            if (spent_time > max_spent_time)
                max_spent_time = spent_time;

            fmt::print("{:06d}: {:6d} {}\n", block_size, max_spent_time, spent_time);
        }
    });

    for (size_t i = 0; i < pool_size; ++i) {
        workers.push_back(std::async(std::launch::async, generator, &queue, 3 + i, pool_size));
    }

    int ret = 0;
    for (auto &f : workers) {
        ret += f.get();
    }

    queue.push(0, 0);
    logger.join();

    return ret;
}
