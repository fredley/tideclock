An Arduino project for building a WiFi-powered tide clock.

Consists of two components: First, a lambda function that scrapes (and caches) the BBC tide tables, and translates this directly to a number of steps starting from High Tide (12 o' clock). Second, an Arduino Nano 33 IOT project that fetches this information and reports the current tide using a stepper motor, updating every 5 minutes.

The dial does not simply vary uniformly between High and Low tide, but takes into account water height (so moves more sinusoidally between high and low).

The Arduino WiFi connection can be re/configured via BLE to save having to reprogram. The stepper motor position can also be manually reset if required.

To set up the 'server-side' component on AWS (you could of course host it yourself elsewhere if suitable, but this setup costs ~$1/mo, or free during first year):

* Create a Lambda function with a python 3 runtime
* Create a Bucket to store the cache
* Make sure the lambda has permissions to write to the bucket
* Create an API Gateway endpoint that backs onto the lambda
* Update the Arduino project with the hostname and endpoint to connect to this

The Arduino project requires the following:

* An Arduino Nano 333 IOT
* A 28BYJ-48 Stepper moter and ULN2003 driver board
* A push to break switch (to reset the stepper) + pulldown resisitor
* An LED

The Arduino operates as follows:
* At boot, it tries to connect to Wifi with stored credentials. If this fails, it enables BLE and the LED starts blinking
* When successfully connected, the LED goes solid, and the SSID and password can be set via the two available characteristics, using a BLE debug app (e.g. nRF Connect for Android)
* Once disconnected, the LED goes out, and the WiFi connection is attempted again
* Once successfully connected, the credentials are stored in Flash memory
* Every 5 minutes, the board connects to the given URL to fetch the number of steps to turn, and updates accordingly
* In order to reset the stepper position (since it has no 'memory', and the Flash memory on the board is unsuitable for many writes), hold the button down until the dial points to High Tide, then release, the clock will then adjust to the correct offset.
