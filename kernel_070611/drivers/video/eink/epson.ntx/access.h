#ifndef S1D_ACCESS_H
#define S1D_ACCESS_H

  BOOL BusIssueInitHW(void);
  BOOL BusIssueReset(void);
  int BusIssueFlashOperation(PS1D13532_FLASH_PACKAGE pFlashControl);

  int   BusIssueCmd(unsigned ioctlcmd,s1d13521_ioctl_cmd_params *params,int numparams);
  void  BusIssueWriteBuf(u16 *ptr16, unsigned copysize16);
  void  BusIssueReadBuf(u16 *ptr16, unsigned copysize16);
  u16   BusIssueReadReg(u16 Index);
  int   BusIssueWriteReg(u16 Index, u16 Value);
  int   BusIssueWriteRegX(u16 Index, u16 Value);
  int   BusIssueCmdX(unsigned Cmd);
  int   BusIssueWriteRegBuf(u16 Index, PUINT16 pData, UINT32 Length);
  int   BusWaitForHRDY(void);
  void  BusIssueDoRefreshDisplay(unsigned cmd,unsigned mode);
  void  BusIssueInitDisplay(void);
  void  BusIssueInitRegisters(void);

#endif

