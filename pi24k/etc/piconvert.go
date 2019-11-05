package main

import (
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"
)

const ketaMax = 24575

var htmlData string
var pos int

func main() {
	downloadPiHTML()
	fmt.Print("#define KETA_MAX ")
	fmt.Println(ketaMax)
	fmt.Println("PROGMEM static const uint8_t pi24k[] = {")
	for keta := ketaMax; keta >= 0; keta -= 36 {
		fmt.Print("    ")
		for i := 0; i < 3; i++ {
			v0 := getPiValue() + getPiValue()*10 + getPiValue()*100
			v1 := getPiValue() + getPiValue()*10 + getPiValue()*100
			v2 := getPiValue() + getPiValue()*10 + getPiValue()*100
			v3 := getPiValue() + getPiValue()*10 + getPiValue()*100
			b0 := v0 & 0xFF
			b1 := (v0>>8 | v1<<2) & 0xFF
			b2 := (v1>>6 | v2<<4) & 0xFF
			b3 := (v2>>4 | v3<<6) & 0xFF
			b4 := v3 >> 2 & 0xFF
			fmt.Printf("0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X,", b0, b1, b2, b3, b4)
			if i < 2 {
				fmt.Print(" ")
			}
		}
		fmt.Println()
	}
	fmt.Println("};")
}

func downloadPiHTML() {
	url := "http://www.tstcl.jp/ja/randd/pi.php"
	resp, err := http.Get(url)
	if err == nil {
		defer resp.Body.Close()
		bytes, _ := ioutil.ReadAll(resp.Body)
		htmlData = string(bytes)
		pos = strings.Index(htmlData, "<pre>")
	}
}

func getPiValue() int {
	var digit byte
	for {
		digit = htmlData[pos]
		pos++
		if digit >= '0' && digit <= '9' {
			break
		}
	}
	return int(digit - '0')
}
