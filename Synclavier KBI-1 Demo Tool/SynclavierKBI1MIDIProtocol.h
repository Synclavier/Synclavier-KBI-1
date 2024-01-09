//
//  Synclavier MIDI Protocol - SynclavierKBI1MIDIProtocol.h
//
//  Created by Cameron Jones on 2023-08-28.

#ifndef SynclavierKBI1MIDIProtocol_h
#define SynclavierKBI1MIDIProtocol_h

// MIDI Interface to KBI-1

// MIDI Channel 1 (0x00)
//  - Note On, Note Off, Aftertouch (VK Only), per note
//  - Pitch Bend
//  - Controller movemnts
//      0x01 = Mod Wheel
//      0x02 = Breath Controller
//      0x07 = Pedal 1 (Volume  Pedal)
//      0x0B = Pedal 2 (Effects Pedal)
//      0x10 = Ribbon Controller (relative controller movements)
//
//      0x40 = Sustain    pedal
//      0x41 = Portamento pedal
//      0x42 = Hold       pedal (Sostenuto)
//      0x43 = Repeat     pedal
//      0x44 = Arpeggiate pedal
//      0x45 = Punch In   pedal

const int SynclavierKBI1MIDIProtocolNoteChannel = 0;            // Note: channel numbers in this file are zero-based

// The Ribbon controller also puts out dedidcated information on its own MIDI channel.
// Ribbon sends pitch bend messages on this channel.
const int SynclavierKBI1MIDIProtocolRibbonChannel = 5;          // Note: channel numbers in this file are zero-based


//  - NRPN messages to/from Channel 2 (0x01)
//    NRPN messages provide a way for a computer application to interrogate whether
//    a VK or ORK is connected and powered on. Setting a value of 0x3FFF for a parameter
//    provides a means to interrogate its current value.
//      NRPN Parameter 0
//          Product identification (e.g. 0 == Synclavier KBI-1)
//          Send parameter 0, value 0x3FFF to interrogate
//          KBI-1 responds with parameter 0, value 0 (also sends during power on)
//
//      NRPN Parameter 1
//          Operational status
//          Send parameter 1, value 0x3FFF to request status. Status will be sent when it changes.
//          Send parameter 1, value 0x0000 to clear the display and all buttons (initial condition)
//          KBI-1 sends parameter 1, value 0 - no ORK or VK connected (or connected and turned off)
//          KBI-1 sends parameter 1, value 1 - ORK connected
//          KBI-1 sends parameter 1, value 2 - VK  connected
//
//      See NRPN notes below - order is MSB/LSB and both must be sent for both parameter number and value.
//      When an app starts it should send parameter 0, value 0x3FFF to find out what Synclavier
//      hardware is connected (e.g. a KBI-1). If the connected hardware is a KBI-1 app should
//      send parameter 1, value 0x3FFF to find out if an ORK or VK is connected. If either a
//      ORK or VK is connected the app should send Parameter 1, value 0 to clear the dsiplay to
//      initial conditions.
const int SynclavierKBI1MIDIProtocolNRPNMessageWhoAreYou = 0;       // Recommend sending once per second once MIDI device is detected
const int SynclavierKBI1MIDIProtocolNRPNMessageHereIAm   = 0;
const int SynclavierKBI1MIDIProtocolNRPNMessageStatus    = 1;       // Once KBI-1 is detected, recommen poling status once per second to detect keyboard on/off
const int SynclavierKBI1MIDIProtocolNRPNMessageClear     = 2;       // Clear display and turn off all lights. Typically sent by host when status indicates VK or ORK connected
const int SynclavierKBI1MIDIProtocolNRPNMessageEcho      = 3;       // Echo's message back for testing
const int SynclavierKBI1MIDIProtocolNRPNMessageRefresh   = 4;       // Request from KBI-1 to host for full refresh of buttons and display
const int SynclavierKBI1MIDIProtocolNRPNAskValue         = 0x3FFF;

// SynclavierKBI1MIDIProtocolNRPNMessageWhoAreYou responses
const int SynclavierKBI1MIDIProtocolNRPNMessageIAmKBI1   = 0;
const int SynclavierKBI1MIDIProtocolNRPNMessageIAmRegen  = 1;

// SynclavierKBI1MIDIProtocolNRPNMessageStatus responses
const int SynclavierKBI1MIDIProtocolNRPNMessageNoOneHome = 0;
const int SynclavierKBI1MIDIProtocolNRPNMessageORKHere   = 1;
const int SynclavierKBI1MIDIProtocolNRPNMessageVKHere    = 2;

const int SynclavierKBI1MIDIProtocolNRPNChannel = 1;                // Note: channel numbers in this file are zero-based


// MIDI Channel 2 (0x01)
//  - Pitch bend for Knob
//  - Note on then off for each ORK button press
//  - Sending Note On will light a button, Note Off will turn it off
//  - Note On must have velocity 127; not on velocity of 0 is treated as note off.
//  Button 0 is the top left.
//  Buttons  0 - 7  are the top row in the left-most panel
//  Buttons  8 - 15 are the row under  0 -  7
//  Buttons 16 - 23 are the row under  9 - 15
//  Buttons 24 - 31 are the row under 16 - 23
//  Buttons 32 - 39 are the row to the right of  0 -  7
//  Buttons 40 - 47 are the row under 32 - 39
//  ...

const int SynclavierKBI1MIDIProtocolKnobChannel = 1;                // Note: channel numbers in this file are zero-based
const int SynclavierKBI1MIDIProtocolORKChannel  = 1;


// MIDI Channel 3 (0x02)
//  - Note on then off for first 128 VK buttons. Note on sent when button is pressed
//  - Note On must have velocity 127; not on velocity of 0 is treated as note off.
//  - Sending Note On will light a button, Note Off will turn it off

const int SynclavierKBI1MIDIProtocolVKChannel = 2;                  // Note: channel numbers in this file are zero-based

const int SynclavierKBI1MIDIProtocolButtonOff       =   0;          // Velocity to send. VK only shows off, blinking and ON.
const int SynclavierKBI1MIDIProtocolButtonHeld      =  32;
const int SynclavierKBI1MIDIProtocolButtonBlinking  =  64;
const int SynclavierKBI1MIDIProtocolButtonOn        = 127;

// MIDI Channel 4 (0x03)
//  - Notes 0 - 31. Note on sent when button is pressed
//  - Note On must have velocity 127; not on velocity of 0 is treated as note off.
//  - The right-most panel of 32 buttons on the VK

const int SynclavierKBI1MIDIProtocolVKAltChannel = 3;       // Note: channel numbers in this file are zero-based


// MIDI Channel 5 (0x04) - Send to KBI-1.
//
// There are message sequences to address the two lines of the VK display
// as well as the 4 digits and units indicators on the ORK
// The displays are sent as a sequence of ASCII characters.
// Note there is a separate decimal point for each character position
// The decimal point precedes the digit it goes with.
//
// Sample ORK strings:
//    1m
// 440.0h
// 1.000a
//
// (m for millseconds unit, h for hertz unit, a for arbitrary unit, v for volume unit LED).
//
// The characters are specified with the note-on velocity.
// A note-off message indicates end-of line.
// The display is updated when the end-of-line message is received
//
// Note on note 0 = message for ORK
// Note on note 1 = message for top   line of VK
// Note on Note 2 = message for lower line of VK
//
// For example - send to MIDI Channel 5 (0x04)
//
// '   .0' (3 spaces, period, 0)
// <Note On  Ch 0x04><Note 0 0x00><Space 0x20>
// <Note On  Ch 0x04><Note 0 0x00><Space 0x20>
// <Note On  Ch 0x04><Note 0 0x00><Space 0x20>
// <Note On  Ch 0x04><Note 0 0x00><.     0x2E>
// <Note On  Ch 0x04><Note 0 0x00><0     0x30>
// <Note Off Ch 0x04><Note 0 0x00><      0x00> (causes display to actually be written)

const int SynclavierKBI1MIDIProtocolORKDisplay                     = 0;
const int SynclavierKBI1MIDIProtocolVKDisplayLine0                 = 1;
const int SynclavierKBI1MIDIProtocolVKDisplayLine1                 = 2;

// These note numbers address the VK display in 4 sections separated into characters, and decimal points.
const int SynclavierKBI1MIDIProtocolVKDisplayLine0Section0Chars    = 3;    // VK Characters
const int SynclavierKBI1MIDIProtocolVKDisplayLine0Section1Chars    = 4;
const int SynclavierKBI1MIDIProtocolVKDisplayLine1Section0Chars    = 5;
const int SynclavierKBI1MIDIProtocolVKDisplayLine1Section1Chars    = 6;
const int SynclavierKBI1MIDIProtocolVKDisplayLine0Section0Decimals = 7;    // VK Characters
const int SynclavierKBI1MIDIProtocolVKDisplayLine0Section1Decimals = 8;
const int SynclavierKBI1MIDIProtocolVKDisplayLine1Section0Decimals = 9;
const int SynclavierKBI1MIDIProtocolVKDisplayLine1Section1Decimals = 10;

static constexpr int SynclavierKBI1MIDIProtocolVKMIDINoteForCharSection[2][2] =
{
    {SynclavierKBI1MIDIProtocolVKDisplayLine0Section0Chars, SynclavierKBI1MIDIProtocolVKDisplayLine0Section1Chars},
    {SynclavierKBI1MIDIProtocolVKDisplayLine1Section0Chars, SynclavierKBI1MIDIProtocolVKDisplayLine1Section1Chars},
};

static constexpr int SynclavierKBI1MIDIProtocolVKMIDINoteForDecimalSection[2][2] =
{
    {SynclavierKBI1MIDIProtocolVKDisplayLine0Section0Decimals, SynclavierKBI1MIDIProtocolVKDisplayLine0Section1Decimals},
    {SynclavierKBI1MIDIProtocolVKDisplayLine1Section0Decimals, SynclavierKBI1MIDIProtocolVKDisplayLine1Section1Decimals},
};

const int SynclavierKBI1MIDIProtocolDisplayChannel = 4;       // Note: channel numbers in this file are zero-based

// NRPN - https://en.wikipedia.org/wiki/NRPN
// Unlike other MIDI controllers (such as velocity, modulation, volume, etc.), NRPNs require more
// than one item of controller data to be sent. First, controller 99 - NRPN Most Significant Byte
// (MSB) - followed by 98 - NRPN Least Significant Byte (LSB) sent as a pair specify the parameter
// to be changed. Controller 6 then sets the value of the relevant parameter. Controller 38 may
// optionally then be sent as a fine adjustment to the value set by controller 6.
//
// So KBI-1 requires both MSB and LSB to be sent, with MSB first, both for the parameter number and the value.
// The action is taken or the requested parameter is returned when the value LSB is received.

class SynclavierKBI1MIDIProtocolNRPN {
public:
    typedef unsigned char NRPNByte;

    static const NRPNByte MIDICtl = 0xb0;
    static const NRPNByte nrpnMSB = 0x63;
    static const NRPNByte nrpnLSB = 0x62;
    static const NRPNByte dataMSB = 0x06;
    static const NRPNByte dataLSB = 0x26;

    inline SynclavierKBI1MIDIProtocolNRPN(int param, int data, int chan) {
        nrpn1Message[0] = MIDICtl + (chan & 0xF);       // MIDI CC message adressed to channel
        nrpn1Message[1] = nrpnMSB;                      // Param MSB
        nrpn1Message[2] = (param >> 7) & 0x7F;          // 7 MSB
        
        nrpn2Message[0] = MIDICtl + (chan & 0xF);       // MIDI CC message adressed to channel
        nrpn2Message[1] = nrpnLSB;                      // Param LSB
        nrpn2Message[2] = param & 0x7F;                 // 7 LSB

        data1Message[0] = MIDICtl + (chan & 0xF);       // MIDI CC message adressed to channel
        data1Message[1] = dataMSB;                      // Data MSB
        data1Message[2] = (data >> 7) & 0x7F;           // 7 MSB
        
        data2Message[0] = MIDICtl + (chan & 0xF);       // MIDI CC message adressed to channel
        data2Message[1] = dataLSB;                      // Data LSB
        data2Message[2] = data & 0x7F;                  // 7 LSB
    }
    
    NRPNByte* nrpn1() {return &nrpn1Message[0];}
    NRPNByte* nrpn2() {return &nrpn2Message[0];}
    NRPNByte* data1() {return &data1Message[0];}
    NRPNByte* data2() {return &data2Message[0];}
private:
    NRPNByte nrpn1Message[3];
    NRPNByte nrpn2Message[3];
    
    NRPNByte data1Message[3];
    NRPNByte data2Message[3];
};

#endif
