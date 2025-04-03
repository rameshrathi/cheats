# Deep Dive into the Linux Netfilter Framework

Netfilter is the core packet processing framework *inside* the Linux kernel. It provides the infrastructure for packet filtering, Network Address Translation (NAT), packet mangling, connection tracking, and packet queuing to userspace. It's the foundation upon which tools like `iptables`, `nftables`, and libraries like `libnetfilter_queue` are built.

## Core Architecture

Netfilter operates by placing **hooks** at strategic points within the kernel's network stack. As network packets traverse the stack, they trigger these hooks. Kernel modules or userspace programs (via specific mechanisms) can register functions to run at these hook points, allowing them to inspect, modify, drop, or accept packets.

### 1. Hooks

Hooks are well-defined points in the packet's journey through the kernel. For IPv4 and IPv6, the primary hooks are:

* **`NF_INET_PRE_ROUTING`**: Triggered soon after a packet arrives on a network interface, before any routing decisions have been made. Used for DNAT (Destination NAT), redirection, and early filtering/mangling. Associated with `raw`, `mangle`, and `nat` (specifically PREROUTING chain) tables.
* **`NF_INET_LOCAL_IN`**: Triggered after a routing decision determines the packet is destined for the local machine itself. Used for filtering incoming packets (`filter` table INPUT chain). Associated with `mangle` and `filter` tables.
* **`NF_INET_FORWARD`**: Triggered after a routing decision determines the packet should be forwarded to another interface (i.e., the Linux machine is acting as a router). Used for filtering forwarded packets (`filter` table FORWARD chain). Associated with `mangle` and `filter` tables.
* **`NF_INET_LOCAL_OUT`**: Triggered for any packet generated locally by a process on the machine, right after it enters the network stack and before routing. Used for filtering outgoing packets (`filter` table OUTPUT chain), SNAT (Source NAT), and mangling. Associated with `raw`, `mangle`, `nat` (OUTPUT chain), and `filter` tables.
* **`NF_INET_POST_ROUTING`**: Triggered just before a packet (either forwarded or locally generated) leaves the machine via a network interface, after routing decisions. Used for SNAT and final mangling. Associated with `mangle` and `nat` (POSTROUTING chain) tables.

Other protocol families (like ARP, Bridging) have their own sets of hooks (e.g., `NF_ARP_IN`, `NF_BR_PRE_ROUTING`).

**Packet Flow (Simplified IPv4):**
 -->[FORWARD]------>[POST_ROUTING]--> Network Out
                                   /                  ^ (mangle, filter)
Network In -->[PRE_ROUTING]--> Routing --               |
(raw,mangle,nat)  Decision   \                  | (mangle, nat)
-->[LOCAL_IN]-----> Local Process
(mangle, filter)
^
                                                 |
Local Process -->[LOCAL_OUT]------> Routing ----------|
(raw,mangle,nat,filter)  Decision

### 2. Tables

Tables group rules based on their purpose. The kernel processes tables in a specific order for each hook. Standard tables include:

* **`raw`**: Processed first. Primarily used with the `NOTRACK` target to disable connection tracking for specific packets (performance optimization). Hooks into `PREROUTING` and `OUTPUT`.
* **`mangle`**: Used for modifying packet headers (e.g., TTL, ToS/DSCP, setting packet marks). Hooks into all 5 main hooks.
* **`nat`**: Used for Network Address Translation. Has specific chains:
    * `PREROUTING` (Hook `NF_INET_PRE_ROUTING`): For Destination NAT (DNAT) - changing the destination address of incoming packets.
    * `OUTPUT` (Hook `NF_INET_LOCAL_OUT`): For DNAT of locally generated packets.
    * `POSTROUTING` (Hook `NF_INET_POST_ROUTING`): For Source NAT (SNAT) / Masquerading - changing the source address of outgoing packets.
    * `INPUT` (Hook `NF_INET_LOCAL_IN`): Less common, can be used for DNAT adjustments for local packets *after* the initial OUTPUT DNAT.
* **`filter`**: The most common table, used for standard packet filtering (allowing or blocking packets). Hooks into `LOCAL_IN` (INPUT chain), `FORWARD` (FORWARD chain), and `LOCAL_OUT` (OUTPUT chain). This is where typical firewall rules reside.
* **`security`**: Used by Mandatory Access Control systems like SELinux to apply security labels to packets. Hooks into `LOCAL_IN`, `FORWARD`, and `LOCAL_OUT`. Processed after the `filter` table.

`nftables` introduces a more flexible concept where tables are just containers, and chains explicitly define their hook, type, and priority.

### 3. Chains

Chains are ordered lists of rules within a table.
* **Built-in Chains**: Directly associated with Netfilter hooks (e.g., `INPUT`, `OUTPUT`, `FORWARD`, `PREROUTING`, `POSTROUTING` within their respective tables). The kernel automatically sends packets from the corresponding hook to these chains.
* **User-defined Chains**: Custom chains created by the administrator. They act like subroutines; a rule in a built-in or another user-defined chain can jump (`-j CHAIN_NAME`) to a user-defined chain for processing. If the packet reaches the end of a user-defined chain without a final verdict, it returns to the calling chain just after the jump rule.

### 4. Rules

A rule consists of:
* **Matchers**: Criteria used to select packets (e.g., source/destination IP address/mask, source/destination port, protocol, interface, connection state, packet marks, payload content). `iptables` and `nftables` provide numerous built-in matchers and support loadable extension modules (`-m module_name`) for more complex matching (e.g., `-m state`, `-m conntrack`, `-m tcp`, `-m comment`, `-m iprange`).
* **Target/Verdict**: An action to take if the packet matches all criteria.
    * **Terminating Targets (Verdicts):**
        * `ACCEPT`: Let the packet continue its traversal through the network stack.
        * `DROP`: Silently discard the packet. No response is sent.
        * `REJECT` (Target Extension): Discard the packet but send an error response (e.g., ICMP port unreachable).
        * `QUEUE`: Pass the packet to a userspace application via the `NFQUEUE` mechanism.
        * `RETURN`: Stop processing rules in the current chain and return to the rule following the jump in the calling chain. If called from a built-in chain, the chain's default policy is applied.
    * **Non-Terminating Targets:**
        * `LOG`: Log information about the packet (usually via syslog). Processing continues to the next rule.
        * `MARK`: Set an internal Netfilter mark on the packet (used for later rules or routing decisions).
        * `CONNMARK`: Set a mark on the *connection* tracking entry associated with the packet.
        * `MASQUERADE`, `SNAT`, `DNAT`, `REDIRECT` (NAT Targets): Perform address/port translation. Typically terminating within the `nat` table context.
        * `TRACE`: Mark packets for detailed debugging across Netfilter chains/tables.
        * `CT`: Interact with the connection tracking system (e.g., assign helper, set timeout).
    * **Jump Target:** `-j USER_CHAIN`: Jump to a user-defined chain.

### 5. Connection Tracking (`nf_conntrack`)

This is a crucial subsystem for stateful firewalls and NAT.
* **Function:** It monitors packets belonging to network connections and keeps track of their state.
* **Mechanism:** When the first packet of a potential connection arrives (e.g., TCP SYN), `nf_conntrack` creates an entry. This entry stores information like source/destination IPs and ports, protocol, timeouts, and the connection state. Subsequent packets are matched against existing entries.
* **States:**
    * `NEW`: First packet seen for a connection (e.g., SYN).
    * `ESTABLISHED`: A connection that has seen traffic in both directions (e.g., after SYN-ACK).
    * `RELATED`: A secondary connection related to an existing one (e.g., FTP data connection, ICMP error related to a TCP connection). Requires helper modules (`nf_conntrack_*`).
    * `INVALID`: Packet doesn't match any known connection state or is malformed (e.g., TCP ACK with no matching connection). Often dropped.
    * `UNTRACKED`: Connection tracking has been explicitly disabled for this packet (e.g., via `raw` table `NOTRACK` target).
    * Other states like `SYN_SENT`, `SYN_RECV`, `FIN_WAIT`, `CLOSE_WAIT`, etc., exist internally.
* **Usage:** The `conntrack` match (`-m conntrack --ctstate STATE` in `iptables`, or `ct state { new, established, ... }` in `nftables`) allows rules to filter based on these states (e.g., allow `ESTABLISHED,RELATED` traffic back in, while only allowing specific `NEW` connections). NAT heavily relies on conntrack to correctly translate subsequent packets in a flow.

## Netfilter APIs & Interaction

Interaction with Netfilter occurs at both the kernel and userspace levels.

### Kernel Level API (for Kernel Module Developers)

This is the lowest-level, most direct way to interact with Netfilter.

1.  **`struct nf_hook_ops`**: The core structure used to register a hook function.
    ```c
    #include <linux/netfilter.h>

    struct nf_hook_ops {
        struct list_head    list;      // Internal list linkage
        nf_hookfn           *hook;     // Pointer to YOUR hook function
        struct net_device   *dev;      // Specific device (or NULL for all)
        void                *priv;     // Private data passed to hook function
        u_int8_t            pf;        // Protocol Family (e.g., PF_INET, PF_INET6)
        unsigned int        hooknum;   // Hook number (e.g., NF_INET_PRE_ROUTING)
        int                 priority;  // Execution order (lower numbers run first)
    };

    // Your hook function signature:
    typedef unsigned int nf_hookfn(void *priv,
                                   struct sk_buff *skb, // The packet buffer!
                                   const struct nf_hook_state *state);
    ```
    * `skb`: The `struct sk_buff *` (socket buffer) contains the packet data and associated metadata. You manipulate this structure to inspect/modify the packet.
    * `state`: Provides context like the hook number, input/output device, protocol family.
    * **Return Value:** The hook function *must* return a Netfilter verdict:
        * `NF_ACCEPT`: Allow the packet.
        * `NF_DROP`: Drop the packet.
        * `NF_STOLEN`: Packet consumed by the hook function (won't proceed further).
        * `NF_QUEUE`: Queue the packet to userspace (if a queue handler is configured).
        * `NF_REPEAT`: Call this hook function again (use with caution).

2.  **Registration/Unregistration**:
    ```c
    #include <linux/netfilter_ipv4.h> // or _ipv6, _arp, etc.

    // Register hook(s)
    int nf_register_net_hook(struct net *net, const struct nf_hook_ops *ops);
    int nf_register_net_hooks(struct net *net, const struct nf_hook_ops *ops, unsigned int n); // Register multiple

    // Unregister hook(s)
    void nf_unregister_net_hook(struct net *net, const struct nf_hook_ops *ops);
    void nf_unregister_net_hooks(struct net *net, const struct nf_hook_ops *ops, unsigned int n);
    ```
    * `net`: Pointer to the network namespace (often `&init_net` for the default namespace).
    * `ops`: Pointer to your `nf_hook_ops` structure(s).

### Userspace Level APIs & Tools

Userspace applications interact with Netfilter indirectly via specific kernel subsystems and libraries.

1.  **`iptables`/`ip6tables`/`arptables`/`ebtables` (Legacy Tools)**:
    * **API:** Command-line interface.
    * **Working:** These tools communicate with the kernel's `xtables` modules using `setsockopt` calls on special sockets. They translate user-friendly rules into a binary representation (ruleset blob) that the kernel loads and uses to configure the Netfilter hooks. Each tool manages specific tables/chains relevant to its protocol.

2.  **`nftables` (Modern Tool & Framework)**:
    * **API:** Command-line interface (`nft`). Also provides `libnftables` for programmatic interaction (though direct use is less common than using the `nft` tool).
    * **Working:** `nftables` uses the newer `nf_tables` kernel subsystem via Netlink socket communication (`NFNL_SUBSYS_NFTABLES`). It offers a more unified syntax, atomic ruleset updates, better performance, and integrates IPv4, IPv6, ARP, bridging, etc., under one framework. It replaces the legacy `xtables` infrastructure.

3.  **`libnetfilter_queue` (Userspace Packet Queuing)**:
    * **API:** C library (`libnfnetlink` is a dependency).
    * **Working:** Used in conjunction with the `QUEUE` or `NFQUEUE` target in `iptables`/`nftables`.
        * `nfq_open()`: Opens a connection to the kernel queue subsystem.
        * `nfq_bind_pf()`: Binds the handle to a protocol family.
        * `nfq_create_queue()`: Binds to a specific queue number and registers a callback function.
        * `nfq_set_mode()`: Specifies how much packet data to copy (metadata only or full packet).
        * *Callback Function*: Receives queued packets (`struct nfq_data *`).
        * `nfq_get_payload()`: Extracts packet data from `nfq_data`.
        * `nfq_set_verdict()`: Sends the verdict (`NF_ACCEPT`, `NF_DROP`, etc.) and potentially modified packet data back to the kernel for the specific packet ID.
        * `nfq_close()`: Closes the connection.
    * Communication happens over Netlink sockets (`NFNL_SUBSYS_QUEUE`).

4.  **`libnetfilter_conntrack` (Connection Tracking Interaction)**:
    * **API:** C library (`libnfnetlink` is a dependency).
    * **Working:** Allows userspace programs to query, dump, create, update, and delete connection tracking entries maintained by `nf_conntrack`.
        * `nfct_open()`: Opens a connection (specifying subsystems like connection tracking or expectations).
        * `nfct_query()`: Sends commands to the kernel (e.g., dump entries, flush entries). Uses callbacks to deliver results.
        * `nfct_create()`, `nfct_update()`, `nfct_destroy()`: Functions to manipulate specific conntrack entries.
    * Communication happens over Netlink sockets (`NFNL_SUBSYS_CTNETLINK`).

5.  **`libnfnetlink` (Low-level Netlink Library)**:
    * **API:** C library.
    * **Working:** Provides the fundamental functions for opening Netlink sockets, sending/receiving Netlink messages, and handling attributes. It's the foundation upon which `libnetfilter_queue`, `libnetfilter_conntrack`, and `nftables` (internally) are built. Direct use is complex and usually unnecessary unless building a custom Netlink-based tool.

## Conclusion

Netfilter is a complex but extremely powerful and flexible framework at the heart of Linux networking. Its hook-based architecture allows fine-grained control over packets at various stages of processing. While kernel module development offers the ultimate performance and control, userspace tools (`iptables`, `nftables`) and libraries (`libnetfilter_queue`, `libnetfilter_conntrack`) provide accessible and robust ways to leverage Netfilter's capabilities for firewalling, NAT, monitoring, and custom packet handling applications. Understanding the core concepts of hooks, tables, chains, rules, targets, and connection tracking is essential for effectively utilizing and troubleshooting Netfilter-based systems.



## Developing Network Packet Filtering in Linux with C

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
