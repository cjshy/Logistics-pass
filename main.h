#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sqlite3.h>

#define BUFSIZE 1024
#define MAX_EVENTS 100  //可同时存在客户端
#define length_pack 10  //快递单号长度
typedef struct queryNode
{
    char ** presult;
    int row;
    int col;
}QN;

typedef struct  register_user //注册用户信息
{
	char name[128];
	char passwd[128];
    int flage; //0冻结 1激活 
}REG;

typedef struct loginames
{
	char name[128];
	int fd;
}LOG;

typedef struct user_info //用户信息
{
    char name[128];
    char id[128];//快递单号
    char receiver[128];
    int n; //快递数量
}USER;

typedef struct package_info //包裹信息
{
    char id[128]; //快递单号
    char sender[128]; //发件人
    char sender_addr[128]; //发件人地址
    char sender_phone[128]; //发件人电话
    char receiver[128]; //收件人
    char dest_addr[128]; //收件人地址
    char dest_phone[128]; //收件人电话
    float weight; //重量
    char volume[128]; //体积 大件  小件 中件
    char content[128]; //种类
    char time[128]; //时间
    char company[128]; //所属公司
    int freight; //运费
}PACK;

typedef struct message
{
    int type;
    char buf[2024];
}MES;

typedef struct courier_info //快递员信息
{
    char name[128];//姓名
    char id[128];//工号
    char  company[128]; //所属公司
    int status; //状态 0空闲 1忙碌 2休息
    int flage;
}COUR;

typedef struct courier_package //快递员运单信息
{
    char id[128]; //快递单号
    char sender[128]; //发件人
    char sender_addr[128]; //发件人地址
    char sender_phone[128]; //发件人电话
    char receiver[128]; //收件人
    char dest_addr[128]; //收件人地址
    char dest_phone[128]; //收件人电话
    char content[128]; //种类
    char time[128]; //时间
    char courier_name[128]; //快递员姓名
    char courier_id[128]; //快递员工号
    char company[128]; //所属公司
    // int standard_fee; //运费标准 
    // float fee; //运费
    int status; //状态 0待派送 1派送中 2已签收 3运单异常
}CPACK;


typedef struct net_info //网点信息
{
    char name[128]; //网点名称
    char addr[128]; //网点地址
    char phone[128]; //网点电话
}NET;

typedef struct chat_info //评论信息
{
    char name[128]; //用户名
    char content[1024]; //内容
    char dest_name[128]; //目标用户名
}CHAT;

typedef struct txt
{
    char volume[128]; //体积 大件  小件 中件
    float flage;//标准运费
}TXT;



/*
1.注册(11.注册成功  10.注册失败)  
2.登录(root .24)(21.登录成功 22.用户在线 20.登录失败)  211.我要寄件  212.网点查询  213.运单追踪  214.运费咨询  215.交流反馈（2151.私聊  2152.群聊  2153.评论区  21544.返回上一级） 216.取消订单   217.我要取件  218.退出
241.用户管理 242.物流管理  243.修改运费标准  244.退出
2411.删除用户  2412.解冻用户  2413.冻结用户  2414.查询用户  2415.返回上一级
2421.快递员管理（24211.新增快递员 24212.删除快递员 24213.查询快递员  24214.修改快递员信息 24215.返回上一级）   2422.快递状态修改  2423.快递统计  2424.网点管理(24241.新增网点 24242.删除网点 24243.查询网点 24244.返回上一级)  2425.返回上一级
2431.修改运费  2432.返回上一级
3.游客访问   31.查看评论区
4。修改密码(40.修改失败  41.修改成功)  
5.退出
*/

