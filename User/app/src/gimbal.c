/**
	|--------------------------------- Copyright --------------------------------|
	|                                                                            |
	|                      (C) Copyright 2019,海康平头哥,                         |
	|           1 Xuefu Rd, Huadu Qu, Guangzhou Shi, Guangdong Sheng, China      |
	|                           All Rights Reserved                              |
	|                                                                            |
	|           By(GCU The wold of team | 华南理工大学广州学院机器人野狼队)         |
	|                    https://github.com/GCUWildwolfteam                      |
	|----------------------------------------------------------------------------|
	|--FileName    : gimbal.c                                              
	|--Version     : v2.0                                                          
	|--Author      : 海康平头哥                                                     
	|--Date        : 2019-01-26             
	|--Libsupports : 
	|--Description :                                                     
	|--FunctionList                                                     
	|-------1. ....                                                     
	|          <version>:                                                     
	|     <modify staff>:                                                       
	|             <data>:                                                       
	|      <description>:                                                        
	|-------2. ...                                                       
	|-----------------------------declaration of end-----------------------------|
 **/
#include "gimbal.h" 
/* -------------- 模块自定义标志位宏 ----------------- */
#define  SCAN_MODE          (0x80000000U)//扫描模式
#define  PC_SHOOT_MODE      (0x40000000U)//自瞄打击模式
#define  DELEC_USER_MODE    (0x00FFFFFFU)//清除用户自定义标志位
/* -------------- 私有宏 ----------------- */
	#define QUEUE_LEN      5U//深度为5
	#define  QUEUE_SIZE    8U//长度为5;
/* -------------- 结构体声明 ----------------- */
	gimbalStruct gimbal_t;//云台结构体
  RM6623Struct yaw_t;//yaw轴电机
	postionPidStruct yawOuterLoopPid_t;//yaw 电机外环pid
	speedPidStruct yawInnerLoopPid_t;//yaw 电机内环pid
	GM6020Struct pitch_t;//pitch轴电机
	postionPidStruct pitchOuterLoopPid_t;//pitch 电机外环pid
	speedPidStruct pitchInnerLoopPid_t;//pitch 电机内环pid
/* -------------- 任务句柄 ----------------- */
		osThreadId startRammerTaskHandle; 
/* -------------- 外部链接 ----------------- */
		extern osThreadId startGimbalTaskHandle;
/* -------------- 发送队列 ----------------- */
  xQueueHandle gimbal_queue;
/* ----------------- 任务钩子函数 -------------------- */
	void StartRammerTask(void const *argument);
/* -------------- 私有函数 ----------------- */
/* ----------------- 临时变量 -------------------- */
    int16_t taddd =0;
    int16_t xianfuweizhi =3000;
    int16_t xianfusudu = 5000;//STUCK_BULLET_THRE
    int16_t yawxianfu = 1900;
		int16_t pitchxianfu =10000;
		int16_t sudupitchxianfu =10000;
	/**
	* @Data    2019-01-27 17:09
	* @brief   云台结构体初始化
	* @param  CAN_HandleTypeDef* hcanx（x=1,2）
	* @retval  void
	*/
	void GimbalStructInit(const dbusStruct* pRc_t,const pcDataStruct* pPc_t)
	{
		gimbal_t.pRc_t = pRc_t;//遥控数据地址
    gimbal_t.pPc_t = pPc_t;//小电脑数据地址
    gimbal_t.status = MOD_READ;
    gimbal_t.prammer_t =RammerInit();
		gimbal_t.pYaw_t = YawInit();
		gimbal_t.pPitch_t = PitchInit();
  /* ------ 开启摩擦轮 ------- */
		BrushlessMotorInit();
  /* ------ 创建云台发送队列 ------- */
	  gimbal_queue	= xQueueCreate(QUEUE_LEN,QUEUE_SIZE);//一定要在用之前创建队列
 	/* ------ 设置初始化标志位 ------- */
		SET_BIT(gimbal_t.status,INIT_OK);
	/* ------ 挂起任务，等待初始化 ------- */
		vTaskSuspend(startGimbalTaskHandle);
	/* ------ 设置机器人初始化状态 ------- */
		SetGimBalInitStatus();
 /* ------ 创建拨弹任务 ------- */
	osThreadDef(startRammerTask,StartRammerTask,osPriorityNormal,0,RAMMER_HEAP_SIZE);
	startRammerTaskHandle = osThreadCreate(osThread(startRammerTask),NULL);
	}
/**
	* @Data    2019-01-28 11:40
	* @brief   云台数据解析
	* @param   void
	* @retval  void
	*/
	void GimbalParseDate(uint32_t id,uint8_t *data)
	{
		switch (id)
		{
			case RAMMER_RX_ID:
				RM2006ParseData(gimbal_t.prammer_t,data);
     gimbal_t.prammer_t->real_angle = RatiometricConversion(gimbal_t.prammer_t->tem_angle,7000,36,&(gimbal_t.prammer_t->last_real),&(gimbal_t.prammer_t->coefficient),gimbal_t.status);
  #ifdef ANTI_CLOCK_WISE  //逆时针为正方向
        AntiRM2006ParseData(gimbal_t.prammer_t,data);
  #endif
				break;
			case YAW_RX_ID:
				RM6623ParseData(gimbal_t.pYaw_t,data);
				gimbal_t.pYaw_t->real_angle = NoRatiometricConversion(gimbal_t.pYaw_t->tem_angle,5000,5,&(gimbal_t.pYaw_t->last_real),&(gimbal_t.pYaw_t->coefficient),gimbal_t.status);
				break;
			case PITCH_RX_ID:
        GM6020ParseData(gimbal_t.pPitch_t,data);
				break;
		
			default:
				break;
		}
    SET_BIT(gimbal_t.status,RX_OK);
	}
/**
	* @Data    2019-02-14 21:01
	* @brief   云台控制
	* @param   void
	* @retval  void
	*/
  int16_t i = 0;
  float iii = 0.1;
	void GimbalAutoControl(void)
	{
		int16_t yaw=0;
    int16_t pitch=0;
    int16_t rammer=0;
    //ControlSwitch();//控制模式切换
//    i +=(int16_t)(gimbal_t.pRc_t->ch1 * iii);
//        if(i>20480)
//        {
//          i = i-20480;
//        }
//        else if(i<0)
//        {
//          i = 20480 + i;
//        }
//        gimbal_t.pYaw_t->target = i;
//    rammer = RammerPidControl(gimbal_t.prammer_t->target);
    
    yaw = YawPidControl(gimbal_t.pYaw_t->target);
    pitch = PitchPidControl(gimbal_t.pPitch_t->target);
    GimbalCanTx(yaw,pitch,rammer);
	}
/**
	* @Data    2019-02-15 15:10
	* @brief   云台数据发送
	* @param   void
	* @retval  void
	*/
	HAL_StatusTypeDef GimbalCanTx(int16_t yaw,int16_t pitch,int16_t rammer)
	{
		uint8_t s[8];
    memset(s,0,8);
    s[0] = (uint8_t)(yaw>>8) & 0xFF;
    s[1] = (uint8_t)yaw;
    s[2] = (uint8_t)(pitch>>8) & 0xFF;
    s[3] = (uint8_t)pitch;
		s[4] = (uint8_t)(rammer>>8) & 0xFF;
		s[5] = (uint8_t)rammer;
   	xQueueSendToBack(gimbal_queue,s,0);
		return HAL_OK;
	}
  /**
  * @Data    2019-03-15 23:14
  * @brief   获取云台状态
  * @param   void
  * @retval  void
  */
  uint32_t GetGimbalStatus(void)
  {
    return gimbal_t.status;
  }
  /**
  * @Data    2019-03-13 03:48
  * @brief   获取云台结构体地址 可读写，不能乱调用
  * @param   void
  * @retval  void
  */
  gimbalStruct *RWGetgimbalStructAddr(void)
  {
    return &gimbal_t;
  }
/**
	* @Data    2019-03-16 18:04
	* @brief   拨弹电机任务
	* @param   void
	* @retval  void
	*/
	void StartRammerTask(void const *argument)
	{
		for(;;)
		{
      RammerControl();
			osDelay(10);
		}
	}
 /**
	 * @Data    2019-03-19 17:58
	 * @brief   云台初始化状态设置
	 * @param   void
	 * @retval  void
	 */
	 void SetGimBalInitStatus(void)
	 {
		/* -------------- 拨弹参数设置 ----------------- */
		   gimbal_t.prammer_t->target =  gimbal_t.prammer_t->real_angle; //目标值
			/* ------ 外环pid参数 ------- */
				gimbal_t.prammer_t->ppostionPid_t->kp = 4;
				gimbal_t.prammer_t->ppostionPid_t->kd = 0;
				gimbal_t.prammer_t->ppostionPid_t->ki = 0;
				gimbal_t.prammer_t->ppostionPid_t->error = 0;
				gimbal_t.prammer_t->ppostionPid_t->last_error = 0;//上次误差
				gimbal_t.prammer_t->ppostionPid_t->integral_er = 0;//误差积分
				gimbal_t.prammer_t->ppostionPid_t->pout = 0;//p输出
				gimbal_t.prammer_t->ppostionPid_t->iout = 0;//i输出
				gimbal_t.prammer_t->ppostionPid_t->dout = 0;//k输出
				gimbal_t.prammer_t->ppostionPid_t->pid_out = 0;//pid输出
			/* ------ 内环pid参数 ------- */
				gimbal_t.prammer_t->pspeedPid_t->kp = 0.7;
				gimbal_t.prammer_t->pspeedPid_t->kd = 0.07;
				gimbal_t.prammer_t->pspeedPid_t->ki = 0.01;
				gimbal_t.prammer_t->pspeedPid_t->error = 0;
				gimbal_t.prammer_t->pspeedPid_t->last_error = 0;//上次误差
				gimbal_t.prammer_t->pspeedPid_t->before_last_error = 0;//上上次误差
				gimbal_t.prammer_t->pspeedPid_t->integral_er = 0;//误差积分
				gimbal_t.prammer_t->pspeedPid_t->pout = 0;//p输出
				gimbal_t.prammer_t->pspeedPid_t->iout = 0;//i输出
				gimbal_t.prammer_t->pspeedPid_t->dout = 0;//k输出
				gimbal_t.prammer_t->pspeedPid_t->pid_out = 0;//pid输出
        gimbal_t.prammer_t->pspeedPid_t->limiting=RAMMER_LIMIMT_CUT;
		/* -------------- yaw轴电机参数设置 ----------------- */
		gimbal_t.pYaw_t->target = gimbal_t.pYaw_t->real_angle;//设置启动目标值
						/* ------ 外环pid参数 ------- */
				gimbal_t.pYaw_t->ppostionPid_t->kp = -0.5;
				gimbal_t.pYaw_t->ppostionPid_t->kd = -1;
				gimbal_t.pYaw_t->ppostionPid_t->ki = -0.04;
				gimbal_t.pYaw_t->ppostionPid_t->error = 0;
				gimbal_t.pYaw_t->ppostionPid_t->last_error = 0;//上次误差
				gimbal_t.pYaw_t->ppostionPid_t->integral_er = 0;//误差积分
        gimbal_t.pYaw_t->ppostionPid_t->integral_limint = 3000;
				gimbal_t.pYaw_t->ppostionPid_t->pout = 0;//p输出
				gimbal_t.pYaw_t->ppostionPid_t->iout = 0;//i输出
				gimbal_t.pYaw_t->ppostionPid_t->dout = 0;//k输出
				gimbal_t.pYaw_t->ppostionPid_t->pid_out = 0;//pid输出
			/* ------ 内环pid参数 ------- */
				gimbal_t.pYaw_t->pspeedPid_t->kp = 0;
				gimbal_t.pYaw_t->pspeedPid_t->kd = 0;
				gimbal_t.pYaw_t->pspeedPid_t->ki = 0;
				gimbal_t.pYaw_t->pspeedPid_t->error = 0;
				gimbal_t.pYaw_t->pspeedPid_t->last_error = 0;//上次误差
				gimbal_t.pYaw_t->pspeedPid_t->before_last_error = 0;//上上次误差
				gimbal_t.pYaw_t->pspeedPid_t->integral_er = 0;//误差积分
				gimbal_t.pYaw_t->pspeedPid_t->pout = 0;//p输出
				gimbal_t.pYaw_t->pspeedPid_t->iout = 0;//i输出
				gimbal_t.pYaw_t->pspeedPid_t->dout = 0;//k输出
				gimbal_t.pYaw_t->pspeedPid_t->pid_out = 0;//pid输出
        gimbal_t.pYaw_t->pspeedPid_t->limiting=YAW_LIMIMT_CUT;
		/* -------------- pitch轴 ----------------- */
				gimbal_t.pPitch_t->target = gimbal_t.pPitch_t->real_angle;//设置启动目标值
						/* ------ 外环pid参数 ------- */
				gimbal_t.pPitch_t->ppostionPid_t->kp = 10;
				gimbal_t.pPitch_t->ppostionPid_t->kd = 10;
				gimbal_t.pPitch_t->ppostionPid_t->ki = 0.1;
				gimbal_t.pPitch_t->ppostionPid_t->error = 0;
				gimbal_t.pPitch_t->ppostionPid_t->last_error = 0;//上次误差
				gimbal_t.pPitch_t->ppostionPid_t->integral_er = 0;//误差积分
				gimbal_t.pPitch_t->ppostionPid_t->pout = 0;//p输出
				gimbal_t.pPitch_t->ppostionPid_t->iout = 0;//i输出
				gimbal_t.pPitch_t->ppostionPid_t->dout = 0;//k输出
				gimbal_t.pPitch_t->ppostionPid_t->pid_out = 0;//pid输出
        gimbal_t.pPitch_t->ppostionPid_t->integral_limint = LINT_LIMINT;//积分限幅
			/* ------ 内环pid参数 ------- */
				gimbal_t.pPitch_t->pspeedPid_t->kp = 9;
				gimbal_t.pPitch_t->pspeedPid_t->kd = 30;
				gimbal_t.pPitch_t->pspeedPid_t->ki = 0.4;
				gimbal_t.pPitch_t->pspeedPid_t->error = 0;
				gimbal_t.pPitch_t->pspeedPid_t->last_error = 0;//上次误差
				gimbal_t.pPitch_t->pspeedPid_t->before_last_error = 0;//上上次误差
				gimbal_t.pPitch_t->pspeedPid_t->integral_er = 0;//误差积分
				gimbal_t.pPitch_t->pspeedPid_t->pout = 0;//p输出
				gimbal_t.pPitch_t->pspeedPid_t->iout = 0;//i输出
				gimbal_t.pPitch_t->pspeedPid_t->dout = 0;//k输出
				gimbal_t.pPitch_t->pspeedPid_t->pid_out = 0;//pid输出
        gimbal_t.pPitch_t->pspeedPid_t->limiting=PITCH_LIMIMT_CUT;
     	/* ------ 设置启动标志位 ------- */  
        SET_BIT(gimbal_t.status,START_OK);  
	 }
  /**
  * @Data    2019-03-19 23:28
  * @brief   拨弹PID控制
  * @param   void
  * @retval  void
  */
  int16_t RammerPidControl(int16_t rammer)
  {
    int16_t temp_pid_out;
    /* -------- 外环 --------- */
    gimbal_t.prammer_t->ppostionPid_t->error = GIMBAL_CAL_ERROR(rammer,gimbal_t.prammer_t->real_angle);
    gimbal_t.prammer_t->ppostionPid_t->pid_out = PostionPid(gimbal_t.prammer_t->ppostionPid_t,gimbal_t.prammer_t->ppostionPid_t->error);
    gimbal_t.prammer_t->ppostionPid_t->pid_out = MAX(gimbal_t.prammer_t->ppostionPid_t->pid_out,xianfuweizhi); //限做大值
    gimbal_t.prammer_t->ppostionPid_t->pid_out = MIN(gimbal_t.prammer_t->ppostionPid_t->pid_out,-xianfuweizhi); //限做小值
    /* -------- 内环 --------- */
    gimbal_t.prammer_t->pspeedPid_t->error = GIMBAL_CAL_ERROR(gimbal_t.prammer_t->ppostionPid_t->pid_out,gimbal_t.prammer_t->real_speed);
		gimbal_t.prammer_t->pspeedPid_t->pid_out = SpeedPid(gimbal_t.prammer_t->pspeedPid_t,gimbal_t.prammer_t->pspeedPid_t->error);
  	temp_pid_out = MAX(gimbal_t.prammer_t->pspeedPid_t->pid_out,xianfusudu); //限做大值
    temp_pid_out = MIN(gimbal_t.prammer_t->pspeedPid_t->pid_out,-xianfusudu); //限做小值
    return temp_pid_out;
  }
  /**
  * @Data    2019-03-21 01:39
  * @brief   yaw轴pid控制
  * @param   void
  * @retval  void
  */
  int16_t YawPidControl(int16_t yaw)
  {
    int16_t temp_pid_out;
    gimbal_t.pYaw_t->ppostionPid_t->error = CalculateError((yaw),(gimbal_t.pYaw_t->real_angle),15000,(20480));//YAW_CAL_ERROR(yaw,gimbal_t.pYaw_t->real_angle);
    gimbal_t.pYaw_t->ppostionPid_t->pid_out = PostionPid(gimbal_t.pYaw_t->ppostionPid_t,gimbal_t.pYaw_t->ppostionPid_t->error);
    temp_pid_out = MAX(gimbal_t.pYaw_t->ppostionPid_t->pid_out,yawxianfu);
    temp_pid_out = MIN( gimbal_t.pYaw_t->ppostionPid_t->pid_out,-yawxianfu);
    return temp_pid_out;
  }
  /**
  * @Data    2019-03-21 01:40
  * @brief   pitch轴pid控制
  * @param   void
  * @retval  void
  */
  int16_t PitchPidControl(int16_t pitch)
  {
    int16_t temp_pid_out;
       /* -------- 外环 --------- */
    gimbal_t.pPitch_t->ppostionPid_t->error = CalculateError((pitch),( gimbal_t.pPitch_t->real_angle),5500,(8192));//GIMBAL_CAL_ERROR(pitch,gimbal_t.pPitch_t->real_angle);
    gimbal_t.pPitch_t->ppostionPid_t->pid_out = PostionPid(gimbal_t.pPitch_t->ppostionPid_t,gimbal_t.pPitch_t->ppostionPid_t->error);
    temp_pid_out = MAX(gimbal_t.pPitch_t->ppostionPid_t->pid_out,pitchxianfu);
    temp_pid_out = MIN( gimbal_t.pPitch_t->ppostionPid_t->pid_out,-pitchxianfu);
//       /* -------- 内环 --------- */
//    gimbal_t.pPitch_t->pspeedPid_t->error = GIMBAL_CAL_ERROR(gimbal_t.pPitch_t->ppostionPid_t->pid_out,gimbal_t.pPitch_t->real_speed);
//		gimbal_t.pPitch_t->pspeedPid_t->pid_out = SpeedPid(gimbal_t.pPitch_t->pspeedPid_t,gimbal_t.pPitch_t->pspeedPid_t->error);
//  	temp_pid_out = MAX(gimbal_t.pPitch_t->pspeedPid_t->pid_out,sudupitchxianfu); //限做大值
//    temp_pid_out = MIN(gimbal_t.pPitch_t->pspeedPid_t->pid_out,-sudupitchxianfu); //限做小值
    return temp_pid_out;
  }
  
  //待测试代码
  /**
  * @Data    2019-03-21 01:39
  * @brief   yaw轴pid控制
  * @param   void
  * @retval  void
  */
  int16_t YawPidControl_dai(int16_t yaw_err)
  {
    int16_t temp_pid_out;
    gimbal_t.pYaw_t->ppostionPid_t->pid_out = PostionPid(gimbal_t.pYaw_t->ppostionPid_t,yaw_err);
    temp_pid_out = MAX(gimbal_t.pYaw_t->ppostionPid_t->pid_out,yawxianfu);
    temp_pid_out = MIN( gimbal_t.pYaw_t->ppostionPid_t->pid_out,-yawxianfu);
    return temp_pid_out;
  }
  /**
  * @Data    2019-03-21 01:40
  * @brief   pitch轴pid控制
  * @param   void
  * @retval  void
  */
  int16_t PitchPidControl_dai(int16_t pitch_err)
  {
    int16_t temp_pid_out;
       /* -------- 外环 --------- */
    gimbal_t.pPitch_t->ppostionPid_t->pid_out = PostionPid(gimbal_t.pPitch_t->ppostionPid_t,pitch_err);
    temp_pid_out = MAX(gimbal_t.pPitch_t->ppostionPid_t->pid_out,pitchxianfu);
    temp_pid_out = MIN( gimbal_t.pPitch_t->ppostionPid_t->pid_out,-pitchxianfu);
//       /* -------- 内环 --------- */
//    gimbal_t.pPitch_t->pspeedPid_t->error = GIMBAL_CAL_ERROR(gimbal_t.pPitch_t->ppostionPid_t->pid_out,gimbal_t.pPitch_t->real_speed);
//		gimbal_t.pPitch_t->pspeedPid_t->pid_out = SpeedPid(gimbal_t.pPitch_t->pspeedPid_t,gimbal_t.pPitch_t->pspeedPid_t->error);
//  	temp_pid_out = MAX(gimbal_t.pPitch_t->pspeedPid_t->pid_out,sudupitchxianfu); //限做大值
//    temp_pid_out = MIN(gimbal_t.pPitch_t->pspeedPid_t->pid_out,-sudupitchxianfu); //限做小值
    return temp_pid_out;
  }
  
   //待测试代码
  
  
  
  
/**
	* @Data    2019-03-20 21:27
	* @brief   yaw轴电机初始化
	* @param   void
	* @retval  void
	*/
	RM6623Struct* YawInit(void)
	{
	yaw_t.id = YAW_RX_ID ;//电机can的 ip
	yaw_t.target = 0 ;		 //目标值
	yaw_t.tem_target = 0 ;//临时目标值
	yaw_t.real_current = 0 ; //真实电流
	yaw_t.real_angle = 0 ;//真实角度
	yaw_t.tem_angle = 0 ;//临时角度
	yaw_t.zero = 0 ;			 //电机零点
	yaw_t.Percentage = 0 ;//转换比例（减速前角度:减速后的角度 = x:1
	yaw_t.thresholds = 0 ; //电机反转阀值
  yaw_t.error = 0 ;//当前误差
  yaw_t.last_real = 0 ;
  yaw_t.coefficient = 0 ;
		/* ------ 外环pid地址 ------- */
  yaw_t.ppostionPid_t = &yawOuterLoopPid_t ;
			/* ------ 内环pid地址 ------- */
	yaw_t.pspeedPid_t = &yawInnerLoopPid_t ;

			return &yaw_t;
	}
	/**
	* @Data    2019-03-20 21:27
	* @brief   pitch轴电机初始化
	* @param   void
	* @retval  void
	*/
	GM6020Struct* PitchInit(void)
	{
	pitch_t.id = YAW_RX_ID ;//电机can的 ip
	pitch_t.target = 0 ;		 //目标值
	pitch_t.tem_target = 0 ;//临时目标值
	pitch_t.real_current = 0 ; //真实电流
	pitch_t.real_angle = 0 ;//真实角度
	pitch_t.tem_angle = 0 ;//临时角度
	pitch_t.zero = 0 ;			 //电机零点
	pitch_t.Percentage = 0 ;//转换比例（减速前角度:减速后的角度 = x:1
	pitch_t.thresholds = 0 ; //电机反转阀值
  pitch_t.error = 0 ;//当前误差
  pitch_t.last_real = 0 ;
  pitch_t.coefficient = 0 ;
		/* ------ 外环pid地址 ------- */
  pitch_t.ppostionPid_t = &pitchOuterLoopPid_t ;
			/* ------ 内环pid地址 ------- */
	pitch_t.pspeedPid_t = &pitchInnerLoopPid_t ;
			return &pitch_t;
	}
  
  
  //待测试代码
/**
* @Data    2019-03-20 21:27
* @brief   云台扫描探索模式目标值//待测试
* @param   void
* @retval  void
*/
uint8_t scflag = 0;
void ScanningToExplore(void)
{
  if((gimbal_t.status&SCAN_MODE) != SCAN_MODE)
  {
    //目标值切换，状态切换
    gimbal_t.pitch_scan_target = gimbal_t.pPitch_t->real_angle;
    gimbal_t.yaw_scan_target = gimbal_t.pYaw_t->real_angle;
    //清除pc控制标志位，设置标志位
     CLEAR_BIT(gimbal_t.status,PC_SHOOT_MODE);
     SET_BIT(gimbal_t.status,SCAN_MODE);
  }
    if(gimbal_t.pitch_scan_target < 1000)
    {
      gimbal_t.pitch_scan_target += 1;
    }
    if(gimbal_t.pitch_scan_target > 2500)
    {
       gimbal_t.pitch_scan_target -= 1;
    }
    gimbal_t.yaw_scan_target +=10;
}
/**
* @Data    2019-03-20 21:27
* @brief   云台自瞄pc控制模式目标值//待测试
* @param   void
* @retval  void
*/
void PcControlMode(void)
{
  if((gimbal_t.status&PC_SHOOT_MODE) != PC_SHOOT_MODE)
  {
    //目标值切换，状态切换
     CLEAR_BIT(gimbal_t.status,SCAN_MODE);//清除pc控制标志位
     SET_BIT(gimbal_t.status,PC_SHOOT_MODE);
  }
  
}
/**
* @Data    2019-03-20 21:27
* @brief   控制权切换//待测试
* @param   void
* @retval  void
*/
void ControlSwitch(void)
{
 switch (gimbal_t.pPc_t->commot)
 {
   case 0:
     ScanningToExplore();
     break;
   case 1:
     PcControlMode();
     break;
   default:
	    break;
 }
}
  //待测试代码
/*-----------------------------------file of end------------------------------*/
