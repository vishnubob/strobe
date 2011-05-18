#include <Servo.h>

Servo ESC;

void setup()
{
    ESC.attach(8);
}

void loop()
{
    Prompt();
}

void Prompt()
{
    static long v = 0;

    SerialUSB.print("Value: ");
    SerialUSB.println(v);
    SerialUSB.print("Servo: ");
    SerialUSB.println(ESC.read());
    SerialUSB.print("> ");

    while (SerialUSB.available() == 0) 
    {}

    char ch = SerialUSB.read();

    switch(ch) 
    {
        case '0'...'9':
            v = v * 10 + ch - '0';
            break;
        case '-':
            v *= -1;
            break;
        case 'z':
            v = 0;
            break;
        case 's':
            v = max(0, min(180, v));
            ESC.write(v);
            v = 0;
            break;
        case 'q':
            ESC.write(0);
            break;
        case 'w':
            ESC.write(180);
            break;
    }
    SerialUSB.println("");
}
    


