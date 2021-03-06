/* 
 *******************************************************************************
 *
 * File:         seq_stock_tweets.cpp
 * Description:  sequential stock_tweets for verification
 * Author:       Maurizio Drocco
 * Language:     C++
 * Status:       Experimental
 * Created on:   Jan 18, 2017
 *
 *******************************************************************************
 */

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <unordered_map>
#include <chrono>

#include "defs.h"

class TweetProcessor {
public:
    TweetProcessor(std::string &stock_fname)
    {
        /* bring tags to memory */
        std::ifstream stocks_file(stock_fname);
        std::string stock_name;
        while (stocks_file.good())
        {
            stocks_file >> stock_name;
            stock_names.insert(stock_name);
        }
    }

    void operator()(std::string tweet)
    {
        std::string stock;
        unsigned len;
        if (filter(tweet, stock, len))
        {
            if (cnt_map.find(stock) == cnt_map.end())
            {
                /* first occurrence of stock */
                cnt_map[stock] = 0;
            }

            len_map[stock] =
                    (cnt_map[stock] > 0) ? reduce(len, len_map[stock]) : len;

            if (++cnt_map[stock] == WIN_SIZE)
            {
                /* reset the reduce entry for stock */
                emit(stock, len_map[stock]);
                cnt_map[stock] = 0;
            }
        }
    }

    void finalize()
    {
//        for (auto it : len_map)
//        {
//            if (cnt_map[it.first] > 0)
//                emit(it.first, it.second);
//        }
    }

private:
    const unsigned WIN_SIZE = 8;
    std::unordered_map<std::string, unsigned> cnt_map;
    std::unordered_map<std::string, unsigned> len_map;
    std::set<std::string> stock_names;

    bool filter(const std::string &tweet, std::string &stock, unsigned &len)
    {
        bool single_stock = false;
        len = 0;

        /* tokenize the tweet */
        std::istringstream f(tweet);
        std::string s;
        while (std::getline(f, s, ' '))
        {
            /* count token length + blank space */
            len += s.size() + 1;

            /* stock name occurrence */
            if (stock_names.find(s) != stock_names.end())
            {

                if (!single_stock)
                {
                    /* first stock name occurrence */
                    stock = s;
                    single_stock = true;
                }
                else if (s != stock)
                    return false;
            }
        }

        return single_stock;
    }

    unsigned reduce(unsigned x, unsigned y)
    {
        return std::max(x, y);
    }

    void emit(const std::string &stock, unsigned len)
    {
        std::cout << "<" << stock << ", " << len << ">" << std::endl;
    }
};

int sock_init(std::string server_name, int port)
{
    int option = 1;
    struct hostent *server = nullptr;
    struct sockaddr_in serv_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (sockfd < 0)
    {
        fprintf(stderr, "ERROR opening socket");
        exit(1);
    }
    server = gethostbyname(server_name.c_str());
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr,
    server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "ERROR connecting");
        exit(1);
    }

    return sockfd;
}

void sock_fini(int sockfd)
{
    close(sockfd);
}

void core_loop(int sockfd, TweetProcessor &proc)
{
    std::string line;
    std::string tail;
    char buffer[256];
    int n = 0;

    while (read(sockfd, buffer, sizeof(buffer) - 1) > 0)
    {
        if (n < 0)
        {
            fprintf(stderr, "ERROR reading from socket");
            exit(1);
        }
        else
        {
            tail.append(buffer);
            std::istringstream f(tail);
            while (std::getline(f, line, '-'))
            {
                if (!f.eof())
                {
                    /* process the tweet */
                    proc(line);
                    tail.clear();
                }
                else
                {
                    // trunked line, store for next parsing
                    tail.clear();
                    tail.append(line);
                }
            }
            bzero(buffer, sizeof(buffer));
        }
    }

//    proc.finalize();
}

int main(int argc, char** argv)
{
    /* parse command line */
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0];
        std::cerr << " <stock names file>"
                << " <tweet socket host> <tweet socket port>\n";
        return -1;
    }
    std::string stock_fname = argv[1];
    std::string server_name = argv[2];
    int server_port = atoi(argv[3]);

    /* start timer */
    auto t0 = std::chrono::high_resolution_clock::now();

    /* initialize the socket */
    int sockfd = sock_init(server_name, server_port);

    /* create the tweet processor */
    TweetProcessor proc(stock_fname);

    /* read tweets */
    core_loop(sockfd, proc);

    /* finalize the socket */
    sock_fini(sockfd);

    /* stop timer */
    auto t1 = std::chrono::high_resolution_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::seconds>(t1 - t0);

    std::cout << "done in " << d.count() << " s\n";

    return 0;
}
