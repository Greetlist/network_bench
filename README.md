## Project
Network Traffic Bandwith Throughput Written in C++, inspired by [goben](https://github.com/udhos/goben).

## Compile
In Project Root Directory

```
xmake f -m release
xmake
```

You will get only one Binary, used as Server and Client.

## Usage
Total CLI Args will be shown with `-help`

```
./build/linux/x86_64/release/network_bench -help
```

### Example
Run As Server:
```
./build/linux/x86_64/release/network_bench -listen_addr "0.0.0.0:10000"
```

Run As Client:
```
./build/linux/x86_64/release/network_bench -server_addr "192.168.100.100:10000" -send_rate 2.75M
```
