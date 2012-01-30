// Standard ASCII symbols
#define CR   0x0D
#define LF   0x0A
#define BS   0x08
#define ESC  0x1B
#define SP   0x20
#define DEL  0x7F

// Frame Creation
#define MAXFRAMESIZE 100
#define LENGTHOFMESSAGE 5

// AT commands
#define MAXCOMMANDSIZE 10
#define LOWDESTADDRSTR "2C"
#define LOWDESTADDRHEX 0x2C
#define MYADDR "2D"

#define BAUD 9600
#define MAXTRIES 10

//------------XBeeTX_Init-----------------
// Initalizes the XBeeTX unit, prints status to VCom
// Input: none
// Output: none
void XBeeTX_Init(void);

//------------XBee_SendTxFrame------------
// Outputs Frame to XBee
// Input: Pointer to frame to send, length of frame
// Output: none
void XBee_SendTxFrame(unsigned char *stringBuffer, int length);

//------------XBee_SendString-------------
// Outputs String to XBee
// Input: Pointer to string to send, must be NULL terminated
// Output: none
void XBee_SendString(unsigned char *stringBuffer);

//------------XBee_RecieveString----------
// Creates string until <enter>, will only create a string as
// long as max
// Input: Pointer to string to write to, max number of characters in string
// Output: None
void XBee_RecieveString(unsigned char *stringBuffer, unsigned short max);

//------------Create_TransferFrame---------
// Creates frame to transmit
// Input: Message to transmit, destination for frame, length of message to send
// Output: Length of frame
int Create_TransferFrame(unsigned char *message, unsigned char *frame, unsigned short length);

//------------XBee_TxStatus---------
// Determines if last frame was sent correctly by looking at frame recieved
// Input: None
// Output: 1 = frame was not sent properly, 0 = frame was sent properly
char XBee_TxStatus(void);
