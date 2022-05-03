//////////////////////////////
// Konfiguration
//////////////////////////////

const uint8_t LN_SendPin = 7;

const boolean Uhlenbrock_Offset = true; //bei true ist die erste Adresse mit 1 bezeichnet, sonst mit 0. Entspricht der Nomenklatur kommerzieller Produkte
const boolean Elekdra_Feedback = false; //bei true ist die Adressierung kompatibel zu Elekdra bzw. Watti-Platinen, sonst wie bei Uhlenbrock, Deloof, John Jabour etc.

#define Thrown            0b00
#define Closed            0b01
#define Abzweig           0b00
#define Gerade            0b01
#define Afbuigend         0b00
#define Recht             0b01
#define Lage_Gerade       0b10
#define Lage_Abzweigend   0b01
#define Lage_Undefiniert  0b00
#define Frei              0b00
#define Besetzt           0b01

/*#define SPD_STOP          0x00
#define SPD_GO_10         0x08
#define SPD_GO_20         0x09
#define SPD_GO_30         0x0A
#define SPD_GO_40         0x0B
#define SPD_GO_50         0x0C
#define SPD_GO_60         0x0D
#define SPD_GO_70         0x0E
#define SPD_GO_80         0x0F
#define SPD_GO_100        0x10
#define SPD_GO_110        0x11
#define SPD_GO_120        0x12
#define SPD_GO            0x04
#define SPD_STOP_EMRED    0x02
#define SPD_SHUNT_GO      0x14
#define SPD_DARK          0x15
#define SPD_KENNLICHT     0x1F
#define SPD_SUBSTITUTION  0x01
#define SPD_COUNTERLINE   0x03*/

#define SPD_STOP          0x00
#define SPD_GO_10         0x02
#define SPD_GO_20         0x04
#define SPD_GO_30         0x06
#define SPD_GO_40         0x08
#define SPD_GO_50         0x0A
#define SPD_GO_60         0x0C
#define SPD_GO_70         0x0E
#define SPD_GO_80         0x10
#define SPD_GO_90         0x12
#define SPD_GO_100        0x14
#define SPD_GO_110        0x16
#define SPD_GO_120        0x18
#define SPD_GO_130        0x1A
#define SPD_GO            0x3F
#define SPD_STOP_EMRED    0x40
#define SPD_SHUNT_GO      0x41
#define SPD_DARK          0x42
#define SPD_KENNLICHT     0x43
#define SPD_JOKER         0x44
#define SPD_SUBSTITUTION  0x45
#define SPD_COUNTERLINE   0x46
#define SPD_CAUTIOUSNESS  0x47

//////////////////////////////
// Ab hier nichts ändern
//////////////////////////////
// Variablen
//////////////////////////////

static  lnMsg        *LnPacket;
static  LnBuf        LnTxBuffer;

uint8_t              Buf_WriteIndex;
uint8_t              Buf_Load;
unsigned long        Buf_Steptime;
boolean              Buf_Sperre;
const uint8_t        N_SENDBUFFER = 64;

struct {
  uint8_t  Command;  //0: Nix, sonst Opcode
  uint16_t Address;
  uint8_t  State;
  uint8_t  Rest_Time;
} SendBuffer[N_SENDBUFFER];

/////////////////////////////////////////////////////////////////////
// Funktionen
/////////////////////////////////////////////////////////////////////

void getWeiche(uint16_t Address, uint8_t State);
void getWeicheLage(uint16_t Address, uint8_t State);
void getSignal(uint16_t Address, uint8_t SPD_AX, uint8_t SPD_XA, uint8_t SE_STAT);
void getSensor(uint16_t Address, uint8_t State);
void getGPON();
void getGPOFF();
void SendBufferSetWriteIndex();

LN_STATUS setWeiche(uint16_t Address, uint8_t Direction, uint8_t Output = 1);
LN_STATUS setWeicheLage(uint16_t Address, uint8_t State);
LN_STATUS setSignal(uint16_t Address, uint8_t SPD_AX, uint8_t SPD_XA = 0, uint8_t SE_STAT = 0);
LN_STATUS setSensor(uint16_t Address, uint8_t State);
LN_STATUS setGPON();
LN_STATUS setGPOFF();

void ReceiveLoconet()
{
  uint16_t decAddress;
  uint8_t State;
  
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive() ;
  if( LnPacket )
  {
    //handle OPC_SE messages for signal states
    if (LnPacket->se.command == OPC_SE)
    {
      decAddress = (LnPacket->se.addr_l | ( LnPacket->se.addr_h << 7 )) ;
      if (Uhlenbrock_Offset)
      {
        decAddress++;
      }
      
      //State = LnPacket->se.se2;
      //Serial.print("Receive Signal, Address "); Serial.print(decAddress); Serial.print(", State "); Serial.println(State);     
      getSignal(decAddress, LnPacket->se.se2, LnPacket->se.se3, LnPacket->se.se1);
    }
    //handle OPC_SW_REQ messages for turnout states
    if (LnPacket->srq.command == OPC_SW_REQ)
    {
      decAddress = (LnPacket->srq.sw1 | ( ( LnPacket->srq.sw2 & 0x0F ) << 7 )) ;
      if (Uhlenbrock_Offset)
      {
        decAddress++;
      }
      
      State = ((LnPacket->srq.sw2 & 0x20) >> 5) ;
      if ((LnPacket->srq.sw2 & 0xD0) == 0x10) //nur Befehle mit ON=1 beachten, keine OFF-Befehle
      {
        //Serial.print("Receive Weiche, Address "); Serial.print(decAddress); Serial.print(", State "); Serial.println(State); 
        getWeiche(decAddress, State);
      }
    }
    //handle OPC_SW_REP messages for turnout states
    if (LnPacket->srq.command == OPC_SW_REP)
    {
      decAddress = (LnPacket->srq.sw1 | ( ( LnPacket->srq.sw2 & 0x0F ) << 7 )) ;
      if (Uhlenbrock_Offset)
      {
        decAddress++;
      }
      
      State = ((LnPacket->srq.sw2 & 0x30) >> 4) ;
      if ((LnPacket->srq.sw2 & 0xC0) == 0x00) //nur Befehle mit 00CT-Struktur beachten
      {
        //Serial.print("Receive Weiche, Address "); Serial.print(decAddress); Serial.print(", State "); Serial.println(State); 
        getWeicheLage(decAddress, State);
      }
    }
    //handle OPC_INPUT_REP messages for occupancy states
    //according to Uhlenbrock-Code: High=occupied, Low=free, adresses expanded to 4096 with Bit I as LSB.
    //according to Elekdra-Code: High=free, Low=occupied, adresses up to 2048 with Bit I=0
    if (LnPacket->srq.command == OPC_INPUT_REP)
    {
      decAddress = (LnPacket->ir.in1 | ( ( LnPacket->ir.in2 & 0x0F ) << 7 )) ;
      State = ((LnPacket->ir.in2 & 0x10) >> 4);
      if (Elekdra_Feedback)
      {
        State = 1-State;
      }
      else
      {
        decAddress = (decAddress << 1) + ((LnPacket->ir.in2 & 0x20) >> 5);  
      }
      if (Uhlenbrock_Offset)
      {
        decAddress++;
      }
      //Serial.print("Receive Sensor, Address "); Serial.print(decAddress); Serial.print(", State "); Serial.println(State);
      getSensor(decAddress, State); 
    }
    if (LnPacket->srq.command == OPC_GPON)
    {
      getGPON();  
    }
    if (LnPacket->srq.command == OPC_GPOFF)
    {
      getGPOFF();  
    } 
  }
}

LN_STATUS setWeiche(uint16_t Address, uint8_t Direction, uint8_t Output = 1)
{
  lnMsg SendPacket ;
  
  if (Uhlenbrock_Offset)
  {
    Address--;
  }
  
  SendPacket.data[0] = OPC_SW_REQ ;
  SendPacket.data[1] = Address & 0x7F ;
  SendPacket.data[2] = ((Address >> 7) & 0x0F) ;
  
  if( Output )
    SendPacket.data[2] |= OPC_SW_REQ_OUT ;
  
  if( Direction )
    SendPacket.data[2] |= OPC_SW_REQ_DIR ;

  if (Address < 2048)
  {
    return LocoNet.send( &SendPacket ) ;
  }
}

LN_STATUS setWeicheLage(uint16_t Address, uint8_t State)
{
  lnMsg SendPacket ;
  
  if (Uhlenbrock_Offset)
  {
    Address--;
  }
  
  SendPacket.data[0] = OPC_SW_REP ;
  SendPacket.data[1] = Address & 0x7F ;
  SendPacket.data[2] = ((Address >> 7) & 0x0F) ;
  
  SendPacket.data[2] |= (State & 0x03) << 4 ;
  
  if (Address < 2048)
  {
    return LocoNet.send( &SendPacket ) ;
  }
}

LN_STATUS setSignal(uint16_t Address, uint8_t SPD_AX, uint8_t SPD_XA = 0, uint8_t SE_STAT = 0)
{
  lnMsg SendPacket ;

  if (Uhlenbrock_Offset)
  {
    Address--;
  }

  SendPacket.data[0] = OPC_SE ;
  SendPacket.data[1] = 0x09 ;
  SendPacket.data[2] = (Address >> 7) & 0x7F ;
  SendPacket.data[3] = Address & 0x7F ;
  SendPacket.data[4] = 0x01 ;
  SendPacket.data[5] = SE_STAT ;
  SendPacket.data[6] = SPD_AX ;
  SendPacket.data[7] = SPD_XA ;
  
  if (Address < 16384)
  {
    return LocoNet.send( &SendPacket ) ;
  }
}

LN_STATUS setSensor(uint16_t Address, uint8_t State)
{
  lnMsg SendPacket ;
  uint8_t I;

  if (Uhlenbrock_Offset)
  {
    Address--;
  }
  I=0;
  if (!Elekdra_Feedback)
  {
    I= Address%2;
    Address >>= 1;
  }
  else
  {
    State = 1-State;  
  }
  
  SendPacket.data[0] = OPC_INPUT_REP ;
  SendPacket.data[1] =  Address       & 0x7F ;
  SendPacket.data[2] = (Address >> 7) & 0x7F ;
 
  if (I)     {SendPacket.data[2] |= 0x20;}
  if (State) {SendPacket.data[2] |= 0x10;}
  SendPacket.data[2] |= 0x40; //X=1
  
  if (((Address < 4096) && (!Elekdra_Feedback)) || ((Address < 2048) && (Elekdra_Feedback)))
  {
    return LocoNet.send( &SendPacket ) ;
  }
}

LN_STATUS setGPON()
{
  lnMsg SendPacket ;
  SendPacket.data[0] = OPC_GPON ;
  return LocoNet.send( &SendPacket ) ;
}

LN_STATUS setGPOFF()
{
  lnMsg SendPacket ;
  SendPacket.data[0] = OPC_GPOFF ;
  return LocoNet.send( &SendPacket ) ;
}

void setWeicheBuf(uint16_t Address, uint8_t State)
{
  Buf_Sperre = true;  //Ausführung sperren
  if (Buf_Load == N_SENDBUFFER)
  {
    setWeiche(Address, State);
  }
  else
  {
    SendBuffer[Buf_WriteIndex].Command = OPC_SW_REQ;
    SendBuffer[Buf_WriteIndex].Address = Address;
    SendBuffer[Buf_WriteIndex].State   = State;
    Buf_Load++;
    SendBuffer[Buf_WriteIndex].Rest_Time += Buf_Load;
    SendBufferSetWriteIndex();
  }
  Buf_Sperre = false;
}

void setDelayWeiche(uint8_t DelayTime, uint16_t Address, uint8_t State)
{
  if (Buf_Load == N_SENDBUFFER)
  {
    setWeiche(Address, State);
  }
  else
  {
    SendBuffer[Buf_WriteIndex].Rest_Time = DelayTime;
    setWeicheBuf(Address, State);
  }
}

void setWeicheLageBuf(uint16_t Address, uint8_t State)
{
  Buf_Sperre = true;  //Ausführung sperren
  if (Buf_Load == N_SENDBUFFER)
  {
    setWeicheLage(Address, State);
  }
  else
  {
    SendBuffer[Buf_WriteIndex].Command = OPC_SW_REP;
    SendBuffer[Buf_WriteIndex].Address = Address;
    SendBuffer[Buf_WriteIndex].State   = State;
    Buf_Load++;
    SendBuffer[Buf_WriteIndex].Rest_Time += Buf_Load;
    SendBufferSetWriteIndex();
  }
  Buf_Sperre = false;
}

void setSignalBuf(uint16_t Address, uint8_t State)
{
  Buf_Sperre = true;  //Ausführung sperren
  if (Buf_Load == N_SENDBUFFER)
  {
    setSignal(Address, State);
  }
  else
  {
    SendBuffer[Buf_WriteIndex].Command = OPC_SE;
    SendBuffer[Buf_WriteIndex].Address = Address;
    SendBuffer[Buf_WriteIndex].State   = State;
    Buf_Load++;
    SendBuffer[Buf_WriteIndex].Rest_Time += Buf_Load;
    SendBufferSetWriteIndex();
  }
  Buf_Sperre = false;
}

void setDelaySignal(uint8_t DelayTime, uint16_t Address, uint8_t State)
{
  if (Buf_Load == N_SENDBUFFER)
  {
    setSignal(Address, State);
  }
  else
  {
    SendBuffer[Buf_WriteIndex].Rest_Time = DelayTime;
    setSignalBuf(Address, State);
  }
}

void setSensorBuf(uint16_t Address, uint8_t State)
{
  Buf_Sperre = true;  //Ausführung sperren
  if (Buf_Load == N_SENDBUFFER)
  {
    setSensor(Address, State);
  }
  else
  {
    SendBuffer[Buf_WriteIndex].Command = OPC_INPUT_REP;
    SendBuffer[Buf_WriteIndex].Address = Address;
    SendBuffer[Buf_WriteIndex].State   = State;
    Buf_Load++;
    SendBuffer[Buf_WriteIndex].Rest_Time += Buf_Load;
    SendBufferSetWriteIndex();
  }
  Buf_Sperre = false;
}

void setDelaySensor(uint8_t DelayTime, uint16_t Address, uint8_t State)
{
  if (Buf_Load == N_SENDBUFFER)
  {
    setSensor(Address, State);
  }
  else
  {
    SendBuffer[Buf_WriteIndex].Rest_Time = DelayTime;
    setSensorBuf(Address, State);
  }
}

void SendBufferInit()
{
  uint8_t i;
  Buf_Steptime = 0;
  Buf_WriteIndex = 0;
  Buf_Load = 0;
  Buf_Sperre = false;
  
  for (i=0; i<N_SENDBUFFER; i++)
  {
    SendBuffer[i].Command = 0;
    SendBuffer[i].Address = 0;
    SendBuffer[i].State= 0;
    SendBuffer[i].Rest_Time= 0;
  }
}

void SendBufferSetWriteIndex()
{
  uint8_t Buf_Temp_Index;
  Buf_Temp_Index = Buf_WriteIndex;
  Buf_WriteIndex = (Buf_WriteIndex + 1)%N_SENDBUFFER;
  while ((SendBuffer[Buf_WriteIndex].Command != 0) && (Buf_WriteIndex != Buf_Temp_Index))
  {
    Buf_WriteIndex = (Buf_WriteIndex + 1)%N_SENDBUFFER;
  }
}

void SendBufferExec()
{
  uint8_t i;
  if ((looptime > Buf_Steptime) && !Buf_Sperre && (Buf_Load > 0))
  {
    Buf_Steptime = looptime + 100;
    for (i=0; i<N_SENDBUFFER; i++)
    {
      if (SendBuffer[i].Rest_Time > 0)
        SendBuffer[i].Rest_Time--;
      else
        switch(SendBuffer[i].Command)
        {
          case OPC_SW_REQ:
            setWeiche(SendBuffer[i].Address, SendBuffer[i].State);
            SendBuffer[i].Command = 0; 
            Buf_Load--;
            break;
          case OPC_SW_REP:
            setWeicheLage(SendBuffer[i].Address, SendBuffer[i].State);
            SendBuffer[i].Command = 0; 
            Buf_Load--;
            break;
          case OPC_SE:
            setSignal(SendBuffer[i].Address, SendBuffer[i].State);
            SendBuffer[i].Command = 0; 
            Buf_Load--;
            break;
          case OPC_INPUT_REP:
            setSensor(SendBuffer[i].Address, SendBuffer[i].State);
            SendBuffer[i].Command = 0; 
            Buf_Load--;
            break;
        }       
    }  
  }
}

void Setup_Loconet()
{
  LocoNet.init(LN_SendPin);
  SendBufferInit();
}

void Loop_Loconet()
{
  ReceiveLoconet();
  SendBufferExec();
}
