/*
 * LufaLayer.c
 * (c) 2014 flabbergast
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

/** \file
 *
 *  Main source file for the VirtualSerialMouse demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

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

/* by flabbergast:
 * implementation of exported functions
 * ** basic functions **
 */
void init(void)
{
  disk_read_only = true;
  disk_state = DISK_STATE_INITIAL;

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
  #if (defined(__AVR_ATxmega128A3U__))
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
uint8_t usb_keyboard_current_key_GLOBAL = 0;
uint8_t usb_keyboard_current_modifier_GLOBAL = 0;
bool usb_keyboard_send_current_data_GLOBAL = false;

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
    KeyboardReport->KeyCode[0] = usb_keyboard_current_key_GLOBAL;
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
  usb_keyboard_current_key_GLOBAL = key;
  usb_keyboard_current_modifier_GLOBAL = modifier;
  usb_keyboard_send_current_data_GLOBAL = true;
  return true;
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
  // Unused (but mandatory for the HID class driver) in this demo, since there are no Host->Device reports
}

