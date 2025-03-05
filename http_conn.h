#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include<sys/uio.h>
//分散 - 聚集 I/O 允许程序在一次系统调用中读写多个非连续的内存块，而不需要多次调用系统 I/O 函数
#include "locker.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    /*HTTP请求方法，但我们仅支持GET*/
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    /*主状态机的三种可能状态，分别表示：当前正在分析请求行，当前正在分析头部字段 正在分析请求体*/
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    /*服务器处理HTTP请求的结果：NO_REQUEST表示请求不完整，需要继续读取客户数
据；GET_REQUEST表示获得了一个完整的客户请求；BAD_REQUEST表示客户请求有语法错
误；FORBIDDEN_REQUEST表示客户对资源没有足够的访问权限；INTERNAL_ERROR表示服
务器内部错误；CLOSED_CONNECTION表示客户端已经关闭连接了*/
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    /*从状态机的三种可能状态，即行的读取状态，分别表示：读取到一个完整的行、行出错和行数据尚且不完整*/
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
    /*行的读取状态？？？？？*/

public:
    http_conn(){}
    ~http_conn(){}

public:
    void init( int sockfd, const sockaddr_in& addr );
    void close_conn( bool real_close = true );
    void process();
    bool read();
    bool write();

private:
    void init();
    HTTP_CODE process_read();
    bool process_write( HTTP_CODE ret );

    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    //获取当前正在解析的行的起始地址
    char* get_line() { return m_read_buf + m_start_line; }
    /*从状态机，用于解析出一行内容*/
    LINE_STATUS parse_line();

    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );//status:200 title:OK 
    bool add_headers( int content_length );
    bool add_content_length( int content_length ); //content-length:
    bool add_linger(); //connection:keep-alive
    bool add_blank_line();
    //向 HTTP 响应缓冲区添加一个空行。在 HTTP 协议中，空行用于分隔响应头和响应内容

public:
    static int m_epollfd;
    static int m_user_count; //所有类对象共享的用户数量

private:
    int m_sockfd;
    sockaddr_in m_address;

    char m_read_buf[ READ_BUFFER_SIZE ];
    int m_read_idx; //m_read_idx 指向缓冲区中当前已读取数据的末尾位置
    /*在 read 函数中，当从套接字读取数据到 m_read_buf 时，会更新 m_read_idx 的值，以反映当前已读取数据的长度。*/
    int m_checked_idx; 
    //m_checked_idx 指向当前正在解析的数据的位置。
    int m_start_line; 
    //该变量用于记录当前正在解析的行在 m_read_buf 缓冲区中的起始位置。
    char m_write_buf[ WRITE_BUFFER_SIZE ];
    int m_write_idx;

    CHECK_STATE m_check_state; //主状态机的当前状态
    METHOD m_method;

    char m_real_file[ FILENAME_LEN ];
    char* m_url;
    char* m_version;
    char* m_host;
    int m_content_length;
    bool m_linger; //表示 HTTP 连接是否需要保持长连接（Keep - Alive）

    char* m_file_address; //该指针用于指向通过内存映射（mmap）将磁盘文件映射到进程地址空间后的起始地址
    //在 do_request 函数中，当服务器确定客户端请求的是一个文件时，会调用 mmap 函数将文件映射到内存，并将映射后的起始地址赋值给 m_file_address。
    //之后，在 process_write 函数中，会使用这个指针将文件内容发送给客户端。
    struct stat m_file_stat;
    /*我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示
被写内存块的数量*/
    struct iovec m_iv[2];
    int m_iv_count;
};

#endif
