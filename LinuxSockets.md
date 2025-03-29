```markdown
# Linux Networking API Functions and Their Working

## 1. Socket Creation and Configuration

### `socket(int domain, int type, int protocol)`
- **Purpose**: Creates an endpoint for communication and returns a file descriptor.
- **Parameters**:
  - `domain`: Address family (`AF_INET`, `AF_INET6`, `AF_UNIX`).
  - `type`: Socket type (`SOCK_STREAM` for TCP, `SOCK_DGRAM` for UDP).
  - `protocol`: Usually 0 (default protocol for the type).
- **Working**:
  - Allocates a socket descriptor.
  - Initializes internal kernel structures for the socket.
  - Associates the socket with the protocol stack.
  - Returns a file descriptor that can be used for further operations.

### `socketpair(int domain, int type, int protocol, int sv[2])`
- **Purpose**: Creates a pair of connected sockets for inter-process communication.
- **Working**:
  - Creates two connected sockets in `sv[0]` and `sv[1]`.
  - Useful for IPC between parent and child processes.

### `setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)`
- **Purpose**: Configures options on a socket.
- **Working**:
  - Modifies socket behavior at the specified `level`.
  - Common options include `SO_REUSEADDR`, `SO_KEEPALIVE`.

### `getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)`
- **Purpose**: Retrieves options for a socket.

## 2. Binding and Listening

### `bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)`
- **Purpose**: Assigns an address and port to a socket.
- **Working**:
  - Links the socket with a specific IP and port.
  - Ensures that incoming packets are directed to the correct socket.

### `listen(int sockfd, int backlog)`
- **Purpose**: Prepares a socket for accepting connections.
- **Working**:
  - Moves the socket into a listening state.
  - `backlog` defines the number of pending connections before refusal.

## 3. Connection Handling

### `accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)`
- **Purpose**: Accepts an incoming connection on a listening socket.
- **Working**:
  - Blocks until a client attempts a connection.
  - Returns a new socket file descriptor for communication.

### `connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)`
- **Purpose**: Establishes a connection to a remote socket.
- **Working**:
  - Initiates a handshake (e.g., TCP three-way handshake).

## 4. Data Transmission

### `send(int sockfd, const void *buf, size_t len, int flags)`
- **Purpose**: Sends data over a connected socket.
- **Working**:
  - Places data in the send buffer.
  - Initiates transmission through the network stack.

### `recv(int sockfd, void *buf, size_t len, int flags)`
- **Purpose**: Receives data from a connected socket.

### `write(int fd, const void *buf, size_t count)`
- **Purpose**: Writes data to a socket.

### `read(int fd, void *buf, size_t count)`
- **Purpose**: Reads data from a socket.

## 5. Address Resolution

### `getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)`
- **Purpose**: Resolves a hostname to an IP address.
- **Working**:
  - Uses DNS or local resolution.

### `inet_ntoa(struct in_addr in)` / `inet_ntop(int af, const void *src, char *dst, socklen_t size)`
- **Purpose**: Converts an IP address to a string format.

## 6. Socket Shutdown and Closing

### `shutdown(int sockfd, int how)`
- **Purpose**: Disables sending/receiving on a socket.

### `close(int sockfd)`
- **Purpose**: Closes a socket and releases resources.

## 7. Multiplexing and Event Handling

### `select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)`
- **Purpose**: Monitors multiple file descriptors for events.
- **Working**:
  - Uses polling to detect readiness for I/O.

### `poll(struct pollfd *fds, nfds_t nfds, int timeout)`
- **Purpose**: Similar to `select`, but with an event structure.

### `epoll_create(int size)`, `epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)`, `epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)`
- **Purpose**: Efficient event-driven I/O.
- **Working**:
  - Uses a kernel event queue for fast I/O readiness detection.

## 8. Network Interface and Configuration

### `getifaddrs(struct ifaddrs **ifap)`
- **Purpose**: Retrieves a list of network interfaces.
- **Working**:
  - Fetches interface addresses and configuration details.

## 9. Raw Sockets and Packet Manipulation

### `socket(AF_PACKET, SOCK_RAW, int protocol)`
- **Purpose**: Allows direct access to network packets.
- **Working**:
  - Bypasses the normal TCP/IP stack.
  - Useful for network monitoring and packet crafting.

## 10. Netlink API (for Kernel Communication)

### `socket(AF_NETLINK, SOCK_RAW, int protocol)`
- **Purpose**: Communicates with the Linux kernel.

## 11. Advanced Sockets (UNIX domain, Zero-copy)

### `sendfile(int out_fd, int in_fd, off_t *offset, size_t count)`
- **Purpose**: Transfers data between file descriptors with zero-copy.

### `splice(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags)`
- **Purpose**: Moves data between file descriptors efficiently.
```
