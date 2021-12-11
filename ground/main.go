package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net"
	"strings"
	"time"

	"github.com/gdamore/tcell/v2"
	"github.com/hpcloud/tail"
	"github.com/pkg/errors"
	"github.com/rivo/tview"
)

type myBox struct {
	tv *tview.TextView
}

func (mb *myBox) boxCapture(event *tcell.EventKey) *tcell.EventKey {
	log.Printf("%s: %v (%v)", event.Name(), event.Key(), event.Rune())
	mb.tv.ScrollToEnd()

	return event
}

func runCommand(cmd string) error {
	conn, err := net.Dial("unix", "/var/stubborn/comsock")
	if err != nil {
		return errors.Wrap(err, "failed to dial")
	}

	defer conn.Close()

	log.Printf("-> %s\n", cmd)

	_, err = conn.Write([]byte(cmd))
	if err != nil {
		return errors.Wrap(err, "failed to write")
	}

	var b [1024]byte
	n, err := conn.Read(b[:])
	if err != nil {
		return errors.Wrap(err, "failed to read")
	}

	if n > 0 {
		log.Printf("<- %s\n", strings.TrimSpace(string(b[0:n])))
	} else {
		log.Println("<- <empty response>")
	}

	return nil
}

func newPrimitive(text string) tview.Primitive {
	return tview.NewTextView().
		SetTextAlign(tview.AlignCenter).
		SetText(text)
}

type metricData struct {
	Label string
	Value string
}

type telemetryData struct {
	NOW     int
	UP      int
	LOOP    int
	COM     int
	RSSI    int
	ERR     int
	MOTORA  int
	MOTORB  int
	CAM_LEN int
	IMPACT  int
}

func updateMetrics(a *tview.Application, t *tview.Table, m []metricData) {
	a.QueueUpdateDraw(func() {
		for n, md := range m {
			t.
				SetCell(n, 0, tview.NewTableCell(md.Label).SetTextColor(tcell.ColorBisque)).
				SetCell(n, 1, tview.NewTableCell(md.Value).
					SetTextColor(tcell.ColorAquaMarine).
					SetAlign(tview.AlignRight))
		}
	})
}

func newMetrics(td telemetryData) []metricData {
	metricData := []metricData{
		{Label: "Up Time", Value: fmt.Sprintf("%v", time.Duration(td.UP)*time.Second)},
		{Label: "Error", Value: fmt.Sprintf("%d", td.ERR)},
		{Label: "RSSI", Value: fmt.Sprintf("%d", td.RSSI)},
		{Label: "COM Seq", Value: fmt.Sprintf("%d", td.COM)},
		{Label: "MOTOR", Value: fmt.Sprintf("%d / %d", td.MOTORA, td.MOTORB)},
		{Label: "Cam Len", Value: fmt.Sprintf("%dB", td.CAM_LEN)},
		{Label: "Last Loop", Value: fmt.Sprintf("%dms", td.LOOP)},
		{Label: "Impact", Value: fmt.Sprintf("%d", td.IMPACT)},
	}

	return metricData
}

func runMetrics(a *tview.Application, tbl *tview.Table) {
	t, err := tail.TailFile("/var/stubborn/to.json", tail.Config{Location: &tail.SeekInfo{Offset: 0, Whence: 2}, Follow: true})
	if err != nil {
		log.Println("failed opening to.json:", err)
		return
	}

	hasImpact := false
	imgAvailable := false
	moving := false

	td := telemetryData{}

	for line := range t.Lines {
		//log.Println(line.Text)

		err = json.Unmarshal([]byte(line.Text), &td)
		if err != nil {
			log.Println("error parsing telemetry:", err)
			continue
		}

		m := newMetrics(td)
		updateMetrics(a, tbl, m)

		if !hasImpact && td.IMPACT == 1 {
			log.Println("IMPACT detected")
			hasImpact = true
		} else if td.IMPACT == 0 {
			hasImpact = false
		}

		if !imgAvailable && td.CAM_LEN > 0 {
			log.Println("CAM Image downloading")
			imgAvailable = true
		} else if imgAvailable && td.CAM_LEN == 0 {
			imgAvailable = false
			log.Println("CAM Image downloaded")
		}

		if !moving && td.MOTORA != 0 || td.MOTORB != 0 {
			moving = true
			log.Println(fmt.Sprintf("Motors engaged %d / %d", td.MOTORA, td.MOTORB))
		} else if moving && td.MOTORA == 0 && td.MOTORB == 0 {
			log.Println("Motors stopped")
			moving = false
		}
	}
}

func main() {
	app := tview.NewApplication()
	//box := tview.NewBox().SetBorder(true).SetTitle("Hello, world!")

	txt := tview.NewTextView().
		SetChangedFunc(func() { app.Draw() })
	txt.SetBorder(true).
		SetTitle("LOGS")

	log.Default().SetOutput(txt)
	log.Println("Ground Station V2 Initialized")

	/*
		err := fillFromFile(txt, "main.go")
		if err != nil {
			panic(err)
		}
	*/

	go func() {
	}()

	mb := myBox{tv: txt}
	txt.SetInputCapture(mb.boxCapture)

	table := tview.NewTable().SetBorders(false)
	table.Box.SetBorder(true)
	table.Box.SetTitle("TELEMETRY")

	go runMetrics(app, table)

	inputField := tview.NewInputField().
		SetLabel("Command: ").
		SetFieldWidth(32)
		//SetAcceptanceFunc(tview.InputFieldInteger).

	inputField.SetDoneFunc(func(key tcell.Key) {
		cmd := inputField.GetText()

		if key == tcell.KeyEnter {

			go func() {
				err := runCommand(cmd)
				if err != nil {
					log.Println("failed running command: ", err)
					return
				}
			}()
		}

		inputField.SetText("")
	})

	grid := tview.NewGrid().
		SetRows(1, 10, 1, 0).
		SetColumns(50, 0).
		//SetBorders(true).
		AddItem(newPrimitive("Stubborn GDS"), 0, 0, 1, 2, 0, 0, false).
		AddItem(table, 1, 0, 1, 1, 0, 0, false).
		AddItem(txt, 1, 1, 3, 1, 0, 0, false)
	grid.AddItem(inputField, 2, 0, 1, 1, 0, 0, true)

	if err := app.SetRoot(grid, true).Run(); err != nil {
		panic(err)
	}
}
