package main

import (
	"fmt"
	"math/rand"
	"strings"
	"time"
)

type CPSPreset string

const (
	PresetQUIC     CPSPreset = "QUIC Initial"
	PresetDNS      CPSPreset = "DNS-over-UDP"
	PresetSIP      CPSPreset = "SIP"
	PresetTelegram CPSPreset = "Telegram-like"
	PresetRussia   CPSPreset = "RF-2026"
	PresetIran     CPSPreset = "Iran"
)

func GenerateCPS(preset CPSPreset) string {
	rand.Seed(time.Now().UnixNano())
	tags := []string{
		fmt.Sprintf("<b 0x%08x>", rand.Uint32()&0xC7FFFFFF),
		"<t>",
		fmt.Sprintf("<r %d>", 20+rand.Intn(80)),
		fmt.Sprintf("<rc %d>", 8+rand.Intn(16)),
	}
	if rand.Intn(2) == 0 {
		tags = append(tags, "<rd 4>")
	}
	return strings.Join(tags, "")
}

func main() {
	fmt.Println("=== StealthAmnezia CPS Generator ===")
	for _, p := range []CPSPreset{PresetQUIC, PresetDNS, PresetRussia, PresetIran} {
		fmt.Printf("%s → %s\n", p, GenerateCPS(p))
	}
}