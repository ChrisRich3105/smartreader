# esphome_im350
Custom Component for support of the Siemens IM350 and the ISKRA AM550 (Smart Meter from a local provider in Austria/Carinthia.)

The solution provided here is heavily based on the implementations of the standalone version in [https://github.com/Andre-Schuiki/esphome_im350](https://github.com/Andre-Schuiki/esphome_im350).
I adapted the code to also use it with the ISKRA smartmeter and directly publish the data on MQTT.
Also the code was ported to the ESP8266, instead of the ESP32 solution.

For the hardware, a lot of information can be found under [https://www.photovoltaikforum.com/thread/139837-siemens-im350/?pageNo=10](https://www.photovoltaikforum.com/thread/139837-siemens-im350/?pageNo=10).

# Usage/Compiling
For compiling you need **Visual Studio Code + Platform IO Plugin** for installation instructions see: [https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode)
1. Download this folder
2. Open the folder in Platform IO
3. Create a secrets.h file (see secrets.h.example file)
4. Build
5. Upload to ESP via serial port

# Using OTA
Note: First Upload must be over the Serial Port!
1. Edit platformio.ini and enter the ip address of your ESP.
2. Use Upload/Monitor Option as usual.

# Provider Documentation for the Customer Interface of the Smart Meter
* [Provider Information for IMx50 1](docs/provider_informations/im350.pdf)
* [Provider Information for IMx50 2](docs/provider_informations/IMx50_1.pdf)
* [Provider Information for IMx50 3](docs/provider_informations/IMx50_2.pdf)
* [Provider Information for Iskra](docs/provider_informations/iskra.pdf)

# Requirements
* ESP8622 board
* **Your Decryption (Block Cipher Key) -> you can get this key from your grid provider!**
* 1xRJ12 Cable (or you build your own of another cable with 6 lines if you have the crimp tools and jacks)
* 1xResistors 4.7k-10k
* A strip board
* Some Wires


