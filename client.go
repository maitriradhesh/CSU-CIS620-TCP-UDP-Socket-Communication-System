package main

import (
    "bufio"
    "fmt"
    "net"
    "os"
    "strconv"
    "strings"
    "time"
)

func main() {

    //Resolving UDP Connection Local Address
    localAdr, err := net.ResolveUDPAddr("udp", "0.0.0.0:0") // Use 0.0.0.0:0 pattern to let the system pick any available address and port
    if err != nil {
        fmt.Println("Error in resolving the local address:", err)
        return
    }

    // Resolving broadcast address
    broadcastAdr, err := net.ResolveUDPAddr("udp", "137.148.205.255:14585")
    if err != nil {
        fmt.Println("Error in resolving the broadcast address:", err)
        return
    }

    // Listening on UDP port
    conn, err := net.ListenUDP("udp", localAdr)
    if err != nil {
        fmt.Println("Error listening on UDP port:", err)
        return
    }
    defer conn.Close()

    // Broadcasting a UDP message
    msg := []byte("GET BANK620 ")
    _, err = conn.WriteToUDP(msg, broadcastAdr)
    if err != nil {
        fmt.Println("Error broadcasting message:", err)
        return
    }

    // Setting a time limit for reading 
    err = conn.SetReadDeadline(time.Now().Add(5 * time.Second))
    if err != nil {
        fmt.Println("Error setting read time limit:", err)
        return
    }

    // Read reply from UDP
    reply := make([]byte, 50)
    n, addr, err := conn.ReadFromUDP(reply)
    if err != nil {
        fmt.Println("Error reading the UDP reply:", addr, err)
        return
    }

    // Converting reply to IP and port number
    tokens := strings.Split(string(reply[:n-1]), ":")
    ip := tokens[0] 
    port, err := strconv.Atoi(tokens[1])
    if err != nil {
        fmt.Println("Error in conversion of IP and port:", err)
        return
    }

    fmt.Printf("Service provided by %s at port %d\n", ip, port)

    // Establish TCP connection with the server
    servAdr := fmt.Sprintf("%s:%d", ip, port)
    tcp_conn, err := net.Dial("tcp", servAdr)
    if err != nil {
        fmt.Println("Error establishing TCP connection:", err)
        return
    }
    defer tcp_conn.Close()

    // Create a scanner to read user input from command line
    reader := bufio.NewReader(os.Stdin)

    for {
        fmt.Print("> ")
        cmdline,_:=reader.ReadString('\n')
        _, err = tcp_conn.Write([]byte(cmdline))
        res := make([]byte, 200)
        _, err = tcp_conn.Read(res)
        if err != nil {
            fmt.Println("Error listening to message from server:", err)
            return
        }
        fmt.Println(string(res))
    }
}
