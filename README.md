# BuzzBeat
MIDI Drum Machine

Purpose:
The point was to create cheap Drum-Machine MIDI sequencer which can be connected via MIDI cable to some music keyboard or Tone Generator with drums sound bank. Drum Machine will have Sync-Out 2ppq compatible with Korg’s Volca series musical equipment, Swing option with control via Potentiometer, and ability to saving few patterns. There also second MIDI female connector, MIDI clock out which sends clock timer via MIDI cable to devices without jack 3,5 sync in.

Parameters:
    • 7 Drums to choice, which can play simultaneously
    • BPM range between 70 – 300 bpm
    • Swing Mode
    • Sync-Out
    • Note length change (useful for hats)
    • Saving and Loading created drum patterns
	

Used Parts:
    • Arduino Nano (and USB cable for programming / power supply)
    • 2 * MIDI Female Connector
    • Audio Jack 3,5 Female Connector
    • LCD 16x2 screen with I2C converter
    • Numeric Keyboard
    • 2 * 200ohm resistors
    • 2 * 100k ohm potentiometer
    • On-Off Switch
    • DC 2.1/5.5 female connector
    • Keyboard Keys stickers

# Required Libraries:
This project utilizes the following Arduino libraries:
    •[EEPROM.h](https://www.arduino.cc/en/Reference/EEPROM) - Library for managing EEPROM memory.
    •[SoftwareSerial.h](https://www.arduino.cc/en/Reference/SoftwareSerial) - Library for emulating serial communication on other pins.
    •[LiquidCrystal_I2C.h](https://github.com/johnrickman/LiquidCrystal_I2C) - Library for controlling an LCD display via I2C.
    •[Keypad.h](https://github.com/Chris--A/Keypad) - Library for handling matrix keypads.
