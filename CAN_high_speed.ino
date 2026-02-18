#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS 10
MCP_CAN CAN(CAN_CS);

/* =========================
   ALIVE COUNTERS
   ========================= */
uint8_t pcmAlive  = 0;
uint8_t absAlive  = 0;
uint8_t bcmAlive  = 0;
uint8_t fuelAlive = 0;

/* =========================
   TIMING
   ========================= */
unsigned long t10    = 0;
unsigned long t20    = 0;
unsigned long t100   = 0;
unsigned long tSweep = 0;

/* =========================
   IGNITION STATES
   ========================= */
enum { IGN_OFF, IGN_ACC, IGN_RUN };
uint8_t ignState = IGN_OFF;

/* =========================
   FLAGS
   ========================= */
bool sweepEnable = true;
bool milOn = false;
bool turnLeft = false;
bool turnRight = false;
bool hazard = false;

/* =========================
   VALUES
   ========================= */
int rpm = 0;
int speed = 90;        // km/h
uint8_t fuel = 120;   // raw
uint8_t coolant = 90; // °C

/* =========================
   PCM ALIVE (0x0C0)
   ========================= */
void sendPCMAlive() {
  uint8_t d[8] = {0};
  d[0] = 0x01;
  d[5] = pcmAlive;
  d[6] = (ignState == IGN_RUN);
  d[7] = 0xA5;

  CAN.sendMsgBuf(0x0C0, 0, 8, d);
  pcmAlive = (pcmAlive + 1) & 0x0F;
}

/* =========================
   RPM (0x0C8)
   ========================= */
void sendRPM() {
  uint16_t raw = rpm * 4;
  uint8_t d[8] = {0};
  d[2] = raw >> 8;
  d[3] = raw & 0xFF;
  CAN.sendMsgBuf(0x0C8, 0, 8, d);
}

/* =========================
   ABS / SPEED (0x0B4)
   ========================= */
void sendABS() {
  uint16_t raw = speed * 100;
  uint8_t d[8] = {0};
  d[0] = raw >> 8;
  d[1] = raw & 0xFF;
  d[5] = absAlive;
  CAN.sendMsgBuf(0x0B4, 0, 8, d);
  absAlive = (absAlive + 1) & 0x0F;
}

/* =========================
   BCM / IGN (0x3B2)
   ========================= */
void sendBCM() {
  uint8_t d[8] = {0};

  if (ignState == IGN_ACC) d[0] = 0x10;
  if (ignState == IGN_RUN) d[0] = 0x20;

  d[5] = bcmAlive;
  CAN.sendMsgBuf(0x3B2, 0, 8, d);
  bcmAlive = (bcmAlive + 1) & 0x0F;
}

/* =========================
   FUEL (0x3A6)
   ========================= */
void sendFuel() {
  uint8_t d[8] = {0};
  d[1] = fuel;
  d[5] = fuelAlive;
  d[6] = 0x03;
  //CAN.sendMsgBuf(0x388, 0, 8, d); // 0x3A6
  fuelAlive = (fuelAlive + 1) & 0x0F;
  unsigned char fuel_data[8] = {0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  CAN.sendMsgBuf(0x388, 0, 8, fuel_data);
}

/* =========================
   COOLANT (0x420)
   ========================= */
void sendCoolant() {
  uint8_t d[8] = {0};
  d[0] = coolant + 40; // Ford offset
  d[2] = 0xff;
  d[6] = 0x01;
  CAN.sendMsgBuf(0x420, 0, 8, d);
}

/* =========================
   MIL (0x3E8)
   ========================= */
void sendMIL() {
  uint8_t d[8] = {0};
  if (milOn) d[0] = 0x80;
  CAN.sendMsgBuf(0x3E8, 0, 8, d);
}

/* =========================
   TURN SIGNALS (0x3C3)
   ========================= */
void sendTurnSignals() {
  uint8_t d[8] = {0};
  if (turnLeft)  d[0] |= 0x01;
  if (turnRight) d[0] |= 0x02;
  if (hazard)    d[0] |= 0x04;
  //CAN.sendMsgBuf(0x3C3, 0, 8, d);
  CAN.sendMsgBuf(0x430, 0, 8, d);
}

/* =========================
   SWEEP LOGIC
   ========================= */
void updateSweep() {
  if (!sweepEnable) return;

  rpm += 200;
  if (rpm > 6000) rpm = 0;

  speed += 5;
  if (speed > 200) speed = 0;

  fuel += 2;
  if (fuel > 240) fuel = 10;
}

/* =========================
   CAN SNIFFER
   ========================= */
void sniffCAN() {
  if (CAN.checkReceive() != CAN_MSGAVAIL) return;

  long unsigned int rxId;
  unsigned char len;
  unsigned char buf[8];

  CAN.readMsgBuf(&rxId, &len, buf);

  //if (rxId == 0x190) return;
  //if (rxId == 0x430) return;
  
  Serial.print(millis());
  Serial.print(" 0x");
  Serial.print(rxId, HEX);
  Serial.print(" [");
  Serial.print(len);
  Serial.print("] ");

  for (uint8_t i = 0; i < len; i++) {
    if (buf[i] < 0x10) Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

/* =========================
   SERIAL COMMANDS
   ========================= */
void handleSerial() {
  //Serial.println("input1 ready");
  if (!Serial.available()) return;
  Serial.println("input2 ready");
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  Serial.println(cmd);
  if (cmd == "i") ignState = IGN_OFF;
  else if (cmd == "a") ignState = IGN_ACC;
  else if (cmd == "r") ignState = IGN_RUN;
  else if (cmd == "s") sweepEnable = true;
  else if (cmd.startsWith("rpm ")) { rpm = cmd.substring(4).toInt(); sweepEnable = false; }
  else if (cmd.startsWith("spd ")) { speed = cmd.substring(4).toInt(); sweepEnable = false; }
  else if (cmd.startsWith("tmp ")) coolant +=5; //cmd.substring(4).toInt();
  else if (cmd.startsWith("fuel ")) fuel = cmd.substring(5).toInt();
  else if (cmd == "mil on") milOn = true;
  else if (cmd == "mil off") milOn = false;
  else if (cmd == "l on")  { turnLeft = true;  turnRight = hazard = false; }
  else if (cmd == "l off") turnLeft = false;
  else if (cmd == "r on")  { turnRight = true; turnLeft = hazard = false; }
  else if (cmd == "r off") turnRight = false;
  else if (cmd == "h on")  { hazard = true; turnLeft = turnRight = false; }
  else if (cmd == "h off") hazard = false;
}

/* =========================
   SETUP
   ========================= */
void setup() {
  Serial.begin(115200);

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    delay(200);
  }

  //CAN.setMode(MCP_NORMAL);
  // Sätt läget till Normal Mode
  byte status = CAN.setMode(MCP_NORMAL);

  if(status == MCP2515_OK) {
    Serial.println("Läge ändrat till Normal Mode.");
  } else {
    Serial.println("Misslyckades med att byta läge.");
  }
  Serial.println("Ford cluster bench simulator ready");
}

/* =========================
   LOOP
   ========================= */
void loop() {
  unsigned long now = millis();

  sniffCAN();
  handleSerial();

  if (now - t10 >= 10) {
    if (ignState == IGN_RUN) {
      sendPCMAlive();
      sendRPM();
      unsigned char ford_data[8] = {0x80, 0x06, 0x01, 0x50, 0x3C, 0x0D, 0x03, 0x9A};
      CAN.sendMsgBuf(0x190, 0, 8, ford_data);
      unsigned char rpmMsg[8] = {0x02, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      //                                                      speed
      unsigned char ecu_status[8] = {0x0E, 0xE0, 0x00, 0x08, 0x2E, 0x20, 0x00, 0x00};
      unsigned char focus_speed[8] = {0x00, 0x00, 0x00, 0x00, 0x4E, 0x20, 0x00, 0x00};
      CAN.sendMsgBuf(0x201, 0, 8, ecu_status);
      //unsigned char temp_status[8] = {0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      //CAN.sendMsgBuf(0x420, 0, 8, temp_status);
      //unsigned char abs_speed[8] = {0x4E, 0x20, 0x4E, 0x20, 0x4E, 0x20, 0x4E, 0x20};
      //CAN.sendMsgBuf(0x4B0, 0, 8, abs_speed);
      // Status OK för ABS, Bromsar och ESP
      unsigned char abs_ok[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

      // Hjulhastighet 0 km/h (Offset 10000 = 0x2710)
      unsigned char wheel_speed_zero[8] = {0x27, 0x10, 0x27, 0x10, 0x27, 0x10, 0x27, 0x10};
      CAN.sendMsgBuf(0x212, 0, 8, abs_ok);
      //CAN.sendMsgBuf(0x4B0, 0, 8, wheel_speed_zero);
      unsigned char airbag_ok[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      CAN.sendMsgBuf(0x050, 0, 8, airbag_ok);
    }
    t10 = now;
  }

  if (now - t20 >= 20) {
    if (ignState == IGN_RUN) 
    {
      //Serial.println("ABS");
      sendABS();
    }
    t20 = now;
  }

  if (now - t100 >= 100) {
    sendBCM();
    sendFuel();
    sendCoolant();
    sendMIL();
    sendTurnSignals();
    t100 = now;
  }

  if (now - tSweep >= 50) {
    updateSweep();
    tSweep = now;
  }
}
