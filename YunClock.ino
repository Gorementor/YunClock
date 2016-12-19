// ------------------------------------------------------
// Header files

#include <SPI.h>
#include <Wire.h>
#include <Process.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// ------------------------------------------------------
// Defines and Globals

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define RTC_I2C_ADDRESS 0x68 // rtc I2C address
#define OLED_I2C_ADDRESS 0x3C // oled I2C address

#define OLED_RESET 4  // oled reset pin
#define US_TRIGGER 12 // ultrasound trigger pin
#define US_ECHO 13    // ultrasound echo pin

#define US_MAX_DISTANCE 40  // distance in cm for activating the display

Process Date;
byte Seconds = 0;
byte Minutes = 0;
byte Hours = 0;
byte DayOfWeek = 0;
byte Days = 0;
byte Months = 0;
byte Years = 0;


// ------------------------------------------------------
// Forward declarations

Adafruit_SSD1306 display(OLED_RESET); // reset oled

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
	return ((val / 10 * 16) + (val % 10));
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
	return ((val / 16 * 10) + (val % 16));
}

// init time for rtc
void setTime(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
	// sets time and date data to DS3231
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	Wire.write(0); // set next input to start at the seconds register
	Wire.write(decToBcd(second)); // set seconds
	Wire.write(decToBcd(minute)); // set minutes
	Wire.write(decToBcd(hour)); // set hours
	Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
	Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
	Wire.write(decToBcd(month)); // set month
	Wire.write(decToBcd(year)); // set year (0 to 99)
	Wire.endTransmission();
}

// read values from rtc
void readTime(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year)
{
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	Wire.write(0); // set RTC register pointer to 00h
	Wire.endTransmission();
	Wire.requestFrom(RTC_I2C_ADDRESS, 7);
	// request seven bytes of data from RTC starting from register 00h
	*second = bcdToDec(Wire.read() & 0x7f);
	*minute = bcdToDec(Wire.read());
	*hour = bcdToDec(Wire.read() & 0x3f);
	*dayOfWeek = bcdToDec(Wire.read());
	*dayOfMonth = bcdToDec(Wire.read());
	*month = bcdToDec(Wire.read());
	*year = bcdToDec(Wire.read());
}

void displayTime()
{
	// retrieve data from RTC
	readTime(&Seconds, &Minutes, &Hours, &DayOfWeek, &Days, &Months, &Years);

	if (Hours < 10)
	{
		display.print("0");
	}
	display.print(Hours, DEC);
	// convert the byte variable to a decimal number when displayed
	display.print(":");

	if (Minutes < 10)
	{
		display.print("0");
	}
	display.print(Minutes, DEC);
	display.print(":");
	if (Seconds < 10)
	{
		display.print("0");
	}
	display.print(Seconds, DEC);
	display.print(" ");

	switch (DayOfWeek) {
	case 0: // (E)rror
		display.print("E");
		break;
	case 1:	// (M)ontag
	case 3: // (M)ittwoch
		display.print("M");
		break;
	case 2: // (D)ienstag
	case 4: // (D)onnerstag
		display.print("D");
		break;
	case 5: // (F)reitag
		display.print("F");
		break;
	case 6: // (S)amstag
	case 7: // (S)onntag
		display.print("S");
		break;
	}

	if (Days < 10)
	{
		display.print("0");
	}

	display.print(Days, DEC);
	display.print(".");

	if (Months < 10)
	{
		display.print("0");
	}
	display.print(Months, DEC);
	display.print(".");

	display.print(Years, DEC);
	display.print(" ");

	switch (DayOfWeek) {
	case 0: // E(r)ror
		display.print("r");
		break;
	case 1: // M(o)ntag
	case 4: // D(o)nnerstag
	case 7: // S(o)nntag
		display.print("o");
		break;
	case 2: // D(i)enstag
	case 3: // M(i)ttwoch
		display.print("i");
		break;
	case 5: // F(r)eitag
		display.print("r");
		break;
	case 6: // S(a)mstag
		display.print("a");
		break;
	}
}

// synchronize date and time from open wrt
void synchronizeTime()
{
	if (!Date.running())
	{
		Date.begin("date");
		Date.addParameter("+%T-%D-%u");
		Date.run();
	}

	while (Date.available() > 0)
	{
		String timeString = Date.readString();

		byte firstColon = timeString.indexOf(":");
		byte secondColon = timeString.lastIndexOf(":");
		byte thirdColon = timeString.indexOf("-");
		byte fourthColon = timeString.indexOf("/");
		byte fifthColon = timeString.lastIndexOf("/");
		byte sixthColon = timeString.lastIndexOf("-");

		Hours = timeString.substring(0, firstColon).toInt();
		Minutes = timeString.substring(firstColon + 1, secondColon).toInt();
		Seconds = timeString.substring(secondColon + 1, thirdColon).toInt();
		Months = timeString.substring(thirdColon + 1, fourthColon).toInt();
		Days = timeString.substring(fourthColon + 1, fifthColon).toInt();
		Years = timeString.substring(fifthColon + 1, sixthColon).toInt();
		DayOfWeek = timeString.substring(sixthColon + 1).toInt();
	}

	// init rtc - set initial time
	// time: seconds, minutes, hours, weekday, day, month, year
	setTime(Seconds, Minutes, Hours, DayOfWeek, Days, Months, Years);
}

long checkDistance()
{
	long duration, distance;
	digitalWrite(US_TRIGGER, LOW); // init trigger
	delayMicroseconds(2);
	digitalWrite(US_TRIGGER, HIGH); // send us ping
	delayMicroseconds(10);  // await echo
	digitalWrite(US_TRIGGER, LOW); // reset trigger
	duration = pulseIn(US_ECHO, HIGH);
	return distance = (duration / 2) / 29.1;
}

// ------------------------------------------------------
// Setup

void setup()
{
	Wire.begin();
	Bridge.begin();

	// get time from OpenWRT
	synchronizeTime();

	// init oled - reset buffer
	display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
	display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.display(); // inherit changes

					   // init us - set pin direction
	pinMode(US_TRIGGER, OUTPUT);
	pinMode(US_ECHO, INPUT);
}

// ------------------------------------------------------
// Loop operation

void loop()
{
	int waitSec = 0;

	while (checkDistance() < US_MAX_DISTANCE)
	{

		if (waitSec > 50)
		{
			display.clearDisplay();
			display.setCursor(0, 0);
			display.print("Sync...");
			display.display();
			synchronizeTime();
			delay(1000); // wait
			waitSec = 0;
		}

		display.clearDisplay();
		display.setCursor(0, 0);
		displayTime(); // display the real-time clock data
		display.display();
		delay(100); // wait

		waitSec++;
	}

	display.clearDisplay();
	display.display();

	if ((Hours == 0) && (Minutes == 5) && ((Seconds == 0) || (Seconds == 1)))
	{ // sync once a day
		synchronizeTime();
		delay(1000); // wait
	}

	delay(1000); // wait
}
