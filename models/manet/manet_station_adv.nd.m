MIL_3_Tfile_Hdr_ 140A 140A opnet 6 3E6757F8 4792BD91 49 mdlinux1 opnet 0 0 none none 0 0 none D345FB2A 1212E 0 0 0 0 0 0 1789 0                                                                                                                                                                                                                                                                                                                                                                                                 Ф═gЅ      8   e   ░  њ  ќ  јВ  Ъ╣  фв  ┐й  ┐┼ А Ц      node   IP   station   WLAN   DHCP   wireless_laptop   wireless_laptop           Wireless LAN Workstation    :   General Node Functions:       -----------------------       ,The manet_station_adv node model represents    +a raw packet generator transmitting packets   over IP and WLAN           
Protocols:   
----------   IP, IEEE 802.11               Interconnections:       -----------------       Either of the following:       1) 1 WLAN connection at 1 Mbps,       2) 1 WLAN connection at 2 Mbps,       !3) 1 WLAN connection at 5.5 Mbps,        4) 1 WLAN connection at 11 Mbps                Attributes:       -----------   '"MANET Traffic Generation Parameters":    &Attribute to specify the rate at which   &raw unformatted packets are generated.       )"IP Forwarding Rate": specifies the rate    *(in packets/second) at which the node can    "perform a routing decision for an    'arriving packet and transfer it to the    appropriate output interface.       )"IP Gateway Function": specifies whether    *the local IP node is acting as a gateway.    )Workstations should not act as gateways,    (as they only have one network interface.           <<Summary>>   /General Function: Raw packet generation over IP       #Supported Protocols: IP, IEEE802.11       Port Interface Description:       '  1 WLAN connection at 1,2,5.5,11 Mbps        +      1AD-HOC Routing ParametersAD-HOC Routing Protocol      $ip.manet_mgr.AD-HOC Routing Protocol                                                                      )AD-HOC Routing ParametersAODV Parameters      ip.manet_mgr.AODV Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       ARP Parameters      arp.ARP Parameters                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       	BGP Based      ip.BGP L2VPN/VPLS Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       CPU Background Utilization      CPU.background utilization                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       CPU Resource Parameters      CPU.Resource Parameters                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       ;VPN.Network Based.L2VPN/VPLS InstancesCross Connect Groups      ip.Cross Connect Groups                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       @VPN.Network Based.L2VPN/VPLS InstancesCross-Connects Parameters      %ip.mpls_mgr.Cross-Connects Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       DHCPv6 Client Parameters      dhcp.DHCPv6 Client Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       DHCPv6 Server Parameters      dhcp.DHCPv6 Server Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       (AD-HOC Routing ParametersDSR Parameters      ip.manet_mgr.DSR Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       DVMRP Parameters      ip.DVMRP Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       (AD-HOC Routing ParametersGRP Parameters      ip.manet_mgr.GRP Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       IGMP Parameters      ip.ip_igmp_host.IGMP Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       ReportsIP Forwarding Table      ip.IP Forwarding Table                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       IP Gateway Function      
ip.gateway                                                                                  IP Host Parameters      ip.ip host parameters              ђ                                               count                                                                        ЦZ             list   	          	                                                          Interface Information                                   count          
          
      list   	      
          
                                     Allows the configuration of:       1. Number of IP Interfaces   2. IP Address for each intf.    3. Subnet Mask for each    intf.   4. MTU for each IP intf.       5. Datagram compression    scheme used for each    
interface.       6. IP multicast enabling and    disabling per interface.       7. QoS management scheme for    each interface including    output queuing processing    mechanism (FIFO, WFQ,    Priority Queuing, Custom    Queuing), dropping mechanism    "(RED/WRED), reservation mechanism    (RSVP) and bandwith    management mechanism (CAR).   count                                                                       Number of IP interfaces for a    given node.                         ЦZ             list   	          	                                                          Name                          IF0                                 IF0      IF0         KName of this interface. This attribute does not affect simulation behavior.ЦZ             Address                          Auto Assigned                                 Auto Assigned      Auto Assigned      No IP Address      No IP Address         |Represents the IP address of the interface. The IP address should be specified in dotted decimal notation eg. 198.24.46.89.        ЇThe value "Auto Assigned" indicates that the IP Auto Addressing feature of the IP model will pick an "appropriate" value for this attribute.        ћThe value "Unnumbered" should be used only while modeling "Unnumbered Point-to-Point Links" feature when using OSPF as the dynamic routing protocol.       ]When an interface is assigned "No IP Address", it will not participate in routing/forwarding.ЦZ             Subnet Mask                          Auto Assigned                                 Auto Assigned      Auto Assigned      Class A (natural)      	255.0.0.0      Class B (natural)      255.255.0.0      Class C (natural)      255.255.255.0         !The Subnet Mask of the interface.       QThe subnet mask should be specified in dotted decimal notation eg. 255.255.254.0.       ѓThe value "Auto Assigned" indicates that a subnet mask will be chosen based on the major network of its corresponding IP address.     ЦZ             MTU                  bytes             Ethernet                                      Not Used      ■          Ethernet     ▄          FDDI     v          ATM     v          IP     ▄          
Token Ring     p          Frame Relay     ▄          WLAN     	           
   џThe Maximum Transfer Unit (MTU) for the interface. MTU values depend on the data link technology that will be used by this interface for packet transfer.        1The MTU should have a minimum value representing    (the size of the IP header i.e. 20 bytes.       0Convenient symbol map values for some data link    technologies are provided.             ЦZ             Compression Information                    љ      None                                 None      None      !Default TCP/IP Header Compression      !Default TCP/IP Header Compression      !Default Per-Interface Compression      !Default Per-Interface Compression      'Default Per-Virtual Circuit Compression      'Default Per-Virtual Circuit Compression         ┘Name of the compression scheme used for this interface. This name is used as a reference to the details of the scheme. These can be found under the "IP Compression Information" attribute of "IP Attribute Config" node    ?model, which can be found in the utilities object palette.                ip_active_attrib_handler   (ip_compression_profile_get_click_handler            ЦZ             IPv6 Parameters                             None                                 None               count          
          
      list   	      
            Global Address(es)          
      None   
   
         HIPv6 addresses of this interface can be configured under this attribute.   count                                                                       Number of IP interfaces for a    given node.                         ЦZ             list   	          	                                                          Link-Local Address                          
Not Active                                 Default EUI-64      Default      
Not Active      
Not Active        LTo assign a link local address automatically, leave this attribute as Default EUI-64. If the link local address is configured manually, the first 64 bits must be FE80:: as specified in RFC 2373: Sec 2.5.8. Irrespective of whether the address was specified manually or automatically, it will be assumed to have a prefix length of 64.        ЦZ             Global Address(es)                             None                                 None               count          
          
      list   	      
          
         ▓In addition to the Link Local address, every IPv6 enabled interface must also have at least one global address. Global addresses are configured by adding a row to this attribute.   count                                0                                       0                 1                2                            ЦZ             list   	          	                                                          Address                          
Not Active                                 
Not Active      
Not Active        eUnlike link-local addresses, global addresses cannot be automatically assigned. The address needs to be explicitly specified under this attribute. However if the  Address Type  attribute is set to EUI-64, only the first 64 bits of the address need to be specified. The remaining 64 bits of the address will be set to an interface ID unique to the interface.                    ЦZ             Prefix Length                  bits             64                      @             48      0          64      @          128      ђ             їThe length of the IPv6 prefix (in bits) has to be specified here. If the address type is set to EUI-64, the prefix length must be set to 64.        ЦZ             Address Type                              
Non EUI-64                                         EUI-64                
Non EUI-64                    sIndicates whether the value specified under the Address attribute represents the entire address or just the prefix.                ЦZ          ЦZ          ЦZ             Router Solicitation Parameters                             Default                                 Default               count          
          
      list   	      
          
         QAttributes related to router solicitation can be configured under this attribute.                                           count                                                                   1                            ЦZ             list   	          	                                                          Maximum Router Solicitations                                3                                       0                 1                2                3                4                5                   cMaximum number of router solicitation messages that would be sent out on this interface at startup.       ЎIf the node does not receive a response even after sending these many solicitation messages, the node concludes that there are no routers in the network.ЦZ             Router Solicitation Interval                sec   љ      uniform (4.0, 4.5)                                 bernoulli (mean)      bernoulli (mean)      $binomial (num_samples, success_prob)      $binomial (num_samples, success_prob)      chi_square (mean)      chi_square (mean)      constant (mean)      constant (mean)      erlang (scale, shape)      erlang (scale, shape)      exponential (mean)      exponential (mean)      extreme (location, scale)      extreme (location, scale)      fast_normal (mean, variance)      fast_normal (mean, variance)      gamma (scale, shape)      gamma (scale, shape)      geometric (success_prob)      geometric (success_prob)      laplace (mean, scale)      laplace (mean, scale)      logistic (mean, scale)      logistic (mean, scale)      lognormal (mean, variance)      lognormal (mean, variance)      normal (mean, variance)      normal (mean, variance)      pareto (location, shape)      pareto (location, shape)      poisson (mean)      poisson (mean)      power function (shape, scale)      power function (shape, scale)      rayleigh (mean)      rayleigh (mean)      triangular (min, max)      triangular (min, max)      uniform (4.0, 4.5)      uniform (4.0, 4.5)      uniform_int (min, max)      uniform_int (min, max)      weibull (shape, scale)      weibull (shape, scale)      scripted (filename)      scripted (filename)         ЄIf the node does not receive a response to the router solicitation message it sent, it will retransmit the router solicitation message.       ЉThe distribution used to compute the interval between successive router solicitation messages (in seconds) can be specified using this attribute.   oms_dist_configure    oms_dist_conf_dbox_click_handler   $oms_dist_conf_dbox_new_value_handler        ЦZ             Router Solicitation Delay                sec                 1 second                                               1 second   ?­                	2 seconds   @                 	3 seconds   @                   џDuration (in seconds) to wait after sending the maximum number of router solicitations allowed before concluding that there are no routers in the network.ЦZ          ЦZ          ЦZ             Neighbor Cache Parameters                             Default                                 Default               count          
          
      list   	      
          
         ЂAttributes used by Address Resolution and the Neighbor Unreachability Detection algorithm can be configured under this attribute.   count                                                                        ЦZ             list   	          	                                                          Maximum Queue Size                  packets             4                                      1                4                10      
          	Unlimited                   ЇMaximum number of packets to be sent to a particular address that will be queued while address resolution is being performed for the address.       XIf the number of packets exceeds the specified value, the oldest packet will be dropped.ЦZ             Neighbor Solicit Interval                msec   љ      uniform (1000, 1500)                                 bernoulli (mean)      bernoulli (mean)      $binomial (num_samples, success_prob)      $binomial (num_samples, success_prob)      chi_square (mean)      chi_square (mean)      constant (mean)      constant (mean)      erlang (scale, shape)      erlang (scale, shape)      exponential (mean)      exponential (mean)      extreme (location, scale)      extreme (location, scale)      fast_normal (mean, variance)      fast_normal (mean, variance)      gamma (scale, shape)      gamma (scale, shape)      geometric (success_prob)      geometric (success_prob)      laplace (mean, scale)      laplace (mean, scale)      logistic (mean, scale)      logistic (mean, scale)      lognormal (mean, variance)      lognormal (mean, variance)      normal (mean, variance)      normal (mean, variance)      pareto (location, shape)      pareto (location, shape)      poisson (mean)      poisson (mean)      power function (shape, scale)      power function (shape, scale)      rayleigh (mean)      rayleigh (mean)      triangular (min, max)      triangular (min, max)      uniform (1000, 1500)      uniform (1000, 1500)      uniform_int (min, max)      uniform_int (min, max)      weibull (shape, scale)      weibull (shape, scale)      scripted (filename)      scripted (filename)         ЮThe time, in milliseconds, between retransmitted Neighbor Solicitation messages. Used by address resolution and Neighbor Unreachability Detection algorithms.   oms_dist_configure    oms_dist_conf_dbox_click_handler   $oms_dist_conf_dbox_new_value_handler        ЦZ             Maximum Solicitation Attempts                                3                                         1                2                3                4                5                   ~Maximum number of solicitation messages that will be sent for an address before concluding that address resolution has failed.ЦZ             Base Reachable Time                  msec        u0                                             
15 seconds     :ў          
30 seconds     u0          
60 seconds     Ж`             іA base value used for computing the random value for which a neighbor is considered reachable after receiving a reachability confirmation.       dThe actual value will be a uniformly-distributed  random value between 0.5 and 1.5 times this value.       ЪOn host interfaces, if a router advertisement received has the advertised reachable time value set, it will overwrite the value specified using this attribute.       :<<< This attribute does not affect simulation behavior >>>ЦZ          ЦZ          ЦZ          ЦZ          ЦZ             Description                          N/A                                 N/A      N/A         Description of the surrounding    interface (e.g., connected to     OSPF ABR, etc.). This attribute    does not affect simulation    	behavior.ЦZ             Layer 2 Mappings                             None                                 None               count          
          
      list   	      
            ATM PVC Mapping          
      None   
      Frame Relay PVC Mapping          
      None   
   
         ┴This attribute can be used to specify names for layer-2 connectivity. For example if a ATM PVC name is specified, all packets exiting this IP interface will be forced to take the PVC specified.   count                                                                     ЦZ             list   	          	                                                          ATM PVC Mapping                             None                                 None               count          
           
      list   	      
       
          count                                                                          ЦZ             list   	          	                                                          
IP Address                          Any                                 Any      0.0.0.0         YIP Destination Address to be matched with an outgoing PVC in this multi-point interface.        dA multi-point interfaces connects to one or several ATM VCs, each leading to a different IP address.ЦZ             ATM PVC Name                    љ                                            LThe name of the ATM VC that connects with a destination IP address specified   Fin the IP Destination Address attribute on this multipoint interface.        dA multi-point interfaces connects to one or several ATM VCs, each leading to a different IP address.   ip_active_attrib_handler   #ip_layer2_mapping_get_click_handler            ЦZ          ЦZ          ЦZ             Frame Relay PVC Mapping                             None                                 None               count          
           
      list   	      
       
          count                                                                          ЦZ             list   	          	                                                          
IP Address                          Any                                 Any      0.0.0.0         YIP Destination Address to be matched with an outgoing PVC in this multi-point interface.        lA multi-point interfaces connects to one or several Frame Relay VCs, each leading to a different IP address.ЦZ             Frame Relay PVC Name                    љ                                            ЏThe name of the Frame Relay VC that connects with a destination IP address specified in the IP Destination Address attribute on this multipoint interface.        lA multi-point interfaces connects to one or several Frame Relay VCs, each leading to a different IP address.   ip_active_attrib_handler   #ip_layer2_mapping_get_click_handler            ЦZ          ЦZ          ЦZ             VLAN Identifier                                None                     ■             None                    ┤Specifies the identifier of the VLAN to which this subinterface belongs. A valid VID (VLAN Identifier) is an integer between 1 and 4094, inclusive. Any invalid VID will be ignored.       ШPlease note that there should not be also another subinterface of the same physical interface belonging to the same VLAN. In other words, within the domain of a physical interface, there has to be 1:1 relation between the subinterfaces and VLANs.ЦZ          ЦZ          ЦZ          ЦZ          ЦZ             Passive RIP Routing                              Disabled                                         Enabled                Disabled                    ЈDetermines if the surrounding node performs forwarding decisions based on information learnt by RIP (i.e., by RIP operating in a passive mode).ЦZ             Default Route                          Auto Assigned                                 Auto Assigned      Auto Assigned         :When IP routing is not enabled or when route table lookup    д(when routing is enabled) does not yield a route, the "default gateway" attribute is used (if set by the user) as the default next hop to which IP datagrams are sent.       юThe value "Auto Assigned" implies that the a specific interface has not been specified as the default gateway address, and one will be chosen automatically.       8In order to correctly set this attribute, specify the IP   @addresses of the interface that will act as the default gateway.    ЦZ             Static Routing Table                             None                                 None               count          
           
      list   	      
       
         If the "gateway" attribute    is set to "enabled" then the    information in this    attribute is used to    configure the type of    routing that will be done,    e.g. use RIP or OSPF as the    dynamic routing protocol or    use a user-defined static    routing table.                count                                                                        1                2                   The number of "static"    routing table entries that    will be specified by the    user.     ЦZ             list   	          	                                                          Destination Address                          Specify ...                                 Specify ...      0.0.0.0         6The IP address of either a destination interface or a    Cdestination network, in dotted decimal notation, e.g. 123.45.67.89.       ДIn order for this static entry to be used for routing, this address MUST be part of of the set of interface (or network) IP addresses that exist in the network model. ЦZ             Subnet Mask                          Class Based                                 Class Based      0.0.0.0         8The mask that corresponds to the Destination Address in    Cthis row, expressed in dotted decimal notation, e.g. 255.255.192.0.       5If the default value of "Class Based" is chosen, the    Nmask that corresponds to the IP address class of the destination will be used.ЦZ             Next Hop                          Specify ...                                 Specify ...      0.0.0.0      
   XThe IP address of a neighboring router's interface,that is used as the next hop for the    Scorresponding destination, expressed in dotted decimal notation, e.g. 123.45.67.89.                                ЦZ             Administrative Weight                            
                                           10      
          20                   8Specifies the Administrative Weight of a routing entry.        БThe Administrative Weight is used by IP to determine which protocol gets to insert a primary route into the IP Common Route Table, if there are multiple protocols    њtrying to insert a route to the same destination. The routing protocol (or protocol process instance) with a lower Administrative Weight will win.        ЦZ          ЦZ          ЦZ             IPv6 Default Route                          Auto Assigned                                 Auto Assigned      Auto Assigned         nThe default IPv6 route for host. This should be a valid IPv6 address of adjacent router which is IPv6 capable.       .Currently manual assigned route must be used. ЦZ             Multicast Mode                              Disabled                                         Disabled                 Enabled                   DEnable this flag to allow multicast applications to run on this hostЦZ          ЦZ             ip_active_attrib_handler           'ip_rte_protocols_iface_info_set_handler                 IP Multicast Group-to-RP Table      !ip.IP Multicast Group-to-RP Table                                                                                  IP Processing Information      ip.ip processing information                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       IP QoS Parameters      ip.ip qos parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       IP Slot Information      ip.ip slot information                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       IPSec Parameters      ip.ip security parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       IPv6 Parameters      ip.ipv6 parameters                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       $L2TPL2TP Control Channel Parameters      "ip.L2TP Control Channel Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                        LACP System Priority      ip.LACP System Priority                    ╠(                                                            	LDP Based      ip.LDP L2VPN/VPLS Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       #MANET Traffic Generation Parameters      &traf_src.Traffic Generation Parameters                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       MSDP Parameters      ip.MSDP Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       Mainframe Parameters      $CPU.mframe_base.Mainframe Parameters                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       )ReportsMainframe Workload Activity Table      1CPU.mframe_base.Mainframe Workload Activity Table                                                                                  NAT Parameters      ip.NAT Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       )AD-HOC Routing ParametersOLSR Parameters      manet_rte_mgr.OLSR Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       PIM Parameters      ip.PIM Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       %PIM-DVMRP Interoperability Parameters      (ip.PIM-DVMRP Interoperability Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       PIM-SM Routing Table      ip.PIM-SM Routing Table                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       %Server: Advanced Server Configuration      !CPU.Advanced Server Configuration                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                        Server: Modeling Method      CPU.Compatibility Mode                                                                                   .AD-HOC Routing ParametersTORA/IMEP Parameters      !ip.manet_mgr.TORA/IMEP Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       VRF Instances      ip.VRF Instances                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                       ReportsVRF Table      ip.VRF Table                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                        Wireless LAN MAC Address      wireless_lan_mac.Address                                                                                   Wireless LAN Parameters      (wireless_lan_mac.Wireless LAN Parameters                                                              count                                                                            ЦZ             list   	          	                                                        ЦZ                       ip.IGMP Parameters      ip.IGMP Parameters                                                              count                                                                         ЦZ             list   	          	                                                        ЦZ                    :   AD-HOC Routing Protocol            None      AODV Parameters               Default      ARP Parameters         
      Default   
   	BGP Based               None      CPU Background Utilization         
      None   
   CPU Resource Parameters         
      Single Processor   
   Cross Connect Groups               None      Cross-Connects Parameters         
      Not Configured   
   DHCPv6 Client Parameters         
      Disabled   
   DHCPv6 Server Parameters         
      Disabled   
   DSR Parameters               Default      DVMRP Parameters               Not Configured      GRP Parameters               Default      IGMP Parameters               Default      IP Forwarding Table         
      Do Not Export   
   IP Gateway Function         
       Disabled   
   IP Host Parameters         
            count          
          
      list   	      
            Interface Information          
            count          
          
      list   	      
            MTU           
       WLAN   
      IPv6 Parameters          
      None   
      Layer 2 Mappings          
      None   
   
   
      Static Routing Table          
      None   
   
   
   IP Multicast Group-to-RP Table         
       Do Not Export   
   IP Processing Information         
            count          
          
      list   	      
            Datagram Forwarding Rate          
           Efficiency Mode   
   
   
   IP QoS Parameters         
      None   
   IP Slot Information         
      NOT USED   
   IPSec Parameters         
      None   
   IPv6 Parameters         
            count          
          
      list   	      
          
   
   L2TP Control Channel Parameters               None      LACP System Priority          
       32768   
   	LDP Based               None      #MANET Traffic Generation Parameters         
      None   
   MSDP Parameters               Not Configured      Mainframe Parameters         
      Not Configured   
   !Mainframe Workload Activity Table         
       Do Not Export   
   NAT Parameters         
      Not Configured   
   OLSR Parameters               Default      PIM Parameters               Not Configured      %PIM-DVMRP Interoperability Parameters               Not Configured      PIM-SM Routing Table               Do Not Export      %Server: Advanced Server Configuration         
      Sun Ultra 10 333 MHz   
   Server: Modeling Method                 
Simple CPU      
TIM source         
   ip   
   TORA/IMEP Parameters               Default      VRF Instances               None      	VRF Table         
      Do Not Export   
   Wireless LAN MAC Address          
       Auto Assigned   
   Wireless LAN Parameters         
      Default   
   altitude         
               
   altitude modeling            relative to subnet-platform      	condition         
          
   financial cost            0.00      ip.IGMP Parameters               Not Configured      ip.Pseudowire Classes               None      ip.ip multicast information         
      Default   
   ip.ip router parameters         
            count          
          
      list   	      
            Interface Information          
            count          
          
      list   	      
            QoS Information          
      None   
   
   
      Loopback Interfaces          
            count          
          
      list   	      
            Name          
   Loopback   
   
   
      Static Routing Table          
      None   
   
   
   ip.manet_mgr.MANET Gateway                Disabled      ip.mpls_mgr.MPLS Parameters                     count          
          
      list   	      
          
      ip.nhrp.NHRP Parameters               None      phase         
               
   priority          
           
   role                   user id          
           
              џ   l          
   ip_encap   
       
   ip_encap_v4   
          	processor                       џ   ╚          
   arp   
       
   	ip_arp_v4   
          	processor                       џ   џ   	       
   ip   
       
   ip_dispatch   
          	processor                   !manet_mgr.AD-HOC Routing Protocol         	   None   	      manet_mgr.AODV Parameters         	      Default   	      manet_mgr.DSR Parameters         	      Default   	      manet_mgr.GRP Parameters         	      Default   	      manet_mgr.TORA/IMEP Parameters         	      Default   	      :   џ   >          
   traf_src   
       
   manet_dispatcher   
          	processor                   Traffic Generation Parameters         	      None   	        $   >          
   CPU   
       
   
server_mgr   
          	processor                   Compatibility Mode          	       
Simple CPU   	      Resource Parameters         	      Single Processor   	      background utilization         	      None   	      З   џ   Ш          
   wireless_lan_mac   
       
   wlan_dispatch   
          	processor                   Ш   Ш   l          
   udp   
       
   
rip_udp_v3   
          	processor                   Э  R   l          
   manet_rte_mgr   
       
   manet_rte_mgr   
          	processor                   x   Ш   >          
   dhcp   
       
   dhcp_mgr   
          	processor                	  Щ   >  $          
   wlan_port_rx_0_0   
       
            count          
          
      list   	      
            	data rate         
A.ёђ           
      packet formats         
   !unformatted,wlan_control,wlan_mac   
      	bandwidth         
@Н|            
      min frequency         
@б┬            
   
   
       
   dpsk   
       ?­                                          
   NONE   
       
   
wlan_power   
          dra_bkgnoise             
dra_inoise             dra_snr          
   wlan_ber   
       
   
wlan_error   
       
   wlan_ecc   
          ra_rx                       nd_radio_receiver        ■   Ш  $          
   wlan_port_tx_0_0   
       
            count          
          
      list   	      
            	data rate         
A.ёђ           
      packet formats         
   wlan_control,wlan_mac   
      	bandwidth         
@Н|            
      min frequency         
@б┬            
      power         
?PbMмыЕЧ       
   
   
       
   dpsk   
       
   wlan_rxgroup   
       
   
wlan_txdel   
          dra_closure          
   wlan_chanmatch   
       
   NONE   
       
   wlan_propdel   
          ra_tx                       nd_radio_transmitter                           д   t   ╗   t   ╗   Ћ   д   Ћ   
       
   strm_8   
       
   src stream [0]   
       
   dest stream [0]   
                                              
@U         
                                        nd_packet_stream                       Ї   ќ   y   ќ   y   t   Ї   t   
       
   strm_9   
       
   src stream [0]   
       
   dest stream [0]   
                                              
@          
                                        nd_packet_stream                      д   Ю   ║   Ю   ║   К   д   К          
   port_0   
       
   src stream [1]   
       
   dest stream [0]   
                                              
@U         
                                        nd_packet_stream         ip addr index           
           
                                                                                         Ї   К   {   К   {   А   Ї   А          
   	in_port_0   
       
   src stream [1]   
       
   dest stream [1]   
                                              
@          
                                        nd_packet_stream         ip addr index           
           
                                                                               :          Б   A   ╣   A   ╣   e   д   e   
       
   	strm_4111   
       
   src stream [0]   
       
   dest stream [1]   
                                                                                                nd_packet_stream                :      ћ   d   |   d   |   ?   њ   ?   
       
   	strm_4112   
       
   src stream [1]   
       
   dest stream [0]   
                                              
@        
                                        nd_packet_stream            З         Ї   в   z   в   z   ¤   Ї   ¤   
       
   	strm_4109   
       
   src stream [1]   
       
   dest stream [4]   
                                              
@          
                                        nd_packet_stream            З  ■      б   Ы   §   Ы   §     
       
   tx   
       
   src stream [0]   
       
   dest stream [0]   
                                              
@U         
                                        nd_packet_stream               З      д   л   И   л   И   в   д   в   
       
   	strm_4110   
       
   src stream [4]   
       
   dest stream [1]   
                                              
@U         
                                        nd_packet_stream          	  Щ  З      >     >   з   Ї   з   
       
   rx   
       
   src stream [0]   
       
   dest stream [0]   
                                              
@          
                                        nd_packet_stream          
      Ш      Ц   h   В   h   
       
   	strm_4113   
       
   src stream [2]   
       
   dest stream [0]   
                                              
@В!       
                                        nd_packet_stream            Ш  Э         f  L   f   
       
   	strm_4114   
       
   src stream [0]   
       
   dest stream [0]   
                                              
@ы       
                                        nd_packet_stream            Ш          Ж   o   ц   o   
       
   	strm_4115   
       
   src stream [1]   
       
   dest stream [2]   
                                                                                                nd_packet_stream            Э  Ш     F   p   §   p   
       
   	strm_4116   
       
   src stream [0]   
       
   dest stream [1]   
                                                                                                nd_packet_stream            x  Ш      з   8   з   a   
       
   dhcp_to_udp   
       
   src stream [0]   
       
   dest stream [2]   
                                              
@ ф        
                                        nd_packet_stream            Ш  x      Ю     U   Ђ   
       
   udp_to_dhcp   
       
   src stream [2]   
       
   dest stream [0]   
                                              
@          
                                        nd_packet_stream           ■  З      ж  (   д   Ч          
   txstat   
       
   channel [0]   
       
   radio transmitter.busy   
       
   
instat [1]   
                                              
           
                                                            н▓IГ%ћ├}              н▓IГ%ћ├}              
@ф         
                                        nd_statistic_wire           Щ  З      J  +   Љ            
   rxstat   
       
   channel [0]   
       
   !radio receiver.received power (W)   
       
   
instat [0]   
                                              
           
       
           
                                           
               
       
=4АмW1└ў       
       
@ф         
                                        nd_statistic_wire     {      Ф   +ip.Broadcast Traffic Received (packets/sec)   (Broadcast Traffic Received (packets/sec)           IP   bucket/default total/sum_time   linear   IP   'ip.Broadcast Traffic Sent (packets/sec)   $Broadcast Traffic Sent (packets/sec)           IP   bucket/default total/sum_time   linear   IP   +ip.Multicast Traffic Received (packets/sec)   (Multicast Traffic Received (packets/sec)           IP   bucket/default total/sum_time   linear   IP   'ip.Multicast Traffic Sent (packets/sec)   $Multicast Traffic Sent (packets/sec)           IP   bucket/default total/sum_time   linear   IP    ip.Traffic Dropped (packets/sec)   Traffic Dropped (packets/sec)           IP   bucket/default total/sum_time   linear   IP   !ip.Traffic Received (packets/sec)   Traffic Received (packets/sec)           IP   bucket/default total/sum_time   linear   IP   ip.Traffic Sent (packets/sec)   Traffic Sent (packets/sec)           IP   bucket/default total/sum_time   linear   IP   ip.Processing Delay (sec)   Processing Delay (sec)           IP    bucket/default total/sample mean   linear   IP   "ip.Ping Replies Received (packets)   Ping Replies Received (packets)           IP   bucket/default total/count   square-wave   IP   ip.Ping Requests Sent (packets)   Ping Requests Sent (packets)           IP   bucket/default total/count   square-wave   IP   ip.Ping Response Time (sec)   Ping Response Time (sec)           IP    bucket/default total/sample mean   discrete   IP   %ip.Background Traffic Delay --> (sec)   "Background Traffic Delay --> (sec)           IP   normal   linear   IP   %ip.Background Traffic Delay <-- (sec)   "Background Traffic Delay <-- (sec)           IP   normal   linear   IP    wireless_lan_mac.Load (bits/sec)   Load (bits/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   &wireless_lan_mac.Throughput (bits/sec)   Throughput (bits/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   )wireless_lan_mac.Media Access Delay (sec)   Media Access Delay (sec)           Wireless Lan    bucket/default total/sample mean   linear   Wireless Lan   wireless_lan_mac.Delay (sec)   Delay (sec)           Wireless Lan    bucket/default total/sample mean   linear   Wireless Lan   &ip.Forwarding Memory Free Size (bytes)   #Forwarding Memory Free Size (bytes)           IP Processor   !bucket/default total/time average   linear   IP Processor   ip.Forwarding Memory Overflows   Forwarding Memory Overflows           IP Processor   sample/default total   linear   IP Processor   'ip.Forwarding Memory Queue Size (bytes)   $Forwarding Memory Queue Size (bytes)           IP Processor   !bucket/default total/time average   linear   IP Processor   0ip.Forwarding Memory Queue Size (incoming bytes)   -Forwarding Memory Queue Size (incoming bytes)           IP Processor   !bucket/default total/time average   linear   IP Processor   2ip.Forwarding Memory Queue Size (incoming packets)   /Forwarding Memory Queue Size (incoming packets)           IP Processor   !bucket/default total/time average   linear   IP Processor   )ip.Forwarding Memory Queue Size (packets)   &Forwarding Memory Queue Size (packets)           IP Processor   !bucket/default total/time average   linear   IP Processor   "ip.Forwarding Memory Queuing Delay   Forwarding Memory Queuing Delay           IP Processor    bucket/default total/sample mean   discrete   IP Processor    ip.Queuing Delay Deviation (sec)   Queue Delay Variation (sec)           IP Interface   sample/default total/   linear   IP Interface   &ip.Background Traffic Flow Delay (sec)   #Background Traffic Flow Delay (sec)           IP    bucket/default total/sample mean   linear   IP   ip.Buffer Usage (bytes)   Buffer Usage (bytes)           IP Interface   !bucket/default total/time average   linear   IP Interface   ip.Buffer Usage (packets)   Buffer Usage (packets)           IP Interface   !bucket/default total/time average   linear   IP Interface   *ip.CAR Incoming Traffic Dropped (bits/sec)   'CAR Incoming Traffic Dropped (bits/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   -ip.CAR Incoming Traffic Dropped (packets/sec)   *CAR Incoming Traffic Dropped (packets/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   *ip.CAR Outgoing Traffic Dropped (bits/sec)   'CAR Outgoing Traffic Dropped (bits/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   -ip.CAR Outgoing Traffic Dropped (packets/sec)   *CAR Outgoing Traffic Dropped (packets/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   ip.Queuing Delay   Queuing Delay           IP Interface    bucket/default total/sample mean   linear   IP Interface   ip.RED Average Queue Size   RED Average Queue Size           IP Interface   !bucket/default total/time average   linear   IP Interface   ip.RSVP Allocated Bandwidth   RSVP Allocated Bandwidth           IP Interface   normal   sample-hold   IP Interface   ip.RSVP Allocated Buffer   RSVP Allocated Buffer           IP Interface   normal   sample-hold   IP Interface   ip.Traffic Dropped (bits/sec)   Traffic Dropped (bits/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface    ip.Traffic Dropped (packets/sec)   Traffic Dropped (packets/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   ip.Traffic Received (bits/sec)   Traffic Received (bits/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   !ip.Traffic Received (packets/sec)   Traffic Received (packets/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   ip.Traffic Sent (bits/sec)   Traffic Sent (bits/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   ip.Traffic Sent (packets/sec)   Traffic Sent (packets/sec)           IP Interface   bucket/default total/sum_time   linear   IP Interface   ip.Maintenance Buffer Size   Maintenance Buffer Size           DSR   normal   discrete   DSR   ip.Number of Hops per Route   Number of Hops per Route           DSR    bucket/default total/sample mean   linear   DSR   ip.Request Table Size   Request Table Size           DSR   normal   discrete   DSR   ip.Route Cache Access Failure   Route Cache Access Failure           DSR   bucket/default total/sum   linear   DSR   ip.Route Cache Access Success   Route Cache Access Success           DSR   bucket/default total/sum   linear   DSR   ip.Route Cache Size   Route Cache Size           DSR   normal   discrete   DSR   ip.Route Discovery Time   Route Discovery Time           DSR    bucket/default total/sample mean   linear   DSR   &ip.Routing Traffic Received (bits/sec)   #Routing Traffic Received (bits/sec)           DSR   bucket/default total/sum_time   linear   DSR   &ip.Routing Traffic Received (pkts/sec)   #Routing Traffic Received (pkts/sec)           DSR   bucket/default total/sum_time   linear   DSR   "ip.Routing Traffic Sent (bits/sec)   Routing Traffic Sent (bits/sec)           DSR   bucket/default total/sum_time   linear   DSR   "ip.Routing Traffic Sent (pkts/sec)   Routing Traffic Sent (pkts/sec)           DSR   bucket/default total/sum_time   linear   DSR   ip.Send Buffer Size   Send Buffer Size           DSR   normal   discrete   DSR   $ip.Total Traffic Received (bits/sec)   !Total Traffic Received (bits/sec)           DSR   bucket/default total/sum_time   linear   DSR   $ip.Total Traffic Received (pkts/sec)   !Total Traffic Received (pkts/sec)           DSR   bucket/default total/sum_time   linear   DSR    ip.Total Traffic Sent (bits/sec)   Total Traffic Sent (bits/sec)           DSR   bucket/default total/sum_time   linear   DSR    ip.Total Traffic Sent (pkts/sec)   Total Traffic Sent (pkts/sec)           DSR   bucket/default total/sum_time   linear   DSR   &ip.Total Acknowledgement Requests Sent   #Total Acknowledgement Requests Sent           DSR   bucket/default total/sum   linear   DSR   ip.Total Acknowledgements Sent   Total Acknowledgements Sent           DSR   bucket/default total/sum   linear   DSR   ip.Total Cached Replies Sent   Total Cached Replies Sent           DSR   bucket/default total/sum   linear   DSR   &ip.Total Non Propagating Requests Sent   #Total Non Propagating Requests Sent           DSR   bucket/default total/sum   linear   DSR   ip.Total Packets Dropped   Total Packets Dropped           DSR   bucket/default total/sum   linear   DSR   ip.Total Packets Salvaged   Total Packets Salvaged           DSR   bucket/default total/sum   linear   DSR   "ip.Total Propagating Requests Sent   Total Propagating Requests Sent           DSR   bucket/default total/sum   linear   DSR   &ip.Total Replies Sent from Destination   #Total Replies Sent from Destination           DSR   bucket/default total/sum   linear   DSR   ip.Total Route Errors Sent   Total Route Errors Sent           DSR   bucket/default total/sum   linear   DSR   ip.Total Route Replies Sent   Total Route Replies Sent           DSR   bucket/default total/sum   linear   DSR   ip.Total Route Requests Sent   Total Route Requests Sent           DSR   bucket/default total/sum   linear   DSR   traf_src.Delay (secs)   Delay (secs)           MANET    bucket/default total/sample mean   linear   MANET   $traf_src.Traffic Received (bits/sec)   Traffic Received (bits/sec)           MANET   bucket/default total/sum_time   linear   MANET   'traf_src.Traffic Received (packets/sec)   Traffic Received (packets/sec)           MANET   bucket/default total/sum_time   linear   MANET    traf_src.Traffic Sent (bits/sec)   Traffic Sent (bits/sec)           MANET   bucket/default total/sum_time   linear   MANET   #traf_src.Traffic Sent (packets/sec)   Traffic Sent (packets/sec)           MANET   bucket/default total/sum_time   linear   MANET   *ip.Dropped Unroutable IP Packet (pkts/sec)   'Dropped Unroutable IP Packet (pkts/sec)           	TORA_IMEP   bucket/default total/sum_time   discrete   	TORA_IMEP   +ip.IMEP Control Traffic Received (bits/sec)   (IMEP Control Traffic Received (bits/sec)           	TORA_IMEP   bucket/default total/sum_time       	TORA_IMEP   +ip.IMEP Control Traffic Received (pkts/sec)   (IMEP Control Traffic Received (pkts/sec)           	TORA_IMEP   bucket/default total/sum_time       	TORA_IMEP   'ip.IMEP Control Traffic Sent (bits/sec)   $IMEP Control Traffic Sent (bits/sec)           	TORA_IMEP   bucket/default total/sum_time       	TORA_IMEP   'ip.IMEP Control Traffic Sent (pkts/sec)   $IMEP Control Traffic Sent (pkts/sec)           	TORA_IMEP   bucket/default total/sum_time       	TORA_IMEP   ip.IMEP Number of Neighbors   IMEP Number of Neighbors           	TORA_IMEP   normal       	TORA_IMEP   ip.IMEP Retransmissions   IMEP Retransmissions           	TORA_IMEP   bucket/default total/sum   discrete   	TORA_IMEP   .ip.IMEP ULP (TORA) Traffic Received (bits/sec)   +IMEP ULP (TORA) Traffic Received (bits/sec)           	TORA_IMEP   bucket/default total/sum_time       	TORA_IMEP   .ip.IMEP ULP (TORA) Traffic Received (pkts/sec)   +IMEP ULP (TORA) Traffic Received (pkts/sec)           	TORA_IMEP   bucket/default total/sum_time       	TORA_IMEP   *ip.IMEP ULP (TORA) Traffic Sent (bits/sec)   'IMEP ULP (TORA) Traffic Sent (bits/sec)           	TORA_IMEP   bucket/default total/sum_time       	TORA_IMEP   *ip.IMEP ULP (TORA) Traffic Sent (pkts/sec)   'IMEP ULP (TORA) Traffic Sent (pkts/sec)           	TORA_IMEP   bucket/default total/sum_time       	TORA_IMEP   ip.Route Discovery Delay   Route Discovery Delay           	TORA_IMEP   normal   discrete   	TORA_IMEP   "ip.Unroutable IP Packet Queue Size   Unroutable IP Packet Queue Size           	TORA_IMEP   bucket/default total/max value   discrete   	TORA_IMEP   ip.Number of Hops per Route   Number of Hops per Route           AODV    bucket/default total/sample mean   linear   AODV   ip.Packet Queue Size   Packet Queue Size           AODV   normal   discrete   AODV   ip.Route Discovery Time   Route Discovery Time           AODV    bucket/default total/sample mean   linear   AODV   ip.Route Table Size   Route Table Size           AODV   normal   discrete   AODV   &ip.Routing Traffic Received (bits/sec)   #Routing Traffic Received (bits/sec)           AODV   bucket/default total/sum_time   linear   AODV   &ip.Routing Traffic Received (pkts/sec)   #Routing Traffic Received (pkts/sec)           AODV   bucket/default total/sum_time   linear   AODV   "ip.Routing Traffic Sent (bits/sec)   Routing Traffic Sent (bits/sec)           AODV   bucket/default total/sum_time   linear   AODV   "ip.Routing Traffic Sent (pkts/sec)   Routing Traffic Sent (pkts/sec)           AODV   bucket/default total/sum_time   linear   AODV   ip.Total Cached Replies Sent   Total Cached Replies Sent           AODV   bucket/default total/sum   linear   AODV   ip.Total Packets Dropped   Total Packets Dropped           AODV   bucket/default total/sum   linear   AODV   &ip.Total Replies Sent from Destination   #Total Replies Sent from Destination           AODV   bucket/default total/sum   linear   AODV   ip.Total Route Errors Sent   Total Route Errors Sent           AODV   bucket/default total/sum   linear   AODV   ip.Total Route Replies Sent   Total Route Replies Sent           AODV   bucket/default total/sum   linear   AODV   ip.Total Route Requests Sent   Total Route Requests Sent           AODV   bucket/default total/sum   linear   AODV   !ip.Total Route Requests Forwarded   Total Route Requests Forwarded           AODV   bucket/default total/max value   linear   AODV   manet_rte_mgr.MPR Status   
MPR Status           OLSR   normal   linear   OLSR   1manet_rte_mgr.Routing Traffic Received (bits/sec)   #Routing Traffic Received (bits/sec)           OLSR   bucket/default total/sum_time   linear   OLSR   1manet_rte_mgr.Routing Traffic Received (pkts/sec)   #Routing Traffic Received (pkts/sec)           OLSR   bucket/default total/sum_time   linear   OLSR   -manet_rte_mgr.Routing Traffic Sent (bits/sec)   Routing Traffic Sent (bits/sec)           OLSR   bucket/default total/sum_time   linear   OLSR   -manet_rte_mgr.Routing Traffic Sent (pkts/sec)   Routing Traffic Sent (pkts/sec)           OLSR   bucket/default total/sum_time   linear   OLSR   'manet_rte_mgr.Total Hello Messages Sent   Total Hello Messages Sent           OLSR   bucket/default total/sum   linear   OLSR   )manet_rte_mgr.Total TC Messages Forwarded   Total TC Messages Forwarded           OLSR   bucket/default total/sum   linear   OLSR   $manet_rte_mgr.Total TC Messages Sent   Total TC Messages Sent           OLSR   bucket/default total/sum   linear   OLSR   CPU.CPU Service Time   CPU Service Time           Server Jobs    bucket/default total/sample mean   linear   Server Jobs   ip.Number of Hops -->   Number of Hops -->           IP    bucket/default total/sample mean   linear   IP   ip.Number of Hops <--   Number of Hops <--           IP    bucket/default total/sample mean   linear   IP   $CPU.Total Jobs Queued Before Startup    Total Jobs Queued Before Startup           Server Jobs   bucket/default total/sum_time   linear   Server Jobs   %CPU.Jobs Queued Before Startup by Job   !Jobs Queued Before Startup by Job           Server Jobs   bucket/default total/sum_time   linear   Server Jobs   CPU.Total Jobs Rejected   Total Jobs Rejected           Server Jobs   bucket/default total/sum_time   linear   Server Jobs   CPU.Jobs Rejected by Job   Jobs Rejected by Job           Server Jobs   bucket/default total/sum_time   linear   Server Jobs   CPU.Startup Queue Size by Job   Startup Queue Size by Job           Server Jobs    bucket/default total/sample mean   linear   Server Jobs   CPU.Total Startup Wait Time   Total Startup Wait Time           Server Jobs    bucket/default total/sample mean   linear   Server Jobs   CPU.Startup Wait Time by Job   Startup Wait Time by Job           Server Jobs    bucket/default total/sample mean   linear   Server Jobs   CPU.Total Startup Queue Size   Total Startup Queue Size           Server Jobs    bucket/default total/sample mean   linear   Server Jobs   (wireless_lan_mac.AC Queue Size (packets)   AC Queue Size (packets)           WLAN (Per HCF Access Category)   !bucket/default total/time average   linear   WLAN (Per HCF Access Category)   :wireless_lan_mac.Data Dropped (Buffer Overflow) (bits/sec)   )Data Dropped (Buffer Overflow) (bits/sec)           WLAN (Per HCF Access Category)   bucket/default total/sum_time   linear   WLAN (Per HCF Access Category)   Cwireless_lan_mac.Data Dropped (Retry Threshold Exceeded) (bits/sec)   2Data Dropped (Retry Threshold Exceeded) (bits/sec)           WLAN (Per HCF Access Category)   bucket/default total/sum_time   linear   WLAN (Per HCF Access Category)   wireless_lan_mac.Delay (sec)   Delay (sec)           WLAN (Per HCF Access Category)    bucket/default total/sample mean   linear   WLAN (Per HCF Access Category)    wireless_lan_mac.Load (bits/sec)   Load (bits/sec)           WLAN (Per HCF Access Category)   bucket/default total/sum_time   linear   WLAN (Per HCF Access Category)   )wireless_lan_mac.Media Access Delay (sec)   Media Access Delay (sec)           WLAN (Per HCF Access Category)    bucket/default total/sample mean   linear   WLAN (Per HCF Access Category)   &wireless_lan_mac.Throughput (bits/sec)   Throughput (bits/sec)           WLAN (Per HCF Access Category)   bucket/default total/sum_time   linear   WLAN (Per HCF Access Category)   %wireless_lan_mac.Queue Size (packets)   Queue Size (packets)           Wireless Lan   !bucket/default total/time average   linear   Wireless Lan   &wireless_lan_mac.Backoff Slots (slots)   Backoff Slots (slots)           Wireless Lan   bucket/default total/sum   linear   Wireless Lan   0wireless_lan_mac.Control Traffic Rcvd (bits/sec)   Control Traffic Rcvd (bits/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   3wireless_lan_mac.Control Traffic Rcvd (packets/sec)   "Control Traffic Rcvd (packets/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   0wireless_lan_mac.Control Traffic Sent (bits/sec)   Control Traffic Sent (bits/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   3wireless_lan_mac.Control Traffic Sent (packets/sec)   "Control Traffic Sent (packets/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   :wireless_lan_mac.Data Dropped (Buffer Overflow) (bits/sec)   )Data Dropped (Buffer Overflow) (bits/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   Cwireless_lan_mac.Data Dropped (Retry Threshold Exceeded) (bits/sec)   2Data Dropped (Retry Threshold Exceeded) (bits/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   -wireless_lan_mac.Data Traffic Rcvd (bits/sec)   Data Traffic Rcvd (bits/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   0wireless_lan_mac.Data Traffic Rcvd (packets/sec)   Data Traffic Rcvd (packets/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   -wireless_lan_mac.Data Traffic Sent (bits/sec)   Data Traffic Sent (bits/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   0wireless_lan_mac.Data Traffic Sent (packets/sec)   Data Traffic Sent (packets/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   #wireless_lan_mac.Load (packets/sec)   Load (packets/sec)           Wireless Lan   bucket/default total/sum_time   linear   Wireless Lan   2wireless_lan_mac.Retransmission Attempts (packets)   !Retransmission Attempts (packets)           Wireless Lan   bucket/default total/sum   linear   Wireless Lan   dhcp.Solicit Message Count   Solicit Message Count           DHCP   normal   discrete   DHCP   'dhcp.Rapid Commit Solicit Message Count   "Rapid Commit Solicit Message Count           DHCP   normal   discrete   DHCP   dhcp.Advertise Message Count   Advertise Message Count           DHCP   normal   discrete   DHCP   dhcp.Request Message Count   Request Message Count           DHCP   normal   discrete   DHCP   dhcp.Reply Message Count   Reply Message Count           DHCP   normal   discrete   DHCP   %dhcp.Rapid Commit Reply Message Count    Rapid Commit Reply Message Count           DHCP   normal   discrete   DHCP   dhcp.Renew Message Count   Renew Message Count           DHCP   normal   discrete   DHCP   dhcp.Rebind Message Count   Rebind Message Count           DHCP   normal   discrete   DHCP   dhcp.Retransmissions   Retransmissions           DHCP   normal   discrete   DHCP   dhcp.Prefixes Assigned   Prefixes Assigned           DHCP   normal   discrete   DHCP   dhcp.Prefix Assignment Renewals   Prefix Assignment Renewals           DHCP   normal   discrete   DHCP   dhcp.Addresses Assigned   Addresses Assigned           DHCP   normal   discrete   DHCP    dhcp.Address Assignment Renewals   Address Assignment Renewals           DHCP   normal   discrete   DHCP   dhcp.Transaction Delay   Transaction Delay           DHCP   normal   linear   DHCP   ip.End-to-end Delay (sec)   End-to-end Delay (sec)           IP    bucket/default total/sample mean   discrete   IP   #ip.End-to-end Delay Variation (sec)    End-to-end Delay Variation (sec)           IP    bucket/default total/sample mean   discrete   IP   &ip.Routing Traffic Received (bits/sec)   #Routing Traffic Received (bits/sec)           GRP   bucket/default total/sum_time   linear   GRP   &ip.Routing Traffic Received (pkts/sec)   #Routing Traffic Received (pkts/sec)           GRP   bucket/default total/sum_time   linear   GRP   "ip.Routing Traffic Sent (bits/sec)   Routing Traffic Sent (bits/sec)           GRP   bucket/default total/sum_time   linear   GRP   "ip.Routing Traffic Sent (pkts/sec)   Routing Traffic Sent (pkts/sec)           GRP   bucket/default total/sum_time   linear   GRP   $ip.Total Traffic Received (bits/sec)   !Total Traffic Received (bits/sec)           GRP   bucket/default total/sum_time   linear   GRP   $ip.Total Traffic Received (pkts/sec)   !Total Traffic Received (pkts/sec)           GRP   bucket/default total/sum_time   linear   GRP    ip.Total Traffic Sent (bits/sec)   Total Traffic Sent (bits/sec)           GRP   bucket/default total/sum_time   linear   GRP    ip.Total Traffic Sent (pkts/sec)   Total Traffic Sent (pkts/sec)           GRP   bucket/default total/sum_time   linear   GRP   ip.Total Number of Backtracks   Total Number of Backtracks           GRP   bucket/default total/sum   linear   GRP   3ip.Packets Dropped (Destination unknown) (bits/sec)   0Packets Dropped (Destination unknown) (bits/sec)           GRP   bucket/default total/sum_time   linear   GRP   5ip.Packets Dropped (No available neighbor) (bits/sec)   2Packets Dropped (No available neighbor) (bits/sec)           GRP   bucket/default total/sum_time   linear   GRP   *ip.Packets Dropped (TTL expiry) (bits/sec)   'Packets Dropped (TTL expiry) (bits/sec)           GRP   bucket/default total/sum_time   linear   GRP   %ip.Packets Dropped (Total) (bits/sec)   "Packets Dropped (Total) (bits/sec)           GRP   bucket/default total/sum_time   linear   GRP          machine type       workstation   Model Attributes      14.5.A-January18-2008                 interface type       
IEEE802.11   interface class       ip      6IP Host Parameters.Interface Information [<n>].Address      
IP Address   :IP Host Parameters.Interface Information [<n>].Subnet Mask      IP Subnet Mask       wlan_port_tx_<n>_0   wlan_port_rx_<n>_0           