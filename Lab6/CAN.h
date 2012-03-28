//---------- CAN_Init-----------------
// Initializes the CAN peripheral
// Input: none
// Output: none
int CAN_Init(void);

//---------- CANTX_Thread-----------------
// Thread that transmits measurement data over the CAN network
// Input: none
// Output: none
void CANTX_Thread(void);

//---------- CANRX_Thread-----------------
// Thread that recieves measurement data over the CAN network
// Input: none
// Output: none
void CANRX_Thread(void);

