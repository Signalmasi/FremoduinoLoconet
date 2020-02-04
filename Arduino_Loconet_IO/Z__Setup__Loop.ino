//////////////////////////////
// Ab hier nichts ändern
//////////////////////////////

void setup()
{
  //Serial.begin(9600);
  
  Setup_InputOutput();
  
  Setup_Loconet();
}

void loop()
{  
  looptime = millis();
  
  Loop_InputOutput();
  
  Loop_Loconet();
}
