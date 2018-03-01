#ifndef __ELOG_FILE_H__
#define __ELOG_FILE_H__

#include <elog.h>
#include <elog_file_cfg.h>

#ifdef __cplusplus
extern "C" {
#endif
    
/* elog_file.c */
ElogErrCode elog_file_init(void); 
void elog_file_enable(bool enable); 
void elog_file_read(void); 
void elog_file_write(const char *log, size_t size); 
void elog_file_clean(void); 
void elog_file_flush(void); 

#ifdef __cplusplus
}
#endif

#endif /* __ELOG_FILE_H__ */
