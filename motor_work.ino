
int enablePin[] = {10, 5};
int motorPin1[] = {9, 7};
int motorPin2[] = {8, 6};

int joyYPin = 0;
int joyXPin = 1;

void setMotor(int motor, int speed) {
  int out = 0;
  
  if (speed > 0) {
    digitalWrite(motorPin1[motor], HIGH);
    digitalWrite(motorPin2[motor], LOW);

    out = speed;
  } else {
    digitalWrite(motorPin1[motor], LOW);
    digitalWrite(motorPin2[motor], HIGH);

    out = -speed;
  }
  
  analogWrite(enablePin[motor], out);
}

char *cmd = NULL;
int motorASpeed = 0;
int motorBSpeed = 0;

const int MAX_CMD = 64;

int lastPing = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  for (int motor = 0; motor < 2; motor++) {
    pinMode(enablePin[motor], OUTPUT);
    pinMode(motorPin1[motor], OUTPUT);
    pinMode(motorPin2[motor], OUTPUT);
  }

  cmd = (char*)malloc(MAX_CMD);

  setMotor(0, 0);
  setMotor(1, 0);

  Serial.println("HELLO");
}



void loop() {
  

  //if (millis() - lastPing > 1000) {
    //Serial.println("PONG");
    //lastPing = millis(); 
  //}

  if (Serial.available() > 0) {
    size_t n = Serial.readBytesUntil('\n', cmd, MAX_CMD);
    if (n == 0 || n == MAX_CMD) {
      Serial.println("invalid command");
    } else {
      cmd[n] = 0; // for extra safety
      
      Serial.print("parsing '");
      Serial.print(cmd);
      Serial.println("'");
      
      if (parseCmd(cmd)) {
        Serial.println("OK");
      } else {
        Serial.println("FAIL - failed to parse");
      }
    }

    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  setMotor(0, motorASpeed);
  setMotor(1, motorBSpeed);
}

bool parseCmd(char *cmd) {
  int v1, v2;

  if (strncmp(cmd, "FWD ", 4) == 0) {
    Serial.println("got a FWD");
    Serial.println(cmd+4);
    v1 = atoi(cmd+4);
    Serial.println(v1);
    if (v1 > 0 && v1 < 256) {
      motorASpeed = v1;
      motorBSpeed = v1;

      return true;
    }
  } else if (strncmp(cmd, "BCK ", 4) == 0) {
    v1 = atoi(cmd+4);
    if (v1 > 0 && v1 < 256) {
      motorASpeed = -v1;
      motorBSpeed = -v1;

      return true;
    }
  } else if (strncmp(cmd, "STOP", 4) == 0) {
      motorASpeed = 0;
      motorBSpeed = 0;

      return true;
  }

  return false;
}
