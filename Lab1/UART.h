// Standard ASCII symbols

#ifndef ASCII
  #define ASCII
  #define CR   0x0D
  #define LF   0x0A
  #define TAB  0x09
  #define BS   0x08
  #define BACKSPACE 0x08
  #define ESC  0x1B
  #define SP   0x20
  #define DEL  0x7F
  #define HOME 0x0A
  #define NEWLINE 0x0D
  #define RETURN 0x0D
#endif


#define BAUD 9600
#define MAXTRIES 10

void UART0_Init(void);
