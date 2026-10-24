/******************************************************************************
 LIBSN.H - the SN Systems host file calls (PSXCD.H includes <libsn.h>).

 Reconstructed. MKFILE.C uses them only without BLOCK_FILE_IO, which the
 Windows 95 build defines; nothing implements them.
******************************************************************************/
#ifndef __LIBSN_H
#define __LIBSN_H

int PCinit(void);
int PCopen(char *name, int flags, int perms);
int PCcreat(char *name, int perms);
int PClseek(int fd, int offset, int mode);
int PCread(int fd, char *buff, int len);
int PCwrite(int fd, char *buff, int len);
int PCclose(int fd);

#endif /* __LIBSN_H */
