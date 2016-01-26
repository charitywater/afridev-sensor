
Producing the Upgrade Application Message
=============================================

1.
Build the MSP 430 Application.  It produces the files Outpour_MSP430.out.

2.
Create the ROM file (a bat file, createRom.bat contains the command below)
C:\ti\ccsv5\tools\compiler\msp430_4.2.1\bin\hex430.exe outpour_app_to_rom.cmd

The outpour_app_to_rom.cmd contains the following:

Outpour_MSP430.out
--image
--memwidth 16
--ti_txt
--order MS 

ROMS
{
	EPROM1: org = 0xC800, len = 0x2800, romwidth = 16, fill = 0xFFFF
		files = { outpour_MSP430_rom.txt }
}

The output file is called outpour_MSP430_rom.txt.

3.
Remove the "@c8000" from the top line and the "q" from the bottom line.

4.
Run the python script outpourRomToMsg.py which looks for the outpour_MSP430_rom.txt file and writes the output to stdio which can be redirected into a file.

====================================================================================================
Combine the APP + Boot

type Outpour_MSP430.txt > outpour_App_Boot_MSP430.txt
type Outpour_Boot_MSP430.txt > outpour_App_Boot_MSP430.txt

====================================================================================================

App File names:

Outpour_MSP430.out
Outpour_MSP430_rom.txt
Outpour_MSP430_msg.txt

Final output name(s)
Output_App_MSP430_vx_y_hint_date_time_checksum.txt
Output_App_MSP430_vx_y_hint_date_time_checksum.map
Output_App_MSP430_vx_y_hint_date_time.msg

Boot File names:
Outpour_Boot_MSP430.txt
Outpour_Boot_MSP430_vx_y_hint_date_time_checksum.txt

Combined App + Boot:
Outpour_App_Boot_MSP430_vx_y_hint_date_time_checksum.txt


====================================================================================================



