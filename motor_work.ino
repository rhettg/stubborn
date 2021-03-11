
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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  for (int motor = 0; motor < 2; motor++) {
    pinMode(enablePin[motor], OUTPUT);
    pinMode(motorPin1[motor], OUTPUT);
    pinMode(motorPin2[motor], OUTPUT);
  }

  setMotor(0, 0);
  setMotor(1, 0);
}

int motorASpeed = 0;
int motorBSpeed = 0;
bool autoControl = false;

void loop() {
  int joyYPosition = analogRead(joyYPin);
  int joyXPosition = analogRead(joyXPin);

  joyYPosition = 512;
  joyXPosition = 512;

  if (autoControl && (abs(joyYPosition - 512) > 128 || abs(joyXPosition - 512) > 128)) {
    autoControl = false;
    Serial.println("manual control enabled");
  }


  if (Serial.available() > 0) {
    motorASpeed = Serial.parseInt();
    motorBSpeed = Serial.parseInt();

    if (!autoControl) {
      Serial.println("manual control disabled");
      autoControl = true;
    }
    
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  if (!autoControl) {
    int YVector = map(joyYPosition, 0, 1023, -255, 255);
    int XVector = map(joyXPosition, 0, 1023, -255, 255);
  
    //Serial.print(XVector);
    //Serial.print(",");
    //Serial.println(YVector);
    
    int newMotorASpeed = constrain(YVector - XVector, -255, 255);
    int newMotorBSpeed = constrain(YVector + XVector, -255, 255);
  
    if (abs(newMotorASpeed) < 10) {
      newMotorASpeed = 0;
    }
    
    if (abs(newMotorBSpeed) < 10) {
      newMotorBSpeed = 0;
    }
  
    if (abs(newMotorASpeed - motorASpeed) > 20 || abs(newMotorBSpeed - motorBSpeed) > 20) {
      Serial.print(newMotorASpeed);
      Serial.print(", ");
      Serial.println(newMotorBSpeed);
    }
  
    motorASpeed = newMotorASpeed;
    motorBSpeed = newMotorBSpeed;
  }

  setMotor(0, motorASpeed);
  setMotor(1, motorBSpeed);
}
