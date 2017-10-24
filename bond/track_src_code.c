bond ec:d6:8a:0c:c9:d7
tap 82:d2:4a:54:82:bc
254 80:f6:2e:87:14:1f 

vim rte_eth_bond_pmd.c
#if 1
   if (bonded_eth_dev->data->dev_conf.rxmode.hw_vlan_strip)
      {
          slave_eth_dev->data->dev_conf.rxmode.hw_vlan_strip = 1;
      }
#endif


	diag = (*dev->dev_ops->dev_start)(dev);
	if (diag == 0)
		dev->data->dev_started = 1;
	else
		return diag;



static struct rte_eth_conf port_conf = {
	.rxmode = {
		.mq_mode = ETH_MQ_RX_NONE,
		.max_rx_pkt_len = ETHER_MAX_LEN,
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload enabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.rx_adv_conf = {
		.rss_conf = {
			.rss_key = NULL,
			.rss_hf = ETH_RSS_IP,
		},
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
	.intr_conf = {
		.lsc = 1,
	},
};


static uint16_t
bond_ethdev_rx_burst_alb(void *queue, struct rte_mbuf **bufs, uint16_t nb_pkts)
{

#ifdef BOND_DEBUG
		/* 改变目的Mac地址为当前bond口的Mac地址,这里暂时先写死 */
		rte_eth_macaddr_get(0, &eth_h->d_addr);
#endif

}


static uint16_t
bond_ethdev_tx_burst_alb(void *queue, struct rte_mbuf **bufs, uint16_t nb_pkts)
{
	......
		for (i = 0; i < nb_pkts; i++) {
		eth_h = rte_pktmbuf_mtod(bufs[i], struct ether_hdr *);
		ether_type = eth_h->ether_type;
		offset = get_vlan_offset(eth_h, &ether_type);

		if (ether_type == rte_cpu_to_be_16(ETHER_TYPE_ARP)) {

			......

			slave_idx = bond_mode_alb_arp_xmit(eth_h, offset, internals);

			/* Change src mac in eth header */
			DEBUG_INFO( "Change source MAC as Slave MAC,ID=%d", slave_idx);
			rte_eth_macaddr_get(slave_idx, &eth_h->s_addr);

			......

		} else {

			......

			/* If packet is not ARP, send it with TLB policy */
			slave_bufs[RTE_MAX_ETHPORTS][slave_bufs_pkts[RTE_MAX_ETHPORTS]] =
					bufs[i];
			slave_bufs_pkts[RTE_MAX_ETHPORTS]++;
			
			......

		}
	}
	......
}

void
rte_eth_macaddr_get(uint8_t port_id, struct ether_addr *mac_addr)
{
	struct rte_eth_dev *dev;

	RTE_ETH_VALID_PORTID_OR_RET(port_id);
	dev = &rte_eth_devices[port_id];
	ether_addr_copy(&dev->data->mac_addrs[0], mac_addr);
}

static uint16_t
bond_ethdev_tx_burst_tlb(void *queue, struct rte_mbuf **bufs, uint16_t nb_pkts)
{

	......

	struct rte_eth_dev *primary_port =
			&rte_eth_devices[internals->primary_port];

	......


	for (i = 0; i < num_of_slaves; i++) {

		rte_eth_macaddr_get(slaves[i], &active_slave_addr);
		for (j = num_tx_total; j < nb_pkts; j++) {
			if (j + 3 < nb_pkts)
				rte_prefetch0(rte_pktmbuf_mtod(bufs[j+3], void*));

			ether_hdr = rte_pktmbuf_mtod(bufs[j], struct ether_hdr *);
			if (is_same_ether_addr(&ether_hdr->s_addr, &primary_slave_addr))
				ether_addr_copy(&active_slave_addr, &ether_hdr->s_addr);
#if defined(RTE_LIBRTE_BOND_DEBUG_ALB) || defined(RTE_LIBRTE_BOND_DEBUG_ALB_L1)
					mode6_debug("TX IPv4:", ether_hdr, slaves[i], &burstnumberTX);
#endif
		}

		num_tx_total += rte_eth_tx_burst(slaves[i], bd_tx_q->queue_id,
				bufs + num_tx_total, nb_pkts - num_tx_total);
#ifdef BOND_DEBUG
			printf( "Send non-ARP packets Throuth port ID = %d\n", i );
#endif
		if (num_tx_total == nb_pkts)
			break;
	}

	return num_tx_total;
}




======================================================================================================================================

内核里面的代码
/***
 * 返回值:
 * RX_HANDLER_CONSUMED 不做进一步处理
 * RX_HANDLER_ANOTHER skb->dev被改变，进行新一轮接收处理
 * RX_HANDLER_EXACT 强制传送
 * RX_HANDLER_PASS 什么都不做，过时的SKB
 * */
static rx_handler_result_t bond_handle_frame(struct sk_buff **pskb)
{
	struct sk_buff *skb = *pskb;
	struct slave *slave;
	struct bonding *bond;
	int (*recv_probe)(const struct sk_buff *, struct bonding *,
			  struct slave *);
	int ret = RX_HANDLER_ANOTHER;

	/***
	 * 检测skb是否共享，如果是共享的，clone出一份新的skb，老的skb计数减1
	 * */
	skb = skb_share_check(skb, GFP_ATOMIC);
	if (unlikely(!skb))
		return RX_HANDLER_CONSUMED;

	*pskb = skb;

	/***
	 * 读-拷贝修改 RCU保护
	 * */
	slave = bond_slave_get_rcu(skb->dev);
	bond = slave->bond;

	if (bond->params.arp_interval)
		slave->dev->last_rx = jiffies;

	/***
	 * 需要使用 ACCESS_ONCE() 的两个条件是
	 * 1. 在无锁的情况下访问全局变量
	 * 2. 对该变量的访问可能被编译器优化成合并成一次或者拆分成多次
	 * */
	recv_probe = ACCESS_ONCE(bond->recv_probe);
	if (recv_probe) {
		ret = recv_probe(skb, bond, slave);
		if (ret == RX_HANDLER_CONSUMED) {
			consume_skb(skb);
			return ret;
		}
	}

	if (bond_should_deliver_exact_match(skb, slave, bond)) {
		return RX_HANDLER_EXACT;
	}

	skb->dev = bond->dev;

	/***
	 * 设置dev->prive_flags加上IFF_BRIDGE_PORT，这样它就不能再作为其他br的从设备了
	 * */
	if (bond->params.mode == BOND_MODE_ALB &&
	    bond->dev->priv_flags & IFF_BRIDGE_PORT &&
	    skb->pkt_type == PACKET_HOST) {
		/***
		 * PACKET_HOST表示到本地主机的包
		 * PACKET_BROADCAST表示物理层广播包
		 * PACKET_MULTICAST 以太层多播报文
		 * PACKET_OTHERHOST 发往其他主机的报文(或混杂报文)
		 *  PACKET_OUTGOING  本机发出的数据包
		 * */

		if (unlikely(skb_cow_head(skb,
					  skb->data - skb_mac_header(skb)))) {
			kfree_skb(skb);
			return RX_HANDLER_CONSUMED;
		}
		memcpy(eth_hdr(skb)->h_dest, bond->dev->dev_addr, ETH_ALEN);
	}

	return ret;
}

static int __netif_receive_skb_core(struct sk_buff *skb, bool pfmemalloc)
{
	......

	/***
	 * rx_handler是特殊接收过程
	 * 例如网桥和bond需要用到这个
	 * rcu_read_lock()和rcu_read_unlock()用来保持一个读者的RCU临界区,
	 * 在该临界区内不允许发生上下文切换.
	 * rcu_dereference():读者调用它来获得一个被RCU保护的指针.
	 * rcu_assign_pointer():写者使用该函数来为被RCU保护的指针分配一个新的值k,
	 * 这样是为了安全从写者到读者更改其值.这个函数会返回一个新值
	 * */
	rx_handler = rcu_dereference(skb->dev->rx_handler);
	if (rx_handler) {
		if (pt_prev) {
			ret = deliver_skb(skb, pt_prev, orig_dev);
			pt_prev = NULL;
		}
		switch (rx_handler(&skb)) {

		/* 不做进一步处理 */
		case RX_HANDLER_CONSUMED:
			ret = NET_RX_SUCCESS;
			goto unlock;

		/* skb->dev被改变，进行新一轮接收处理 */
		case RX_HANDLER_ANOTHER:
			goto another_round;

		/* 强制传送 */
		case RX_HANDLER_EXACT:
			deliver_exact = true;

		/* 什么都不做，过时的SKB */
		case RX_HANDLER_PASS:
			break;

		default:
			BUG();
		}
	}

	......
}


static int __netif_receive_skb(struct sk_buff *skb)
{
	int ret;

	if (sk_memalloc_socks() && skb_pfmemalloc(skb)) {
		unsigned long pflags = current->flags;

		/*
		 * PFMEMALLOC skbs are special, they should
		 * - be delivered to SOCK_MEMALLOC sockets only
		 * - stay away from userspace
		 * - have bounded memory usage
		 *
		 * Use PF_MEMALLOC as this saves us from propagating the allocation
		 * context down to all allocation sites.
		 */
		current->flags |= PF_MEMALLOC;
		ret = __netif_receive_skb_core(skb, true);
		tsk_restore_flags(current, pflags, PF_MEMALLOC);
	} else
		ret = __netif_receive_skb_core(skb, false);

	return ret;
}

/**
 *	netif_receive_skb - process receive buffer from network
 *	@skb: buffer to process
 *
 *	netif_receive_skb() is the main receive data processing function.
 *	It always succeeds. The buffer may be dropped during processing
 *	for congestion control or by the protocol layers.
 *
 *	This function may only be called from softirq context and interrupts
 *	should be enabled.
 *
 *	Return values (usually ignored):
 *	NET_RX_SUCCESS: no congestion
 *	NET_RX_DROP: packet was dropped
 */
int netif_receive_skb(struct sk_buff *skb)
{
	net_timestamp_check(netdev_tstamp_prequeue, skb);

	if (skb_defer_rx_timestamp(skb))
		return NET_RX_SUCCESS;

#ifdef CONFIG_RPS
	if (static_key_false(&rps_needed)) {
		struct rps_dev_flow voidflow, *rflow = &voidflow;
		int cpu, ret;

		rcu_read_lock();

		cpu = get_rps_cpu(skb->dev, skb, &rflow);

		if (cpu >= 0) {
			ret = enqueue_to_backlog(skb, cpu, &rflow->last_qtail);
			rcu_read_unlock();
			return ret;
		}
		rcu_read_unlock();
	}
#endif
	return __netif_receive_skb(skb);
}
EXPORT_SYMBOL(netif_receive_skb);

/**
 *	netdev_rx_handler_register - register receive handler
 *	@dev: device to register a handler for
 *	@rx_handler: receive handler to register
 *	@rx_handler_data: data pointer that is used by rx handler
 *
 *	Register a receive hander for a device. This handler will then be
 *	called from __netif_receive_skb. A negative errno code is returned
 *	on a failure.
 *
 *	The caller must hold the rtnl_mutex.
 *
 *	For a general description of rx_handler, see enum rx_handler_result.
 */
int netdev_rx_handler_register(struct net_device *dev,
			       rx_handler_func_t *rx_handler,
			       void *rx_handler_data)
{
	ASSERT_RTNL();

	if (dev->rx_handler)
		return -EBUSY;

	/* Note: rx_handler_data must be set before rx_handler */
	rcu_assign_pointer(dev->rx_handler_data, rx_handler_data);
	rcu_assign_pointer(dev->rx_handler, rx_handler);

	return 0;
}
EXPORT_SYMBOL_GPL(netdev_rx_handler_register);

/* enslave device <slave> to bond device <master> */
int bond_enslave(struct net_device *bond_dev, struct net_device *slave_dev)
{
	......

	res = netdev_rx_handler_register(slave_dev, bond_handle_frame,
					 new_slave);
	
	......
}

static const struct net_device_ops bond_netdev_ops = {
	.ndo_init		= bond_init,
	.ndo_uninit		= bond_uninit,
	.ndo_open		= bond_open,
	.ndo_stop		= bond_close,
	.ndo_start_xmit		= bond_start_xmit,
	.ndo_select_queue	= bond_select_queue,
	.ndo_get_stats64	= bond_get_stats,
	.ndo_do_ioctl		= bond_do_ioctl,
	.ndo_change_rx_flags	= bond_change_rx_flags,
	.ndo_set_rx_mode	= bond_set_multicast_list,
	.ndo_change_mtu		= bond_change_mtu,
	.ndo_set_mac_address	= bond_set_mac_address,
	.ndo_neigh_setup	= bond_neigh_setup,
	.ndo_vlan_rx_add_vid	= bond_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= bond_vlan_rx_kill_vid,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_netpoll_setup	= bond_netpoll_setup,
	.ndo_netpoll_cleanup	= bond_netpoll_cleanup,
	.ndo_poll_controller	= bond_poll_controller,
#endif
	.ndo_add_slave		= bond_enslave,
	.ndo_del_slave		= bond_release,
	.ndo_fix_features	= bond_fix_features,
};

static void bond_setup(struct net_device *bond_dev)
{
	
	......

	/* Initialize the device entry points */
	ether_setup(bond_dev);
	bond_dev->netdev_ops = &bond_netdev_ops;
	bond_dev->ethtool_ops = &bond_ethtool_ops;
	bond_set_mode_ops(bond, bond->params.mode);

	......
}

int bond_create(struct net *net, const char *name)
{
	
	......

	bond_dev = alloc_netdev_mq(sizeof(struct bonding),
				   name ? name : "bond%d",
				   bond_setup, tx_queues);
	if (!bond_dev) {
		pr_err("%s: eek! can't alloc netdev!\n", name);
		rtnl_unlock();
		return -ENOMEM;
	}

	......
}

/*
 * enum rx_handler_result - Possible return values for rx_handlers.
 * @RX_HANDLER_CONSUMED: skb was consumed by rx_handler, do not process it
 * further.
 * @RX_HANDLER_ANOTHER: Do another round in receive path. This is indicated in
 * case skb->dev was changed by rx_handler.
 * @RX_HANDLER_EXACT: Force exact delivery, no wildcard.
 * @RX_HANDLER_PASS: Do nothing, passe the skb as if no rx_handler was called.
 *
 * rx_handlers are functions called from inside __netif_receive_skb(), to do
 * special processing of the skb, prior to delivery to protocol handlers.
 *
 * Currently, a net_device can only have a single rx_handler registered. Trying
 * to register a second rx_handler will return -EBUSY.
 *
 * To register a rx_handler on a net_device, use netdev_rx_handler_register().
 * To unregister a rx_handler on a net_device, use
 * netdev_rx_handler_unregister().
 *
 * Upon return, rx_handler is expected to tell __netif_receive_skb() what to
 * do with the skb.
 *
 * If the rx_handler consumed to skb in some way, it should return
 * RX_HANDLER_CONSUMED. This is appropriate when the rx_handler arranged for
 * the skb to be delivered in some other ways.
 *
 * If the rx_handler changed skb->dev, to divert the skb to another
 * net_device, it should return RX_HANDLER_ANOTHER. The rx_handler for the
 * new device will be called if it exists.
 *
 * If the rx_handler consider the skb should be ignored, it should return
 * RX_HANDLER_EXACT. The skb will only be delivered to protocol handlers that
 * are registered on exact device (ptype->dev == skb->dev).
 *
 * If the rx_handler didn't changed skb->dev, but want the skb to be normally
 * delivered, it should return RX_HANDLER_PASS.
 *
 * A device without a registered rx_handler will behave as if rx_handler
 * returned RX_HANDLER_PASS.
 */

enum rx_handler_result {
	RX_HANDLER_CONSUMED,
	RX_HANDLER_ANOTHER,
	RX_HANDLER_EXACT,
	RX_HANDLER_PASS,
};

I/G位决定该地址是个人地址(0)还是组地址(1)，当为组地址情况下,如果所有的地址位都为1，那么I/G位就表示这是一个广播地址
U/L位决定该地址是本地分配的（1）还是统一分配的（0)
22位法定地址通常是由IEEE分配给网络设备生产厂商的，全世界每一家有生产网络接口设备的厂家都必须获得IEEE指定的一个22位地址，而且不许该地址和另外的厂家相同
24位用户地址则是由用户自己可以配置和修改的，但是要确保你修改的地址不能和别人有相同的地方
可见，以上48位MAC地址中，真正可以由用户修改的是后24位。用户通常不必去修改该地址

Mac组播地址与IP组播地址之间由对应关系,由固定转换算法

sk_buff->pkt_type
这个变量表示帧的类型，分类是由L2的目的地址来决定的。
这个值在网卡驱动程序中由函数eth_type_trans通过判断目的以太网地址来确定。
如果目的地址是FF:FF:FF:FF:FF:FF，则为广播地址，pkt_type = PACKET_BROADCAST；
如果最高位为1,则为组播地址，pkt_type = PACKET_MULTICAST；
如果目的mac地址跟本机mac地址不相等，则不是发给本机的数据报，pkt_type = PACKET_OTHERHOST；
否则就是缺省值PACKET_HOST。
/* Packet types */
#define PACKET_HOST		0		/* To us		*/
#define PACKET_BROADCAST	1		/* To all		*/
#define PACKET_MULTICAST	2		/* To group		*/
#define PACKET_OTHERHOST	3		/* To someone else 	*/
#define PACKET_OUTGOING		4		/* Outgoing of any type */
/* These ones are invisible by user level */
#define PACKET_LOOPBACK		5		/* MC/BRD frame looped back */
#define PACKET_FASTROUTE	6		/* Fastrouted frame	*/

内核 模式6 发包
int bond_alb_xmit(struct sk_buff *skb, struct net_device *bond_dev)
{
	struct bonding *bond = netdev_priv(bond_dev);
	struct ethhdr *eth_data;
	struct alb_bond_info *bond_info = &(BOND_ALB_INFO(bond));
	struct slave *tx_slave = NULL;
	static const __be32 ip_bcast = htonl(0xffffffff);
	int hash_size = 0;
	int do_tx_balance = 1;
	u32 hash_index = 0;
	const u8 *hash_start = NULL;
	int res = 1;
	struct ipv6hdr *ip6hdr;

	skb_reset_mac_header(skb);
	eth_data = eth_hdr(skb);

	/* make sure that the curr_active_slave do not change during tx
	 */
	read_lock(&bond->curr_slave_lock);

	switch (ntohs(skb->protocol)) {
	case ETH_P_IP: {
		const struct iphdr *iph = ip_hdr(skb);

		if (ether_addr_equal_64bits(eth_data->h_dest, mac_bcast) ||
		    (iph->daddr == ip_bcast) ||
		    (iph->protocol == IPPROTO_IGMP)) {
			do_tx_balance = 0;
			break;
		}
		hash_start = (char *)&(iph->daddr);
		hash_size = sizeof(iph->daddr);
	}
		break;
	case ETH_P_IPV6:
		/* IPv6 doesn't really use broadcast mac address, but leave
		 * that here just in case.
		 */
		if (ether_addr_equal_64bits(eth_data->h_dest, mac_bcast)) {
			do_tx_balance = 0;
			break;
		}

		/* IPv6 uses all-nodes multicast as an equivalent to
		 * broadcasts in IPv4.
		 */
		if (ether_addr_equal_64bits(eth_data->h_dest, mac_v6_allmcast)) {
			do_tx_balance = 0;
			break;
		}

		/* Additianally, DAD probes should not be tx-balanced as that
		 * will lead to false positives for duplicate addresses and
		 * prevent address configuration from working.
		 */
		ip6hdr = ipv6_hdr(skb);
		if (ipv6_addr_any(&ip6hdr->saddr)) {
			do_tx_balance = 0;
			break;
		}

		hash_start = (char *)&(ipv6_hdr(skb)->daddr);
		hash_size = sizeof(ipv6_hdr(skb)->daddr);
		break;
	case ETH_P_IPX:
		if (ipx_hdr(skb)->ipx_checksum != IPX_NO_CHECKSUM) {
			/* something is wrong with this packet */
			do_tx_balance = 0;
			break;
		}

		if (ipx_hdr(skb)->ipx_type != IPX_TYPE_NCP) {
			/* The only protocol worth balancing in
			 * this family since it has an "ARP" like
			 * mechanism
			 */
			do_tx_balance = 0;
			break;
		}

		hash_start = (char*)eth_data->h_dest;
		hash_size = ETH_ALEN;
		break;
	case ETH_P_ARP:
		do_tx_balance = 0;
		if (bond_info->rlb_enabled) {
			tx_slave = rlb_arp_xmit(skb, bond);
		}
		break;
	default:
		do_tx_balance = 0;
		break;
	}

	if (do_tx_balance) {
		hash_index = _simple_hash(hash_start, hash_size);
		tx_slave = tlb_choose_channel(bond, hash_index, skb->len);
	}

	if (!tx_slave) {
		/* unbalanced or unassigned, send through primary */
		tx_slave = bond->curr_active_slave;
		bond_info->unbalanced_load += skb->len;
	}

	if (tx_slave && SLAVE_IS_OK(tx_slave)) {
		if (tx_slave != bond->curr_active_slave) {
			memcpy(eth_data->h_source,
			       tx_slave->dev->dev_addr,
			       ETH_ALEN);
		}

		res = bond_dev_queue_xmit(bond, skb, tx_slave->dev);
	} else {
		if (tx_slave) {
			_lock_tx_hashtbl(bond);
			__tlb_clear_slave(bond, tx_slave, 0);
			_unlock_tx_hashtbl(bond);
		}
	}

	read_unlock(&bond->curr_slave_lock);

	if (res) {
		/* no suitable interface, frame not sent */
		kfree_skb(skb);
	}
	return NETDEV_TX_OK;
}
