//
//  main.cpp
//  Synclavier KBI-1 Demo Tool
//
//  Created by Cameron Jones on 1/8/24.
//

#include <stdio.h>

#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#include "SynclavierKBI1MIDIProtocol.h"

// Bare-bones example of communicating with KBI-1 using Macintosh Core Midi.
// This demo program includes no error recovery.

// Simple interface to Mac OS Core MIDI
static MIDIClientRef   MU_MIDIClientRef;        // MIDIClientRef for communicating with MIDI services
static MIDIPortRef     MU_MIDIOutputPort;       // MIDIPortRef for sending   data to   MIDIServices
static MIDIPortRef     MU_MIDIInputPort;        // MIDIPortRef for receiving data from MIDIServices
static MIDIEndpointRef kbi1InputRef;
static MIDIEndpointRef kbi1OutputRef;
static CFRunLoopRef    mainRunLoop;

// KBI-1 Management
static bool kbi1Connected;
static int  kbi1Status;
static int  kbi1Time;
static int  midiParamMSB,    midiParamLSB,    midiDataMSB,    midiDataLSB;
static int  midiParamMSBSeq, midiParamLSBSeq, midiDataMSBSeq, midiDataLSBSeq;
static int  midiReceivedSequence;
static int  secondsCounter;
static int  testButton = -1;

void ME_Send3Bytes(MIDIEndpointRef cableNum, unsigned char byte1, unsigned char byte2, unsigned char byte3) {
    MIDIPacketList pktlist;
    MIDIPacket&    pkt = *MIDIPacketListInit(&pktlist);
    
    pktlist.numPackets = 1;
    
    pkt.timeStamp = 0;
    pkt.length    = 3;
    pkt.data[0]   = byte1;
    pkt.data[1]   = byte2;
    pkt.data[2]   = byte3;
    
    MIDISend(MU_MIDIOutputPort, kbi1OutputRef, &pktlist);
}

void ME_SendNRPN(MIDIEndpointRef cableNum, int param, int value)
{
    SynclavierKBI1MIDIProtocolNRPN nrpnMessage(param, value, SynclavierKBI1MIDIProtocolNRPNChannel);
    
    ME_Send3Bytes(cableNum, nrpnMessage.nrpn1()[0], nrpnMessage.nrpn1()[1], nrpnMessage.nrpn1()[2]);
    ME_Send3Bytes(cableNum, nrpnMessage.nrpn2()[0], nrpnMessage.nrpn2()[1], nrpnMessage.nrpn2()[2]);
    ME_Send3Bytes(cableNum, nrpnMessage.data1()[0], nrpnMessage.data1()[1], nrpnMessage.data1()[2]);
    ME_Send3Bytes(cableNum, nrpnMessage.data2()[0], nrpnMessage.data2()[1], nrpnMessage.data2()[2]);
}

void ME_SendDisplay(MIDIEndpointRef cableNum, int whichChannel, int whichDisplay, const char* display)
{
    while (display && *display)
        ME_Send3Bytes(cableNum, 0x90 + whichChannel, whichDisplay, (*display++) & 0x7F);

    ME_Send3Bytes(cableNum, 0x80 + whichChannel, whichDisplay, 0);
}

void ME_SendButton(MIDIEndpointRef cableNum, int whichChannel, int whichButton, int velData)
{
    // Note on with velocity of 0 indicates note off
    ME_Send3Bytes(cableNum, 0x90 + whichChannel, whichButton, velData);
}

void MU_ReadProc(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon)
{
    MIDIEndpointRef endPoint = (MIDIEndpointRef) (long long) srcConnRefCon;
    
    // Not from KBI-1
    if (endPoint != kbi1InputRef)
        return;
    
    auto&   lst = *pktlist;
    auto    ppk = &lst.packet[0];

    // We are handed a MIDIPacketList by a callback from MIDI Services.
    // We forward the MIDIPacketLists to MIDI Event processing where they are converted to Synclavier MIDI Events.
    // We will be called back below (HandleMIDIMessage or HandleMIDISysex) with the assembled message.
    for (int i=0; i<lst.numPackets; i++, ppk = MIDIPacketNext(ppk)) {
        auto& packet = *ppk;
        auto  pData  = &packet.data[0];
        auto  length = packet.length;
        
        while (length > 0) {
            auto byte1 = *pData++;
            length     -= 1;
            
            // data byte?
            if (byte1 < 0x80)
                continue;
            
            // System Real-Time Messages - one byte. KBI-1 will not send these.
            if (byte1 >= 0xF8)
                continue;
            
            // System Common Messages. KBI-1 will not send these so just bail. They are variable-length.
            if (byte1 >= 0xF0)
                break;
            
            // Must be complete message
            if (length == 0)
                break;
            
            auto byte2 = *pData++;
            length     -= 1;

            // Ignore program change
            if ((byte1 & 0xF0) == 0xC)
                continue;
            
            // Ignore channel pressure
            if ((byte1 & 0xF0) == 0xD)
                continue;
            
            // Must be complete message
            if (length == 0)
                break;
            
            auto byte3 = *pData++;
            length     -= 1;

            // NRPN messages all come on MIDI Channel 2 (0x01). MIDI Channel 1 (0x0) is where
            // the note on/off messages come in. Other MIDI Channel uses are described in SynclavierKBI1MIDIProtocol.h.
            if ((byte1 & 0xF) != SynclavierKBI1MIDIProtocolNRPNChannel)
                continue;
            
            // Ignore unless controller movement.
            if ((byte1 & 0xF0) != 0xB0)
                continue;
            
            // Handle NRPN messages from KBI-1.
            bool midiNRPNReceived = false;
            
            if (byte2 == SynclavierKBI1MIDIProtocolNRPN::nrpnMSB) {
                midiParamMSB      = byte3;
                midiParamMSBSeq   = midiReceivedSequence;
            }
            else if (byte2 == SynclavierKBI1MIDIProtocolNRPN::nrpnLSB) {
                midiParamLSB      = byte3;
                midiParamLSBSeq   = midiReceivedSequence;
            }
            else if (byte2 == SynclavierKBI1MIDIProtocolNRPN::dataMSB) {
                midiDataMSB      = byte3;
                midiDataMSBSeq   = midiReceivedSequence;
            }
            else if (byte2 == SynclavierKBI1MIDIProtocolNRPN::dataLSB) {
                midiDataLSB      = byte3;
                midiDataLSBSeq   = midiReceivedSequence;
                midiNRPNReceived = true;
            }
                            
            // One more processed
            midiReceivedSequence++;
            
            // Require all 4 packets to be sent and sent in order.
            // This will prevent dropped packets from being handled.
            if (midiNRPNReceived) {
                if ((midiParamMSBSeq == midiReceivedSequence-4)
                &&  (midiParamLSBSeq == midiReceivedSequence-3)
                &&  (midiDataMSBSeq  == midiReceivedSequence-2)
                &&  (midiDataLSBSeq  == midiReceivedSequence-1)) {
                    int param = midiParamMSB<<7 | midiParamLSB;
                    int data  = midiDataMSB <<7 | midiDataLSB;
                    
                    // Process NRPN message on main application thread.
                    CFRunLoopPerformBlock(mainRunLoop, kCFRunLoopDefaultMode, ^{

                        // KBI-1 could now be unplugged
                        if (kbi1InputRef == 0 || kbi1OutputRef == 0)
                            return;

                        // Look for Identification as KBI-1
                        if (param == SynclavierKBI1MIDIProtocolNRPNMessageHereIAm && data == SynclavierKBI1MIDIProtocolNRPNMessageIAmKBI1) {
                            if (!kbi1Connected) {
                                kbi1Connected = true;
                                
                                printf("KBI-1 connected.\n");
                                
                                // Ask KBI1 to report whether ORK or VK is connected
                                ME_SendNRPN(kbi1OutputRef, SynclavierKBI1MIDIProtocolNRPNMessageStatus, SynclavierKBI1MIDIProtocolNRPNAskValue);
                            }
                        }
                        
                        // Look for ORK or VK connected. Software polls status once per second
                        else if (param == SynclavierKBI1MIDIProtocolNRPNMessageStatus) {
                            int newStatus = data;
                            
                            // Record activity
                            kbi1Time = secondsCounter;
                            
                            // Look for change in status
                            if (newStatus != kbi1Status) {
                                
                                if (newStatus != SynclavierKBI1MIDIProtocolNRPNMessageNoOneHome) {
                                    
                                    kbi1Status = newStatus;
                                    
                                    if (kbi1Status == SynclavierKBI1MIDIProtocolNRPNMessageORKHere)
                                        printf("Ork connected.\n");
                                    
                                    else if (kbi1Status == SynclavierKBI1MIDIProtocolNRPNMessageVKHere)
                                        printf("VK connected.\n");
                                    
                                    else
                                        printf("Unknown keyboard connected.\n");

                                    // Begin by clearing ORK/VK Display
                                    ME_SendNRPN(kbi1OutputRef, SynclavierKBI1MIDIProtocolNRPNMessageClear, 0);
                                    
                                    // Start LED test
                                    testButton = -1;
                                }
                                
                                // Else ork disconnected or powered down. Remove the buttons from update list
                                else {
                                    
                                    kbi1Status = newStatus;
                                    
                                    printf("ORK/VK disconnected.\n");
                                }
                            }
                        }
                        
                        // Refresh. KBI-1 asks for refresh after we time out by breaking into debugger. That is the KBI-1 is reconnecting
                        // to us so it needs a refresh. We never noticed it going away.
                        else if (param == SynclavierKBI1MIDIProtocolNRPNMessageRefresh && data == SynclavierKBI1MIDIProtocolNRPNAskValue)  {

                            // Must respond with clear to start things going agin
                            if (kbi1Status != SynclavierKBI1MIDIProtocolNRPNMessageNoOneHome)
                                ME_SendNRPN(kbi1OutputRef, SynclavierKBI1MIDIProtocolNRPNMessageClear, 0);
                        }
                    });
                    
                    // In this command-line tool example we have to wake up the main thread explictly.
                    CFRunLoopWakeUp(mainRunLoop);
                }
            }
        }
    }
}

void MU_PollForKBI1()
{
    if (kbi1InputRef == 0) {
        auto n = MIDIGetNumberOfSources();
        
        for (int which = 0; which < n; which++) {
            CFStringRef epNameRef = NULL;

            auto endpointRef = MIDIGetSource(which);
            
            if (endpointRef == 0)
                continue;
            
            if (MIDIObjectGetStringProperty(endpointRef, kMIDIPropertyName, &epNameRef) != 0)
                continue;
            
            if (epNameRef == nullptr)
                continue;
            
            if (CFStringCompare(epNameRef, CFSTR("Synclavier KBI-1"), 0) == kCFCompareEqualTo) {
                kbi1InputRef = endpointRef;
                printf("KBI-1 input  found 0x%08x.\n", kbi1InputRef);
                
                MIDIPortConnectSource(MU_MIDIInputPort, kbi1InputRef, (void*) (long long) kbi1InputRef);
            }
            
            CFRelease(epNameRef);
        }
    }
    
    if (kbi1OutputRef == 0) {
        auto n = MIDIGetNumberOfDestinations();
        
        for (int which = 0; which < n; which++) {
            CFStringRef epNameRef = NULL;

            auto endpointRef = MIDIGetDestination(which);
            
            if (endpointRef == 0)
                continue;
            
            if (MIDIObjectGetStringProperty(endpointRef, kMIDIPropertyName, &epNameRef) != 0)
                continue;
            
            if (epNameRef == nullptr)
                continue;
            
            if (CFStringCompare(epNameRef, CFSTR("Synclavier KBI-1"), 0) == kCFCompareEqualTo) {
                kbi1OutputRef = endpointRef;
                printf("KBI-1 output found 0x%08x.\n", kbi1OutputRef);
            }
            CFRelease(epNameRef);
        }
    }
}

// Callback - MIDI services calls us back when ports are added or removed (e.g. a USB MIDI device is plugged in.
// We respond by performing a callback into the app.
// We are called on the main application thread.
void MU_Notify(const MIDINotification *message, void *refCon)
{
    // Simply poll for the device again
    kbi1InputRef = kbi1OutputRef = NULL;
    
    MU_PollForKBI1();
}


// Start the show.
int main(int argc, const char * argv[]) {

    MIDIClientCreate    (CFSTR("KBI-1 Test Program"), MU_Notify, NULL, &MU_MIDIClientRef);
    MIDIInputPortCreate (MU_MIDIClientRef, CFSTR("KBI-1 Test Program"), MU_ReadProc, NULL, &MU_MIDIInputPort);
    MIDIOutputPortCreate(MU_MIDIClientRef, CFSTR("KBI-1 Test Program"), &MU_MIDIOutputPort);
    
    if (MU_MIDIClientRef == 0 || MU_MIDIInputPort == 0 || MU_MIDIOutputPort == 0) {
        printf("Could not connect to MIDI services. Terminating.\n");
        exit(0);
    }
    
    printf("Start of KBI-1 Demo. Waiting for KBI-1.\n");
    
    mainRunLoop = CFRunLoopGetMain();

    while (1) {

        // Recommended practice it to poll KBI-1 periodically. This detects the keyboard
        // being turned off for example. A more comprehensive example would register for
        // MIDI topology changes to be notified when the KBI-1 MIDI USB Device was
        // removed or reconnected.
        
        // Wait for device to show up. Also detect going away.
        if (kbi1InputRef == 0 || kbi1InputRef == 0) {
            if (kbi1Connected) {
                printf("KBI-1 unplugged.\n");
                
                kbi1Connected = false;
                kbi1Status    = 0;
            }
            
            MU_PollForKBI1();
            
            // Poll immediately if found
            if (kbi1InputRef != 0 && kbi1OutputRef != 0)
                continue;
        }
        
        // Poll for KBI1 if it is not connected
        else if (!kbi1Connected)
            ME_SendNRPN(kbi1OutputRef, SynclavierKBI1MIDIProtocolNRPNMessageWhoAreYou, SynclavierKBI1MIDIProtocolNRPNAskValue);
        
        // Look for it going away (e.g. powered down, maybe stopped in debugger) if it is connected
        else if (kbi1Status != 0 && (secondsCounter > kbi1Time + 5)) {
            printf("KBI-1 timout\n");
            
            kbi1Connected = false;
            kbi1Status    = 0;
        }
        
        // Else if KBI1 connected, poll for ORK or VK being available
        else
            ME_SendNRPN(kbi1OutputRef, SynclavierKBI1MIDIProtocolNRPNMessageStatus, SynclavierKBI1MIDIProtocolNRPNAskValue);
        
        // Display
        if (kbi1Status == SynclavierKBI1MIDIProtocolNRPNMessageORKHere) {
            char number[10];
            snprintf(number, sizeof(number), "%4d", secondsCounter % 1000);
            ME_SendDisplay(kbi1OutputRef, SynclavierKBI1MIDIProtocolDisplayChannel, SynclavierKBI1MIDIProtocolORKDisplay, number);
            
            if (testButton >= 0)
                ME_SendButton(kbi1OutputRef, SynclavierKBI1MIDIProtocolORKChannel, testButton, 0);
            
            // Lights. We just need the music now.
            testButton = (testButton + 1) & 0x7F;
            ME_SendButton(kbi1OutputRef, SynclavierKBI1MIDIProtocolORKChannel, testButton, 127);
        }
        
        if (kbi1Status == SynclavierKBI1MIDIProtocolNRPNMessageVKHere) {
            char number[20];
            snprintf(number, sizeof(number), "%10d", secondsCounter);
            ME_SendDisplay(kbi1OutputRef, SynclavierKBI1MIDIProtocolDisplayChannel, SynclavierKBI1MIDIProtocolVKDisplayLine0, number);
            
            if (testButton >= 0)
                ME_SendButton(kbi1OutputRef, SynclavierKBI1MIDIProtocolVKChannel, testButton, 0);
            
            // Lights. We just need the music now.
            testButton = (testButton + 1) & 0x7F;
            ME_SendButton(kbi1OutputRef, SynclavierKBI1MIDIProtocolVKChannel, testButton, 127);
        }
        
        // Wait for things to happen
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, false);
        
        secondsCounter++;
    }
    
    return 0;
}
