# A Stubborn Pursuit of a Path

This is a hobby-scale ([Yak](https://www.yakcollective.org/projects/yak-rover/))
Rover project for exploring cheap ways of building Mars ready exploration
platforms.

... or it's just a little Arduino powered robot that runs around the floor of my garage.

> Many are stubborn in pursuit of the path they have chosen, few in pursuit of the goal.
>
> - Friedrich Nietzsche

You say that like it's a bad thing.

Primarily this is an exploration of whatever I find interesting in the embedded systems space.
It's a chance to catch up on some education I've missed and learn some new technologies.

The development process is:

1. Build something minimal but interesting.
1. See where it breaks
1. Goto 1.

### Yak Rover

As part of the [Yak Rover](https://www.yakcollective.org/projects/yak-rover/) project,
Stubborn borrows and (hopefully) contributes to a larger effort to develop
technology for an open-source Mars rover.

## Components

### CiC

[`README.md`](libraries/CiC)

The flight control system framework that powers both the Rover and Ground is a
bespoke C library providing capabilities that I deem necessary. Much of my focus
has been on telemetry and debuggability of an increasingly sophisticated
software stack. I mean, on Mars who is going plug into the Serial port and hit
the reset button for you?

### Rover

[`rover.ino`](rover/rover.ino)

The Stubborn Rover itself today is a simple beast of easy to assemble off the shelf-components. This first
iteration is based on the parts found in an [Elegoo Most Complete Starter
Kit](https://www.amazon.com/EL-KIT-008-Project-Complete-Ultimate-TUTORIAL) with
some minimal additions.

* Arduino Duo
* L293D Dual H-Bridge Motor Controller
* Shock Switch
* Powerboost 1000 / 3.7 V Battery
* RFM69 Transceiver
* Adafruit Mini Robot Chassis Kit

There is of course a mess of loose wires to round all this out.

The plan is to find the shortcomings from this simple setup and iterate.

### Ground

To communicate with the Rover, a ground station is needed. This is built around:

* Raspberry Pi 4
* RFM69 Transceiver (identical to the Rover's)

The software stack is built around several sub-components organized as Unixy as
possible. These components ideally handle a single task and are easily
replacable as mission demands evolve.

#### rfm69

[`rfm69`](rfm69/bin/rfm69)

This python script uses CircuitPython to interface with the rfm69 transceiver
hardware. It presents a unix domain socket on the system `/var/stubborn/radio`
which other processes can use to send and receive data over the radio. 

Bytes written to the socket are sent directly over the radio (and hopefully
received by the Rover).

Bytes recieved are sent to to the listening client.

Only a single client can connect at a time.

#### comsock

This C program is built on the same CiC stack as the rover and provides a
high-level communication interface to the Rover.

Client connect to comsock over a unix domain socket (`/var/stubborn/comsock`)
and send commands to the rover such as:

    FWD 255

The server will send this command to the Rover and display the result:

    OK

Or in the event of an error:

    ERR 3

The comsock service also receives telemetry data from the Rover. It's writen in
two formats.

`/var/stubborn/to` is a tab-delimited data file in the format:

    NOW=2403777634	UP=167495	LOOP=0	COM=1	RSSI=229	ERR=2

`/var/stubborn/to.json` is JSON formatted:

    {"NOW": 1632030343, "UP": 19252, "LOOP": 0, "RSSI": 229, "COM": 2}

#### Ground Data System (gds)

The Ground Data System is a user interface for the ground station that uses the
underlying components to receive commands from the user and display telemetry
about the Rover's operation.

It's written in Go in uses the `tview` library to present a fun terminal-based UI.

## Usage

### Tests

Automated testing is currently limited to CiC library code. See more [here](libraries/CiC#testing)

### Build & Upload

Rather than rely on the Arduino IDE, scripts powered by `arduino-cli` are available. Wrapper scripts ([Scripts-To-Rule-Them-All](https://github.com/github/scripts-to-rule-them-all)) is available)

* [`rover/script/build`](rover/script/build)
* [`rover/script/upload`](rover/script/upload)

### Ground Infrastructure

Ground station services are managed using systemd. Unit files are for radio and comsock services.

gds can be run directly using `go`:

    go run main.go

This may change as this component increases in complexity.