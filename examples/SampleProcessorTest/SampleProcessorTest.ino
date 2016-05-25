// Demo Code for SampleProcessor Library
// Craig Versek, May 2016

#include <SampleProcessor.h>
#include <SerialCommand.h>

#define arduinoLED 13   // Arduino LED on board
#define CAPACITY 10

SampleProcessor sProc;
SerialCommand sCmd(Serial);         // The demo SerialCommand object, initialize with any Stream object

void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off

  sProc.begin(CAPACITY);

  Serial.begin(9600);

  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("ON",    LED_on);     // Turns LED on
  sCmd.addCommand("OFF",   LED_off);    // Turns LED off
  sCmd.addCommand("ENQ",   ENQ_sCmd_action_handler);        // Enque a packet
  sCmd.addCommand("DEQ?",  DEQ_sCmd_query_handler);  // Converts two arguments to integers and echos them back
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
  Serial.println("Ready");
}

void loop() {
  int num_bytes = sCmd.readSerial();      // fill the buffer
  if (num_bytes > 0){
    sCmd.processCommand();  // process the command
  }
  delay(10);
}


void LED_on(SerialCommand this_sCmd) {
  this_sCmd.println("LED on");
  digitalWrite(arduinoLED, HIGH);
}

void LED_off(SerialCommand this_sCmd) {
  this_sCmd.println("LED off");
  digitalWrite(arduinoLED, LOW);
}

void ENQ_sCmd_action_handler(SerialCommand this_sCmd) {
  char *arg;
  arg = this_sCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL) {    // As long as it existed, take it
    int len = max(strlen(arg), SampleProcessor::DATA_BUFFER_SIZE);
    SampleProcessor::Packet pkt;
    for(int i=0; i < len; i++){
      pkt.data[i] = arg[i];
    }
    pkt.length  = len;
    pkt.timestamp = micros();
    pkt.flags   = 0x00;
    SampleProcessor::STATUS status;
    status = sProc.enqueue_rawdata(pkt);
    if (status != SampleProcessor::SUCCESS){
      this_sCmd.print(F("ERROR: SampleProcessor.enqueue_rawdata returned errorcode "));
      this_sCmd.println(status);
    }
    else{
      this_sCmd.print(F("OK"));
    }
  }
  else {
    this_sCmd.println(F("ERROR: ENQ requires 1 argument 'data'"));
  }
}

void DEQ_sCmd_query_handler(SerialCommand this_sCmd) {
  char *arg;
  arg = this_sCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL) {    // As long as it existed, take it
    this_sCmd.println(F("ERROR: DEQ require no arguments"));
  }
  else {
    SampleProcessor::Packet pkt;
    SampleProcessor::STATUS status;
    status = sProc.enqueue_rawdata(pkt);
    if (status != SampleProcessor::SUCCESS){
      this_sCmd.print(F("ERROR: SampleProcessor.dequeue_rawdata returned errorcode "));
      this_sCmd.println(status);
    }
    else{
      this_sCmd.print(F("data: 0x"));
      for (size_t i=0; i < pkt.length;i++){
        if(pkt.data[i] < 16){this_sCmd.print("0");}
          this_sCmd.print(pkt.data[i],HEX);
          this_sCmd.print(" ");
      }
      this_sCmd.println();
      this_sCmd.print(F("length: "));
      this_sCmd.println(pkt.length);
      this_sCmd.print(F("timestamp: "));
      this_sCmd.println(pkt.timestamp);
      this_sCmd.print(F("flags: 0x"));
      this_sCmd.println(pkt.flags,HEX);
    }
  }
}


// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(SerialCommand this_sCmd) {
  SerialCommand::CommandInfo command = this_sCmd.getCurrentCommand();
  this_sCmd.print("Did not recognize \"");
  this_sCmd.print(command.name);
  this_sCmd.println("\" as a command.");
}
