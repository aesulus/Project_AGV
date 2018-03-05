Project AGV
===========

``` Note: This guide is intended for use only as an introduction. It is NOT a substitue for reading the full Marvelmind documentation. ```

Description
-----------
Project AGV is a firmware program written in C++ for the Arduino Uno and the Marvelmind platforms. This program was developed for and tested on RC car pictured below; however, it may be modified to suit the needs of the user.
![The barebones car with 9V battery](/images/car_barebones.jpg)

### Version
This code was last run and successfully tested on:
* Marvelmind Dashboard SW v5.43.0.2518
* Marvelmind Modem firmware version 2017_08_06_modem_hw45_sw5_72_r433MHz_f554bd2
* Marvelmind Beacon firmware version 2017_08_06_beacon_hw45_sw5_72_r433MHz_0ee171c_full

**Always check to make sure compatibility is maintained if using future versions of the above.**

Wireless communication
----------------------
The presence of objects other than air in between any two beacons WILL distort the time of flight for each ultrasonic pulse,
leading to erroneous results.
![Marvelmind Information Flow Diagram](/images/marvelmind_info_flow_diagram.jpg)

Motor control/Steering
----------------------
The RC car comes with three motors: the front and rear axles are each driven by one DC motor, and the third DC motor drives the steering.
All three motors are controlled by an [L293D](http://www.ti.com/lit/ds/symlink/l293.pdf), a 2-channel H-bridge. One channel controls both axle motors, and the other controls the steering. Due to the way the steering was designed on this car, the car can only steer hard left, hard right, or align the wheels in the neutral position. The turning radius of the car is **very large**. How large, exactly? [You may calculate it yourself.](http://www.davdata.nl/math/turning_radius.html)

Wiring
------
*(to be added)*
![The car wiring](/images/car_wiring.jpg)

Resources
---------
* [Trilateration](https://www.youtube.com/watch?v=4O3ZVHVFhes&ab_channel=unfa)
* [Marvelmind Downloads Page](https://marvelmind.com/download/)
* [Marvelmind Help Page (a.k.a. their forums)](https://marvelmind.com/forum/viewforum.php?f=2&sid=1e9c4a5210b932fe14f09fd02badf70d)
* [Serial Communication](https://learn.sparkfun.com/tutorials/serial-communication)
* [State Machines](https://en.wikipedia.org/wiki/Finite-state_machine_)
  * [Moore and Mealy Machines](https://www.tutorialspoint.com/automata_theory/moore_and_mealy_machines.htm)
* [L293D Datasheet](http://www.ti.com/lit/ds/symlink/l293.pdf)
