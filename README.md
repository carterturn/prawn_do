Raspberry Pi Pico (Prawn) Digital Outputs
=========================================

This is a basic array of digital outputs, designed to be used with labscript, running on the Pi Pico.

The Pi Pico is programmed with a list of 16 digital outputs over serial, then outputs each element of the list to the first 16 pins, incrementing on the rising edge of a trigger (pin 16).

The first element of the list is output when the run command is sent, without waiting for a trigger.

The minimum trigger width depends on the wiring and PIO frequency. On a breadboard, a pulse width of at least 1us seems to be required.

Serial Interface
================

Digital outputs are appended (always appended) by the `add` command. `add` (followed by a newline) starts appending mode. Each subsequent line is either an unsigned integer up to 16 bits (in an ASCII representation, e.g. `0x0F0F`) or the keyword `end`, which ends appending mode.

The current sequence can be read by the dump command: `dmp`.

The current sequence can be cleared by the clear command: `cls`.

The sequence can be run by the run command: `run`.

A running sequence can be aborted by the abort command: `abt`. Note that after aborting, the Pi Pico must finish the currently queued ramp, so it should be triggered at least once.

Wiring
======

Pins 0-15 of the Pi Pico are the digital outputs. They correspond to each bit of the unsigned integers added via the add command.

Pin 16 is the trigger. On rising edge, the next integer is sent to the outputs.

Design Notes
============

The general approach of this program is to use a single CPU core to prepare a large memory buffer of unsigned, 16 bit integers, then pass this buffer to the programmable IO via direct memory access. The PIO program is quite simple, with a single out instruction occuring per trigger (on the rising edge). The autopull feature is used to transfer data from the PIO fifo to the output shift register after each out instructuion. The DMA then refills the fifo as necessary.
