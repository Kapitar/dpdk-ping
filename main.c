#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mbuf_core.h>
#include <rte_ether.h>
#include <stdlib.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define MBUF_NUM 8192
#define MBUF_CACHE_SIZE 256
#define BURST_SIZE 32

static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t rxd_num = RX_RING_SIZE; // number of RX descriptors
	uint16_t txd_num = TX_RING_SIZE; // number of TX descriptors

	struct rte_eth_conf port_conf;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_rxconf rxconf;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	memset(&port_conf, 0, sizeof(struct rte_eth_conf));

	int ret = rte_eth_dev_info_get(port, &dev_info);
	if (ret != 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-ret));
		return ret;
	}

	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	ret = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (ret != 0) {
    	printf("Error configuring ethernet device (port %u) info: %s\n",
    			port, strerror(-ret));
	    return ret;
	}

	ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &rxd_num, &txd_num);
	if (ret != 0) {
    	printf("Error adjusting RX/TX descriptor sizes (port %u) info: %s\n",
     			port, strerror(-ret));
	    return ret;
	}

    rxconf = dev_info.default_rxconf;
    rxconf.offloads = port_conf.rxmode.offloads;
	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (uint16_t queue_id = 0; queue_id < rx_rings; queue_id++) {
		ret = rte_eth_rx_queue_setup(port, queue_id, rxd_num,
				rte_eth_dev_socket_id(port), &rxconf, mbuf_pool);
		if (ret < 0) {
           	printf("Error setting up RX queue (port %u) info: %s\n",
                 			port, strerror(-ret));
		    return ret;
		}
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (uint16_t queue_id = 0; queue_id < tx_rings; queue_id++) {
		ret = rte_eth_tx_queue_setup(port, queue_id, txd_num,
				rte_eth_dev_socket_id(port), &txconf);
		if (ret < 0) {
 	        printf("Error setting up TX queue (port %u) info: %s\n",
                 			port, strerror(-ret));
		    return ret;
		}
	}

	/* Starting Ethernet port. 8< */
	ret = rte_eth_dev_start(port);
	/* >8 End of starting of ethernet port. */
	if (ret < 0) {
        printf("Error staring ETH (port %u) info: %s\n",
                 			port, strerror(-ret));
	    return ret;
	}

	/* Display the port MAC address. */
	struct rte_ether_addr addr;
	ret = rte_eth_macaddr_get(port, &addr);
	if (ret != 0) {
        printf("Error getting MAC address (port %u) info: %s\n",
                 			port, strerror(-ret));
	    return ret;
	}

	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port, RTE_ETHER_ADDR_BYTES(&addr));

	/* ENA on AWS doesn't support promiscuous mode - ignore the error */
	rte_eth_promiscuous_enable(port);

	return 0;
}

static __rte_noreturn void lcore_main(void) {
	uint16_t port;

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	RTE_ETH_FOREACH_DEV(port) {
		if (rte_eth_dev_socket_id(port) >= 0 &&
				rte_eth_dev_socket_id(port) !=
						(int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
					"polling thread.\n\tPerformance will "
					"not be optimal.\n", port);
	}

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	for (;;) {
		/*
		 * Receive packets on a port and forward them on the paired
		 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
		 */
		RTE_ETH_FOREACH_DEV(port) {
			/* Get burst of RX packets, from first port of pair. */
			struct rte_mbuf *bufs[BURST_SIZE];

			const uint16_t rx_num = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);

			if (unlikely(rx_num == 0))
				continue;

			for (uint16_t i = 0; i < rx_num; i++) {
			    struct rte_mbuf* message = bufs[i];
                struct rte_ether_hdr *eth = rte_pktmbuf_mtod(message, struct rte_ether_hdr*);

                printf("Received a message");

			    rte_pktmbuf_free(message);
			}
		}
	}
}

int main(int argc, char* argv[]) {
    // Initialize EAL
    int ret = rte_eal_init(argc, argv);
    if (ret == -1) {
        rte_exit(EXIT_FAILURE, "Error initializing EAL");
    }

    // Make sure NIC uses DPDK
    uint16_t ports_num = rte_eth_dev_count_avail();
    if (ports_num < 0) {
        rte_exit(EXIT_FAILURE, "No ports found");
    }

    // Initialize mempool for mbuf
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        MBUF_NUM * ports_num,
        MBUF_CACHE_SIZE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id()
    );

    if (mbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Error initializing MBUF pool");
    }

    // Initialize ports
    uint16_t port_id;
   	RTE_ETH_FOREACH_DEV(port_id) {
		if (port_init(port_id, mbuf_pool) != 0)
		    rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", port_id);
    }

    lcore_main();

    return 0;
}
