# EcoFlow login-key table

`login_key.bin` (65280 bytes) is a **public static table** from the EcoFlow
mobile app crypto path. It is **not** derived from your account, serial, or
user id.

- Upstream decoder: [rabits/ef-ble-reverse `util/login_key_gen.py`](https://github.com/rabits/ef-ble-reverse/blob/master/util/login_key_gen.py)
  (no CLI args / env vars — base64 blob only). Run that util and copy the
  resulting `login_key.bin` here if you ever need to refresh the table.
- Build embed: `scripts/embed_ecoflow_login_key.py` reads this file and emits
  gitignored `EcoFlowLoginKeyData.gen.cpp`.

Runtime BLE auth still needs device env vars: `ECOFLOW_BLE_ADDRESS`,
`ECOFLOW_SERIAL`, `ECOFLOW_USER_ID` (MD5 token).
