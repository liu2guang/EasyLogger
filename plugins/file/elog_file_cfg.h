#ifndef _ELOG_FILE_CFG_H_
#define _ELOG_FILE_CFG_H_

#include <elog.h>

/* EasyLogger file log plugin's using buffer mode */
#if defined(PKG_EASYLOGGER_FILE_USING_BUF_MODE)
    #define ELOG_FILE_USING_BUF_MODE
#endif

/* EasyLogger file log plugin's log file path name */
#if defined(PKG_EASYLOGGER_FILE_NAME)
    #define ELOG_FILE_NAME      PKG_EASYLOGGER_FILE_NAME 
#endif

/* EasyLogger file log plugin's RAM buffer size */
#if defined(PKG_EASYLOGGER_FILE_BUF_SIZE)
    #define ELOG_FILE_BUF_SIZE  PKG_EASYLOGGER_FILE_BUF_SIZE
#endif

#endif /* _ELOG_FILE_CFG_H_ */
