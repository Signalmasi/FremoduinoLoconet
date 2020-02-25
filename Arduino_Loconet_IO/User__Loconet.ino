void getWeiche(uint16_t Address, uint8_t State) //Reaktion auf Weichenbefehl auf dem Loconet
{

}

void getWeicheLage(uint16_t Address, uint8_t State) //Reaktion auf Weichenrückmeldung auf dem Loconet
{
  //State = 0x00: Lage_Undefiniert
  //State = 0x01: Lage_Abzweigend
  //State = 0x10: Lage_Gerade  
}

void getSignal(uint16_t Address, uint8_t SPD_AX, uint8_t SPD_XA, uint8_t SE_STAT) //Reaktion auf Signalbefehl auf dem Loconet
{

}

void getSensor(uint16_t Address, uint8_t State) //Reaktion auf Rückmeldebefehl auf dem Loconet
{
  //setWeiche(17, 0);
}

void getGPON() //Reaktion auf GlobalPowerOn auf dem Loconet
{
   
}

void getGPOFF() //Reaktion auf GlobalPowerOff auf dem Loconet
{
  //Tu was  
}

/////////////////////////////////////////////////////////////////////
// Ab hier nichts ändern
/////////////////////////////////////////////////////////////////////

// Weiche 0 = thrown = abzweig     1 = closed = gerade
// Input  0 = frei = Low           1 = besetzt = High

/* Beispiele

Weiche stellen

setWeicheBuf(82, Abzweig); //Stellt Weiche 82 auf Abzweig
------------------------------------------------------------------------
Signal stellen

setSignalBuf(106, SPD_GO); //Stellt Signal 106 auf Fahrt
------------------------------------------------------------------------
Sensor melden

setSensorBuf(314, Besetzt); //Abschnitt 314 als besetzt melden
------------------------------------------------------------------------
Nothalt senden

setGPOFF();

und zurücknehmen

setGPON();


*/
