#include "main.h"
int sockfd;
char loginname[128] = {0};        // 登录用户名
unsigned int szIPToInt(char *pIP) // 把字符串IP转为网络字节序整数IP
{
    unsigned int ip;
    inet_pton(AF_INET, pIP, &ip); // 把字符串IP转成数字IP
    return ip;
}

char *intIPToStr(unsigned int ip) // 把网络字节序整数IP转为字符串IP
{
    char *szIP = malloc(16);
    inet_ntop(AF_INET, &ip, szIP, 16);
    printf("%s\n", szIP);
    return szIP;
}

struct sockaddr_in getAddr(char *pszIp, uint16_t port) // 输入IP和端口，返回一个地址结构体
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;               // IPV4地址族
    addr.sin_addr.s_addr = szIPToInt(pszIp); // IP地址
    addr.sin_port = htons(port);             // 端口号，要转成大端字节序
    return addr;
}

void show(char *txt) // 显示界面
{
    FILE *fp = fopen(txt, "rb");
    char buf[BUFSIZ];

    int send = 1;
    while (send == 1)
    {
        int ret = 0;
        while (ret != BUFSIZ)
        {
            ret += fread(buf, 1, sizeof(buf), fp);
            if (feof(fp) == 1)
            {
                send = 0;
                break;
            }
        }
        printf("%s", buf);
    }
    putchar(10);
    fclose(fp);
    memset(buf, 0, sizeof(buf));
};

void _register(int fd) // 注册
{
    REG reg;
    printf("请输入用户名和密码\n");
    scanf("%s%s", reg.name, reg.passwd);
    MES msg;
    msg.type = 1;
    memcpy(msg.buf, &reg, sizeof(reg));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    if (msg.type = 11) // 注册成功
    {
        printf("%s\n", msg.buf);
    }
    else if (msg.type = 10) // 注册失败
    {
        printf("%s\n", msg.buf);
    }
}

int _login(int fd) // 登录
{
    REG reg;
    printf("请输入用户名和密码\n");
    scanf("%s%s", reg.name, reg.passwd);
    MES msg;
    msg.type = 2;
    memcpy(msg.buf, &reg, sizeof(reg));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    if (msg.type == 21) // 登录成功
    {
        strcpy(loginname, reg.name);
        printf("%s\n", msg.buf);
        return 1;
    }
    else if (msg.type == 20) // 登录失败
    {
        printf("%s\n", msg.buf);
        return 2;
    }
    else if (msg.type == 22) // 用户已在线
    {
        printf("%s\n", msg.buf);
        return 2;
    }
}

int _login_root(int fd) // 管理员登录
{
    REG reg;
    printf("请输入用户名和密码\n");
    scanf("%s%s", reg.name, reg.passwd);
    MES msg;
    msg.type = 24;
    memcpy(msg.buf, &reg, sizeof(reg));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    if (msg.type == 21) // 登录成功
    {
        strcpy(loginname, reg.name);
        printf("%s\n", msg.buf);
        return 1;
    }
    else if (msg.type == 20) // 登录失败
    {
        printf("%s\n", msg.buf);
        return 2;
    }
    else if (msg.type == 22) // 用户已在线
    {
        printf("%s\n", msg.buf);
        return 2;
    }
}

int ch_user(int fd) // 修改密码
{
    REG reg;
    printf("输入您的用户名和旧密码：\n");
    scanf("%s%s", reg.name, reg.passwd);
    MES msg;
    msg.type = 4;
    memcpy(msg.buf, &reg, sizeof(reg));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    if (msg.type == 40) // 修改错误
    {
        printf("%s\n", msg.buf);
        return -1;
    }
    else if (msg.type == 41) // 修改成功
    {
        printf("输入新密码：");
        scanf("%s", reg.passwd);
        memcpy(msg.buf, &reg, sizeof(reg));
        write(fd, &msg, sizeof(msg));
        read(fd, &msg, sizeof(msg));
        printf("%s\n", msg.buf);
        return 1;
    }
}

char *gettime(char *time_string, size_t size) // 获取当地时间
{
    time_t current_time;
    struct tm *local_time;
    // 获取当前时间
    current_time = time(NULL);
    // 将时间转换为本地时间
    local_time = localtime(&current_time);
    // 格式化时间字符串
    strftime(time_string, size, "%Y-%m-%d %H:%M:%S", local_time);
    return time_string;
}

void send_pack(int fd) // 我要寄件
{
    PACK pack;
    MES msg;
    msg.type = 211; // 我要寄件
    printf("请输入发件人名字：");
    scanf("%s", pack.sender);
    printf("请输入发件人地址：");
    scanf("%s", pack.sender_addr);
    printf("请输入发件人电话：");
    scanf("%s", pack.sender_phone);
    printf("请输入收件人名字：");
    scanf("%s", pack.receiver);
    printf("请输入收件人的地址：");
    scanf("%s", pack.dest_addr);
    printf("请输入收件人电话：");
    scanf("%s", pack.dest_phone);
    printf("请输入物品重量：");
    scanf("%f", &pack.weight);
    printf("请输入物品体积(大件  中件  小件)：");
    scanf("%s", pack.volume);
    printf("请输入物品类型：");
    scanf("%s", pack.content);
    gettime(pack.time, sizeof(pack.time));
    printf("选择以下快递公司的一家：\n");
    printf("申通,圆通,顺丰,极兔\n");
    scanf("%s", pack.company);
    memcpy(msg.buf, &pack, sizeof(pack));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void pack_track(int fd) // 运单追踪
{
    USER user;
    MES msg;
    msg.type = 213;               // 运单追踪
    strcpy(user.name, loginname); // 登录用户名
    memcpy(msg.buf, &user, sizeof(user));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));

    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    if (msg.type == 2130) // 未查询到该用户的运单
    {
        printf("%s\n", msg.buf);
    }
    else if (msg.type == 2131) // 查询到该用户的运单
    {
        USER *p = (USER *)msg.buf;
        int n = p->n;
        printf("您有%d个运单\n", n);
        while (n > 0)
        {
            read(fd, &msg, sizeof(msg));
            printf("%s\n", msg.buf);
            n--;
        }
    }
}

void root_user_del(int fd) // 删除用户
{
    REG reg;
    MES msg;
    printf("请输入用户名：");
    scanf("%s", reg.name);
    msg.type = 2411; // 删除用户
    memcpy(msg.buf, &reg, sizeof(reg));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_user_unfreeze(int fd) // 解冻用户
{
    REG reg;
    MES msg;
    printf("请输入用户名：");
    scanf("%s", reg.name);
    msg.type = 2412; // 解冻用户
    memcpy(msg.buf, &reg, sizeof(reg));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_user_freeze(int fd) // 冻结用户
{
    REG reg;
    MES msg;
    printf("请输入用户名：");
    scanf("%s", reg.name);
    msg.type = 2413; // 冻结用户
    memcpy(msg.buf, &reg, sizeof(reg));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_user_query(int fd) // 查询用户
{
    REG reg;
    MES msg;
    printf("请输入用户名：");
    scanf("%s", reg.name);
    msg.type = 2414; // 查询用户
    memcpy(msg.buf, &reg, sizeof(reg));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_Courier_add(int fd) // 新增快递员
{
    COUR cour;
    MES msg;
    printf("请输入快递员姓名：");
    scanf("%s", cour.name);
    printf("请输入快递员所属公司（顺丰，申通，圆通，极兔）：");
    scanf("%s", cour.company);
    cour.status = 0;  // 默认空闲
    msg.type = 24211; // 新增快递员
    memcpy(msg.buf, &cour, sizeof(cour));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_Courier_del(int fd) // 删除快递员
{
    COUR cour;
    MES msg;
    printf("请输入需要解雇的快递员id：");
    scanf("%s", cour.id);
    msg.type = 24212; // 删除快递员
    memcpy(msg.buf, &cour, sizeof(cour));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_Courier_query(int fd) // 查询快递员
{
    COUR cour;
    MES msg;
    printf("请输入需要查询的快递员姓名：");
    scanf("%s", cour.name);
    msg.type = 24213; // 查询快递员
    memcpy(msg.buf, &cour, sizeof(cour));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    int n = *(int *)msg.buf;
    if (n == 0)
    {
        read(fd, &msg, sizeof(msg));
        printf("%s\n", msg.buf);
    }
    else if (n == 1)
    {
        read(fd, &msg, sizeof(msg));
        printf("%s\n", msg.buf);
    }
    else if (n > 1)
    {
        printf("存在%d个重名快递员\n", n);
        while (n > 0)
        {
            read(fd, &msg, sizeof(msg));
            printf("%s\n", msg.buf);
            n--;
        }
    }
}

void root_Courier_update(int fd) // 快递状态更新
{
    CPACK cpack; // 快递单信息
    MES msg;
    printf("请输入快递单号：");
    scanf("%s", cpack.id);
    msg.type = 2422; // 快递状态更新
    memcpy(msg.buf, &cpack, sizeof(cpack));
    write(fd, &msg, sizeof(msg));

    printf("更新快递状态：(0待派送 1派送中 2已签收 3运单异常)\n");
    printf("请输入新的快递状态：");
    scanf("%d", &cpack.status);
    memcpy(msg.buf, &cpack, sizeof(cpack));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_Courier_statistics(int fd) // 快递统计
{
    MES msg;
    msg.type = 2423; // 快递统计
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    int n = *(int *)msg.buf;
    printf("共有%d个快递单\n", n);
    while (n > 0)
    {
        read(fd, &msg, sizeof(msg));
        printf("%s\n", msg.buf);
        n--;
    }
}

void root_net_add(int fd) // 新增网点 24241
{
    MES msg;
    NET net;
    printf("请输入网点名称：");
    scanf("%s", net.name);
    printf("请输入网点地址：");
    scanf("%s", net.addr);
    printf("请输入网点联系电话：");
    scanf("%s", net.phone);
    msg.type = 24241; // 新增网点
    memcpy(msg.buf, &net, sizeof(net));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_net_del(int fd) // 删除网点 24242
{
    MES msg;
    NET net;
    printf("请输入需要删除的网点名称：");
    scanf("%s", net.name);
    msg.type = 24242; // 删除网点
    memcpy(msg.buf, &net, sizeof(NET));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void root_net_query(int fd) // 查询网点 24243
{
    MES msg;
    NET net;
    printf("可根据网点名称或地址查询网点\n");
    printf("1。根据网点名称查询 2.根据网点地址查询：\n");
    int sel = -1;
    scanf("%d", &sel);
    if (sel == 1) // 根据网点名称查询
    {
        printf("请输入需要查询的网点名称：");
        scanf("%s", net.name);
        msg.type = 24243; // 查询网点
        memcpy(msg.buf, &net, sizeof(net));
        write(fd, &msg, sizeof(msg));
    }
    else if (sel == 2) // 根据网点地址查询
    {
        printf("请输入需要查询的网点地址：");
        scanf("%s", net.addr);
        msg.type = 24243; // 查询网点
        memcpy(msg.buf, &net, sizeof(net));
        write(fd, &msg, sizeof(msg));
    }
    else // 输入错误
    {
        printf("输入错误，请重新选择\n");
    }

    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    int n = *(int *)msg.buf;
    printf("存在%d个网点\n", n);
    if (n == 0)
    {
        read(fd, &msg, sizeof(msg));
        printf("%s\n", msg.buf);
    }
    else if (n == 1)
    {
        read(fd, &msg, sizeof(msg));
        printf("%s\n", msg.buf);
    }
    else if (n > 1)
    {
        // printf("存在%d个重名网点\n", n);
        while (n > 0)
        {
            read(fd, &msg, sizeof(msg));
            printf("%s\n", msg.buf);
            n--;
        }
    }
}

void user_freight(int fd) // 运费咨询  214
{
    MES msg;
    PACK pack;
    printf("您正在咨询运费：\n");
    printf("请输入物品重量(/kg)：");
    scanf("%f", &pack.weight);
    printf("请输入物品体积(大件  小件  中件)：");
    scanf("%s", pack.volume);
    memcpy(msg.buf, &pack, sizeof(pack));
    msg.type = 214; // 运费咨询
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void *cli_rec(void *arg)
{
    int fd = *(int *)arg;

    while (1)
    {
        MES msg;
        int size = read(fd, &msg, sizeof(msg));
        if (size <= 0)
        {
            printf("已经与服务器断开连接\n");
            exit(0);
        }
        if(msg.type == 2151)//私聊
        {
            CHAT *chat = (CHAT *)msg.buf;
            printf("%s:[%s]\n",chat->name,chat->content);
        }
        else if(msg.type == 2152)//群聊
        {
            CHAT *chat = (CHAT *)msg.buf;
            printf("收到消息：%s\n",chat->content);
        }
        else if(msg.type == -1)
        {
            CHAT *chat = (CHAT *)msg.buf;
            printf("没有%s这个用户\n",chat->dest_name);
        }
        else if(msg.type == 31)
        {
            printf("%s\n",msg.buf);
        }
        
    }
}

void section(int fd) // 评论区 215
{
    pthread_t tid;
    pthread_create(&tid, NULL, cli_rec, &sockfd);
    MES msg;
    CHAT chat;
    msg.type = 215;               // 评论区
    strcpy(chat.name, loginname); // 登录用户名
    printf("输入quit退出评论区\n");
    printf("请输入您要评论的内容：");
    while (1)
    {
        scanf("%s", chat.content);
        if (strcmp(chat.content, "quit") == 0)
        {
            pthread_cancel(tid);
            break;
        }
        memcpy(msg.buf, &chat, sizeof(chat));
        write(fd, &msg, sizeof(msg));
        memset(msg.buf, 0, sizeof(msg.buf));
    }
}

void vist_section(int fd) // 查看评论区 31
{
    pthread_t tid;
    pthread_create(&tid, NULL, cli_rec, &sockfd);
    MES msg;
    msg.type = 31; // 查看评论区
    write(fd, &msg, sizeof(msg));
    printf("输入quit退出评论区：\n");
    printf("游客需登录之后才能发表评论：\n");
    while (1)
    {
        scanf("%s", msg.buf);
        if (strcmp(msg.buf, "quit") == 0)
        {
            pthread_cancel(tid);
            break;
        }
        else
        {
            printf("请先登录在进行评论\n");
            write(fd, &msg, sizeof(msg));
            memset(msg.buf, 0, sizeof(msg.buf));
        }
    }
}

void ch_freight(int fd) // 修改运费标准 243
{
    MES msg;
    TXT txt;
    msg.type = 243; // 修改运费标准
    printf("请输入新的运费标准：\n");
    printf("1.大件  2.中件  3.小件\n");
    printf("请输入选择：");
    int sel = -1;
    scanf("%d", &sel);
    if (sel == 1) // 大件
    {
        strcpy(txt.volume, "大件");
        printf("请输入新的运费标准：");
        scanf("%f", &txt.flage);
        memcpy(msg.buf, &txt, sizeof(txt));
        write(fd, &msg, sizeof(msg));
    }
    else if (sel == 2) // 中件
    {
        strcpy(txt.volume, "中件");
        printf("请输入新的运费标准：");
        scanf("%f", &txt.flage);
        memcpy(msg.buf, &txt, sizeof(txt));
        write(fd, &msg, sizeof(msg));
    }
    else if (sel == 3) // 小件
    {
        strcpy(txt.volume, "小件");
        printf("请输入新的运费标准：");
        scanf("%f", &txt.flage);
        memcpy(msg.buf, &txt, sizeof(txt));
        write(fd, &msg, sizeof(msg));
    }
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
}

void del_order(int fd) // 取消订单 216
{
    MES msg;
    PACK pack;
    printf("请输入需要取消的订单号：");
    scanf("%s", pack.id);
    msg.type = 216;                 // 取消订单
    strcpy(pack.sender, loginname); // 登录用户名 作为发件人
    memcpy(msg.buf, &pack, sizeof(pack));
    write(fd, &msg, sizeof(msg));

    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void Pickup(int fd) // 我要取件 217
{
    MES msg;
    PACK pack;
    printf("请输入需要取件的订单号：");
    scanf("%s", pack.id);
    msg.type = 217;                   // 我要取件
    strcpy(pack.receiver, loginname); // 登录用户名 作为发件人
    memcpy(msg.buf, &pack, sizeof(pack));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
}

void user_quit(int fd) // 退出登录 218
{
    MES msg;
    LOG log;
    strcpy(log.name, loginname); // 登录用户名
    msg.type = 218;              // 退出登录
    memcpy(msg.buf, &log, sizeof(log));
    write(fd, &msg, sizeof(msg));
    int size = read(fd, &msg, sizeof(msg));
    if (size <= 0)
    {
        printf("已经与服务器断开连接\n");
        exit(0);
    }
    printf("%s\n", msg.buf);
    exit(0);
}

void root_Courier_modify(int fd) // 修改快递员信息 24214
{
    MES msg;
    COUR cour;
    printf("1.修改公司  2.修改状态\n");
    int sel = -1;
    scanf("%d", &sel);
    if (sel == 1) // 修改公司
    {
        printf("请输入需要修改的快递员id：");
        scanf("%s", cour.id);
        printf("请输入新的公司名称：");
        scanf("%s", cour.company);
        cour.flage = 1;
        msg.type = 24214; // 修改快递员信息
        memcpy(msg.buf, &cour, sizeof(cour));
        write(sockfd, &msg, sizeof(msg));
        int size = read(sockfd, &msg, sizeof(msg));
        if (size <= 0)
        {
            printf("已经与服务器断开连接\n");
            exit(0);
        }
        printf("%s\n", msg.buf);
    }
    else if (sel == 2) // 修改状态
    {
        printf("请输入需要修改的快递员id：");
        scanf("%s", cour.id);
        printf("请输入新的状态：(0空闲  1忙碌  2休息)\n");
        printf("请输入新的状态：");
        scanf("%d", &cour.status);
        cour.flage = 2;
        msg.type = 24214; // 修改快递员信息
        memcpy(msg.buf, &cour, sizeof(cour));
        write(sockfd, &msg, sizeof(msg));
        int size = read(sockfd, &msg, sizeof(msg));
        if (size <= 0)
        {
            printf("已经与服务器断开连接\n");
            exit(0);
        }
        printf("%s\n", msg.buf);
    }
    else // 输入错误
    {
        printf("输入错误，请重新选择\n");
    }
}

void chat_send(int fd) // 私聊发送 2151
{
    pthread_t tid;
    pthread_create(&tid, NULL, cli_rec, &sockfd);
    MES msg;
    msg.type = 2151;
    CHAT chat;
    printf("请输入对方用户名：");
    scanf("%s", chat.dest_name);
    strcpy(chat.name, loginname); // 登录用户名
    while(1)
    {
        char temp[128];
        memset(temp, 0, sizeof(temp));
        scanf("%s", temp);
        strcpy(chat.content, temp);
        memcpy(msg.buf, &chat, sizeof(CHAT));
        if(strcmp(chat.content, "quit") == 0)
        {
            pthread_cancel(tid);
            break;
        }
        write(fd, &msg, sizeof(msg));
    }
}

void chat_grp(int fd) // 群聊发送 2152
{
    pthread_t tid;
    pthread_create(&tid, NULL, cli_rec, &sockfd);
    CHAT chat;
    strcpy(chat.name, loginname); // 登录用户名
    printf("请输入内容：");
    while(1)
	{
		MES msg;
		msg.type = 2152;
		memset(msg.buf,0,sizeof(msg.buf));
		scanf("%s",chat.content);
        memcpy(msg.buf, &chat, sizeof(CHAT));
		int size =write(fd,&msg,sizeof(msg));
		if(strcmp(chat.content,"quit") == 0 || size <= 0)
		{
			printf("结束通信\n");
			pthread_cancel(tid);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
    unsigned int port = 9000;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("sockeet");
        exit(-1);
    }

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    client.sin_family = AF_INET;
    client.sin_port = htons(port);
    inet_pton(AF_INET, "192.168.80.128", &client.sin_addr.s_addr);

    if (-1 == connect(sockfd, (struct sockaddr *)&client, client_len))
    {
        perror("connect ");
        exit(-1);
    }

    MES msg;
    while (1)
    {
        show("start.txt"); // 显示开始界面
        int sel = -1;
        scanf("%d", &sel);
        switch (sel)
        {
        case 1:
            _register(sockfd); // 注册
            break;
        case 2:
            printf("请选择登录用户\n");
            printf("1.普通用户  2.管理员\n");
            int sel2 = -1;
            scanf("%d", &sel2);
            if (sel2 == 1) // 普通用户
            {
                if (_login(sockfd) == 1)
                {
                user_flage:
                    show("user.txt");
                    int user_sel = -1;
                    scanf("%d", &user_sel);
                    switch (user_sel)
                    {
                    case 1:
                        send_pack(sockfd); // 我要寄件
                        goto user_flage;
                        break;
                    case 2:
                        root_net_query(sockfd); // 查询网点
                        goto user_flage;
                        break;
                    case 3:
                        pack_track(sockfd); // 运单追踪
                        goto user_flage;
                        break;
                    case 4:
                        user_freight(sockfd); // 运费咨询
                        goto user_flage;
                        break;
                    case 5:
                        while (1)
                        {
                            printf("1.私聊  2.群聊  3.评论区  4.返回上一级\n");
                            printf("请输入您的选择：");
                            int user_sel2 = -1;
                            scanf("%d", &user_sel2);
                            if (user_sel2 == 1) // 私聊
                            {
                                chat_send(sockfd); // 私聊发送
                            }
                            else if (user_sel2 == 2) // 群聊
                            {
                                chat_grp(sockfd); // 群聊发送
                            }
                            else if (user_sel2 == 3) // 评论区
                            {
                                 section(sockfd); // 评论区
                            }
                            else if (user_sel2 == 4) // 返回上一级
                            {
                                printf("返回上一级\n");
                                break;
                            }
                            else
                            {
                                printf("输入错误，请重新选择\n");
                            }
                        }
                        goto user_flage;
                        break;
                    case 6:
                        del_order(sockfd); // 取消订单
                        goto user_flage;
                        break;
                    case 7:
                        Pickup(sockfd); // 我要取件
                        goto user_flage;
                        break;
                    case 8:
                        user_quit(sockfd); // 退出登录
                        printf("%s已成功退出\n", loginname);
                        exit(0);
                        break;
                    default:
                        goto user_flage;
                        break;
                    }
                }
                else
                {
                    printf("普通用户登录失败\n");
                }
            }
            else if (sel2 == 2) // 管理员
            {
                if (_login_root(sockfd) == 1)
                {
                root_flage:
                    show("root.txt");
                    int root_sel = -1;
                    scanf("%d", &root_sel);
                    switch (root_sel)
                    {
                    case 1:
                    root_flage2:
                        printf("1.删除用户  2.解冻用户  3.冻结用户  4.查询用户  5.返回上一级\n");
                        printf("请输入您的选择：");
                        int root_sel2 = -1;
                        scanf("%d", &root_sel2);
                        if (root_sel2 == 1) // 删除用户
                        {
                            root_user_del(sockfd);
                            goto root_flage2;
                        }
                        else if (root_sel2 == 2) // 解冻用户
                        {
                            root_user_unfreeze(sockfd);
                            goto root_flage2;
                        }
                        else if (root_sel2 == 3) // 冻结用户
                        {
                            root_user_freeze(sockfd);
                            goto root_flage2;
                        }
                        else if (root_sel2 == 4) // 查询用户
                        {
                            root_user_query(sockfd);
                            goto root_flage2;
                        }
                        else if (root_sel2 == 5) // 返回上一级
                        {
                            printf("返回上一级\n");
                        }
                        else
                        {
                            printf("输入错误，请重新选择\n");
                            goto root_flage2;
                        }
                        goto root_flage;
                        break;
                    case 2:
                    logistics_flage:
                        printf("1.快递员管理  2.快递状态更新 3.快递统计 4.网点管理 5.返回上一级\n");
                        printf("请输入您的选择：");
                        int root_sel3 = -1;
                        scanf("%d", &root_sel3);
                        if (root_sel3 == 1) // 快递员管理
                        {
                            while (1)
                            {
                                printf("1.新增快递员  2.删除快递员  3.查询快递员  4.修改快递员信息 5.返回上一级\n");
                                printf("请输入您的选择：");
                                scanf("%d", &root_sel3);
                                if (root_sel3 == 1) // 新增快递员
                                {
                                    root_Courier_add(sockfd);
                                }
                                else if (root_sel3 == 2) // 删除快递员
                                {
                                    root_Courier_del(sockfd);
                                }
                                else if (root_sel3 == 3) // 查询快递员
                                {
                                    root_Courier_query(sockfd);
                                }
                                else if (root_sel3 == 4) // 修改快递员信息
                                {
                                    root_Courier_modify(sockfd);
                                }
                                else if (root_sel3 == 5) // 返回上一级
                                {
                                    printf("返回上一级\n");
                                    goto logistics_flage;
                                }
                            }
                        }
                        else if (root_sel3 == 2) // 快递状态更新
                        {
                            root_Courier_update(sockfd);
                            goto logistics_flage;
                        }
                        else if (root_sel3 == 3) // 快递统计
                        {
                            root_Courier_statistics(sockfd);
                            goto logistics_flage;
                        }
                        else if (root_sel3 == 4) // 网点管理
                        {
                            while (1)
                            {
                                printf("1.新增网点 2.删除网点 3.查询网点 4.返回上一级\n");
                                printf("请输入您的选择：");
                                int root_sel4 = -1;
                                scanf("%d", &root_sel4);
                                if (root_sel4 == 1) // 新增网点
                                {
                                    root_net_add(sockfd);
                                }
                                else if (root_sel4 == 2) // 删除网点
                                {
                                    root_net_del(sockfd);
                                }
                                else if (root_sel4 == 3) // 查询网点
                                {
                                    root_net_query(sockfd);
                                }
                                else if (root_sel4 == 4) // 返回上一级
                                {
                                    printf("返回上一级\n");
                                    goto root_flage;
                                }
                            }
                        }
                        else if (root_sel3 == 5) // 返回上一级
                        {
                            printf("返回上一级\n");
                        }
                        else
                        {
                            printf("输入错误，请重新选择\n");
                            goto logistics_flage;
                        }
                        goto root_flage;
                        break;
                    case 3:
                        ch_freight(sockfd); // 修改运费标准
                        goto root_flage;
                        break;
                    case 4:
                        printf("管理员%s退出\n", loginname);
                        exit(0);
                        break;
                    default:
                        printf("输入错误，请重新选择\n");
                        goto root_flage;
                        break;
                    }
                }
                else
                {
                    printf("管理员登录失败\n");
                }
            }
            else
            {
                printf("输入错误，请重新选择\n");
            }
            break;
        case 3:
        vist:
            show("user.txt"); // 游客访问
            int vist_sel = -1;
            scanf("%d", &vist_sel);
            switch (vist_sel)
            {
            case 1:
                printf("请先登录，在进行该操作\n");
                goto vist;
                break;
            case 2:
                root_net_query(sockfd); // 网点查询
                goto vist;
                break;
            case 3:
                printf("请先登录，在进行该操作\n");
                goto vist;
                break;
            case 4:
                user_freight(sockfd); // 运费咨询
                goto vist;
                break;
            case 5:
                vist_section(sockfd); // 评论区  游客只能访问不能发言
                goto vist;
                break;
            case 6:
                printf("请先登录，在进行该操作\n");
                goto vist;
                break;
            case 7:
                printf("请先登录，在进行该操作\n");
                goto vist;
                break;
            case 8:
                printf("%s已成功退出\n", loginname);
                exit(0);
                break;
            default:
                printf("输入错误，请重新选择\n");
                goto vist;
                break;
            }

            break;
        case 4:
            ch_user(sockfd); // 修改密码
            break;
        case 5:
            printf("您已成功退出\n"); // 退出
            exit(0);
            break;
        default:
            printf("输入错误，请重新选择\n");
            break;
        }
    }
    close(sockfd);
    return 0;
}