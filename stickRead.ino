/* Define the pins for the analog stick */
#define joyX A0
#define joyY A1

/* Define the pins for the push buttons */
int SwitchPin1 = 2;
int SwitchPin2 = 3;
int SwitchPin3 = 4;
int SwitchPin4 = 5;

/* Define the structure of the controller state */
struct ControllerState
{
  /* Joy Stick Values */
  int xVal;
  int yVal;

  /* Push buttons */
  int btn1;
  int btn2;
  int btn3;
  int btn4;
};

struct ControllerState state;

/* Setup code. Runs on startup */
void setup() 
{
  /* Initialize Serial Communications */
  Serial.begin(9600);

  /* Set push buttons as inputs */
  pinMode(SwitchPin1, INPUT);
  pinMode(SwitchPin2, INPUT);
  pinMode(SwitchPin3, INPUT);
  pinMode(SwitchPin4, INPUT);

  // /* Activate the push buttons */
  digitalWrite(SwitchPin1, HIGH);
  digitalWrite(SwitchPin2, HIGH);
  digitalWrite(SwitchPin3, HIGH);
  digitalWrite(SwitchPin4, HIGH);
  
  /* Initialize the controler state */
  state.btn1 = 0;
  state.btn2 = 0;
  state.btn3 = 0;
  state.btn4 = 0;
  state.xVal = 0;
  state.yVal = 0;
}
 
 /* This code runs repeatedly */
void loop()
{
  String strOutput = "";
  String strX = "";
  String strY = "";
  int pad = 0;
  /* Determine the state of the controller */
  state.xVal = analogRead(joyX);
  state.yVal = analogRead(joyY);
  state.btn1 = digitalRead(SwitchPin1);
  state.btn2 = digitalRead(SwitchPin2);
  state.btn3 = digitalRead(SwitchPin3);
  state.btn4 = digitalRead(SwitchPin4);

  /* Format the message and send over serial */
  strX = String(state.xVal);
  pad = 4 - strX.length();
  for(int p = 0; p < pad; p++)
  {
    strX = "0" + strX;
  }

  strOutput += strX;
  strOutput += "-";

  strY = String(state.yVal);
  pad = 4 - strY.length();
  for(int p = 0; p < pad; p++)
  {
    strY = "0" + strY;
  }

  strOutput += strY;
  strOutput += "-";
  strOutput += String(state.btn1);
  strOutput += "-";
  strOutput += String(state.btn2);
  strOutput += "-";
  strOutput += String(state.btn3);
  strOutput += "-";
  strOutput += String(state.btn4);
  strOutput += "_";
  Serial.println(strOutput);
}
