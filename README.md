# sentry（哨兵代码）v2.0
## 文件层次

* Drivers(HAL库驱动层)
* Middlewares(freertos层)
* MDK-ARM （keil工程文件和编译文件）
* User（用户层）
  + bsp (用户底层配置层)
  + drivers（用户模块设备层）
    - counters(控制器等算法)
    - data_struture(用户自定义数据结构)
    - modules（模块和外设）
  + app（用户层控制层）
  + task(用户任务层)
* Inc（主函数和中断层头文件）
* Src（主函数和中断层源文件）

## 库说明

STM32CubeF4 Firmware Package V1.19.0
## 系统支持
FreeRTOS
#### 系统用户接口
 userFreeRTOSConfig.h
## 开发板支持

RM新板,RM旧板,彬哥第一代板（f427IIHx）

## BSP层接口支持(userdriverconfig.h)
外设|引脚宏接口|端口接口
-|-|-
LED|LED_x(x=1,2,3…)|LED_GPIO
蜂鸣器|BUZZER|BUZZER_GPIO
电源管理|POWER_x(x=1,2,3…)|POWER_GPIO
KEY|KEY_x(x=1,2,3…)|KEY_GPIO
激光|LASER|LASER_GPIO
## 代码命名规则说明

类型|命名规则|示例
-|-|-
 函数名|大驼峰|MyName
 普通变量|全小写,连接加下划线|my_name
 结构体声明|小驼峰加后缀Struct|myNameStruct
 结构体定义|小驼峰加后缀_t|myName_t
 枚举声明|小驼峰加后缀Enum|myNameEnum
 枚举定义|小驼峰加后缀_e|myName_e
 指针类型|相应类型加前缀p|普通变量指针类型pmy_name，结构体指针类型pmyName_t

## 底层外设支持（bsp）

外设|说明
-|-
串口|串口空闲中断+DMA+消息队列不定长度接收
can|can中断+消息队列接收

## 模块设备支持（modules）

模块设备|模块名称
-|-
电机|6623  3508  2006 maxion 6020
遥控|大疆遥控dbus
陀螺仪|GY555
上位机|匿名v2.6 平衡小车之家
编码器|增量AB相欧姆龙 E6A2-CW3C
蓝牙|HC05
超声波|HC-S04

## 数据结构和算法（data_struture，counters）

数据结构和算法|算法名称
-|-
pid控制器|普通pid，模糊pid, 专家pid
数据结构|循环队列
滤波器|加权滑动平均滤波

## 待完成任务
* [X] 拨弹测试
* [ ] 可调节射速发送 根据血量和热量调节射速
* [X] 底盘功率 200j的利用
* [X] 裁判系统数据读取 
* [X] 自瞄模式pid数据过小，寻找解决方案
* [X] 掉帧或掉跟随的之后的预判和巡航模式的切换
* [ ] 对激光测距和超声波滤波
* [X] 系统自检 启动后不能立马发送
* [X] yaw轴当转速度过开是反弹
* [X] 设计底盘和云台控制决策层
* [ ] 更改解析任务为同步事件
* [ ] 救急模式，只跑随机底盘，和直接自瞄的误差值，同时疯狂打弹
## 模块功能要求
  1. 编码器，可变方向增加记数
## 报错提示功能记录
 1. 没供电，can，所处的模块全部不能正常接收，（测试发送能不能成功）供电不正常，不让自检成功
 2. can不能发送，不能接收，同样供电的模块正常，can线断开，可能是can线的接触不良或者线短路，检测所处的can线，不让自检成功

## 调试问题记录
**Data**  : *2019-02-23 10:40*    **author**: *HIKpingtouge*  **log**:    
  1. 串口1接受不正常，遥控的值跳动严重  
     1. 换老旧版本的库就可以，1.24.0版库不行，神他妈坑爹  
     2. can 发送失败，发送值大于600，就发送失败，解决办法同上，（曾经裸机代码的时候也遇到这个问题，待测试用1.24.0配置的can是否能发送)
  
**Data**  : *2019-03-20 11:42*    **author**: *HIKpingtouge*  **log**:    
  1. can接一个电机正常，接上多个电机，就发送不正常  
    1. 多个can接收函数放在一个接收任务中，使得can接收严重掉帧，所以使得电机看起来不正常 
 
**Data**  : *2019-03-22 12:08*    **author**: *HIKpingtouge*    **log**:      
  1. 编写完拨弹处理  

**Data**  : *2019-03-23 01:25*    **author**: *HIKpingtouge*    **log**:      
  1. 解决yaw轴云台以恒定速度转到是反弹问题    
    1. 问题1: 编码器的值累加判断写错，导致不能正常判断反向记数    
    2. 问题2：计算误差函数的过零点处理判断被编译器优化掉    
  2. 和视觉协商新的数据通信方案    
**Data**  : *2019-03-25 01:03*   **author**: *HIKpingtouge*    **log**:      
  1. 添加读取裁判系统数据
  2. 设计完成功率缓存池
  3. 设计底盘控制模式优先级决策
**Data**  : *2019-03-25 17:38*   **author**: *HIKpingtouge*    **log**: 
  1. 底盘增量式 pid 用了位置式pid的误差计算，导致往返跑的时候误差值大于位			置式的过零点阀值，出现跑飞
	2. 云台反弹问题为遥控值累加给目标值，但累加值过大，云台相应慢，误差就会越来也大，当两者误差大于过零处理阀值，就会反弹  

**Data**  : *2019-03-26 21:57*   **author**: *HIKpingtouge*    **log**: 
  1.  设计云台自瞄模式和掉帧缓冲模式的过度，掉帧之后，两者切换之后pid参数不一样，切换到掉帧模式之后，直接令当前位置等于目标值会出现一点点卡顿，待配置软件定时器，定时记录刷新机器人位置
**Data**  : *2019-03-26 16:22*   **author**: *HIKpingtouge*    **log**: 
  1. 待添加底盘如果检测到基地被别人打，就优先驱赶他
	2. 今天完成云台自瞄缓冲模式，和自瞄模式的配合
	3. 明天完成和续航模式配合
	4. 后天开始用超声波测试底盘       
**Data**  : *2019-03-30 22:16*   **author**: *HIKpingtouge*    **log**:      
  1. 编写完底盘巡航模式
	2. 把底盘和云台can分离，
	3. 添加彬哥陀螺仪位状态检测模式（到时候换成轻触开关）
	4. can2的配置不是cube生成的，如果用cube生成，就会影响摩擦轮电机，问题未知。此底层待完善
