## 目的
在实际应用当中，有些应用层希望能够查询到内核保存的conntrack记录，项目提供一种解决这一问题的方案以供参考。

## 思路
在应用层数据和传输层之间插入协议，该协议包含有conntrack记录，需要修改用户层读取数据的部分，先将inject_conntrack.ko插入的协议进行解码，然后再进行数据读取。

## 运行
Linux环境下

```
# 安装模块
make && make install

# 卸载模块
make uninstall

```

## example
这里提供一个用户层golang程序读取conntrack记录的示例。

```
package main

import (
	"fmt"
	"log"
	"net"
)

func main() {
	listener, err := net.Listen("tcp", ":9876")
	if err != nil {
		log.Println(err)
		return
	}

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Println(err)
			break
		}

		go handle(conn)
	}
}

func handle(conn net.Conn) {
	defer conn.Close()

	header := make([]byte, 8)
	for {
		nr, err := conn.Read(header)
		if err != nil {
			log.Println(err)
			break
		}

		if nr != 8 {
			log.Println("header length no match")
			continue
		}

		originDst := fmt.Sprintf("%d.%d.%d.%d", header[0], header[1], header[2], header[3])
		originDstPort := int(header[4]) + int(header[5])<<8
		payloadlength := int(header[6]) + int(header[7])<<8

		fmt.Printf("%s:%d, payload: %d\n", originDst, originDstPort, payloadlength)

		payload := make([]byte, payloadlength)
		nr, err = conn.Read(payload)
		if err != nil {
			log.Println("read payload", err)
			break
		}

		log.Println(string(payload))
	}
}

```

为了运行此程序，需要在iptables加入dnat的规则

```

go build -o test_client test.go

nohup ./test_client &

iptables -t nat -I OUTPUT -p tcp --dst 120.25.214.66 -j DNAT --to 192.168.1.102:9987

curl http://120.25.214.66

tailf nohup.out

```