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
