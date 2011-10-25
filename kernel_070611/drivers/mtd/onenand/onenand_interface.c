/*
 *		This file is for write/read onenand to physical address,
 *		Support a interface for it.
 *											--James 2010.5.12
 */
#include <linux/mtd/mtd.h>
#include "s3c_onenand.h"
#include <linux/mtd/onenand_interface.h> 


size_t retlen = 0x20000;
size_t len 	  = 0x20000;
size_t len_page = 2048;
loff_t from   = 0xffe0000;

int onenand_read_phy_address_para(int part, u_char *buf)
{
	unsigned long addr;
	addr = from - len * part;
	return onenand_read(s3c_onenand_interface, addr, len_page, &len_page, buf);
} 
EXPORT_SYMBOL(onenand_read_phy_address_para);
int onenand_write_phy_address_para(int part, u_char *buf, int size)
{
	struct erase_info instr; //erase info parameter
	int size_buf = 0;
	unsigned long i;
	unsigned long addr;
	
	instr.mtd = s3c_onenand_interface;
	addr = from - len * part;
	instr.addr = addr;
	instr.len  = len;
	instr.callback = 0;
	printk("the  s3c_onenand_interface->name  is %s ...\n", s3c_onenand_interface->name);
	onenand_erase(s3c_onenand_interface ,&instr);
	printk("end of erase ...\n");
	
	size_buf = size;
	printk("the size_buf is %d...\n", size_buf);
	for(i = size_buf; i < len_page; i++)
	{
		buf[i] = 0;
	}
	printk("start write ...\n");
	return onenand_write(s3c_onenand_interface, addr, len_page, &len_page, buf);
} 
EXPORT_SYMBOL(onenand_write_phy_address_para);