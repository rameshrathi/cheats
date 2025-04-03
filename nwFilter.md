# Developing Network Packet Filtering in Linux with C

This guide provides a detailed explanation of how to develop network packet filtering mechanisms in Linux using the C language, primarily focusing on the `libnetfilter_queue` library.

## How Packet Filtering Works in Linux (Netfilter)

At the core of Linux packet filtering is the **Netfilter framework**. It's a set of hooks inside the Linux kernel's networking stack. These hooks allow kernel modules or userspace programs to register callback functions that are invoked as packets traverse the stack at specific points.

**Key Concepts:**

1.  **Hooks:** Predefined points in the network stack where filtering can occur (e.g., `NF_IP_PRE_ROUTING`, `NF_IP_LOCAL_IN`, `NF_IP_FORWARD`, `NF_IP_LOCAL_OUT`, `NF_IP_POST_ROUTING`).
2.  **Tables:** Contain chains (e.g., `filter`, `nat`, `mangle`, `raw`). `iptables` and `nftables` are userspace tools to manage these tables.
3.  **Chains:** Ordered lists of rules within a table (e.g., `INPUT`, `OUTPUT`, `FORWARD`).
4.  **Rules:** Criteria that a packet must match (e.g., source/destination IP, port, protocol).
5.  **Targets/Verdicts:** Actions to take if a packet matches a rule (e.g., `ACCEPT`, `DROP`, `REJECT`, `LOG`, `QUEUE`).

**How `libnetfilter_queue` Fits In:**

Instead of writing complex kernel modules, `libnetfilter_queue` provides a userspace API to interact with Netfilter. You use `iptables` or `nftables` to add a rule with the `NFQUEUE` target. When a packet matches this rule, instead of being immediately ACCEPTed or DROPPed by the kernel, it's enqueued for a userspace program (your C application) to handle.

Your C program, using `libnetfilter_queue`:
* Connects to a specific queue number.
* Receives queued packets from the kernel.
* Inspects the packet data (headers, payload).
* Makes a filtering decision.
* Issues a **verdict** back to the kernel (e.g., `NF_ACCEPT`, `NF_DROP`, `NF_STOLEN`).
* Optionally, it can modify the packet before accepting it (though this is more complex).

## Method: Using `libnetfilter_queue`

This is the most common and recommended method for userspace packet filtering in C.

**Prerequisites:**

1.  **Kernel Support:** Your Linux kernel needs Netfilter and `NFQUEUE` support enabled (usually standard).
2.  **Libraries:** You need `libnfnetlink` and `libnetfilter_queue` development libraries installed.
    * On Debian/Ubuntu: `sudo apt-get update && sudo apt-get install build-essential libnetfilter-queue-dev libnfnetlink-dev`
    * On Fedora/CentOS/RHEL: `sudo dnf update && sudo dnf groupinstall "Development Tools" && sudo dnf install libnetfilter_queue-devel libnfnetlink-devel`
3.  **Privileges:** Running the application typically requires root privileges (`sudo`) to access Netfilter queues.
4.  **`iptables` or `nftables`:** Needed to direct packets to your queue.

**Core Steps:**

1.  **Include Headers:**
    ```c
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <linux/types.h>
    #include <linux/netfilter.h> // Defines verdicts (NF_ACCEPT, NF_DROP etc)
    #include <libnetfilter_queue/libnetfilter_queue.h>
    #include <netinet/ip.h>  // For IP header structure
    #include <netinet/tcp.h> // For TCP header structure
    ```

2.  **Open NFQueue Handle:** Get a handle for communicating with the kernel Netfilter queue subsystem.
    ```c
    struct nfq_handle *h;
    h = nfq_open();
    if (!h) {
        fprintf(stderr, "Error during nfq_open()\n");
        exit(EXIT_FAILURE);
    }
    ```

3.  **Unbind Existing Protocol Family Handlers (Optional but Recommended):** Prevent kernel processing of packets for the specified protocol family (e.g., IPv4) before they reach the queue handler.
    ```c
    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "Error during nfq_unbind_pf()\n");
        // Don't necessarily exit, might still work but less efficient
    }
    ```

4.  **Bind to Protocol Family:** Tell Netfilter you'll be handling packets for this protocol family (e.g., IPv4).
    ```c
    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "Error during nfq_bind_pf()\n");
        nfq_close(h);
        exit(EXIT_FAILURE);
    }
    ```

5.  **Create Queue:** Bind your handle (`h`) to a specific queue number (e.g., 0) and register your callback function (`packet_handler`).
    ```c
    struct nfq_q_handle *qh;
    int queue_num = 0; // The queue number to bind to

    // packet_handler is the function that will process each packet
    qh = nfq_create_queue(h, queue_num, &packet_handler, NULL);
    if (!qh) {
        fprintf(stderr, "Error during nfq_create_queue()\n");
        nfq_close(h);
        exit(EXIT_FAILURE);
    }
    ```

6.  **Set Queue Mode:** Specify how much data of each packet should be copied to userspace.
    * `NFQNL_COPY_PACKET`: Copy the entire packet. Needed for modification or deep inspection.
    * `NFQNL_COPY_META`: Copy only metadata (faster if you only need basic info).
    ```c
    // Copy the entire packet
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "Can't set packet_copy mode\n");
        nfq_destroy_queue(qh);
        nfq_close(h);
        exit(EXIT_FAILURE);
    }
    ```

7.  **Get File Descriptor:** Get the file descriptor associated with the Netfilter queue connection for use in the event loop (e.g., with `select` or `poll`).
    ```c
    int fd;
    fd = nfq_fd(h);
    ```

8.  **Main Loop:** Read packets from the file descriptor and pass them to `libnetfilter_queue` for handling (which will invoke your callback).
    ```c
    char buf[4096] __attribute__ ((aligned));
    int rv;

    while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
        // Call the library function to handle the packet
        // This will internally call your registered packet_handler callback
        nfq_handle_packet(h, buf, rv);
    }

    if (rv < 0) {
        perror("recv failed");
    }
    ```

9.  **Implement the Callback Function (`packet_handler`):** This is where the core filtering logic resides.
    ```c
    /*
     * Callback function signature:
     * int packet_handler(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
     * struct nfq_data *nfa, void *data);
     */
    static int packet_handler(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                              struct nfq_data *nfa, void *data)
    {
        u_int32_t id = 0;
        struct nfqnl_msg_packet_hdr *ph;
        unsigned char *packet_data;
        int payload_len;

        ph = nfq_get_msg_packet_hdr(nfa);
        if (ph) {
            id = ntohl(ph->packet_id);
            // printf("Received packet with ID: %u\n", id);
        } else {
             fprintf(stderr, "Error getting packet header\n");
             // Decide on a default verdict if header is missing
             return nfq_set_verdict(qh, 0, NF_ACCEPT, 0, NULL); // Example: Accept if error
        }

        payload_len = nfq_get_payload(nfa, &packet_data);
        if (payload_len >= 0) {
            // ---- Your Packet Inspection Logic Here ----
            // Example: Parse IP header to get source/destination IP
            struct iphdr *iph = (struct iphdr *)packet_data;
            if (iph->protocol == IPPROTO_TCP) {
                 printf("Received TCP packet, len=%d\n", payload_len);
                 // Add more parsing for TCP header, ports, etc. if needed
                 // struct tcphdr *tcph = (struct tcphdr *)(packet_data + iph->ihl * 4);
                 // ...
            } else if (iph->protocol == IPPROTO_UDP) {
                 printf("Received UDP packet, len=%d\n", payload_len);
                 // ...
            } else if (iph->protocol == IPPROTO_ICMP) {
                 printf("Received ICMP packet, len=%d\n", payload_len);
                 // ...
            }

            // ---- Your Filtering Decision Logic Here ----
            // Example: Drop all packets from a specific source IP
            // struct in_addr src_ip;
            // inet_pton(AF_INET, "192.168.1.100", &src_ip); // Example IP to drop
            // if (iph->saddr == src_ip.s_addr) {
            //     printf("Dropping packet from 192.168.1.100\n");
            //     return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
            // }

        } else {
            fprintf(stderr, "Error getting payload data\n");
            // Decide on a default verdict if payload is missing
             return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL); // Example: Accept if error
        }

        // Default action: Accept the packet
        // printf("Accepting packet ID %u\n", id);
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
    }
    ```
    * **`nfq_get_msg_packet_hdr`:** Retrieves metadata, including the packet ID.
    * **`nfq_get_payload`:** Gets the raw packet data.
    * **Packet Parsing:** You need to cast the `packet_data` pointer to appropriate header structures (`struct iphdr`, `struct tcphdr`, `struct udphdr`) and interpret them according to network protocols. *Be careful with pointer arithmetic and header lengths!*
    * **`nfq_set_verdict`:** Sends the decision back to the kernel.
        * `qh`: The queue handle.
        * `id`: The unique ID of the packet being decided upon.
        * `NF_ACCEPT`: Let the packet continue.
        * `NF_DROP`: Silently drop the packet.
        * `NF_STOLEN`: Packet is handled entirely by userspace (less common).
        * The last two arguments (`data_len`, `buf`) are used if you modified the packet and want to reinject the modified version (set `data_len` to new length and `buf` to the modified data). Set to `0` and `NULL` if not modifying.

10. **Cleanup:** Close the queue and the main handle when done.
    ```c
    printf("Unbinding from queue %d...\n", queue_num);
    nfq_destroy_queue(qh);

    #ifdef INSANE // Optional: More thorough cleanup
    printf("Unbinding from AF_INET\n");
    nfq_unbind_pf(h, AF_INET);
    #endif

    printf("Closing library handle.\n");
    nfq_close(h);

    exit(EXIT_SUCCESS);
    ```

**Compilation:**

```bash
gcc your_filter.c -o your_filter -lnetfilter_queue -lnfnetlink
