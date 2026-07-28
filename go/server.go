package main

import (
	"fmt"
	"io"
	"net"
	// "os"
)

func main() {
	// Start a TCP server on port 6379
	// This is a common port for Redis servers
	// TODO: See if there is a need to close the server after use
	listener, err := net.Listen("tcp", ":6379")
	if err != nil {
		fmt.Println("Error starting server:", err)
		return
	}
	// Close the listener when done
	defer listener.Close()
	// Accept connections from clients
	connection, err := listener.Accept()
	if err != nil {
		fmt.Println("Error accepting connection:", err)
		return
	}
	// Close the connection when done
	defer connection.Close()
	for {
		buffer := make([]byte, 1024)
		// Read data from the connection
		_, err := connection.Read(buffer)
		if err != nil {
			if err == io.EOF {
				// Connection closed by client
				fmt.Println("Client disconnected")
				break
			}
			fmt.Println("Error reading from connection:", err)
			return
		}
		connection.Write([]byte("+OK\r\n"))
	}
}
