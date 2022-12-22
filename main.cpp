#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <thread>

void send_200(int socket) {
    auto response = std::string("HTTP/1.0 200 OK\r\n\r\n");
    send(socket, response.c_str(), response.size(), 0);
}

void send_404(int socket) {
    auto response = std::string("HTTP/1.0 404 NOT FOUND\r\n"
                                "Content-Length: 0\r\n"
                                "Content-Type: text/html\r\n\r\n");
    send(socket, response.c_str(), response.size(), 0);
}

bool get(int socket, std::string request) {
    auto elem_pos = request.find('?');
    if (elem_pos != std::string::npos)
        request = request.substr(0, elem_pos);

    elem_pos = request.find("HTTP");
    if (elem_pos != std::string::npos)
        request = request.substr(0, elem_pos);

    elem_pos = request.find('/');
    auto full_path = request.substr(elem_pos, request.size() - elem_pos).insert(0, ".");
    full_path.erase(std::remove(full_path.begin(), full_path.end(), ' '), full_path.end());

    int fd = open(full_path.c_str(), O_RDONLY);
    if (fd == -1) {
        send_404(socket);
        return false;
    }

    send_200(socket);
    char r_buf[1024];
    while (auto r = read(fd, r_buf, 1024))
        send(socket, r_buf, r, 0);

    close(fd);
    return true;
}

bool worker(int socket) {
    char buf[2048];
    auto r = read(socket, buf, 2048);
    if (r <= 0) {
        shutdown(socket, SHUT_RDWR);
        close(socket);
        return false;
    }
    std::string request(buf, r);
    if (request.size() > 3 && request.substr(0, 3) == "GET")
        get(socket, request);
    else
        send_404(socket);

    shutdown(socket, SHUT_RDWR);
    close(socket);
    return true;
}

int main(int argc, char **argv) {
    int opt, port;
    const char *path;
    auto host_ip = "127.0.0.1";
    while ((opt = getopt(argc, argv, "h:p:d:?")) != -1)
        switch (opt) {
            case 'h':
                host_ip = optarg;
                break;
            case 'p':
                port = std::stoi(optarg);
                break;
            case 'd':
                path = optarg;
                break;
            case '?':
                exit(EXIT_FAILURE);
            default:
                break;
        }

    struct sockaddr_in in{
            AF_INET,
            htons(port)
    };
    inet_aton(host_ip, &in.sin_addr);

    auto master = socket(AF_INET, SOCK_STREAM, 0);
    bind(master, (sockaddr *) &in, sizeof(in));

    if (!fork()) {
        setsid();
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        chdir(path);
        if (listen(master, SOMAXCONN))
            return 1;

        while (true) {
            auto slave = accept(master, nullptr, nullptr);
            std::thread(worker, slave).detach();
        }
    }
    return 0;
}