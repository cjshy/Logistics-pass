#include "main.h"
char name[BUFSIZ];
LOG on_line[1000]; // 在线人数
int oncount = 0;

void sendall(MES msg, int fd) // 群发
{
    for (int i = 0; i < oncount; i++)
    {
        if (on_line[i].fd != fd)
        {
            printf("send to %d\n", on_line[i].fd);
            write(on_line[i].fd, &msg, sizeof(MES));
        }
    }
}

void printQN(QN qn)
{
    for (size_t i = qn.col; i < qn.row * qn.col + qn.col; i++)
    {
        printf("%s\n", qn.presult[i]);
    }
}

char *generateTrackingNumber(int length) // 生成随机快递单号
{
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *result = (char *)malloc((length + 1) * sizeof(char));
    if (result)
    {
        for (int i = 0; i < length; i++)
        {
            int key = rand() % (sizeof(charset) - 1);
            result[i] = charset[key];
        }
        result[length] = '\0'; // 确保字符串以空字符结尾
    }
    return result;
}

void excut_sql(sqlite3 *db, char *psql) // 执行数据库语句
{
    char *msg = 0;
    sqlite3_exec(db, psql, 0, 0, &msg);
    if (msg != 0)
    {
        printf("%s\n", msg);
    }
}

QN get_table(sqlite3 *db, char *pSql) // 返回table表中的数据
{
    QN qn;
    char *msg = 0;
    int data = sqlite3_get_table(db, pSql, &qn.presult, &qn.row, &qn.col, &msg);
    if (data < 0)
    {
        printf("%s\n", msg);
    }
    return qn;
}

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

void register_ser(MES msg, int fd, sqlite3 *db) // 注册
{
    REG *p = (REG *)msg.buf;
    MES temp;
    int flage = 1; // 允许登录标志
    printf("收到注册信息用户名%s 密码%s\n", p->name, p->passwd);
    char buf[512]; // 存放数据库指令
    sprintf(buf, "SELECT * FROM user where name='%s';", p->name);
    QN qn = get_table(db, buf);
    if (qn.row == 0) // 不存在重复用户名
    {
        printf("注册成功\n");
        temp.type = 11; // 注册成功
        sprintf(temp.buf, "用户注册成功\n用户名为%s 密码为%s", p->name, p->passwd);
        write(fd, &temp, sizeof(temp));
        sprintf(buf, "INSERT INTO user VALUES ('%s','%s','%d');", p->name, p->passwd, flage);
        excut_sql(db, buf);
    }
    else if (qn.row != 0) // 存在重复用户名
    {
        temp.type = 10; // 注册失败
        printf("用户名已存在\n");
        sprintf(temp.buf, "用户名已存在,请重新注册");
        write(fd, &temp, sizeof(temp));
    }
}

void login_ser(MES msg, int fd, sqlite3 *db) // 登录
{
    REG *p = (REG *)msg.buf;
    int flage = 1; // 登录成功标志
    MES temp;      // 临时存储发送给客户端的值
    printf("收到登录信息用户名%s 密码%s\n", p->name, p->passwd);
    char buf[512]; // 存放数据库指令
    sprintf(buf, "SELECT * FROM user where name='%s';", p->name);
    QN qn = get_table(db, buf);
    if (qn.row == 0) // 不存在该用户
    {
        temp.type = 20; // 登录失败
        printf("用户名或密码错误\n");
        sprintf(temp.buf, "用户名或密码错误，请重新登录！");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 存在该用户
    {
        sprintf(buf, "SELECT * FROM user where name='%s' and passwd='%s';", p->name, p->passwd);
        qn = get_table(db, buf);
        if (qn.row == 0) // 密码不匹配
        {
            sprintf(buf, "SELECT * FROM user where name='%s' and flage='%d';", p->name, flage);
            qn = get_table(db, buf);
            if (qn.row == 0) // 用户被禁用
            {
                temp.type = 20; // 用户被禁用
                printf("用户被禁用\n");
                sprintf(temp.buf, "用户被禁用，请联系管理员！");
                write(fd, &temp, sizeof(temp));
            }
            else if (qn.row != 0) // 密码不匹配
            {
                temp.type = 20; // 登录失败
                printf("用户名或密码错误\n");
                sprintf(temp.buf, "用户名或密码错误，请重新登录！");
                write(fd, &temp, sizeof(temp));
            }
        }
        else if (qn.row != 0) // 匹配成功
        {
            sprintf(buf, "SELECT * FROM user where name='%s' and flage='%d';", p->name, flage);
            qn = get_table(db, buf);
            {
                if (qn.row == 0) // 用户被禁用
                {
                    temp.type = 20; // 用户被禁用
                    printf("用户被禁用\n");
                    sprintf(temp.buf, "用户被禁用，请联系管理员！");
                    write(fd, &temp, sizeof(temp));
                }
                else if (qn.row != 0) // 登录成功
                {
                    for (int i = 0; i < oncount; i++)
                    {
                        if (strcmp(p->name, on_line[i].name) == 0)
                        {
                            printf("用户在线中\n");
                            sprintf(temp.buf, "用户已在线");
                            temp.type = 22; // 用户在线
                            write(fd, &temp, sizeof(temp));
                            break;
                        }
                    }

                    if (temp.type != 22)
                    {
                        temp.type = 21; // 登录成功
                        strcpy(name, p->name);
                        printf("%s用户成功登录", p->name);
                        sprintf(temp.buf, "%s登录成功", p->name);
                        write(fd, &temp, sizeof(temp));
                        strcpy(on_line[oncount].name, p->name);
                        on_line[oncount++].fd = fd;
                        printf("当前在线人数%d人\n", oncount);
                    }
                }
            }
        }
    }
}

void logiin_ser_root(MES msg, int fd, sqlite3 *db) // 管理员登录
{
    REG *p = (REG *)msg.buf;
    MES temp; // 临时存储发送给客户端的值
    printf("收到登录信息用户名%s 密码%s\n", p->name, p->passwd);
    char buf[512]; // 存放数据库指令
    sprintf(buf, "SELECT * FROM root where name='%s';", p->name);
    QN qn = get_table(db, buf);
    if (qn.row == 0) // 不存在该用户
    {
        temp.type = 20; // 登录失败
        printf("用户名或密码错误\n");
        sprintf(temp.buf, "用户名或密码错误，请重新登录！");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 存在该用户
    {
        sprintf(buf, "SELECT * FROM root where name='%s' and passwd='%s';", p->name, p->passwd);
        qn = get_table(db, buf);
        if (qn.row == 0) // 密码不匹配
        {
            temp.type = 20; // 登录失败
            printf("用户名或密码错误\n");
            sprintf(temp.buf, "用户名或密码错误，请重新登录！");
            write(fd, &temp, sizeof(temp));
        }
        else if (qn.row != 0) // 匹配成功
        {
            for (int i = 0; i < oncount; i++)
            {
                if (strcmp(p->name, on_line[i].name) == 0)
                {
                    printf("用户在线中\n");
                    sprintf(temp.buf, "用户已在线");
                    temp.type = 22; // 用户在线
                    write(fd, &temp, sizeof(temp));
                    break;
                }
            }

            if (temp.type != 22)
            {
                temp.type = 21; // 登录成功
                strcpy(name, p->name);
                printf("%s用户成功登录\n", p->name);
                sprintf(temp.buf, "%s登录成功", p->name);
                write(fd, &temp, sizeof(temp));
                strcpy(on_line[oncount].name, p->name);
                on_line[oncount++].fd = fd;
                printf("当前在线人数%d人\n", oncount);
            }
        }
    }
}

void ch_user(MES msg, int fd, sqlite3 *db) // 修改密码
{
    REG *p = (REG *)msg.buf;
    MES temp; // 临时存储发送给客户端的值
    printf("收到修改信息的用户名%s 密码%s\n", p->name, p->passwd);
    char buf[512]; // 存放数据库指令
    sprintf(buf, "SELECT * FROM user where name='%s' and passwd='%s';", p->name, p->passwd);
    QN qn = get_table(db, buf);
    if (qn.row == 0)
    {
        printf("用户名或密码错误\n");
        temp.type = 40;
        sprintf(temp.buf, "用户名或密码错误");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 匹配成功
    {
        temp.type = 41;
        write(fd, &temp, sizeof(temp));
        read(fd, &msg, sizeof(msg));
        p = (REG *)msg.buf;
        printf("newpasswd = %s\n", p->passwd);
        sprintf(buf, "UPDATE user SET passwd='%s' WHERE name='%s';", p->passwd, p->name);
        excut_sql(db, buf);
        sprintf(temp.buf, "用户密码修改成功");
        printf("修改成功\n");
        write(fd, &temp, sizeof(temp));
    }
}

void send_pack(MES msg, int fd, sqlite3 *db) // 我要寄件
{
    srand(time(NULL)); // 初始化随机数生成器
    PACK pack = *(PACK *)msg.buf;
    CPACK cpack;
    MES temp;
    printf("收到寄件信息 寄件人：%s 寄件地址：%s 寄件人电话：%s \n 收件人：%s 收件地址：%s 收件人电话：%s \n  重量：%.2f  体积：%s  种类：%s  时间：%s\n", pack.sender, pack.sender_addr, pack.sender_phone, pack.receiver, pack.dest_addr, pack.dest_phone, pack.weight, pack.volume, pack.content, pack.time);
    char buf[512];
    // 生成快递单号
    char *tracking_number = generateTrackingNumber(length_pack);
    printf("生成快递单号：%s\n", tracking_number);
    //判断该快递单号在pack表中是否存在
    sprintf(buf, "SELECT * FROM pack WHERE id='%s';", tracking_number);
    QN qn = get_table(db, buf);
    if (qn.row != 0) // 该快递单号已存在
    {
        printf("该快递单号已存在\n");
        temp.type = 50;
        sprintf(temp.buf, "寄件异常，请重新寄件");
        write(fd, &temp, sizeof(temp));
        return;
    }
    // 写入cpack表
    strcpy(cpack.id, tracking_number);
    strcpy(cpack.sender, pack.sender);
    strcpy(cpack.sender_addr, pack.sender_addr);
    strcpy(cpack.sender_phone, pack.sender_phone);
    strcpy(cpack.receiver, pack.receiver);
    strcpy(cpack.dest_addr, pack.dest_addr);
    strcpy(cpack.dest_phone, pack.dest_phone);
    strcpy(cpack.content, pack.content);
    strcpy(cpack.time, pack.time);
    strcpy(cpack.company, pack.company);
    cpack.status = 0;
    // 分派快递员
    sprintf(buf, "SELECT name,id FROM courier WHERE status='%d' and company='%s';", 0, cpack.company); // 筛选出空闲的快递员
    qn = get_table(db, buf);
    int temp_col = qn.col;
    if (qn.row == 0) // 无空闲快递员
    {
        printf("无空闲快递员\n");
        sprintf(temp.buf, "无空闲快递员");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 匹配成功
    {
        for (int i = 0; i < qn.row; i++) // 输出快递员id
        {
            printf("快递员：%s 快递员ID：%s\n", qn.presult[qn.col], qn.presult[qn.col + 1]);
            qn.col += 2;
        }
        qn.col = temp_col;
        sprintf(temp.buf, "快递成功寄出，您的快递由%s公司的%s快递员负责，您的快递单号为%s", cpack.company, qn.presult[qn.col], tracking_number);
        strcpy(cpack.courier_name, qn.presult[qn.col]);
        strcpy(cpack.courier_id, qn.presult[qn.col + 1]);
        write(fd, &temp, sizeof(temp));
        // 写入cpack表
        sprintf(buf, "INSERT INTO cpack VALUES ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%d','%f','%f');", cpack.id, cpack.sender, cpack.sender_addr, cpack.sender_phone, cpack.receiver, cpack.dest_addr, cpack.dest_phone, cpack.content, cpack.time, cpack.courier_name, cpack.courier_id, cpack.company, cpack.status, 0.0, 0.0);
        excut_sql(db, buf);
        // 写入pack表
        sprintf(buf, "INSERT INTO pack VALUES ('%s','%s','%s','%s','%s','%s','%s','%f','%s','%s','%s');", tracking_number, pack.sender, pack.sender_addr, pack.sender_phone, pack.receiver, pack.dest_addr, pack.dest_phone, pack.weight, pack.volume, pack.content, pack.time);
        excut_sql(db, buf);
        // 通知快递员
        sprintf(buf, "SELECT * FROM courier WHERE id='%s' and company='%s';", qn.presult[qn.col + 1], cpack.company);
        QN qn_courier = get_table(db, buf);
        if (qn_courier.row != 0) // 匹配成功
        {
            sprintf(buf, "UPDATE courier SET status='%d' WHERE id='%s' and company='%s';", 1, cpack.courier_id, cpack.company);
            excut_sql(db, buf);
            printf("快递员%s已接单\n", cpack.courier_name);
        }
        // 更新cpack表中的运费以及运费标准
        // 大件
        int n;
        QN qn_cpack;
        if (strcmp(pack.volume, "大件") == 0)
        {
            float std;
            sprintf(buf, "SELECT standard_fee FROM fee WHERE volume='%s';", "大件"); // 从fee表中查找相应标准运费
            qn_cpack = get_table(db, buf);
            if (qn_cpack.row != 0) // 匹配成功
            {
                std = atof(qn_cpack.presult[qn_cpack.col]);
                printf("标准运费：%.2f/kg\n", std);
            }

            sprintf(buf, "SELECT weight FROM pack WHERE volume='%s' AND id ='%s';", "大件", cpack.id); // 筛选出指定id的大件
            qn_cpack = get_table(db, buf);
            n = qn_cpack.row;
            if (qn_cpack.row != 0) // 匹配成功
            {
                while (n > 0)
                {
                    sprintf(buf, "UPDATE cpack SET standard_fee='%f',fee='%f' WHERE id='%s';", std, std * atof(qn_cpack.presult[qn_cpack.col]), cpack.id);
                    n--;
                    qn_cpack.col += 1;
                    excut_sql(db, buf);
                }
            }
            else if (qn_cpack.row == 0) // 无指定id的大件
            {
                printf("无大件\n");
            }
        }

        else if (strcmp(pack.volume, "中件") == 0)
        {
            float std;
            sprintf(buf, "SELECT standard_fee FROM fee WHERE volume='%s';", "中件"); // 从fee表中查找相应标准运费
            qn_cpack = get_table(db, buf);
            if (qn_cpack.row != 0) // 匹配成功
            {
                std = atof(qn_cpack.presult[qn_cpack.col]);
                printf("标准运费：%.2f/kg\n", std);
            }
            // 中件
            sprintf(buf, "SELECT weight FROM pack WHERE volume='%s' AND id ='%s';", "中件", cpack.id); // 筛选出指定id的中件
            qn_cpack = get_table(db, buf);
            n = qn_cpack.row;
            if (qn_cpack.row != 0) // 匹配成功
            {
                while (n > 0)
                {
                    sprintf(buf, "UPDATE cpack SET standard_fee='%f',fee='%f' WHERE id='%s';", std, std * atof(qn_cpack.presult[qn_cpack.col]), cpack.id);
                    n--;
                    qn_cpack.col += 2;
                    excut_sql(db, buf);
                }
            }
            else if (qn_cpack.row == 0) // 无指定id的中件
            {
                printf("无中件\n");
            }
        }
        else if (strcmp(pack.volume, "小件") == 0)
        {
            float std;
            sprintf(buf, "SELECT standard_fee FROM fee WHERE volume='%s';", "小件"); // 从fee表中查找相应标准运费
            qn_cpack = get_table(db, buf);
            if (qn_cpack.row != 0) // 匹配成功
            {
                std = atof(qn_cpack.presult[qn_cpack.col]);
                printf("标准运费：%.2f/kg\n", std);
            }
            // 小件
            sprintf(buf, "SELECT weight FROM pack WHERE volume='%s' AND id ='%s';", "小件", cpack.id); // 筛选出指定id的小件
            qn_cpack = get_table(db, buf);
            n = qn_cpack.row;
            if (qn_cpack.row != 0) // 匹配成功
            {
                while (n > 0)
                {
                    sprintf(buf, "UPDATE cpack SET standard_fee='%f',fee='%f' WHERE id='%s';", std, std * atof(qn_cpack.presult[qn_cpack.col]), cpack.id);
                    n--;
                    qn_cpack.col += 2;
                    excut_sql(db, buf);
                }
            }
            else if (qn_cpack.row == 0) // 无指定id的小件
            {
                printf("无小件\n");
            }
        }
    }

    free(tracking_number);
}

void pack_tracking(MES msg, int fd, sqlite3 *db) // 查询快递
{
    USER user = *(USER *)msg.buf;
    MES temp;
    USER p;
    printf("收到查询信息 用户名：%s\n", user.name);
    char buf[512];
    sprintf(buf, "SELECT * FROM cpack where sender='%s' OR reciver='%s';", user.name, user.name);
    QN qn = get_table(db, buf);
    if (qn.row == 0)
    {
        printf("未查询到该用户的快递信息\n");
        temp.type = 2130; // 未查询到该用户的快递信息
        sprintf(temp.buf, "未查询到该用户的快递信息,没有您的快递");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 匹配成功
    {
        int temp_col = qn.col;
        for (int i = 0; i < qn.row; i++)
        {
            printf("快递单号：%s\n", qn.presult[qn.col]);
            qn.col += 15;
        }
        qn.col = temp_col;
        temp.type = 2131; // 查询成功
        // 依次写入快递单号
        p.n = qn.row;
        memcpy(temp.buf, &p, sizeof(p));
        write(fd, &temp, sizeof(temp)); // 发送快递单号数量
        memset(temp.buf, 0, sizeof(temp.buf));
        int n = p.n;
        while (n > 0)
        {
            sprintf(temp.buf, "快递单号：%s 寄件人：%s 寄件地址：%s 寄件人电话：%s  收件人：%s 收件地址：%s 收件人电话：%s 种类：%s  快递员：%s  快递员ID：%s  快递公司：%s  快递状态：%d  运费：%.2f\n", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2], qn.presult[qn.col + 3], qn.presult[qn.col + 4], qn.presult[qn.col + 5], qn.presult[qn.col + 6], qn.presult[qn.col + 7], qn.presult[qn.col + 9], qn.presult[qn.col + 10], qn.presult[qn.col + 11], atoi(qn.presult[qn.col + 12]), atof(qn.presult[qn.col + 14]));
            qn.col += 15;
            write(fd, &temp, sizeof(temp));
            memset(temp.buf, 0, sizeof(temp.buf));
            n--;
        }
        printf("查询成功\n");
    }
}

void root_user_del(MES msg, int fd, sqlite3 *db) // 删除用户 2411
{
    REG *p = (REG *)msg.buf;
    MES temp;
    printf("收到删除用户信息  用户名：%s\n", p->name);
    char buf[512];
    sprintf(buf, "SELECT * FROM user WHERE name='%s';", p->name);
    QN qn = get_table(db, buf);
    if (qn.row == 0)
    {
        printf("未查询到该用户信息\n");
        sprintf(temp.buf, "未查询到该用户信息");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0)
    {
        sprintf(buf, "DELETE FROM user WHERE name='%s';", p->name);
        excut_sql(db, buf);
        printf("删除成功\n");
        sprintf(temp.buf, "删除用户成功");
        write(fd, &temp, sizeof(temp));
    }
}

void root_user_unfreeze(MES msg, int fd, sqlite3 *db) // 解冻用户 2412
{
    REG *p = (REG *)msg.buf;
    MES temp;
    printf("收到解冻用户信息  用户名：%s\n", p->name);
    char buf[512];
    sprintf(buf, "SELECT * FROM user WHERE name='%s';", p->name);
    QN qn = get_table(db, buf);
    if (qn.row == 0)
    {
        printf("未查询到该用户信息\n");
        sprintf(temp.buf, "未查询到该用户信息");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0)
    {
        sprintf(buf, "UPDATE user SET flage='%d' WHERE name='%s';", 1, p->name);
        excut_sql(db, buf);
        printf("解冻成功\n");
        sprintf(temp.buf, "解冻用户成功");
        write(fd, &temp, sizeof(temp));
    }
}

void root_user_freeze(MES msg, int fd, sqlite3 *db) // 冻结用户 2413
{
    REG *p = (REG *)msg.buf;
    MES temp;
    printf("收到冻结用户信息  用户名：%s\n", p->name);
    char buf[512];
    sprintf(buf, "SELECT * FROM user WHERE name='%s';", p->name);
    QN qn = get_table(db, buf);
    if (qn.row == 0)
    {
        printf("未查询到该用户信息\n");
        sprintf(temp.buf, "未查询到该用户信息");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0)
    {
        sprintf(buf, "UPDATE user SET flage='%d' WHERE name='%s';", 0, p->name);
        excut_sql(db, buf);
        printf("冻结成功\n");
        sprintf(temp.buf, "冻结用户成功");
        write(fd, &temp, sizeof(temp));
    }
}

void root_user_query(MES msg, int fd, sqlite3 *db) // 查询用户 2414
{
    REG *p = (REG *)msg.buf;
    MES temp;
    printf("收到查询用户信息  用户名：%s\n", p->name);
    char buf[512];
    sprintf(buf, "SELECT * FROM user WHERE name='%s';", p->name);
    QN qn = get_table(db, buf);
    if (qn.row == 0)
    {
        printf("未查询到该用户信息\n");
        sprintf(temp.buf, "未查询到该用户信息");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 匹配成功
    {
        int temp_col = qn.col;
        for (int i = 0; i < qn.row; i++)
        {
            printf("用户名：%s 密码：%s 状态：%s\n", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2]);
            qn.col += 3;
        }
        qn.col = temp_col;
        sprintf(temp.buf, "查询成功\n用户名：%s 密码：%s 状态：%s", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2]);
        write(fd, &temp, sizeof(temp));
    }
}

char *root_Courier_id() // 工号生成器
{
    // 使用当前时间作为随机数种子
    srand(time(NULL));

    char *id = (char *)malloc(6 * sizeof(char)); // 分配6个字符的内存，包括结尾的空字符
    if (id)
    {
        for (int i = 0; i < 5; i++)
        {
            int key = rand() % 10;
            id[i] = '0' + key; // 只生成数字
        }
        id[5] = '\0'; // 确保字符串以空字符结尾
    }
    return id;
}

void root_Courier_add(MES msg, int fd, sqlite3 *db) // 新增快递员 24211
{
    char *id = root_Courier_id(); // 生成工号
    COUR *cour = (COUR *)msg.buf;
    MES temp;
    printf("收到新增快递员信息  姓名：%s  工号：%s  公司：%s  状态：%d\n", cour->name, id, cour->company, cour->status);
    char buf[512];
    sprintf(buf, "SELECT * FROM courier WHERE id='%s';", id);
    QN qn = get_table(db, buf);
    if (qn.row != 0) // 存在该快递员
    {
        printf("工号为%s的快递员已存在,请重新输入\n", id);
        sprintf(temp.buf, "工号为%s的快递员已存在", id);
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row == 0) // 不存在该快递员
    {
        sprintf(buf, "INSERT INTO courier VALUES ('%s','%s','%s','%d');", cour->name, id, cour->company, cour->status);
        excut_sql(db, buf);
        printf("新增快递员成功\n");
        sprintf(temp.buf, "新增快递员成功");
        write(fd, &temp, sizeof(temp));
    }
}

void root_Courier_del(MES msg, int fd, sqlite3 *db) // 解雇快递员 24212
{
    COUR *cour = (COUR *)msg.buf;
    MES temp;
    printf("收到解雇快递员信息  id：%s\n", cour->id);
    char buf[512];
    //判断该快递员是否空闲
    sprintf(buf, "SELECT status FROM courier WHERE id='%s';", cour->id);
    QN qn = get_table(db, buf);
    if (qn.row != 0) // 存在该快递员
    {
        int status = atoi(qn.presult[qn.col]);
        if (status == 0 || status == 2) // 该快递员正在空闲
        {
            sprintf(buf, "DELETE FROM courier WHERE id='%s';", cour->id);
            excut_sql(db, buf);
            printf("解雇快递员成功\n");
            sprintf(temp.buf, "解雇快递员成功");
            write(fd, &temp, sizeof(temp));
        }
        else if(status == 1)// 该快递员正在忙碌
        {
            printf("该快递员正在忙碌,请稍后再试\n");
            sprintf(temp.buf, "该快递员正在忙碌,请稍后再试");
            write(fd, &temp, sizeof(temp));
        }
    }
    else if (qn.row == 0) // 不存在该快递员
    {
        printf("未查询到该快递员信息\n");
        sprintf(temp.buf, "未查询到该快递员信息");
        write(fd, &temp, sizeof(temp));
    }
    
}

void root_Courier_query(MES msg, int fd, sqlite3 *db) // 查询快递员 24213
{
    COUR *cour = (COUR *)msg.buf;
    MES temp;
    printf("收到查询快递员信息  姓名：%s\n", cour->name);
    char buf[512];
    sprintf(buf, "SELECT * FROM courier WHERE name='%s';", cour->name);
    QN qn = get_table(db, buf);
    int n = qn.row;
    memcpy(temp.buf, &n, sizeof(n));
    write(fd, &temp, sizeof(temp));
    memset(temp.buf, 0, sizeof(temp.buf));
    if (qn.row == 0) // 不存在该快递员
    {
        printf("未查询到该快递员信息\n");
        sprintf(temp.buf, "未查询到该快递员信息");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 存在该快递员
    {
        int temp_col = qn.col;
        for (int i = 0; i < qn.row; i++)
        {
            printf("姓名：%s 工号：%s 公司：%s 状态：%d\n", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2], atoi(qn.presult[qn.col + 3]));
            qn.col += 4;
        }
        qn.col = temp_col;
        while (n > 0)
        {
            sprintf(temp.buf, "查询成功\n姓名：%s 工号：%s 公司：%s 状态：%d", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2], atoi(qn.presult[qn.col + 3]));
            qn.col += 4;
            write(fd, &temp, sizeof(temp));
            n--;
            memset(temp.buf, 0, sizeof(temp.buf));
        }
    }
}

void root_Courier_update(MES msg, int fd, sqlite3 *db) // 更新快递信息 2422
{
    CPACK *cpack = (CPACK *)msg.buf;
    MES temp;
    printf("收到更新快递信息  快递单号：%s\n", cpack->id);
    char buf[512];
    sprintf(buf, "SELECT * FROM cpack WHERE id='%s';", cpack->id);
    QN qn = get_table(db, buf);
    if (qn.row == 0) // 不存在该快递信息
    {
        printf("未查询到该快递信息\n");
        sprintf(temp.buf, "未查询到该快递信息");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 存在该快递信息
    {
        printf("存在该快递单号\n");
        // 更新快递状态
        read(fd, &temp, sizeof(temp));
        CPACK *cpack_temp = (CPACK *)temp.buf;
        int status = cpack_temp->status;
        printf("收到更新快递状态：%d\n", status);
        // 更新cpack表
        sprintf(buf, "UPDATE cpack SET status='%d' WHERE id='%s';", status, cpack->id);
        excut_sql(db, buf);
        printf("更新快递信息成功\n");
        sprintf(temp.buf, "更新快递信息成功");
        write(fd, &temp, sizeof(temp));
    }
}

void root_Courier_statistics(MES msg, int fd, sqlite3 *db) // 快递统计
{
    MES temp;
    printf("收到快递统计信息\n");
    char buf[512];
    sprintf(buf, "SELECT id,sender,reciver,time,status FROM cpack ;");
    QN qn = get_table(db, buf);
    int n = qn.row;
    memcpy(temp.buf, &n, sizeof(n));
    write(fd, &temp, sizeof(temp));
    memset(temp.buf, 0, sizeof(temp.buf));
    if (qn.row == 0) // 不存在快递信息
    {
        printf("目前没有快递信息\n");
        sprintf(temp.buf, "目前没有快递信息");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 存在快递信息
    {
        int temp_col = qn.col;
        for (int i = 0; i < qn.row; i++)
        {
            printf("快递单号：%s 寄件人：%s 收件人：%s 时间：%s 状态：%d\n", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2], qn.presult[qn.col + 3], atoi(qn.presult[qn.col + 4]));
            qn.col += 5;
        }
        qn.col = temp_col;
        while (n > 0)
        {
            sprintf(temp.buf, "快递统计\n快递单号：%s 寄件人：%s 收件人：%s 时间：%s 状态：%d", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2], qn.presult[qn.col + 3], atoi(qn.presult[qn.col + 4]));
            qn.col += 5;
            write(fd, &temp, sizeof(temp));
            n--;
            memset(temp.buf, 0, sizeof(temp.buf));
        }
    }
}

void root_net_add(MES msg, int fd, sqlite3 *db) // 新增网点
{
    NET *net = (NET *)msg.buf;
    MES temp;
    printf("收到新增网点信息  网点名称：%s  网点地址：%s  联系电话：%s\n", net->name, net->addr, net->phone);
    char buf[512];
    sprintf(buf, "SELECT * FROM net WHERE name='%s';", net->name);
    QN qn = get_table(db, buf);
    if (qn.row != 0) // 存在该网点
    {
        printf("网点名称为%s的网点已存在,请重新输入\n", net->name);
        sprintf(temp.buf, "网点名称为%s的网点已存在", net->name);
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row == 0) // 不存在该网点
    {
        sprintf(buf, "INSERT INTO net VALUES ('%s','%s','%s');", net->name, net->addr, net->phone);
        excut_sql(db, buf);
        printf("新增网点成功\n");
        sprintf(temp.buf, "位于%s的名称为%s的网点新增成功", net->addr, net->name);
        write(fd, &temp, sizeof(temp));
    }
}

void root_net_del(MES msg, int fd, sqlite3 *db) // 删除网点
{
    MES temp;
    NET *net = (NET *)msg.buf;
    printf("收到删除网点信息 网点：%s", net->name);
    char buf[512];
    sprintf(buf, "SELECT * FROM net WHERE name='%s';", net->name);
    QN qn = get_table(db, buf);
    if (qn.row != 0) // 存在这个网点
    {
        sprintf(buf, "DELETE FROM net WHERE name='%s';", net->name);
        excut_sql(db, buf);
        printf("网点%s删除成功\n", net->name);
        sprintf(temp.buf, "网点%s删除成功", net->name);
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row == 0) // 不存在该网点
    {
        printf("不存在%s网点\n", net->name);
        sprintf(temp.buf, "不存在%s网点，删除失败", net->name);
        write(fd, &temp, sizeof(temp));
    }
}

void root_net_query(MES msg, int fd, sqlite3 *db) // 查询网点
{
    MES temp;
    NET *net = (NET *)msg.buf;
    printf("收到查询网点信息 网点：%s\n", net->name);
    char buf[512];
    sprintf(buf, "SELECT * FROM net WHERE name='%s' OR addr='%s';", net->name, net->addr);
    QN qn = get_table(db, buf);
    int n = qn.row;
    memcpy(temp.buf, &n, sizeof(n));
    printf("查询到%d条网点信息\n", n);
    write(fd, &temp, sizeof(temp));
    memset(temp.buf, 0, sizeof(temp.buf));
    if (qn.row == 0) // 不存在该网点
    {
        printf("未查询到该网点信息\n");
        sprintf(temp.buf, "未查询到该网点信息");
        write(fd, &temp, sizeof(temp));
    }
    else if (qn.row != 0) // 存在该网点
    {
        printf("存在名为%s的网点\n", net->name);
        int temp_col = qn.col;
        for (int i = 0; i < qn.row; i++)
        {
            printf("网点名称：%s 网点地址：%s 联系电话：%s\n", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2]);
            qn.col += 3;
        }
        qn.col = temp_col;
        while (n > 0)
        {
            sprintf(temp.buf, "查询成功\n网点名称：%s 网点地址：%s 联系电话：%s", qn.presult[qn.col], qn.presult[qn.col + 1], qn.presult[qn.col + 2]);
            qn.col += 3;
            write(fd, &temp, sizeof(temp));
            n--;
            memset(temp.buf, 0, sizeof(temp.buf));
        }
    }
}

float user_freight(MES msg, int fd, sqlite3 *db) // 运费咨询
{
    MES temp;
    PACK *pack = (PACK *)msg.buf;
    printf("收到运费咨询信息  重量：%.2f 体积：%s\n", pack->weight, pack->volume);
    //  计算运费
    float fee = 0; // 运费
    float standard_fee = 0; //运费标准
    char buf[2024];
    if (strcmp(pack->volume, "大件") == 0)
    {
        //将fee表中的数据读入
        sprintf(buf, "SELECT standard_fee FROM fee WHERE volume='大件';");
        QN qn = get_table(db, buf);
        if (qn.row != 0) // 查询到标准运费
        {
            standard_fee = atof(qn.presult[qn.col]);
            printf("大件的运费标准是%.2f元/kg\n",standard_fee);
        }
        fee = pack->weight * standard_fee;
        printf("运费为%.2f元\n", fee);
        sprintf(temp.buf, "运费为%.2f元", fee);
        write(fd, &temp, sizeof(temp));
        return fee;
    }
    else if (strcmp(pack->volume, "中件") == 0)
    {
       //将fee表中的数据读入
        sprintf(buf, "SELECT standard_fee FROM fee WHERE volume='中件';");
        QN qn = get_table(db, buf);
        if (qn.row != 0) // 查询到标准运费
        {
            standard_fee = atof(qn.presult[qn.col]);
            printf("中件的运费标准是%.2f元/kg\n",standard_fee);
        }
        fee = pack->weight * standard_fee;
        printf("运费为%.2f元\n", fee);
        sprintf(temp.buf, "运费为%.2f元", fee);
        write(fd, &temp, sizeof(temp));
        return fee;
    }
    else if (strcmp(pack->volume, "小件") == 0)
    {
       //将fee表中的数据读入
        sprintf(buf, "SELECT standard_fee FROM fee WHERE volume='小件';");
        QN qn = get_table(db, buf);
        if (qn.row != 0) // 查询到标准运费
        {
            standard_fee = atof(qn.presult[qn.col]);
            printf("小件的运费标准是%.2f元/kg\n",standard_fee);
        }
        fee = pack->weight * standard_fee;
        printf("运费为%.2f元\n", fee);
        sprintf(temp.buf, "运费为%.2f元", fee);
        write(fd, &temp, sizeof(temp));
        return fee;
    }
    else
    {
        printf("未知体积，无法计算运费\n");
        sprintf(temp.buf, "未知体积，无法计算运费");
        write(fd, &temp, sizeof(temp));
        return -1;
    }
}

void chat_history(int fd, sqlite3 *db) // 聊天记录
{
    char buf[2024];
    MES temp;
    temp.type = 31;
    // CHAT *chat = (CHAT *)msg.buf;
    sprintf(buf, "select * from section"); // 显示section表中所有数据
    QN qn = get_table(db, buf);
    int n = qn.row;
    int temp_col = qn.col;
    for (int i = 0; i < n; i++)
    {
        printf("%s:%s\n", qn.presult[qn.col], qn.presult[qn.col + 1]);
        qn.col += 2;
    }
    qn.col = temp_col;
    while (n > 0)
    {
        sprintf(temp.buf, "%s:%s", qn.presult[qn.col], qn.presult[qn.col + 1]);
        qn.col += 2;
        write(fd, &temp, sizeof(temp));
        n--;
        memset(temp.buf, 0, sizeof(temp));
    }
}

void section(MES msg, int fd, sqlite3 *db) // 评论区
{
    CHAT *chat = (CHAT *)msg.buf;
    char names[255];
    strcpy(names, chat->name);
    printf("收到评论信息  姓名：%s  内容：%s\n", chat->name, chat->content);
    char buf[2024];
    char temp[2024];
    sprintf(buf, "%s:[%s]", chat->name, chat->content);
    // 写入section数据库
    sprintf(temp, "INSERT INTO section VALUES ('%s','%s')", chat->name, chat->content);
    excut_sql(db, temp);
    printf("%s\n", buf);
    chat_history(fd, db);
    // sendall(msg,fd);
}

void vist_section(MES msg, int fd, sqlite3 *db) // 查看评论区 31
{
    MES temp;
    printf("收到游客查看评论区信息\n");
    printf("%s\n", msg.buf);
    if (strcmp(msg.buf, "quit") == 0)
    {
        printf("退出评论区\n");
    }
    chat_history(fd, db);
}

void ch_freight(MES msg, int fd, sqlite3 *db) // 修改运费标准 243
{
    TXT *txt = (TXT *)msg.buf;
    MES temp;
    printf("收到修改运费标准信息  体积：%s  标准：%f\n", txt->volume, txt->flage);
    char buf[2024];
    // 改变fee表中的数据
    sprintf(buf, "UPDATE fee SET standard_fee='%f' WHERE volume='%s';", txt->flage, txt->volume);
    excut_sql(db, buf);
    printf("修改运费标准成功\n");
    sprintf(temp.buf, "修改运费标准成功");
    write(fd, &temp, sizeof(temp));
}

void del_pack(MES msg, int fd, sqlite3 *db) // 删除包裹 216
{
    PACK *pack = (PACK *)msg.buf;
    MES temp;
    printf("收到删除包裹信息  包裹id：%s 用户名：%s\n", pack->id, pack->sender);
    char buf[2024];
    // 判断是否为该用户的包裹
    sprintf(buf, "SELECT * FROM pack WHERE id='%s' AND sender='%s';", pack->id, pack->sender);
    QN qn = get_table(db, buf);
    if (qn.row != 0) // 为该用户的包裹
    {
        // 查找该快递的状态
        sprintf(buf, "SELECT status FROM cpack WHERE id='%s' AND sender='%s';", pack->id, pack->sender);
        QN qn = get_table(db, buf);
        if (qn.row != 0) // 查询到该快递的状态
        {
            if (atoi(qn.presult[qn.col]) == 1 || atoi(qn.presult[qn.col]) == 2) // 快递状态为1,2 已发货
            {
                printf("该快递已发货，无法删除\n");
                sprintf(temp.buf, "该快递已发货，无法删除");
                write(fd, &temp, sizeof(temp));
                return;
            }
            else if (atoi(qn.presult[qn.col]) == 0) // 快递状态为0  未发货
            {
                // 找到该快递员的id号
                sprintf(buf, "SELECT courier_id FROM cpack WHERE id='%s' AND sender='%s';", pack->id, pack->sender);
                QN qn = get_table(db, buf);
                if (qn.row != 0) // 查询到该快递员的id号
                {
                    sprintf(buf, "update courier set status=0 where id='%s';", qn.presult[qn.col]); // 将该快递员的状态改为0
                    excut_sql(db, buf);
                }
                else if (qn.row == 0)
                {
                    printf("未找到快递员信息\n");
                }
                sprintf(buf, "DELETE FROM pack WHERE id='%s' AND sender='%s';", pack->id, pack->sender); // 删除pack表中的数据
                excut_sql(db, buf);
                sprintf(buf, "DELETE FROM cpack WHERE id='%s' AND sender='%s';", pack->id, pack->sender); // 删除cpack表中的数据
                excut_sql(db, buf);

                printf("删除包裹成功\n");
                sprintf(temp.buf, "包裹退货成功");
                write(fd, &temp, sizeof(temp));
            }
            else if(atoi(qn.presult[qn.col]) == 3)
            {
                printf("该订单号异常，无法删除\n");
                sprintf(temp.buf, "该订单号异常，无法删除。请联系客服解决\n");
                write(fd, &temp, sizeof(temp));
            }
        }
    }
    else if (qn.row == 0) // 不是该用户的包裹
    {
        printf("不是该用户的包裹\n");
        sprintf(temp.buf, "不是该用户的包裹");
        write(fd, &temp, sizeof(temp));
        return;
    }
}

void Pickup(MES msg, int fd, sqlite3 *db) // 取件 217
{
    PACK *pack = (PACK *)msg.buf;
    MES temp;
    printf("收到取件信息  包裹id：%s 用户名：%s\n", pack->id, pack->receiver);
    char buf[2024];
    // 判断是否为该用户的包裹
    sprintf(buf, "SELECT status FROM cpack WHERE id='%s' AND reciver='%s';", pack->id, pack->receiver);
    QN qn = get_table(db, buf);
    if (qn.row != 0) // 为该用户的包裹
    {
        if (atoi(qn.presult[qn.col]) == 2) // 快递状态为2  已签收
        {
            printf("%s用户成功取件\n", pack->sender);
            // 查找该包裹的快递员id号
            sprintf(buf, "SELECT courier_id FROM cpack WHERE id='%s' AND reciver='%s';", pack->id, pack->receiver);
            QN qn = get_table(db, buf);
            if (qn.row != 0) // 查询到该快递员的id号
            {
                sprintf(buf, "update courier set status=0 where id='%s';", qn.presult[qn.col]); // 将该快递员的状态改为0
                excut_sql(db, buf);

            }
            else if (qn.row == 0)
            {
                printf("未找到快递员信息\n");
            }
            // 删除cpack表中对应数据
            sprintf(buf, "DELETE FROM cpack WHERE id='%s' AND reciver='%s';", pack->id, pack->receiver);
            excut_sql(db, buf);
            // 删除pack表中对应数据
            sprintf(buf, "DELETE FROM pack WHERE id='%s' AND reciver='%s';", pack->id, pack->receiver);
            excut_sql(db, buf);
            sprintf(temp.buf, "%s用户成功取件", pack->receiver);
            write(fd, &temp, sizeof(temp));
        }
        else if (atoi(qn.presult[qn.col]) != 2)
        {
            printf("快递还未到达网点\n");
            sprintf(temp.buf, "快递还未到达网点");
            write(fd, &temp, sizeof(temp));
        }
    }
    else if (qn.row == 0) // 不是该用户的包裹
    {
        sprintf(buf, "SELECT * FROM cpack WHERE id='%s';", pack->id); // 是否存在该订单号
        QN qn = get_table(db, buf);
        if (qn.row != 0) // 存在该订单号
        {
            printf("该包裹不是%s用户的\n", pack->receiver);
            sprintf(temp.buf, "该包裹不是%s用户的", pack->receiver);
            write(fd, &temp, sizeof(temp));
        }
        else if (qn.row == 0) // 不存在该订单号
        {
            printf("该订单号不存在\n");
            sprintf(temp.buf, "该订单号不存在");
            write(fd, &temp, sizeof(temp));
        }
    }
}

void user_quit(MES msg, int fd, sqlite3 *db) // 用户退出 218
{
    LOG *login = (LOG *)msg.buf;
    MES temp;
    printf("收到用户退出信息  用户名：%s\n", login->name);
    for(int i = 0; i < oncount; i++)
    {
        if(on_line[i].fd == fd)
        {
            printf("用户%s退出\n", login->name);
            for(;i<oncount-1;i++)
            {
                on_line[i] = on_line[i+1];
            }
            oncount--;
            printf("当前在线人数%d\n", oncount);
            sprintf(temp.buf, "用户%s退出", login->name);
            write(fd, &temp, sizeof(temp));
            break;
        }
    }
}

void root_Courier_modify(MES msg, int fd, sqlite3 *db)//修改快递员信息 24214
{
    COUR *cour = (COUR *)msg.buf;
    MES temp;
    if(cour->flage == 1)
    {
        printf("收到修改快递员信息  快递员id：%s  公司：%s\n", cour->id,cour->company);
        char buf[2024];
        //判断快递员的状态
        sprintf(buf, "SELECT status FROM courier WHERE id='%s';", cour->id);
        QN qn = get_table(db, buf);
        if(qn.row != 0)
        {
            if(atoi(qn.presult[qn.col]) == 0 || atoi(qn.presult[qn.col]) == 2)//快递员状态为空闲 可更改
            {
                 //更新courier表中的数据
                sprintf(buf, "UPDATE courier SET company='%s' WHERE id='%s';", cour->company, cour->id);
                excut_sql(db, buf);
                printf("修改快递员信息成功\n");
                sprintf(temp.buf, "修改快递员信息成功");
                write(fd, &temp, sizeof(temp));
            }
            else if(atoi(qn.presult[qn.col]) == 1)//快递员状态为忙碌 不可更改
            {
                printf("该快递员正在工作，无法修改\n");
                sprintf(temp.buf, "该快递员正在工作，无法修改");
                write(fd, &temp, sizeof(temp));
            }
        }
        else if(qn.row == 0)//不存在该快递员
        {
            printf("不存在该快递员\n");
            sprintf(temp.buf, "不存在该快递员");
            write(fd, &temp, sizeof(temp));
        }
       
    }
    else if(cour->flage == 2)
    {
        printf("收到修改快递员信息  快递员id：%s  状态：%d\n", cour->id, cour->status);
        char buf[2024];
        //判断该快递员是否存在
        sprintf(buf, "SELECT * FROM courier WHERE id='%s';", cour->id);
        QN qn = get_table(db, buf);
        if(qn.row != 0)//存在该快递员
        {
             //更新courier表中的数据
            sprintf(buf, "UPDATE courier SET status='%d' WHERE id='%s';", cour->status, cour->id);
            excut_sql(db, buf);
            printf("修改快递员信息成功\n");
            sprintf(temp.buf, "修改快递员信息成功");
            write(fd, &temp, sizeof(temp));
        }
        else if(qn.row == 0)//不存在该快递员
        {
            printf("不存在该快递员\n");
            sprintf(temp.buf, "不存在该快递员");
            write(fd, &temp, sizeof(temp));
        }
       
    }
}

void chat_send(MES msg, int fd, sqlite3 *db) // 私聊 2151
{
    int flag =-1;
    CHAT *chat = (CHAT *)msg.buf;
    MES temp;
    printf("收到私聊信息  发送者：%s  接收者：%s  内容：%s\n", chat->name, chat->dest_name, chat->content);
    //找到目标用户
    for(int i = 0; i < oncount; i++)
    {
        if(strcmp(chat->dest_name, on_line[i].name) == 0)
        {
            flag = 1;
            write(on_line[i].fd, &msg, sizeof(msg));
            printf("%s:[%s]", chat->name, chat->content);
            break;
        }
    }

     if (flag != 1)
    {
        for (int i = 0; i < oncount; i++)
        {
            if (strcmp(on_line[i].name, chat->name) == 0)
            {
                msg.type = -1;
                printf("send to fail :%d\n", on_line[i].fd);
                write(on_line[i].fd, &msg, sizeof(MES));
            }
        }
        printf("没有这个用户\n");
    }
}

void chat_grp(MES msg, int fd, sqlite3 *db) // 群聊发送 2152
{
    CHAT *chat = (CHAT *)msg.buf;
    char buf[2024];
    sprintf(buf, "%s:[%s]",chat->name,chat->content);
    printf("收到群聊信息  发送者：%s  内容：%s\n",chat->name,chat->content);
    sendall(msg,fd);
}

int main(int len, char *arg[])
{
    sqlite3 *db = NULL;
    int data = -1;
    // 打开指定的数据库文件，如果不存在将创建一个同名的数据库文件
    data = sqlite3_open("test.db", &db);
    char *msg = 0;
    // 创建一个user表
    char *sql = "CREATE TABLE user(name varchar(255),passwd varchar(255),flage int);";
    sqlite3_exec(db, sql, 0, 0, &msg);
    if (msg != 0)
    {
        if (strcmp("table user already exists", msg) != 0)
        {
            printf("%s\n", msg);
            return -1;
        }
    }
    // 创建一个root表
    sql = "CREATE TABLE root(name varchar(255),passwd varchar(255));";
    sqlite3_exec(db, sql, 0, 0, &msg);
    if (msg != 0)
    {
        if (strcmp("table root already exists", msg) != 0)
        {
            printf("%s\n", msg);
            return -1;
        }
    }
    // 创建一个pack表
    sql = "CREATE TABLE pack(id varchar(255),sender varchar(255),sender_addr varchar(255),send_phone varchar(255),reciver varchar(255),dest_addr varchar(255),dest_phone varchar(255),weight float,volume varchar(255),content varchar(255),time varchar(255));";
    sqlite3_exec(db, sql, 0, 0, &msg);
    if (msg != 0)
    {
        if (strcmp("table pack already exists", msg) != 0)
        {
            printf("%s\n", msg);
            return -1;
        }
    }
    // 创建一个courier表
    sql = "CREATE TABLE courier(name varchar(255),id varchar(255),company varchar(255),status int);";
    sqlite3_exec(db, sql, 0, 0, &msg);
    if (msg != 0)
    {
        if (strcmp("table courier already exists", msg) != 0)
        {
            printf("%s\n", msg);
            return -1;
        }
    }
    // 创建一个cpack表
    sql = "CREATE TABLE cpack(id varchar(255),sender varchar(255),sender_addr varchar(255),send_phone varchar(255),reciver varchar(255),dest_addr varchar(255),dest_phone varchar(255),content varchar(255),time varchar(255),courier_name varchar(255),courier_id varchar(255),company varchar(255),status int,standard_fee int,fee int);";
    sqlite3_exec(db, sql, 0, 0, &msg);
    if (msg != 0)
    {
        if (strcmp("table cpack already exists", msg) != 0)
        {
            printf("%s\n", msg);
            return -1;
        }
    }
    // 创建一个net表
    sql = "CREATE TABLE net(name varchar(255),addr varchar(255),phone varchar(255));";
    sqlite3_exec(db, sql, 0, 0, &msg);
    if (msg != 0)
    {
        if (strcmp("table net already exists", msg) != 0)
        {
            printf("%s\n", msg);
            return -1;
        }
    }
    // 创建一个section表
    sql = "CREATE TABLE section(name varchar(255),txt varchar(2024));";
    sqlite3_exec(db, sql, 0, 0, &msg);
    if (msg != 0)
    {
        if (strcmp("table section already exists", msg) != 0)
        {
            printf("%s\n", msg);
            return -1;
        }
    }

    // 创建一个fee表
    sql = "CREATE TABLE fee(volume varchar(255),standard_fee float);";
    sqlite3_exec(db, sql, 0, 0, &msg);
    if (msg != 0)
    {
        if (strcmp("table fee already exists", msg) != 0)
        {
            printf("%s\n", msg);
            return -1;
        }
    }
    // 套接字
    struct sockaddr_in addr = getAddr("192.168.80.128", 9000); // 获得一个地址结构体

    int fd = socket(AF_INET, SOCK_STREAM, 0); // 创建了一个TCP的套接字（用来通信）

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // 让程序在短时间内能够重复的绑定同一个地址

    if (-1 == bind(fd, (struct sockaddr *)&addr, sizeof(addr))) // 使用套接字把192.168.50.194:9000给绑了，防止被其他程序占用
    {
        printf("绑定地址失败\n");
        return 0;
    }
    listen(fd, 5); // 启动监听功能，同时排队连接的人数，超过这个人数对方将会马上返回连接失败

    struct epoll_event ev, events[MAX_EVENTS]; // 一个事件结构体，需要填写里面的事件和套接字描述符
    int listen_sock, conn_sock, nfds, epollfd;
    epollfd = epoll_create1(0); // 创建一个epoll对象
    if (epollfd == -1)
    {
        perror("epoll_creat1");
        exit(-1);
    }
    // 为服务器准备第一个事件结构体，里面有感兴趣的事件和监听套接字
    ev.events = EPOLLIN; // 可读事件
    ev.data.fd = fd;     // 监听套接字

    // 把监听套接字以及套接字对应事件写入epoll对象
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll_ctl :listen _sock");
    }
    // 循环等待客户端的连接
    while (1)
    {
        // 等待epoll对象里面的套接字 上发生结构体中指定事件，并且把发生的
        // 事件的结构体赋值到数组events中
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
        }

        for (int n = 0; n < nfds; n++)
        {
            if (events[n].data.fd == fd) // 发生监听事件
            {
                struct sockaddr_in cliaddr;
                socklen_t clilen = sizeof(cliaddr);
                int newFd = accept(events[n].data.fd, (struct sockaddr *)&cliaddr, &clilen);
                if (newFd == -1)
                {
                    perror("accept");
                }
                ev.events = EPOLLIN;
                ev.data.fd = newFd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newFd, &ev) == -1)
                {
                    perror("epoll_ctl");
                }
            }
            else
            {
                MES msg;
                int ret = read(events[n].data.fd, &msg, sizeof(msg));
                if (ret <= 0)
                {
                    printf("与客户端%d断开连接\n", events[n].data.fd);
                    ev.events = EPOLLIN;
                    ev.data.fd = events[n].data.fd;
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev) == -1)
                    {
                        perror("epoll_ctl");
                    }
                }
                else
                {
                    if (msg.type == 1) // 注册
                    {
                        register_ser(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 2) // 登录
                    {
                        login_ser(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 24) // root
                    {
                        logiin_ser_root(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 4) // 修改密码
                    {
                        ch_user(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 211) // 我要寄件
                    {
                        send_pack(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 213) // 查询快递
                    {
                        pack_tracking(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 2411) // 删除用户
                    {
                        root_user_del(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 2412) // 解冻用户
                    {
                        root_user_unfreeze(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 2413) // 冻结用户
                    {
                        root_user_freeze(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 2414) // 查询用户
                    {
                        root_user_query(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 24211) // 新增快递员
                    {
                        root_Courier_add(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 24212) // 解雇快递员
                    {
                        root_Courier_del(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 24213) // 查询快递员
                    {
                        root_Courier_query(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 2422) // 更新快递信息
                    {
                        root_Courier_update(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 2423) // 快递统计
                    {
                        root_Courier_statistics(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 24241) // 新增网点
                    {
                        root_net_add(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 24242) // 删除网点
                    {
                        root_net_del(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 24243) // 查询网点
                    {
                        root_net_query(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 214) // 运费咨询
                    {
                        user_freight(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 215) // 评论区
                    {
                        section(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 243) // 修改运费标准
                    {
                        ch_freight(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 31) // 游客查看评论区
                    {
                        vist_section(msg, events[n].data.fd, db);
                    }
                    else if (msg.type == 216) // 用户取消订单
                    {
                        del_pack(msg, events[n].data.fd, db); // 删除包裹
                    }
                    else if (msg.type == 217) // 用户确认收货
                    {
                        Pickup(msg, events[n].data.fd, db); // 确认收货
                    }
                    else if (msg.type == 218) // 用户退出
                    {
                        user_quit(msg, events[n].data.fd, db); // 用户退出
                    }
                    else if(msg.type == 24214) // 修改快递员信息
                    {
                        root_Courier_modify(msg, events[n].data.fd, db); // 修改快递员信息
                    }
                    else if(msg.type == 2151)   //私聊
                    {
                        chat_send(msg, events[n].data.fd, db); // 私聊
                    }
                    else if(msg.type == 2152)   //群聊
                    {
                        chat_grp(msg, events[n].data.fd, db); // 群聊
                    }
                }
            }
        }
    }
    close(fd);
    sqlite3_close(db); // 关闭数据库
    return 0;
}