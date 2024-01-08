# Wireless River Gauge
This is a project for creating a module that can measure the height of a river to give up-to-date information to kayakers. Its still a work in progress but I hope to get some up and running in the next few months.

## Goals
- Low cost to allow for these to be deployed in as many places as possible. Ideally under €50.
- Updates as regularly as possible. Ideally every 15 minutes.
- Small and discreet to prevent vandalism
- Runs off solar power
- Ability to run for a few days in poor weather
- Easy for anyone to build their own
- Low recurring costs, eg: SIM plans, data hosting, etc
- Easy integration into existing platforms for viewing river data.

## Discription 

The module is controlled using a D1 mini ESP8266. (A clone to save costs) This module is super compact and has really good deep sleep capabilities. It also operates on 3.3V which is great when using 3.7v li-ion batteries.

For the sensor, I'm currently using an aj-sr04m ultrasonic sensor. These are great for the project as they are water-resistant and cost less than €5 when sourced from China. They have the ability to run in a low-current sleep mode and have an identical operation to the popular HC-SR04 sensors. They have a range of between 20cm and 600cm which covers basically all Irish rivers.

Data is sent from the module using a sim module. Currently, I'm using a 2G, GSM SIM800L module from SimCOM on a breakout board. (Small red one, doesn't seem to have a name) This module is reasonably quick to connect to the network and also has decent power-saving modes. There's also a super handy ability for it to report back its input voltage which lets us figure out the battery level and report it with the level data.
However, this probably isn't a permanent solution. The module uses a significant amount of current when in operation (up to 2A during transmission bursts) and it only uses 2G networks. 2G networks have already shut down in many countries and it might only be a matter of time before Ireland does the same. SimCOM offers some equivalent modules that might do the trick. I haven't gotten around to testing any yet.

For power, I'm using an 80x80mm 6V 150mA 0.9W solar panel. This is connected to a TP4056 charging module which charges up a 2000mAh 18650 Li-ion battery. The Li-ion battery powers the SIM module directly. The D1 Mini and the ultrasonic sensor require 3.3V which requires the use of a voltage regulator. (HT7333) I'm also using 1000uF and 100nf capacitors with this to ensure a clean output voltage. 
I'm still running tests with the current setup to figure out if it is optimal. From current estimations, the solar panel should be just enough to keep the battery topped up and the battery should last for 4 days of 15-minute interval readings before dying. Solar cells and batteries don't perform their best in cloudy and cold conditions so the current setup may need to be upgraded to handle long periods of poor conditions. (ideal paddling weather unfortunately)

The module will be installed in a water-resistant 115x90x50mm case. This should provide adequate protection from the elements. Mounting the module will be a challenge. It will need to be located in a place where the river isn't too wide so it can measure the most accurate level that will reflect conditions best. (In places where there are lots of small waves on the surface, this might cause inconsistent measurements) Mounting to trees may also cause issues in summer when their leaves block the solar panel from receiving maximum light. One possible solution might be to mount it on a wire strung between trees on either bank although this goes against the goal of discretion. I'll be trying to stick it on a tree branch first and seeing how that works out.

### Sim Card
There are a good few options out there for IoT SIM cards. I'm using Simbase for mine. The card costs €5 to buy and it costs about €4 per year to operate. 1NCE also produce an affordable SIM with a €12 once-off cost to run it or about 10 years.

## Assembly
### Electronics
Gerber files for the PCB are in the project files. I ordered mine through JLCPCB and they arrived in only 2 weeks. All the modules are soldered directly to the PCB per the markings on the board. The battery and solar cell are connected using screw terminals. 

The module can also be assembled using perf board or individual wires if you have the patience for it. The schematic is included in the project files. 

<img src="https://github.com/EoghanODuffy/RiverGauge/assets/23324208/0b6e8ca9-4b3b-40a3-a72e-f3a0a80b2821" width="300" /> <img src="https://github.com/EoghanODuffy/RiverGauge/assets/23324208/c59263bf-6416-4991-8ed2-389a28a85c34" width="300" />

### Programming
The code is uploaded to the ESP using the Arduino IDE. It needs to be set up to work with ESP8266 modules. There's plenty of tutorials for this online. Before connecting any USB to the ESP module remove it from the PCB or remove the battery. (I've fried 3 boards making this project by forgetting to do this) 

All going well the module should start working once reassembled. The battery can be charged up if necessary using the TP4056 onboard USB port.

### Casing
I'll be designing a basic 3D-printed frame to mount the PCB into the case but just placing it inside would probably work fine. 2 holes will need to be drilled. One for the solar panel wire and one for the ultrasonic sensor. The solar panel is mounted to the top of the case using hot glue which creates a waterproof seal around the wire hole. I also add some on the inside around the ultrasonic sensor just to be safe. 

Hopefully, I'll get a bunch of mounts for the case designed which will allow for various mounting options. This part is all still a work in progress while I'm still testing things.







