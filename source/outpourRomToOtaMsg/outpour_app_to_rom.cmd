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
