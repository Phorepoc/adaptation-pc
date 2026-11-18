# RSFW access-point test fixture

`101F9775.txt` is `tests/rsfw/101F9775.txt` with three drives to the same WebDAV server (`http://10.0.2.2:8080/`), one for each
access-point choice the Remote Storage Framework knows (`CRsfwConnectionManager::UseIapL`):

| Drive | Name | Iap | Meaning |
|---|---|---|---|
| J: | jdav | `1` | the IAP by id: the PC board's static Ethernet IAP (known to work) |
| K: | kdav | `*` | the default connection preferences (a `TCommDbConnPref` with no IAP) |
| L: | ldav | `?` | ask the user (the page's "Always ask"; the board has no connection dialog) |
| M: | sdav | `1` | `https://dav.box.com/dav/` over IAP 1: answers 401 without credentials, so a 401 in the engine's trace proves the TLS handshake (the OS TLS provider, the tls layer's ssl.dll) |

Variant run: `build_env` `CENREP_EXTRA=/adapt/1/tests/rsfw-iap/101F9775.txt`. In File Manager open each drive and Connect; the WebDAV
server's access log shows which ones reach it.
