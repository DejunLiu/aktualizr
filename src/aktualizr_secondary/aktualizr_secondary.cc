#include "aktualizr_secondary.h"

#include <future>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "logging.h"
#include "socket_activation.h"
#include "utils.h"

AktualizrSecondary::AktualizrSecondary(const AktualizrSecondaryConfig &config) : config_(config) { open_socket(); }

void AktualizrSecondary::open_socket() {
  if (socket_activation::listen_fds(0) == 1) {
    LOG_INFO << "Using socket activation";
    socket_hdl_ = SocketHandle(new int(socket_activation::listen_fds_start));
    return;
  }

  // manual socket creation
  int socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    throw std::runtime_error("socket creation failed");
  }
  SocketHandle hdl(new int(socket_fd));
  sockaddr_in6 sa;

  memset(&sa, 0, sizeof(sa));
  sa.sin6_family = AF_INET6;
  sa.sin6_port = htons(config_.network.port);
  sa.sin6_addr = IN6ADDR_ANY_INIT;

  int v6only = 0;
  if (setsockopt(*hdl, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) < 0) {
    throw std::runtime_error("setsockopt(IPV6_V6ONLY) failed");
  }

  int reuseaddr = 1;
  if (setsockopt(*hdl, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0) {
    throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
  }

  if (bind(*hdl, reinterpret_cast<const sockaddr *>(&sa), sizeof(sa)) < 0) {
    throw std::runtime_error("bind failed");
  }

  if (listen(*hdl, SOMAXCONN) < 0) {
    throw std::runtime_error("listen failed");
  }

  socket_hdl_ = std::move(hdl);

  LOG_INFO << "Listening on port " << listening_port();
}

void AktualizrSecondary::run() {
  // listen for TCP connections
  std::thread tcp_thread = std::thread([this]() {
    while (true) {
      std::unique_ptr<sockaddr_storage> peer_sa(new sockaddr_storage);
      socklen_t other_sasize = sizeof(*peer_sa);
      int con_fd;

      if ((con_fd = accept(*socket_hdl_, reinterpret_cast<sockaddr *>(peer_sa.get()), &other_sasize)) == -1) {
        break;
      }

      // One thread per connection
      std::thread(&AktualizrSecondary::handle_connection_msgs, this, SocketHandle(new int(con_fd)), std::move(peer_sa))
          .detach();
    }

    channel_.close();
  });

  // listen for messages
  std::shared_ptr<SecondaryPacket> pkt;
  while (channel_ >> pkt) {
    std::cout << "Got packet " << pkt->str() << std::endl;

    if (pkt->msg->variant == "Stop") {
      // Will cause `accept()` to fail and break the loop
      shutdown(*socket_hdl_, SHUT_RDWR);
    }
  }

  tcp_thread.join();
}

/*void AktualizrSecondary::stop() {
  std::shared_ptr<SecondaryPacket> pkt = std::make_shared<SecondaryPacket>(new StopMessage{});

  channel_ << pkt;
}*/

int AktualizrSecondary::listening_port() const {
  sockaddr_storage ss;
  socklen_t len = sizeof(ss);
  in_port_t p;
  if (getsockname(*socket_hdl_, reinterpret_cast<sockaddr *>(&ss), &len) < 0) {
    return -1;
  }

  if (ss.ss_family == AF_INET) {
    sockaddr_in *sa = reinterpret_cast<sockaddr_in *>(&ss);
    p = sa->sin_port;
  } else if (ss.ss_family == AF_INET6) {
    sockaddr_in6 *sa = reinterpret_cast<sockaddr_in6 *>(&ss);
    p = sa->sin6_port;
  } else {
    return -1;
  }

  return ntohs(p);
}

void AktualizrSecondary::handle_connection_msgs(SocketHandle con, std::unique_ptr<sockaddr_storage> addr) {
  std::string peer_name = Utils::ipDisplayName(*addr);
  LOG_INFO << "Opening connection with " << peer_name;
  std::string message_content;
  while (true) {
    uint8_t c;
    if (recv(*con, &c, 1, 0) != 1) {
      break;
    }
    message_content.push_back(c);
    asn1::Deserializer asn1_stream(message_content);
    std::unique_ptr<SecondaryMessage> mes;
    try {
      asn1_stream >> mes;
    } catch (deserialization_error) {
      // TODO: process error
    }
    // TODO: parse packets
    std::unique_ptr<SecondaryPacket> pkt{new SecondaryPacket{*addr, std::move(mes)}};

    channel_ << std::move(pkt);
  }
  LOG_INFO << "Connection closed with " << peer_name;
}
