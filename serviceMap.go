package main

import (
    "fmt"
    "net"
    "os"
    "strings"
)

// Global variable used to store the service name together with IP and port
var serv_table = make(map[string]string)

func CheckError(err error) {
    if err != nil {
        fmt.Println("Error occurred:", err)
        os.Exit(0)
    }
}

// GO routine to handle incoming UDP messages
func HandleMsg(ServerCLNS *net.UDPConn, adr *net.UDPAddr, buf string) {
    fmt.Println("Received from ", adr.IP, ": ", buf)
    tokens := strings.Split(buf, " ")

    // Handling PUT request
    if tokens[0] == "PUT" {
        serv_table["server"] = adr.IP.String() + ":" + tokens[2]
		//fmt.Println(serv_table["server"])
        reply := "OK"
		//fmt.Println("GET request received sending ",serv_table["server"],adr)
        ServerCLNS.WriteToUDP([]byte(reply), adr)
    }

    // Handling the GET request
    if tokens[0] == "GET" {
		//fmt.Println("GET request received sending ",serv_table["server"],addr)
		ServerCLNS.WriteToUDP([]byte(serv_table["server"]), adr)	
    }	
}

func main() {
    ServerAdr, err := net.ResolveUDPAddr("udp", ":14585")
    CheckError(err)

    ServerCLNS, err := net.ListenUDP("udp", ServerAdr)
    CheckError(err)
    defer ServerCLNS.Close()

    buf := make([]byte, 1024)

    for {
        n, adr, err := ServerCLNS.ReadFromUDP(buf)
        go HandleMsg(ServerCLNS, adr, string(buf[0:n]))
        if err != nil {
            fmt.Println("Error:", err)
        }
    }
}
