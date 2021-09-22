#include <ps2_protocol.h>
#include <digitalWriteFast.h>

static PS2P_GLOBAL(mouseps2);    // PS2 Mouse object

static const int kPin_X1 = 9;
static const int kPin_X2 = 10;
static const int kPin_Y1 = 11;
static const int kPin_Y2 = 12;
static const int kPin_B1 = 4;         // Amiga: Left mouse
static const int kPin_B2 = 5;         // Amiga: Right mouse
static const int kPin_B3 = 2;         // Amiga: Middle mouse (Interrupt)
static const int kPin_PS2Data = 8;    // PS2: Data
static const int kPin_PS2Clock = 3;   // PS2: Clock (Interrupt)

const bool g_bChan1[] = {HIGH, LOW,  LOW, HIGH};
const bool g_bChan2[] = {HIGH, HIGH, LOW, LOW };

uint8_t ps2mousetype = 0;
uint8_t g_uStageX, g_uStageY; // our current point in the sequence (index into the channel patterns)
uint32_t g_uLastMouseUpdateX; // last time X channel was updated ... in microseconds
uint32_t g_uLastMouseUpdateY;

volatile uint8_t MouseButton1Status = HIGH;
volatile uint8_t MouseButton2Status = HIGH;
volatile uint8_t MouseButton3Status = HIGH;
volatile uint8_t MouseButton3StatusOld = HIGH;
volatile int8_t WheelPosition;
volatile uint32_t InterruptStartTime;

uint8_t MouseWaitRead()
{
  while (!mouseps2.available());
  return mouseps2.read();
}

// Show an error message by flashing the on-board LED
void err(int flashcount)
{
  while (1)
  {
    for (int i = 0; i < flashcount; i++)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
      delay(300);
    }
    delay(2000);
  }
}

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(kPin_X1, OUTPUT);
  pinMode(kPin_X2, OUTPUT);
  pinMode(kPin_Y1, OUTPUT);
  pinMode(kPin_Y2, OUTPUT);

  pinMode(kPin_B1, OUTPUT);
  pinMode(kPin_B2, OUTPUT);
  pinMode(kPin_B3, INPUT_PULLUP);    // Used by the amiga to signal we want wheel information

  digitalWriteFast(kPin_X1, HIGH);
  digitalWriteFast(kPin_X2, HIGH);
  digitalWriteFast(kPin_Y1, HIGH);
  digitalWriteFast(kPin_Y2, HIGH);
  digitalWriteFast(kPin_B1, HIGH);
  digitalWriteFast(kPin_B2, HIGH);
  digitalWriteFast(kPin_B3, HIGH);

  if (!mouseps2.begin(kPin_PS2Clock, kPin_PS2Data, NULL))
  {
    // Unable to begin PS2 protocol
    err(5);
  }

  mouseps2.write(0xff);                   // Reset
  if (MouseWaitRead() != 0xFA) err(2);
  if (MouseWaitRead() != 0xAA) err(2);
  MouseWaitRead();

  // Enable mouse wheel
  mouseps2.write(0xf3);                   // Set Sample Rate
  if (MouseWaitRead() != 0xFA) err(3);
  mouseps2.write(200);  //
  if (MouseWaitRead() != 0xFA) err(3);

  mouseps2.write(0xf3);                   // Set Sample Rate
  if (MouseWaitRead() != 0xFA) err(3);
  mouseps2.write(100);  //
  if (MouseWaitRead() != 0xFA) err(3);

  mouseps2.write(0xf3);                   // Set Sample Rate
  if (MouseWaitRead() != 0xFA) err(3);
  mouseps2.write(80);  //
  if (MouseWaitRead() != 0xFA) err(3);

  // Read ID
  mouseps2.writeAndWait(0xF2);
  if (MouseWaitRead() != 0xFA) err(4);
  ps2mousetype = MouseWaitRead();

  mouseps2.writeAndWait(0xf4);  // Enable Data Reporting
  if (MouseWaitRead() != 0xFA) err(6);

  attachInterrupt(digitalPinToInterrupt(kPin_B3), kPin_B3_Int, FALLING);
}

void MouseUpdateX()
{
  if (g_bChan1[g_uStageX]) digitalWriteFast(kPin_X1, 1) else digitalWriteFast(kPin_X1, 0);
  if (g_bChan2[g_uStageX]) digitalWriteFast(kPin_X2, 1) else digitalWriteFast(kPin_X2, 0);
}

void MouseUpdateY()
{
  if (g_bChan1[g_uStageY]) digitalWriteFast(kPin_Y1, 1) else digitalWriteFast(kPin_Y1, 0);
  if (g_bChan2[g_uStageY]) digitalWriteFast(kPin_Y2, 1) else digitalWriteFast(kPin_Y2, 0);
}

void MouseUpdate(uint8_t uX, uint8_t uY)
{
  unsigned long uStartTimeMS = millis();

  // convert unsigned byte of X and Y into actual movements. We use a separate bool to keep track of the
  // movement being negative, as that makes later code slightly simpler.
  bool bReverseX = false;
  if (uX & 0x80)
  {
    uX ^= 0xFF;
    ++uX;
    bReverseX = true;
  }

  bool bReverseY = false;
  if (uY & 0x80)
  {
    uY ^= 0xFF;
    ++uY;
    bReverseY = true;
  }

  // uData:
  //   bit 0 - left mouse button
  //   bit 1 - right mouse button
  //   bit 2 - middle mouse button (wheel pressed)
  //   bit 3 - always 1
  //   bit 4 - X sign bit
  //   bit 5 - Y sign bit
  //   bit 6 - X overflow
  //   bit 7 - Y overflow

  // Now we fake the quadrature pulses. The code seems slightly complicated,
  // because I was originally experimenting with different delays between
  // state changes, depending on the speed the mouse was moving. But this
  // doesn't seem to make much difference! (To the Archimedes, anyway.)

  const int kDelayUS = 10; // microseconds
  // (This is a constant, but if I ever decide to put independent, calculated
  // delays for each axis, then this will be what I replace.)

  while (uX || uY) // keep doing this until both uX and uY are 0 (no more movement to simulate)
  {
    uint32_t uTimeNowUS = micros();
    if (uX && (uTimeNowUS - g_uLastMouseUpdateX >= kDelayUS))
    {
      // time to move X axis on to next position
      if (bReverseX)
      {
        if (g_uStageX == 0) g_uStageX = 3; else --g_uStageX;
      }
      else
      {
        g_uStageX = (g_uStageX + 1) & 3;
      }

      --uX;

      MouseUpdateX();
      g_uLastMouseUpdateX = uTimeNowUS;
    }

    if (uY && (uTimeNowUS - g_uLastMouseUpdateY >= kDelayUS))
    {
      // time to move Y axis on to next position
      if (!bReverseY)   // reverse needed for Stream Mode!!!
      {
        if (g_uStageY == 0) g_uStageY = 3; else --g_uStageY;
      }
      else
      {
        g_uStageY = (g_uStageY + 1) & 3;
      }

      --uY;

      MouseUpdateY();
      g_uLastMouseUpdateY = uTimeNowUS;
    }
  }

  while ((millis() - uStartTimeMS) < 5)
  {
    delay(1);
  }
}

// Press/Release mouse button1
// Does nothing if status not changed (conflicts with interrupt protocol)
void MouseButton1(uint8_t b)
{
  if (MouseButton1Status != b)
  {
    if (b) digitalWriteFast(kPin_B1, 1) else digitalWriteFast(kPin_B1, 0);
    MouseButton1Status = b;
  }
}

// Press/Release mouse button2
// Does nothing if status not changed (conflicts with interrupt protocol)
void MouseButton2(uint8_t b)
{
  if (MouseButton2Status != b)
  {
    if (b) digitalWriteFast(kPin_B2, 1) else digitalWriteFast(kPin_B2, 0);
    MouseButton2Status = b;
  }
}

// Press/Release mouse button3
// Does nothing, handled in the interrupt
void MouseButton3(uint8_t b)
{
  if (MouseButton3Status != b)
  {
    MouseButton3StatusOld = MouseButton3Status;
    MouseButton3Status = b;
  }
}

void loop()
{
  int count = mouseps2.available();
  if (count >= 4)
  {
    byte uData = mouseps2.read();
    MouseButton1(!(uData & 1));
    MouseButton2(!(uData & 2));
    MouseButton3(!(uData & 4));

    uint8_t uX = mouseps2.read();
    uint8_t uY = mouseps2.read();
    if (ps2mousetype > 0)
    {
      WheelPosition = mouseps2.read();
    }
    MouseUpdate(uX, uY);
  }
//
//      Serial.print(" T=");
//      Serial.println(millis() - InterruptStartTime);
}

//
// Interrupt routine called whenever kPin_B3 goes LOW
// This is connected to pin 5 on the Amiga (Right mouse button)
// We use the left/middle/right mouse buttons as a protocol, giving us 8 different commands
//
void kPin_B3_Int()
{
  if (millis() - InterruptStartTime < 18)
  {
    InterruptStartTime = millis();
    return;   // Max once per frame or so
  }

  // If the right/left mouse button is currently being pressed do nothing
  if (MouseButton2Status == HIGH && MouseButton1Status == HIGH)
  {
    // We only send 1 code per interrupt
    int code = 0;                       // Do nothing
    if (WheelPosition > 0)              // Scroll Down
    {
      code = 1;
    }
    else if (WheelPosition < 0)         // Scroll Up
    {
      code = 2;
    }
    else if (MouseButton3Status == LOW && MouseButton3StatusOld == HIGH)
    {
      code = 3;
    }
    else if (MouseButton3Status == HIGH && MouseButton3StatusOld == LOW)
    {
      code = 4;
    }

    // 'Press' the buttons
    if (code & 1)
    {
      digitalWriteFast(kPin_B2, LOW);   // Right Mouse
    }
    if (code & 2)
    {
      pinMode(kPin_B3, OUTPUT);         // Middle Mouse
      digitalWriteFast(kPin_B3, HIGH);
    }
    if (code & 4)
    {
      digitalWriteFast(kPin_B1, LOW);   // Left Mouse
    }

    // Wait a bit
    delayMicroseconds(20);

    // Release the buttons
    if (code & 1)
    {
      digitalWriteFast(kPin_B2, HIGH);
    }
    if (code & 2)
    {
      pinMode(kPin_B3, INPUT_PULLUP);   // pinModeFast()??
    }
    if (code & 4)
    {
      digitalWriteFast(kPin_B1, HIGH);
    }

    // There events are handled now
    if (code == 1 || code == 2) WheelPosition = 0;
    if (code == 3 || code == 4) MouseButton3StatusOld = MouseButton3Status;
  }

  InterruptStartTime = millis();
}

