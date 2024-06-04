/*
   Copyright (c) 2019 Stefan Kremser
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/SimpleCLI
 */

// Include Library
#include <SimpleCLI.h>

// Create CLI Object
SimpleCLI cli;

// Commands
Command setOutput;
Command setChannel;
Command setMove;

const int strobePin = 41;
float voltVal = 0;
unsigned int binaryVal = 0;
byte address = B11111111;

// Callback function for ping command
void outputCallback(cmd* c) {
    Command cmd(c); // Create wrapper object
    Argument argVolt = cmd.getArgument("volt");    
    String argVoltVal = argVolt.getValue();
    voltVal = argVoltVal.toInt();
    binaryVal = ((voltVal + 10) / 20) * 65535;   
    
    Serial.println("Output set to " + argVoltVal + "V");
}

void channelCallback(cmd* c) {
    Command cmd(c); // Create wrapper object
    Argument argNum = cmd.getArgument("num");
    String argNumVal = argNum.getValue();
    int num = argNumVal.toInt();
    address = num - 1 + 252;
    PORTA = address;    
    
    Serial.println("Channel " + argNumVal + "  selected");
}

void moveCallback(cmd* c) {
    Command cmd(c); // Create wrapper object
    Argument argBits = cmd.getArgument("bits");    
    String argBitsVal = argBits.getValue();
    unsigned int bitsVal = argBitsVal.toInt();     
    binaryVal += bitsVal;  
    
    Serial.print("Output set to ");
    Serial.print(binaryVal);
}

// Callback in case of an error
void errorCallback(cmd_error* e) {
    CommandError cmdError(e); // Create wrapper object

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand()) {
        Serial.print("Did you mean \"");
        Serial.print(cmdError.getCommand().toString());
        Serial.println("\"?");
    }
}


void setup() {
    // Enable Serial connection at given baud rate
    Serial.begin(9600);

    // Wait a bit until device is started
    delay(2000);
    
    setOutput = cli.addCmd("output", outputCallback);
    setChannel = cli.addCmd("channel", channelCallback);
    setMove = cli.addCmd("move", moveCallback);

    // [Optional] Check if our command was successfully added
    if (!setOutput || !setChannel) {
        Serial.println("Something went wrong :(");
    } else {
        Serial.println("CLI started");
    }

    // Set error Callback
    cli.setOnError(errorCallback);

    // Start the loop
    Serial.println("Type a command");

    setOutput.addArgument("volt");
    setChannel.addArgument("num");
    setMove.addArgument("bits");

    
    DDRA = B11111111; // set PORTA (digital 7~0) to outputs
    DDRC = B11111111; // set PORTA (digital 7~0) to outputs
    DDRL = B11111111; // set PORTA (digital 7~0) to outputs
    
    pinMode(strobePin, OUTPUT); 
}

void loop() {
    // Check if user typed something into the serial monitor
    if (Serial.available()) {
        // Read out string from the serial monitor
        String input = Serial.readStringUntil('\n');

        // Echo the user input
        Serial.print("# ");
        Serial.println(input);

        // Parse the user input into the CLI
        cli.parse(input);
    }
    
    
    PORTC = highByte(binaryVal);
    PORTL = lowByte(binaryVal);
    digitalWrite(strobePin, HIGH);
    digitalWrite(strobePin, LOW);
}
