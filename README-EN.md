## Purpose
provide a solution for querying kernel conntrack record.

## implement
inject_conntrack.ko inject a protocol that contains the conntrack between application layer and transport layer。application program need to decode protoco and them read application data.

## Run

```
# install
make && make install

# uninstall
make install 

```

## example
provided a test.go for read conntrack in userland

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

insert iptables rules to run this example

```

go build -o test_client test.go

nohup ./test_client &

iptables -t nat -I OUTPUT -p tcp --dst 120.25.214.66 -j DNAT --to 192.168.1.102:9876

curl http://120.25.214.66

tailf nohup.out

```