/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "application.h"
#line 1 "/Users/jido/Documents/GitHub/x045-027-laddbox/027-klimatcykel/src/M011-solarxbike.ino"
void mqttPublish(char *event, String msg);
void setup();
void loop();
void measureVA();
#line 1 "/Users/jido/Documents/GitHub/x045-027-laddbox/027-klimatcykel/src/M011-solarxbike.ino"
PRODUCT_ID(5002);
PRODUCT_VERSION(0.2);
/*
 * Project 027-klimatcykel
 * Produkt: ModulX
 * Description: Device till laddbox för mätning och loggning av energi.
 * Author: Lars Lindmark
 * Date:
 */
// This #include statement was manually added by Lars Lindmark.
#include <MQTT.h>
#include "PollingTimer.h"

/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

/* Function prototypes solarXbike --------------------------------------------*/
int analogvalue; // (A0) analog Volt sensor
int sensorValue; // (A1) anolog Amp sensor
float voltage; // Voltage reading at input
// Orginal värde: const float Vpp = 0.001221; // Arduino 5v/1023 = Vpp = 0.00488758553 | Particle 5V/4095 = 0,001221 3,3V/4095 = 0,00080586
const float Vpp = 0.00080586; // Arduino 5v/1023 = Vpp = 0.00488758553 | Particle 5V/4095 = 0,001221 3,3V/4095 = 0.00080586  | 5V/3070 = 0.00162866 | 3,3v/3070=0.00107492
float temp;
int offSet; // Amp sensor Offset the value from the middle of register
float x053_batteryVoltage;
float x053_chargeAmp; //Ampere reading
float x053_solarPower; // Power (W) from solarcell

/* Function prototypes MQTT --------------------------------------------*/
void callback(char *topic, byte *payload, unsigned int length);
MQTT client("skinny.skycharts.net", 1883, callback);

PollingTimer batteryTimer(36000);
volatile int forceReading = 1;

void mqttPublish(char *event, String msg) {
    if (!client.isConnected()) {
         client.connect("x053_" + String(Time.now()));
    }
    if (client.isConnected()) {
        client.publish(event, msg);
    }
}

// MQTT recieve message (not used right now but include if needed)
void callback(char *topic, byte *payload, unsigned int length)
{
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;

//  Serial.print("MQTT rx:");
//  Serial.println(p);
//  setMessage(p);
    forceReading = 1;
}


/* This function is called once at start up ----------------------------------*/
void setup()
{
	//Setup the Tinker application here

	//Register all the Tinker functions
	Particle.function("digitalread", tinkerDigitalRead);
	Particle.function("digitalwrite", tinkerDigitalWrite);
	Particle.function("analogread", tinkerAnalogRead);
	Particle.function("analogwrite", tinkerAnalogWrite);

	//Setup Particle I/O for sensors on pin A0 and A1
    pinMode(D3, OUTPUT); // SmartPower relä, slår till relä vid låg spänning
    pinMode(A0, INPUT); // Analog 0 Input for VOLT?
    pinMode(A1, INPUT); // Analog 1 Input for Amp sensor

    // MQTT connect to the server(unique id by Time.now())
    client.connect("x053_" + String(Time.now()));
    if (client.isConnected()) {
        client.subscribe("updateStats");
    }
    mqttPublish("my-event", "MQTT connected");

    batteryTimer.start();

}

/* This function loops forever --------------------------------------------*/
void loop()
{
	//This will run in a loop


    
       if (client.isConnected()) {
        client.loop();
    }

   int takeReading = forceReading || batteryTimer.interval();
   
   if (!takeReading) {
       return;
   }

  // clear flag
  if (forceReading > 0) {
    forceReading--;
  }
measureVA();

	// Publicera till particle cloud
  	String temp2 = String(x053_batteryVoltage,1); // store voltage in "batteryVoltage" string
  	Particle.publish("x053_batteryVoltage", temp2, PRIVATE); // publish to cloud
  	String temp4 = String(x053_chargeAmp,1); // store ampere in "chargeAmp" string
  	Particle.publish("x053_chargeAmp", temp4, PRIVATE); // publish to cloud
    x053_solarPower = x053_batteryVoltage * x053_chargeAmp; // calculating power from sensor readings A0 and A1
  	String temp5 = String(x053_solarPower,1); // store ampere in "chargeAmp" string
  	Particle.publish("x053_solarPower", temp5, PRIVATE); // publish to cloud
  
 	mqttPublish("x053_batteryVoltage", temp2);
  	mqttPublish("x053_chargeAmp" , temp4);
  	mqttPublish("x053_solarPower" , temp5);

    // delay (3600);
    //delay (360000); // 5 minute delay
}





void measureVA() 
{
//Version 3 (A1 - 550) * (5v/1023 = 0.00488758553 =Vpp) / 0.066 känslighet sensor
    sensorValue = analogRead(A1);
    offSet = sensorValue - 2530; //ursprungsvärde 3070 | testvärde 4096/2= 2048
    voltage = offSet * Vpp; //Vpp = konstant för att översätta spänning till skala
    x053_chargeAmp = voltage / 0.066;

  // check to see what the value of the A0 input is and store it in the int(heltal) variable analogvalue
  // batteryVoltage är ett flyttal som visar decimaler. Formel : batteryVoltage = A0 * 2 / 112
    analogvalue = analogRead(A0);
    temp = analogvalue*2;
    x053_batteryVoltage = temp/75; //Standard calibrering 112
// 3035=54,2v  3012=53,8V : 3007= 53,7V : 3001 = 53,6V :2996= 53,5 :  2938 = 52V : 2800 = 50v : 2700 = 48,21


    
    // Control the relay depending on value reading

   if (analogvalue<2700) {
   // if (batteryVoltage>53) {2968
        digitalWrite(D3,HIGH);
        Particle.publish("my-event","High");
        mqttPublish("my-event", "High");

    }
 else if (analogvalue>2938) {
//    else if (batteryVoltage<53.8) {3012
        digitalWrite(D3,LOW);
        Particle.publish("my-event","Low");
        mqttPublish("my-event", "Low");
   }
   else {

   }


}


/*******************************************************************************
 * Function Name  : tinkerDigitalRead
 * Description    : Reads the digital value of a given pin
 * Input          : Pin
 * Output         : None.
 * Return         : Value of the pin (0 or 1) in INT type
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerDigitalRead(String pin)
{
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		pinMode(pinNumber, INPUT_PULLDOWN);
		return digitalRead(pinNumber);
	}
	else if (pin.startsWith("A"))
	{
		pinMode(pinNumber+10, INPUT_PULLDOWN);
		return digitalRead(pinNumber+10);
	}
	return -2;
}

/*******************************************************************************
 * Function Name  : tinkerDigitalWrite
 * Description    : Sets the specified pin HIGH or LOW
 * Input          : Pin and value
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerDigitalWrite(String command)
{
	bool value = 0;
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(command.substring(3,7) == "HIGH") value = 1;
	else if(command.substring(3,6) == "LOW") value = 0;
	else return -2;

	if(command.startsWith("D"))
	{
		pinMode(pinNumber, OUTPUT);
		digitalWrite(pinNumber, value);
		return 1;
	}
	else if(command.startsWith("A"))
	{
		pinMode(pinNumber+10, OUTPUT);
		digitalWrite(pinNumber+10, value);
		return 1;
	}
	else return -3;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogRead
 * Description    : Reads the analog value of a pin
 * Input          : Pin
 * Output         : None.
 * Return         : Returns the analog value in INT type (0 to 4095)
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerAnalogRead(String pin)
{
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		return -3;
	}
	else if (pin.startsWith("A"))
	{
		return analogRead(pinNumber+10);
	}
	return -2;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogWrite
 * Description    : Writes an analog value (PWM) to the specified pin
 * Input          : Pin and Value (0 to 255)
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerAnalogWrite(String command)
{
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	String value = command.substring(3);

	if(command.startsWith("D"))
	{
		pinMode(pinNumber, OUTPUT);
		analogWrite(pinNumber, value.toInt());
		return 1;
	}
	else if(command.startsWith("A"))
	{
		pinMode(pinNumber+10, OUTPUT);
		analogWrite(pinNumber+10, value.toInt());
		return 1;
	}
	else return -2;
}


