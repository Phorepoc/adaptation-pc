# RSFW remote-drive test fixture

`101F9775.txt` is the Remote Storage Framework's CenRep repository (core `os/config/cenrep/101F9775.txt`, the release defaults) with one
drive configured: **J:** = a WebDAV server at `http://10.0.2.2:8080/` (QEMU's user-mode network gives the host that address), reached over
**IAP 1**, the PC board's static-IP Ethernet IAP (`os/board/commdb_xml.py`: the board has no connection dialog, so the IAP is chosen by id).

Use it in a variant run (`build_env`): `CENREP_EXTRA=/adapt/1/tests/rsfw/101F9775.txt`. The ROM recipe reads the file after the defaults.
`/adapt/1` is this repository's mount in the build container (adaptation-x32 is `/adapt/0`, the order of `board.conf`).
Start a WebDAV server on the host (for example Apache `mod_dav` on port 8080) before booting. Not part of a normal image.
