## ADC microphone 'driver'

Project from the Microcontrollers programming course. 
The STM32F411 microcontroller has a microphone attached. 
This project 
- reads the 8kHz 12bit raw audio data from the device,
- optionally applies the A-law compression
- and sends it to the computer via USART.

 The data can be read with `minicom` and saved directly to the audio file.
 The file can be then played e.g. with `play` command. 
