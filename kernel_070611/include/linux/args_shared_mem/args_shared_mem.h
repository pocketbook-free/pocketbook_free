#ifndef _ARGS_SHARED_MEM_
#define _ARGS_SHARED_MEM_

#define ARGS_SHARED_MEM_BASE		0x5FF80000
#define ARGS_SHARED_MEM_SIZE		(512*1024)

#define	ARGS_MAJOR			120
#define ARGS_DEVICE_NAME		"args_shared_mem"

/** MOVI partitions offset **/
struct movi_offset_t {
	uint	last;
	uint	bl1;
	uint	env;
	uint	bl2;
	uint	kernel;
	uint	rootfs;
	uint  update_kernel;
	uint  update_rootfs;
	uint  init_partitions;
	uint  epd_logo;
	uint  update_epd_logo; 
};

/** MOVI register info **/
struct movi_tcm_reg_t {
	uint	movi_high_capacity;	
	uint	movi_total_blkcnt;
};

/* EX3 Board Version */
enum BoardID_enum {
	BOARD_VERSION_UNKNOWN = 0,
	BOARD_ID_EVT = 1,
	BOARD_ID_DVT = 2,
};

//&*&*&*SJ1_20100114, Add SCID sector.
struct movi_scid_offset_t { 
	uint  scid_device_cert;
	uint  scid_device_key;
};

#define PART_SIZE_SCID_DEVICE_CERT		(5 * 1024)
#define PART_SIZE_SCID_DEVICE_KEY			(5 * 1024)
//&*&*&*SJ2_20100114, Add SCID sector.

/** Args main structure */
struct movi_args_t
{
  struct movi_tcm_reg_t	 tcminfo;
  struct movi_offset_t   ofsinfo;
  enum	BoardID_enum	 board_id;
//&*&*&*SJ1_20100114, Add SCID sector.
	unsigned char device_cert[PART_SIZE_SCID_DEVICE_CERT]; 
	unsigned char device_key[PART_SIZE_SCID_DEVICE_KEY];
//&*&*&*SJ2_20100114, Add SCID sector.
  char ex3_uboot_version[20];
};

#include <linux/io.h>
#define ARGSIO_GET_MOVI_OFFSET            _IOR('A', 0x01, struct movi_offset_t)
#define ARGSIO_GET_MOVI_TCM_REG           _IOR('A', 0x02, struct movi_tcm_reg_t)

#endif /* _ARGS_SHARED_MEM_ */
