# RSFW access-point test fixture

`101F9775.txt` is `tests/rsfw/101F9775.txt` with four drives, three to the same WebDAV server (`http://10.0.2.2:8080/`) and one https, all on
the board's DHCP connection (the "Internet" destination's "Ethernet - DHCP" IAP). The board has one Ethernet interface, which one IAP holds at a time: a drive
that starts a different IAP after another one is up fails (`Connection starting returned -1`, "(Not connected)"), so the rows must not mix the static IAP 1 with
the DHCP ones.

| Drive | Name | Iap | Meaning |
|---|---|---|---|
| J: | jdav | `*` | the default connection preferences (a `TCommDbConnPref` with no IAP: the preferred DHCP IAP) |
| K: | kdav | `*` | the same |
| L: | ldav | `?` | ask the user (the page's "Always ask"): the Select connection dialog, choose Internet |
| M: | sdav | `*` | `https://dav.box.com/dav/` (box.com stopped serving WebDAV in 2023: the TLS handshake and the trust prompt are what shows) |

Variant run: `build_env` `CENREP_EXTRA=/adapt/1/tests/rsfw-iap/101F9775.txt`. In File Manager open each drive and Connect; the WebDAV
server's access log shows which ones reach it.
