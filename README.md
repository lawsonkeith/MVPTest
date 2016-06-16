## Readme - 

Use #ifdef prototype to define target configuration.

### Rev 2 prototype

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
