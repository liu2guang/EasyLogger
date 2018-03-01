#define LOG_TAG "elog.file"

#include <elog_file.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RT_USING_DFS
#error "Easylogger requires an RTT to open the DFS component!"
#endif 

#include <dfs_posix.h> 

#ifdef ELOG_FILE_USING_BUF_MODE
/* flash log buffer */
static char log_buf[ELOG_FILE_BUF_SIZE] = {0};
/* current flash log buffer write position  */
static size_t cur_buf_size = 0;
#endif

/* initialize OK flag */
static bool init_ok = false;
static bool file_enable = false; 
static struct rt_mutex file_log_lock; 

ElogErrCode elog_file_init(void)
{
    ElogErrCode result = ELOG_NO_ERR; 
    
#ifdef ELOG_FILE_USING_BUF_MODE
    /* initialize current flash log buffer write position */
    cur_buf_size = 0;
#endif
    
    /* 创建互斥量 */
    rt_mutex_init(&file_log_lock, "elog file lock", RT_IPC_FLAG_FIFO);
    
    /* initialize OK */
    init_ok = true; 
    
    return result; 
}

static uint32_t file_add(const char *name, char *buff, uint32_t size)
{
    int fd;
    uint32_t wsize;

    /* 追加方式打开文件, 写方式 */
    fd = open(name, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
    if(fd < 0)
    {
        return 0; 
    }

    /* 追加数据 */
    wsize = write(fd, (const void *)buff, size);

    /* 关闭文件 */
    close(fd);
    
    return wsize; 
}

static void log_buf_lock(void) 
{
    rt_mutex_take(&file_log_lock, RT_WAITING_FOREVER); 
}

static void log_buf_unlock(void) 
{
    rt_mutex_release(&file_log_lock); 
}

void elog_file_enable(bool enable)
{
    file_enable = enable; 
}

void elog_file_read(void)
{
    extern void cat(const char *filename); 
    
    /* must be call this function after initialize OK */
    ELOG_ASSERT(init_ok); 
    
    if(file_enable == false)
    {
        rt_kprintf("Easylogger file plugin is not open\n"); 
        return; 
    }
    
    /* lock file log buffer */
    log_buf_lock();
    
    cat((const char *)ELOG_FILE_NAME); 
    
    /* 打印BUF内容 */
#ifdef ELOG_FILE_USING_BUF_MODE 
    rt_device_t console = rt_console_get_device(); 
    rt_uint16_t old_flag = console->open_flag;

    console->open_flag |= RT_DEVICE_FLAG_STREAM;
    rt_device_write(console, 0, (char *)log_buf, cur_buf_size);
    console->open_flag = old_flag;
#endif
    
    /* unlock file log buffer */
    log_buf_unlock(); 
}

void elog_file_clean(void)
{
    int8_t result = 0; 

    /* must be call this function after initialize OK */
    ELOG_ASSERT(init_ok); 
    
    if(file_enable == false)
    {
        rt_kprintf("Easylogger file plugin is not open.\n"); 
        return; 
    }
    
    /* lock file log buffer */
    log_buf_lock();
    
    /* clean all log which in flash */
    result = unlink((const char *)ELOG_FILE_NAME); 

#ifdef ELOG_FLASH_USING_BUF_MODE
    /* reset position */
    cur_buf_size = 0;
#endif

    /* unlock file log buffer */
    log_buf_unlock();

    if(result == 0) 
    {
        log_i("All logs which in file is clean OK.");
    } 
    else 
    {
        log_e("Clean logs which in file has an error!");
    }
}

#ifdef ELOG_FILE_USING_BUF_MODE
void elog_file_flush(void) 
{
    /* must be call this function after initialize OK */
    ELOG_ASSERT(init_ok); 
    
    if(file_enable == false)
    {
        rt_kprintf("Easylogger file plugin is not open.\n"); 
        return; 
    }
    
    /* lock flash log buffer */
    log_buf_lock();
    
    /* write all buffered log to flash */
    file_add((const char *)ELOG_FILE_NAME, (char *)log_buf, cur_buf_size); 
    
    /* reset position */
    cur_buf_size = 0; 
    
    /* unlock flash log buffer */
    log_buf_unlock();
    
    rt_kprintf("EasyLogger file log buffer as been written to the file.\n");
}
#endif

void elog_file_write(const char *log, size_t size)
{
#ifdef ELOG_FILE_USING_BUF_MODE
    size_t write_size = 0, write_index = 0;
#endif
    
    /* must be call this function after initialize OK */
    ELOG_ASSERT(init_ok); 
    
    /* 判断是否使能输出 */
    if(file_enable == false)
    {
        return; 
    }
    
    /* lock file log buffer */
    log_buf_lock();
    
#ifdef ELOG_FILE_USING_BUF_MODE
    while(true) 
    {
        if(cur_buf_size + size > ELOG_FILE_BUF_SIZE) 
        {
            write_size = ELOG_FILE_BUF_SIZE - cur_buf_size;
            elog_memcpy(log_buf + cur_buf_size, log + write_index, write_size);
            write_index += write_size;
            size -= write_size;
            cur_buf_size += write_size;
            /* unlock flash log buffer */
            log_buf_unlock();
            /* write all buffered log to flash, cur_buf_size will reset */
            elog_file_flush();
            /* lock flash log buffer */
            log_buf_lock();
        } 
        else 
        {
            elog_memcpy(log_buf + cur_buf_size, log + write_index, size);
            cur_buf_size += size;
            break;
        }
    }
#else
    /* 直接写入文件 */ 
    file_add((const char *)ELOG_FILE_NAME, (char *)log, size); 
#endif
    
    /* unlock file log buffer */
    log_buf_unlock(); 
}

#if defined(RT_USING_FINSH) && defined(FINSH_USING_MSH)
#include <finsh.h>
static void elog_file(uint8_t argc, char **argv) 
{
    if(argc >= 2) 
    {
        if(!strcmp(argv[1], "on")) 
        {
            elog_file_enable(true); 
        } 
        else if(!strcmp(argv[1], "off")) 
        {
            elog_file_enable(false); 
        } 
        else if(!strcmp(argv[1], "read")) 
        {
            elog_file_read(); 
        } 
        else if(!strcmp(argv[1], "clean")) 
        {
            elog_file_clean();
        } 
        else if(!strcmp(argv[1], "flush")) 
        {
#ifdef ELOG_FILE_USING_BUF_MODE
            elog_file_flush();
#else
            rt_kprintf("EasyLogger file log buffer mode is not open.\n");
#endif
        } 
        else 
        {
            rt_kprintf("Please input elog_file {<on/off>, <read>, <clean>, <flush>}.\n");
        }
    } 
    else 
    {
        rt_kprintf("Please input elog_file {<on/off>, <read>, <clean>, <flush>}.\n");
    }
}
MSH_CMD_EXPORT(elog_file, EasyLogger <on/off> <read> <clean> <flush> flash log);
#endif /* defined(RT_USING_FINSH) && defined(FINSH_USING_MSH) */
