## Readme - 

### Rev 3 prototype

* http://www.i4innovation.co.uk/ enclosure?
* Need to validate MOSFET direct digital synthesis to test LD at 24V Sin to check if alu box is a go-er? 

### Rev 2 prototype

Hardware changes:

* Filters on AI's
* Voltage sampling of 24V
* Just one resistor but fed via dodes 
* PCB LED indication sorted with diodes - lights up in AB pairs now
* Lower test current draw, better heat dissipation
* Smaller, ligher, can be used on a vehicle
* 15-24V power supply range tested
* Uses DCDC PSU
* Hammond extruded alu box

Software changes:

* Faster terminal update, terminal is interactive now
* PSU voltage compensaton
* General refinements anx improvements


Andrew Wrigley

* Still have issues with both cards high failure rates
* Very interested as testing is difficult for that board
* Not effects of variable power supply
* Doesn't know history of boards lying around
* Mentions boards failed out of the box

Matt Knight

* Interested since this is still a cost to him
* Getting spares difficult
* Competetors offering easier to test solutions
 
Tom Mattinson

* Would want one for each board type

### Rev 1 prototype

* Analog sampling seems unreliable
* No voltage compensation
* Can't use on a vehicle due to bulk
* Only button based interaction

### Rev 0 prototype

Wire up as arduino follows:

| Arduino | RS485 module |
|-----|----|----|
| 5V | 5V |
| 0V | 0V |
| 10 | TX-O |
| 11 | RX- I |
| 12 | RTSA |

Wire MVP as:

| MVP | To |
|-----|----|----|
| 1 (Top) | A |
| 2 | B |
| 3 | NC |
| 4 | 0V |
| 5 | 24V |
| 6 | 24V |


![](https://github.com/lawsonkeith/MVPTest/blob/master/MVPComms/DSC_0385.JPG)
