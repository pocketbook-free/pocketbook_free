/*
 *		This headfile is for write/read onenand to physical address,
 *		Support a interface for it.
 *											--James 2010.5.12
 */
 
 //5 parameter areas read cmd
 #define	CMD_PARA_PART0		0
 #define	CMD_PARA_PART1		1
 #define	CMD_PARA_PART2		2
 #define	CMD_PARA_PART3		3
 #define	CMD_PARA_TOUC4		4
 
 extern int onenand_read_phy_address_para(int part, u_char *buf);
 extern int onenand_write_phy_address_para(int part, u_char *buf, int size);