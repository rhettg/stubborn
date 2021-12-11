package main

import (
	"log"
	"net/http"
)

func main() {
	// start a webserver that serves a file from disk
	http.Handle("/", http.StripPrefix("/", http.FileServer(http.Dir("/var/stubborn/"))))

	log.Fatal(http.ListenAndServe(":8080", nil))

}
