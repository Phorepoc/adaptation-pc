# Copyright The Phorepoc Authors
# SPDX-License-Identifier: Apache-2.0

"""commdb_xml -- the Path C board's CommsDat configuration (X4.48), for the REAL CED (ced.exe).

    py tools/phorepoc-x86/commdb_xml.py <out.xml>

Source: the Syborg (QEMU) board's own CED file, oss.FCL.sf.incubator.stemkit/commdb/ethernetced.xml
(a full database: networks, IAPs, bearers, services, preferences, global settings). Its "Syborg
Ethernet - Static IP" IAP is exactly this board's shape -- nullagt.agt -> the ethint NIF ->
EtherPkt.drv -> the Ethernet LDD `enet` + a PDD `ethernet` -- so only the board-specific values change:

  * the PDD: Ethernet.Syborg (virtio-net) -> Ethernet.E1000 (ekern/xk_e1000.cpp)
  * the static addresses: the NAT network QEMU (slirp) and VirtualBox ("NAT") both put a guest on,
    10.0.2.15/24, gateway 10.0.2.2, DNS 10.0.2.3 (both forward it to the host's resolver)
  * the file's DHCP IAP ("WinTAP Ethernet - Dynamic IP", the WinTAP emulator bearer) moved onto this
    board's bearer as "Ethernet - DHCP" (X4.49; both NATs run a DHCP server)
  * the TSY: PhoneTsy (the S60 common TSY) instead of MM, in all modem bearers + global settings
  * the connection preferences: DHCP preferred (Ranking 1 = preferred; 0 = not used), no prompt --
    there is no agent dialog server on this board; the static IAP (id 1) stays selectable by id

Everything else is the file as Syborg shipped it. Every substitution must match exactly once,
so a changed source file fails loudly instead of silently producing a different database.
"""
import os
import sys

# the Symbian source: PHOREPOC_SYMBIAN (ph sets it), else the in-tree baseline/ next to tools/
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SYMBIAN = os.environ.get("PHOREPOC_SYMBIAN") or os.path.join(REPO, "baseline")
SRC = os.path.join(SYMBIAN, "symbian-2010", "oss.FCL.sf.incubator.stemkit", "commdb", "ethernetced.xml")

SUBST = [
    ("<PDDName>Ethernet.Syborg</PDDName>", "<PDDName>Ethernet.E1000</PDDName>"),
    # the "Ethernet No Daemon" LANService (the static-IP IAP's service)
    ("<IpGateway>192.168.0.1</IpGateway>\n       <IpAddr>192.168.0.3</IpAddr>",
     "<IpGateway>10.0.2.2</IpGateway>\n       <IpAddr>10.0.2.15</IpAddr>"),
    ("<IpNameServer1>172.16.2.2</IpNameServer1>\n       <IpNameServer2>172.16.2.4</IpNameServer2>",
     "<IpNameServer1>10.0.2.3</IpNameServer1>\n       <IpNameServer2>0.0.0.0</IpNameServer2>"),
    # X4.49: the file's DHCP IAP (LANService "Ethernet": IpAddrFromServer, NetCfgExtnDhcp/!DhcpServ)
    # sits on the emulator's WinTAP bearer -> put it on this board's Ethernet bearer and name it so
    ("<Name>WinTAP Ethernet - Dynamic IP</Name>\n        <IAPService>LANService.Ethernet</IAPService>\n"
     "        <IAPBearer>LANBearer.EKA2 Emulator Ethernet</IAPBearer>",
     "<Name>Ethernet - DHCP</Name>\n        <IAPService>LANService.Ethernet</IAPService>\n"
     "        <IAPBearer>LANBearer.EKA2 Syborg Ethernet</IAPBearer>"),
    # Ranking 1 is the preferred connection (0 = not used). Syborg ranks its static IAP 1 and asks
    # the user (this board has no agent dialog server to ask with): DHCP becomes the preferred one,
    # no prompt, and the static IAP takes the other record (select it by IAP id: nettest's 3rd arg)
    ("<DialogPref>PROMPT</DialogPref>\n        <BearerSet>LAN</BearerSet>\n"
     "        <IAPRef>IAP.Syborg Ethernet - Static IP</IAPRef>",
     "<DialogPref>DONOTPROMPT</DialogPref>\n        <BearerSet>LAN</BearerSet>\n"
     "        <IAPRef>IAP.Ethernet - DHCP</IAPRef>"),
    ("<DialogPref>DONOTPROMPT</DialogPref>\n        <BearerSet>LAN</BearerSet>\n"
     "        <IAPRef>IAP.WinTAP Ethernet - Dynamic IP</IAPRef>",
     "<DialogPref>DONOTPROMPT</DialogPref>\n        <BearerSet>LAN</BearerSet>\n"
     "        <IAPRef>IAP.Syborg Ethernet - Static IP</IAPRef>"),
    # X4.68: the S60 browser connects through a WAP access point (ApEngine: WAPAccessPoint + a
    # WAPIPBearer naming its IAP). Syborg's file has the WAP AP but no bearer record -> link its only
    # WAP AP (id 1) to the DHCP IAP; the browser's board repository (10008d39) selects AP 1. Its NAME
    # stays: another Syborg record references "WAPAccessPoint.Dummy WAP Settings", and a renamed target
    # made CED abort the whole import (no CommsDat -> no networking at all; found 2026-09-25)
    ("        <ProxyPassword>0</ProxyPassword>\n    </WAPIPBearer>\n</WAPIPBearerTable>",
     "        <ProxyPassword>0</ProxyPassword>\n    </WAPIPBearer>\n"
     "    <WAPIPBearer operation=\"add\">\n"
     "        <Name>Phorepoc Internet bearer</Name>\n"
     "        <AccessPoint>WAPAccessPoint.Dummy WAP Settings</AccessPoint>\n"
     "        <GatewayAddress>0.0.0.0</GatewayAddress>\n"
     "        <WSPOption>CONNECTIONORIENTED</WSPOption>\n"
     "        <Security>FALSE</Security>\n"
     "        <IAPRef>IAP.Ethernet - DHCP</IAPRef>\n"
     "        <ProxyPortNumber>0</ProxyPortNumber>\n"
     "    </WAPIPBearer>\n</WAPIPBearerTable>"),
]

# every occurrence (count checked): the TSY. The Syborg file names MM (Symbian's AT-command multimode
# TSY, which its own ROM did not even carry); an S60 device's modem bearers name the common TSY S60 code
# loads by name, PhoneTsy (phonetsy.tsy -> the licensee TSY, X4.52)
SUBST_ALL = [
    ("<TSYName>MM</TSYName>", "<TSYName>PhoneTsy</TSYName>", 15),
    ("<BearerAvailabilityCheckTSY>mm</BearerAvailabilityCheckTSY>",
     "<BearerAvailabilityCheckTSY>PhoneTsy</BearerAvailabilityCheckTSY>", 1),
]


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    text = open(SRC, encoding="utf-8").read().replace("\r\n", "\n")
    for old, new in SUBST:
        n = text.count(old)
        if n != 1:
            raise SystemExit(f"commdb_xml: expected exactly one match, found {n}: {old[:60]!r}")
        text = text.replace(old, new)
    for old, new, want in SUBST_ALL:
        n = text.count(old)
        if n != want:
            raise SystemExit(f"commdb_xml: expected {want} matches, found {n}: {old[:60]!r}")
        text = text.replace(old, new)
    with open(sys.argv[1], "w", encoding="utf-8", newline="\r\n") as fh:
        fh.write(text)
    print(f"commdb_xml: {sys.argv[1]} ({len(SUBST)} board substitutions over the Syborg file)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
