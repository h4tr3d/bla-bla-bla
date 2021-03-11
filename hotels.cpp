#include "hotels.h"

namespace hotel_processing {

void context::book(time_t time, const std::string &hotel_name, client_id_t client_id, room_t room_count)
{
    m_current_time = time;
    m_hotels[hotel_name].book({time, client_id, room_count});
}

size_t context::clients(const std::string &hotel_name)
{
    return m_hotels[hotel_name].clients(m_current_time);
}

size_t context::rooms(const std::string &hotel_name)
{
    return m_hotels[hotel_name].rooms(m_current_time);
}

namespace priv {

#ifdef CACHED
void hotel::book(booking &&info)
{
    setup(info.rooms, info.client);
    m_bookings.push_back(std::move(info));
}

size_t hotel::clients(time_t current_time)
{
    remove_old(current_time);
    return m_client_bookings.size();
}

size_t hotel::rooms(time_t current_time)
{
    remove_old(current_time);
    return m_rooms;
}

void hotel::setup(room_t rooms, client_id_t client)
{
    m_rooms += rooms;
    ++m_client_bookings[client];
}

void hotel::cleanup(room_t rooms, client_id_t client)
{
    m_rooms -= rooms;
    auto client_it = m_client_bookings.find(client);
    if (--client_it->second == 0) {
        m_client_bookings.erase(client_it);
    }
}

void hotel::remove_old(time_t current_time)
{
    if (m_bookings.empty()) {
        return;
    }

    auto it = m_bookings.begin();
    for (; it != m_bookings.end(); ++it) {
        if (it->time > (current_time - TIME_WINDOW)) {
            break;
        }
        cleanup(it->rooms, it->client);
    }

    if (it != m_bookings.begin())
        m_bookings.erase(m_bookings.begin(), it);
}
#else
void hotel::book(booking &&info)
{
    m_bookings.push_back(std::move(info));
}

size_t hotel::clients(time_t current_time)
{
    remove_old(current_time);

    auto tmp = m_bookings;
    std::sort(tmp.begin(), tmp.end(), [](auto const& a, auto const& b) {
        return a.client < b.client;
    });
    auto last = std::unique(tmp.begin(), tmp.end(), [](auto const& a, auto const& b) {
        return a.client == b.client;
    });
    return std::distance(tmp.begin(), last);
}

size_t hotel::rooms(time_t current_time)
{
    remove_old(current_time);
    return std::accumulate(m_bookings.begin(), m_bookings.end(), size_t(0), [](auto prev, auto const& info) {
        return prev + info.rooms;
    });
}

void hotel::remove_old(time_t current_time)
{
    if (m_bookings.empty()) {
        return;
    }

    auto it = std::upper_bound(m_bookings.begin(), m_bookings.end(), current_time - TIME_WINDOW,
                               [](auto tm, auto const& info) {
                                   return tm < info.time;
                               });
    m_bookings.erase(m_bookings.begin(), it);
}
#endif

} // ::priv
} // ::hotel_processing
