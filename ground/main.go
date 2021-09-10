package main

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"time"

	"github.com/gdamore/tcell/v2"
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

func fillFromFile(w io.Writer, name string) error {
	f, err := os.Open("main.go")
	if err != nil {
		return err
	}
	defer f.Close()

	_, err = io.Copy(w, f)
	if err != nil {
		return err
	}
	return nil
}

func runCommand(cmd string) error {
	conn, err := net.Dial("unix", "/var/stubborn/comsock")
	if err != nil {
		return errors.Wrap(err, "failed to dial")
	}

	defer conn.Close()

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
		log.Println(string(b[0:n]))
	} else {
		log.Println("empty response")
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

	mb := myBox{tv: txt}
	txt.SetInputCapture(mb.boxCapture)

	table := tview.NewTable().SetBorders(false)
	table.Box.SetBorder(true)
	table.Box.SetTitle("TELEMETRY")
	metricData := []metricData{
		{Label: "Mis.T", Value: fmt.Sprintf("%v", 3875*time.Second)},
		{Label: "RSSI", Value: "10db"},
		{Label: "COM SEQ", Value: "13"},
		{Label: "MOTOR", Value: "128 / 128"},
		{Label: "BATT A", Value: "4.89V"},
	}

	for n, md := range metricData {
		table.
			SetCell(n, 0, tview.NewTableCell(md.Label).SetTextColor(tcell.ColorBisque)).
			SetCell(n, 1, tview.NewTableCell(md.Value).
				SetTextColor(tcell.ColorAquaMarine).
				SetAlign(tview.AlignRight))
	}

	inputField := tview.NewInputField().
		SetLabel("Command: ").
		SetFieldWidth(32)
		//SetAcceptanceFunc(tview.InputFieldInteger).

	inputField.SetDoneFunc(func(key tcell.Key) {
		cmd := inputField.GetText()

		if key == tcell.KeyEnter {
			log.Printf("Running: %s\n", inputField.GetText())

			go func() {
				err := runCommand(cmd)
				if err != nil {
					log.Println("failed running command: ", err)
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
