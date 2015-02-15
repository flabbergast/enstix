/*
 * LufaLayer.c
 * (c) 2015 flabbergast
 *  USB and Board layer: provides "easy-to-use" functions for
 *   the main program code.
 *
 *  Portions of code come from a LUFA library Demo (license below).
 */

/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#define _INCLUDED_FROM_LUFALAYER_C_
#include "LufaLayer.h"

/* Includes: */
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "Descriptors.h"
#include "Timer.h"

#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Board/Buttons.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

#include "SCSI/SCSI.h"


/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
  {
    .Config =
      {
        .ControlInterfaceNumber         = INTERFACE_ID_CDC_CCI,
        .DataINEndpoint                 =
          {
            .Address                = CDC_TX_EPADDR,
            .Size                   = CDC_TXRX_EPSIZE,
            .Banks                  = 1,
          },
        .DataOUTEndpoint                =
          {
            .Address                = CDC_RX_EPADDR,
            .Size                   = CDC_TXRX_EPSIZE,
            .Banks                  = 1,
          },
        .NotificationEndpoint           =
          {
            .Address                = CDC_NOTIFICATION_EPADDR,
            .Size                   = CDC_NOTIFICATION_EPSIZE,
            .Banks                  = 1,
          },
      },
  };

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
  {
    .Config =
      {
        .InterfaceNumber                = INTERFACE_ID_Keyboard,
        .ReportINEndpoint               =
          {
            .Address                = KEYBOARD_EPADDR,
            .Size                   = KEYBOARD_EPSIZE,
            .Banks                  = 1,
          },
        .PrevReportINBuffer             = PrevKeyboardHIDReportBuffer,
        .PrevReportINBufferSize         = sizeof(PrevKeyboardHIDReportBuffer),
      },
  };

/** LUFA Mass Storage Class driver interface configuration and state information. This structure is
 *  passed to all Mass Storage Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MS_Device_t Disk_MS_Interface =
  {
    .Config =
      {
        .InterfaceNumber           = INTERFACE_ID_MassStorage,
        .DataINEndpoint            =
          {
            .Address           = MASS_STORAGE_IN_EPADDR,
            .Size              = MASS_STORAGE_IO_EPSIZE,
            .Banks             = 1,
          },
        .DataOUTEndpoint            =
          {
            .Address           = MASS_STORAGE_OUT_EPADDR,
            .Size              = MASS_STORAGE_IO_EPSIZE,
            .Banks             = 1,
          },
        .TotalLUNs                 = TOTAL_LUNS,
      },
  };

  /* Macros: */
    /** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
    #define LEDMASK_USB_NOTREADY      LEDS_LED1

    /** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
    #define LEDMASK_USB_ENUMERATING  (LEDS_LED2 | LEDS_LED3)

    /** LED mask for the library LED driver, to indicate that the USB interface is ready. */
    #define LEDMASK_USB_READY        (LEDS_LED2 | LEDS_LED4)

    /** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
    #define LEDMASK_USB_ERROR        (LEDS_LED1 | LEDS_LED3)

  /* Function Prototypes: */
    void SetupHardware(void);

    void EVENT_USB_Device_Connect(void);
    void EVENT_USB_Device_Disconnect(void);
    void EVENT_USB_Device_ConfigurationChanged(void);
    void EVENT_USB_Device_ControlRequest(void);
    void EVENT_USB_Device_StartOfFrame(void);

    bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                             uint8_t* const ReportID,
                                             const uint8_t ReportType,
                                             void* ReportData,
                                             uint16_t* const ReportSize);
    void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                              const uint8_t ReportID,
                                              const uint8_t ReportType,
                                              const void* ReportData,
                                              const uint16_t ReportSize);

    bool CALLBACK_MS_Device_SCSICommandReceived(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);

    void usb_keyboard_service_write(void);

/* by flabbergast:
 * implementation of exported functions
 * ** basic functions **
 */
void init(void)
{
  // initialise global variables
  usb_keyboard_leds = 0;
  usb_keyboard_send_current_data_GLOBAL = false;
  memset(usb_keyboard_current_keys_GLOBAL, 0, 6);
  usb_keyboard_current_modifier_GLOBAL = 0;
  usb_keyboard_sending_string_GLOBAL = false;
  disk_read_only_GLOBAL = true;
  disk_state_GLOBAL = DISK_STATE_INITIAL;

  // hardware / usb
  SetupHardware();
  LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
  GlobalInterruptEnable();
}

void usb_tasks(void)
{
  MS_Device_USBTask(&Disk_MS_Interface);
  CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
  HID_Device_USBTask(&Keyboard_HID_Interface);
  USB_USBTask();

  if(usb_keyboard_sending_string_GLOBAL)
    usb_keyboard_service_write();
}

/*
 * ** usb_serial **
 */

uint16_t usb_serial_available(void)
{
  return CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface);
}

int16_t usb_serial_getchar(void)
{
  return CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
}

void usb_serial_wait_for_key(void)
{
  while(usb_serial_available() == 0) {
    usb_tasks();
    service_button();
    _delay_ms(10);
  }
}

void usb_serial_flush_input(void)
{
  int16_t i;
  int16_t bufsize = CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface);
  for(i=0; i<bufsize; i++)
    CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
}

void usb_serial_putchar(uint8_t ch)
{
  CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ch);
}

void usb_serial_write(const char* const buffer)
{
  CDC_Device_SendString(&VirtualSerial_CDC_Interface, buffer);
}

void usb_serial_write_P(const char* data)
{
  while(pgm_read_byte(data) != 0)
    usb_serial_putchar(pgm_read_byte(data++));
}

void usb_serial_writeln(const char* const buffer)
{
  CDC_Device_SendString(&VirtualSerial_CDC_Interface, buffer);
  usb_serial_write_P(PSTR("\r\n"));
}

void usb_serial_writeln_P(const char* data)
{
  usb_serial_write_P(data);
  usb_serial_write_P(PSTR("\r\n"));
}

void usb_serial_flush_output(void)
{
  CDC_Device_Flush(&VirtualSerial_CDC_Interface);
}

void usb_serial_readline_refresh(const char *buffer, const uint16_t n, const bool obscure_input)
{
  uint16_t i;
  // delete the previous stuff on the line (plus one extra space)
  usb_serial_putchar('\r');
  for(i=0; i<=n; i++)
    usb_serial_putchar(' ');
  // write out the buffer contents
  usb_serial_putchar('\r');
  for(i=0; i<n; i++)
    if(obscure_input)
      usb_serial_putchar('*');
    else
      usb_serial_putchar(buffer[i]);
}

uint16_t usb_serial_readline(char *buffer, const uint16_t buffer_size, const bool obscure_input)
{
  uint16_t end = 0;
  char c;

  while(true) {
    if(usb_serial_available() > 0) {
      c = (char)usb_serial_getchar();
      switch((char) c) {
        case '\r': // enter
          buffer[end] = 0;
          usb_serial_putchar('\n');
          usb_serial_putchar('\r');
          return end;
          break;
        case '\b': // ^H
        case 127: // backspace
          end--;
          usb_serial_write("\b \b"); // remove the last char from the display
          //usb_serial_readline_refresh(buffer, end, obscure_input);
          break;
        default:
          if(end < buffer_size-1) {
            buffer[end++] = c;
            if(obscure_input)
              usb_serial_putchar('*');
            else
              usb_serial_putchar(c);
          }
          break;
      }
    }
    usb_tasks();
    service_button();
    _delay_ms(10);
  }
  return 0; // never reached
}

bool usb_serial_dtr(void) {
  return(VirtualSerial_CDC_Interface.State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR);
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  /* Disable clock division */
  clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
  /* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
  XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
  XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

  /* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
  XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
  XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

  PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

  /* Hardware Initialization */
  LEDs_Init();
  Buttons_Init();
  USB_Init();
  Timer_Init();

  // AES module soft reset: put the module into a known state
#if (ARCH == ARCH_XMEGA)

    AES.CTRL = AES_RESET_bm;
#endif
}

/*
 * ** button **
 */

bool button_is_pressed = false;
uint32_t button_pressed_timestamp = 0;

/** Checks for the button state */
void service_button(void)
{
  uint8_t     ButtonStatus_LCL = Buttons_GetStatus();

  if (ButtonStatus_LCL & BUTTONS_BUTTON1) {
    if (! button_is_pressed) {
      button_pressed_timestamp = millis10();
      button_is_pressed = true;
    }
  } else {
    button_is_pressed = false;
  }
}

/** returns for how long was the button pressed, in 10/1024 ms */
uint32_t button_pressed_for(void) {
  if(button_is_pressed)
    return (millis10() - button_pressed_timestamp);
  else
    return 0;
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
  LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
  LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
  bool ConfigSuccess = true;

  ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
  ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
  ConfigSuccess &= MS_Device_ConfigureEndpoints(&Disk_MS_Interface);

  USB_Device_EnableSOFEvents();

  LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
  HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
  MS_Device_ProcessControlRequest(&Disk_MS_Interface);
}

/** Mass Storage class driver callback function the reception of SCSI commands from the host, which must be processed.
 *
 *  \param[in] MSInterfaceInfo  Pointer to the Mass Storage class interface configuration structure being referenced
 */
bool CALLBACK_MS_Device_SCSICommandReceived(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
  bool CommandSuccess;

  //LEDs_SetAllLEDs(LEDMASK_USB_BUSY);
  CommandSuccess = SCSI_DecodeSCSICommand(MSInterfaceInfo);
  //LEDs_SetAllLEDs(LEDMASK_USB_READY);

  return CommandSuccess;
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
  HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

/* by flabbergast:
 * global variables used for the HID callback below
 */
char* usb_keyboard_text_ptr;
uint32_t usb_keyboard_last_keypress_time = 0;
// these are defined in .h
//bool usb_keyboard_send_current_data_GLOBAL = false;
//uint8_t usb_keyboard_current_keys_GLOBAL[6] = {0,0,0,0,0,0};
//uint8_t usb_keyboard_current_modifier_GLOBAL = 0;
// 1=num lock, 2=caps lock, 4=scroll lock, 8=compose, 16=kana
//volatile uint8_t usb_keyboard_leds=0;

uint8_t const PROGMEM usb_keyboard_translate_table_US[2*95] = { // US keyboard layout
  HID_KEYBOARD_SC_SPACE , 0, // 32
  HID_KEYBOARD_SC_1_AND_EXCLAMATION, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 33 !
  HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 34 "
  HID_KEYBOARD_SC_3_AND_HASHMARK, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 35 #
  HID_KEYBOARD_SC_4_AND_DOLLAR, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 36 $
  HID_KEYBOARD_SC_5_AND_PERCENTAGE, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 37 %
  HID_KEYBOARD_SC_7_AND_AMPERSAND, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 38 &
  HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE, 0, // 39 '
  HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 40 (
  HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 41 )
  HID_KEYBOARD_SC_8_AND_ASTERISK, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 42 *
  HID_KEYBOARD_SC_EQUAL_AND_PLUS, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 43 +
  HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN, 0, // 44 ,
  HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE, 0, // 45 -
  HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN, 0, // 46 .
  HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK, 0, // 47 /
  HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS, 0, // 48 0
  HID_KEYBOARD_SC_1_AND_EXCLAMATION, 0, // 49 1
  HID_KEYBOARD_SC_2_AND_AT, 0, // 50 2
  HID_KEYBOARD_SC_3_AND_HASHMARK, 0, // 51 3
  HID_KEYBOARD_SC_4_AND_DOLLAR, 0, // 52 4
  HID_KEYBOARD_SC_5_AND_PERCENTAGE, 0, // 53 5
  HID_KEYBOARD_SC_6_AND_CARET, 0, // 54 6
  HID_KEYBOARD_SC_7_AND_AMPERSAND, 0, // 55 7
  HID_KEYBOARD_SC_8_AND_ASTERISK, 0, // 56 8
  HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS, 0, // 57 9
  HID_KEYBOARD_SC_SEMICOLON_AND_COLON, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 58 :
  HID_KEYBOARD_SC_SEMICOLON_AND_COLON, 0, // 59 ;
  HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 60 <
  HID_KEYBOARD_SC_EQUAL_AND_PLUS, 0, // 61 =
  HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 62 >
  HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 63 ?
  HID_KEYBOARD_SC_2_AND_AT, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 64 @
  HID_KEYBOARD_SC_A, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 65 A
  HID_KEYBOARD_SC_B, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 66 B
  HID_KEYBOARD_SC_C, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 67 C
  HID_KEYBOARD_SC_D, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 68 D
  HID_KEYBOARD_SC_E, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 69 E
  HID_KEYBOARD_SC_F, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 70 F
  HID_KEYBOARD_SC_G, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 71 G
  HID_KEYBOARD_SC_H, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 72 H
  HID_KEYBOARD_SC_I, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 73 I
  HID_KEYBOARD_SC_J, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 74 J
  HID_KEYBOARD_SC_K, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 75 K
  HID_KEYBOARD_SC_L, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 76 L
  HID_KEYBOARD_SC_M, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 77 M
  HID_KEYBOARD_SC_N, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 78 N
  HID_KEYBOARD_SC_O, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 79 O
  HID_KEYBOARD_SC_P, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 80 P
  HID_KEYBOARD_SC_Q, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 81 Q
  HID_KEYBOARD_SC_R, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 82 R
  HID_KEYBOARD_SC_S, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 83 S
  HID_KEYBOARD_SC_T, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 84 T
  HID_KEYBOARD_SC_U, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 85 U
  HID_KEYBOARD_SC_V, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 86 V
  HID_KEYBOARD_SC_W, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 87 W
  HID_KEYBOARD_SC_X, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 88 X
  HID_KEYBOARD_SC_Y, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 89 Y
  HID_KEYBOARD_SC_Z, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 90 Z
  HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE, 0, // 91 [
  HID_KEYBOARD_SC_BACKSLASH_AND_PIPE, 0, // 92 backslash
  HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE, 0, // 93 ]
  HID_KEYBOARD_SC_6_AND_CARET, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 94 ^
  HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 95 _
  HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE, 0, // 96 `
  HID_KEYBOARD_SC_A, 0, // 97 a
  HID_KEYBOARD_SC_B, 0, // 98 b
  HID_KEYBOARD_SC_C, 0, // 99 c
  HID_KEYBOARD_SC_D, 0, // 100 d
  HID_KEYBOARD_SC_E, 0, // 101 e
  HID_KEYBOARD_SC_F, 0, // 102 f
  HID_KEYBOARD_SC_G, 0, // 103 g
  HID_KEYBOARD_SC_H, 0, // 104 h
  HID_KEYBOARD_SC_I, 0, // 105 i
  HID_KEYBOARD_SC_J, 0, // 106 j
  HID_KEYBOARD_SC_K, 0, // 107 k
  HID_KEYBOARD_SC_L, 0, // 108 l
  HID_KEYBOARD_SC_M, 0, // 109 m
  HID_KEYBOARD_SC_N, 0, // 110 n
  HID_KEYBOARD_SC_O, 0, // 111 o
  HID_KEYBOARD_SC_P, 0, // 112 p
  HID_KEYBOARD_SC_Q, 0, // 113 q
  HID_KEYBOARD_SC_R, 0, // 114 r
  HID_KEYBOARD_SC_S, 0, // 115 s
  HID_KEYBOARD_SC_T, 0, // 116 t
  HID_KEYBOARD_SC_U, 0, // 117 u
  HID_KEYBOARD_SC_V, 0, // 118 v
  HID_KEYBOARD_SC_W, 0, // 119 w
  HID_KEYBOARD_SC_X, 0, // 120 x
  HID_KEYBOARD_SC_Y, 0, // 121 y
  HID_KEYBOARD_SC_Z, 0, // 122 z
  HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 123 {
  HID_KEYBOARD_SC_BACKSLASH_AND_PIPE, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 124 |
  HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE, HID_KEYBOARD_MODIFIER_LEFTSHIFT, // 125 }
  HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE, HID_KEYBOARD_MODIFIER_LEFTSHIFT // 126 ~
};

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
  USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;
  if (usb_keyboard_send_current_data_GLOBAL)
  {
    uint8_t i;
    for(i=0; i<6; i++)
      KeyboardReport->KeyCode[i] = usb_keyboard_current_keys_GLOBAL[i];
    KeyboardReport->Modifier = usb_keyboard_current_modifier_GLOBAL;
    usb_keyboard_send_current_data_GLOBAL = false;
  }
  *ReportSize = sizeof(USB_KeyboardReport_Data_t);
  return false;
}

/* by flabbergast:
 * usb_keyboard_press(key, mod)
 *  set global variables to let the above callback generate a keypress
 *  returns 'true' if data is set; 'false' when the previous keypress wasn't sent yet
 */
bool usb_keyboard_press(uint8_t key, uint8_t modifier) {
  if(usb_keyboard_send_current_data_GLOBAL)
    return false;
  usb_keyboard_current_keys_GLOBAL[0] = key;
  usb_keyboard_current_modifier_GLOBAL = modifier;
  usb_keyboard_send_current_data_GLOBAL = true;
  return true;
}

/* by flabbergast:
 * usb_keyboard_write(text)
 *  send longer text over keyboard
 *  returns false if there's something being still sent
 */
bool usb_keyboard_write(char *text) {
  if(usb_keyboard_sending_string_GLOBAL)
    return false;
  usb_keyboard_text_ptr = text;
  usb_keyboard_sending_string_GLOBAL = true;
  return true;
}

/* by flabbergast:
 * usb_keyboard_service_write()
 *  internal function
 *  take care of sending keypresses until the whole text is sent
 */
void usb_keyboard_service_write(void) {
  // check if sending and the last keypress has been sent
  if(!usb_keyboard_sending_string_GLOBAL || usb_keyboard_send_current_data_GLOBAL)
    return;
  // check if enough time elapsed since the last keypress
  if( (millis10()-usb_keyboard_last_keypress_time) < USB_KEYBOARD_KEYPRESS_DELAY )
    return;
  // take the next char and send it or end the process
  uint8_t ch = *(usb_keyboard_text_ptr++);
  if(ch == 0) {
    usb_keyboard_sending_string_GLOBAL = false;
    return;
  }
  if(ch == '\n' || ch == '\r') {
    usb_keyboard_press(HID_KEYBOARD_SC_ENTER,0);
  } else if (ch >= 32 && ch <= 126) {
    uint8_t key = pgm_read_byte( usb_keyboard_translate_table_US + 2*(ch-32) );
    uint8_t mod = pgm_read_byte( usb_keyboard_translate_table_US + 2*(ch-32) + 1);
    usb_keyboard_press(key,mod);
  } else {
    return;
  }
  // update time when sent
  usb_keyboard_last_keypress_time = millis10();
  return;
}


/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
  usb_keyboard_leds = *((uint8_t *)ReportData);
}

