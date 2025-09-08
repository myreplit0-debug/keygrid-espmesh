# KeyGrid ESP-MESH

This repo boots a minimal ESP-MESH network for an internal BLE tracking system.

- **mesh_root** → designated root/gateway. Prints inbound reports and replies with ACKs.
- **mesh_node** → generic leaf/router. Sends dummy reports (BLE will be added later).
- **components/app_proto** → defines compact binary packet format.
- **components/mesh_transport** → wraps ESP-MESH init/send/recv with a FreeRTOS RX task.

## Build locally
```bash
cd mesh_root
idf.py set-target esp32s3
idf.py build flash monitor
