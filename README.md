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

[`ground.ino`](ground/ground.ino)

The ground station is essentially a slimmed down version of the Rover, meant to
be teathered to a computer via Serial port.

* Arduino Duo
* RFM69 Transceiver

### CiC

[`README.md`](libraries/CiC)

The flight control system framework that powers both the Rover and Ground is a
bespoke C library providing capabilities that I deem necessary. Much of my focus
has been on telemetry and debuggability of an increasingly sophisticated
software stack. I mean, on Mars who is going plug into the Serial port and hit
the reset button for you?

## Usage

### Tests

Automated testing is currently limited to CiC library code. See more [here](libraries/CiC#testing)

### Build & Upload

Rather than rely on the Arduino IDE, scripts powered by `arduino-cli` are available. Wrapper scripts ([Scripts-To-Rule-Them-All](https://github.com/github/scripts-to-rule-them-all)) is available)

* [`rover/script/build](rover/script/build)
* [`rover/script/upload](rover/script/upload)
* [`ground/script/build](ground/script/build)
* [`ground/script/upload](ground/script/upload)
