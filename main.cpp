#include <iostream>

#include "hotels.h"

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    hotel_processing::context ctx;

    size_t requests_count;
    std::cin >> requests_count;

    for (size_t i = 0; i < requests_count; ++i) {
        std::string request_kind;
        std::cin >> request_kind;

        if (request_kind == "BOOK") {
            hotel_processing::time_t time;
            hotel_processing::client_id_t client;
            hotel_processing::room_t rooms;
            std::string hotel_name;

            std::cin >> time >> hotel_name >> client >> rooms;

            ctx.book(time, hotel_name, client, rooms);

        } else {
            std::string hotel_name;
            std::cin >> hotel_name;

            if (request_kind == "CLIENTS") {
                std::cout << ctx.clients(hotel_name) << '\n';
            } else if (request_kind == "ROOMS") {
                std::cout << ctx.rooms(hotel_name) << '\n';
            }
        }

    }

    return 0;
}
