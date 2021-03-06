
// VEMCUCtlDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "VEMCUCtl.h"
#include "VEMCUCtlDlg.h"
#include "VMHistoryLog.h"
#include "XmlInfoCallbackHandle.h"
#include "WeatherDataDealWith.h"
#include "RequestWeatherInfo.h"
#include "CameraVideoOperate.h"
#include "SipCall.h"
#include <tlhelp32.h>

//add by wx 20151102
#include "SDDElecMap/DLLInterface.h"
//add by wx 2015.11.16
#include "DlgTreeElecMap.h"
#include "DlgCameraAndControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//对话框
CDlgViewOut    g_DlgVideoView[MAXVIEWCH];
CVideoInfoDlg  g_VideoInfoDlg[MAXVIEWCH];
CVideoTagInfoDlg g_VideoTagInfoDlg[MAXVIEWCH];

//////////////////////////////////////////////////////////////////////////
int g_nLinkageSettingVideoId = -1;
int g_nACSSystemVideoId = -1;

//////////////////////////////////////////////////////////////////////////
//托盘
CSystemTray   g_SystemTray;

//////////////////////////////////////////////////////////////////////////
//版本配置文件数据
char g_FileDataInfo[2*1024] = {0};

//表示当前版本信息,初始化为最初版本
char g_szCurrentVersion[2*1024] = {0};

//程序路径
char g_szAppPath[MAX_PATH] = {0};

//////////////////////////////////////////////////////////////////////////
//通用等待事件
CEvent g_CommonEvent;
//告警订阅事件
CEvent g_AcsSubscribeAlarmEvent;

LogManager gLog;

//////////////////////////////////////////////////////////////////////////
CACSSystemClient *g_pACSClient = NULL;
CGWPlay g_GWPlay;
CVEMCUCtlDlg* g_pMainDlg = NULL;
COMMONTHREADPOOL g_CommnThreadPool;
CTcpServerClient g_TcpServerClient;
VM_VIDEO_LINKAGE_PRIORITY_INFO g_VideoLinkagePriorityInfo;
NETHANDLE g_hAssClientHandle;

//////////////////////////////////////////////////////////////////////////
//客户端视频连接方式,0表示只用南瑞平台,1表示直接访问前端设备,2表示尝试一次后访问设备,3表示尝试两次后访问设备
int  g_nClientVideoLinkType = 0;

//////////////////////////////////////////////////////////////////////////
//视频巡视线程记数
VM_THTEAD_VIDEO_OPERATE_NUMBER_INFO g_ThreadVideoOperateNumberInfo;

//////////////////////////////////////////////////////////////////////////
//当前气象
HANDLE  g_hWeatherCurrentInfoEvent = NULL;
int            g_nWeatherCurrentInfoType = GET_WEATHER_INFO_TYPE_MIN;
time_t      g_WeatherCurrentInfoTime = 0;
int           g_nWeatherCurrentInfoIndex = -1;

//历史气象
HANDLE  g_hWeatherHistoryInfoEvent = NULL;
int           g_nWeatherHistoryInfoType = GET_WEATHER_INFO_TYPE_MIN;
time_t      g_WeatherHistoryInfoTime = 0;
int           g_nWeatherHistoryInfoIndex = -1;

//历史告警
HANDLE  g_hWeatherHistoryWarningInfoEvent = NULL;
int           g_nWeatherHistoryWarningInfoType = GET_WEATHER_INFO_TYPE_MIN;
time_t      g_WeatherHistoryWarningInfoTime = 0;
int           g_nWeatherHistoryWarningInfoIndex = -1;

//台风告警
HANDLE  g_hWeatherTyphoonWarningInfoEvent = NULL;
int           g_nWeatherTyphoonWarningInfoType = GET_WEATHER_INFO_TYPE_MIN;
time_t      g_WeatherTyphoonWarningInfoTime = 0;
int           g_nWeatherTyphoonWarningInfoIndex = -1;

//当前微气象
HANDLE  g_hMicroWeatherCurrentInfoEvent = NULL;
int           g_nMicroWeatherCurrentInfoType = GET_WEATHER_INFO_TYPE_MIN;
time_t      g_MicroWeatherCurrentInfoTime = 0;
int           g_nMicroWeatherCurrentInfoIndex = -1;

//历史微气象
HANDLE  g_hMicroWeatherHistoryInfoEvent = NULL;
int           g_nMicroWeatherHistoryInfoType = GET_WEATHER_INFO_TYPE_MIN;
time_t      g_MicroWeatherHistoryInfoTime = 0;
int           g_nMicroWeatherHistoryInfoIndex = -1;

//////////////////////////////////////////////////////////////////////////
int g_nCurrentWeatherDataWinInfoTypeFlag = 1;//0:表示显示微气象,1:表示显示气象
T_CURRENT_WEATHER_DATA_WIN_INFO g_CurrentWeatherDataWinInfo[MAXVIEWCH];
T_HISTORY_WEATHER_DATA_WIN_INFO g_HistoryWeatherDataWinInfo[MAXVIEWCH];
T_CURRENT_MICRO_WEATHER_DATA_WIN_INFO g_CurrentMicroWeatherDataWinInfo[MAXVIEWCH];

//////////////////////////////////////////////////////////////////////////
int   g_nViewOutVideoTypeFlag = 0;//视频按钮显示类型

//////////////////////////////////////////////////////////////////////////
//用户巡视
int  g_nUserStationViewStationCount = 0;//用户要巡视站点数
int  g_nUserStationViewStationId[MAX_STATION_USER_POWER_NUM];//用户要巡视站点ID号
int  g_nUserStationViewStationVoltageClass[MAX_STATION_USER_POWER_NUM];//用户要巡视站点的电压等级
char  g_szUserStationViewStationNum[MAX_STATION_USER_POWER_NUM][64];//用户要巡视站点号码
char  g_szUserStationViewStationName[MAX_STATION_USER_POWER_NUM][256];//用户要巡视站点名称
int  g_nUserStationViewStationCameraCount[MAX_STATION_USER_POWER_NUM];//用户要巡视站点的摄像头数
int  g_nUserStationViewStationNoCheckCameraCount[MAX_STATION_USER_POWER_NUM];//用户巡视站点的未巡视的摄像头数
char g_szUserStationViewOneStationCameraNum[MAX_STATION_USER_POWER_CAMERA_NUM][64];//临时变量,做摄像头信息同步使用,用户巡视站点的摄像头号码
int  g_nUserStationViewOneStationCameraCount = 0;//临时变量,做摄像头信息同步使用,用户巡视站点的摄像头数量
char g_szCurrentStationCameraNum[MAX_STATION_USER_POWER_CAMERA_NUM][64];//临时变量,做摄像头信息同步使用,当前站点的摄像头号码
int  g_nCurrentStationCameraStatus[MAX_STATION_USER_POWER_CAMERA_NUM];//临时变量,做摄像头信息同步使用,当前站点的摄像头状态
int  g_nCurrentStationCameraCount = 0;//临时变量,做摄像头信息同步使用,当前站点的摄像头数量
int  g_nUserStationViewYwbCount = 0;//用户要巡视运维班数
int  g_nUserStationViewYwbId[MAX_STATION_USER_POWER_YWB_NUM] = {0};//用户要巡视运维班ID号

//////////////////////////////////////////////////////////////////////////
long g_port[MAXVIEWCH] = {0};

unsigned	alarmThreadID;
int   g_NzLoginId = -1;

int   g_nWin_Height = 0;
int   g_nWin_Width = 0;

//////////////////////////////////////////////////////////////////////////
//抓图路径
char   g_PictureSaveFilepath[256] = "C:\\capture";

//////////////////////////////////////////////////////////////////////////
//轮巡查询摄像头状态线程
bool g_bCameraStatusSearchThreadExitFlag = false;
HANDLE g_hCameraStatusSearchThread = NULL;
unsigned int g_uCameraStatusSearchThreadID = 0;

//////////////////////////////////////////////////////////////////////////
//辅助系统客户端回调函数,接收服务端传来的数据
int AssClientCB(NETHANDLE handle,char* buf,int len,void* user)
{
	if (buf!=NULL&&len>0)
	{
		printf("收到服务端传来的数据：%s\n",buf);
		char *buf1="hello server!\n";
		ComSendNet(handle,NULL,0,0,0,0,0,buf1,strlen(buf1),1);
	}

	return len;
}

//云台控制
BOOL SendVideoYTControlInfo(char *szInfo,int nInfoLen,int nFlag)
{
	if (szInfo == NULL||nInfoLen  <= 0)
		return FALSE;

	if (g_userpower.userCP.ytcontrol == 0)//权限判断
		return FALSE;

	g_TcpServerClient.StopNetTcpClient(nFlag);
	g_TcpServerClient.StartNetTcpClient(nFlag);
	g_TcpServerClient.TcpClientSendData(szInfo,nInfoLen,nFlag);

	return TRUE;
}

BOOL IsCameraVideoLinkByVideoPlatform(int VideoLinkType,int nCameraType)
{
	if (VideoLinkType != 1||nCameraType == 100)
		return TRUE;

	return FALSE;
}

int VideoDirectDeviceGetDeviceStateCB(unsigned short dvrid,bool state)
{
	return 0;
}

//获取linkageSetting传回来的视频id
int GetLinkageSettingOpenDVideoIdCB(int id)
{
	g_nLinkageSettingVideoId = id;
	return 0;
}

//获取ACSSystem传回来的视频id
int GetACSSystemOpenDVideoIdCB(int id)
{
	g_nACSSystemVideoId = id;
	return 0;
}

int VideoDirectDeviceGetOpenVideoCB(int id, int nresult)
{
	try
	{
		//如果id是LinkageSetting动态库传回来的，直接return掉，不走下面流程，因为下面流程是给主程序用的
		if (id == g_nLinkageSettingVideoId)
		{
			return 0;
		}

		//如果id是ACSSystem动态库传回来的，直接return掉，不走下面流程，因为下面流程是给主程序用的
		if (id == g_nACSSystemVideoId)
		{
			return 0;
		}

		g_VMLog.WriteVmLog("VideoDirectDeviceGetOpenVideoCB-----id=%d,nresult=%d",id,nresult);

		int i = 0;

		//for (i = 0;i < MAXVIEWCH;i++)
		//{
		//	g_DlgVideoView[i].m_VideoViewOutInfo.Lock();
		//	if (g_DlgVideoView[i].m_VideoViewOutInfo.m_nVideoSdkId == id)
		//	{
		//		g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();
		//		break;
		//	}
		//	g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();
		//}

		//限制在主画面以外的窗口打开摄像头
		if (i >= MAXVIEWCH)
		{
			if (nresult == OPENVIDEO_SUCCESS)
			{
				g_VMLog.WriteVmLog("VideoDirectDeviceGetOpenVideoCB出错,在窗口中没有找到设备id-----id=%d,i=%d",id,i);

				if (g_pMainDlg != NULL)
					g_pMainDlg->ThreadPoolDispatchTask(ThreadStopDirectDeviceVideo,(void *)&id,sizeof(id),4);
			}
			return 0;
		}

		if (i<MAXVIEWCH)
		{
			switch(nresult)
			{
			case OPENVIDEO_SUCCESS:
				{
					g_VMLog.WriteVmLog("VideoDirectDeviceGetOpenVideoCB在窗口%d中打开设备%d成功",i,id);

					g_DlgVideoView[i].m_VideoViewOutInfo.Lock();
					if (g_DlgVideoView[i].m_VideoViewOutInfo.m_nDeviceVideoFlag == 0)
					{
						g_DlgVideoView[i].m_VideoViewOutInfo.m_nDeviceVideoFlag = 1;

						g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();

						if (g_pMainDlg != NULL)
						{
							g_pMainDlg->SendMessage(WM_DEVICE_VIDEO_STATUS_ONLINE_NOTIFY_MESSAGE,i,NULL);
						}
					}
					else
					{
						g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();
					}
				}
				break;

			case OPENVIDEO_LOADDLLERROR:
			case OPENVIDEO_FINDDVRTYPEERROR:
				{
					g_VMLog.WriteVmLog("VideoDirectDeviceGetOpenVideoCB在窗口%d中打开设备%d失败-1",i,id);

					ClearViewPlatformAndDeviceInfoByWinID(i);

					g_DlgVideoView[i].m_strStation.SetText("离线");

					if (g_pMainDlg != NULL)
					{
						g_pMainDlg->SetTimer(TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER+i,2*1000,NULL);
					}
				}
				break;

			case OPENVIDEO_LOGINDVRERROR:
			case OPENVIDEO_PLAYERROR:
				{
					g_VMLog.WriteVmLog("VideoDirectDeviceGetOpenVideoCB在窗口%d中打开设备%d失败-2",i,id);

					ClearViewPlatformAndDeviceInfoByWinID(i);

					g_DlgVideoView[i].m_strStation.SetText("离线");

					if (g_pMainDlg != NULL)
					{
						g_pMainDlg->SetTimer(TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER+i,2*1000,NULL);
					}
				}
				break;
			}

			if (g_DlgVideoView[i].m_hCallResultNotifyEvent != NULL)
			{
				SetEvent(g_DlgVideoView[i].m_hCallResultNotifyEvent);
			}
		}
		

		return 0;
	}
	catch(...)
	{

	}
	return -1;
}

int VideoDirectDeviceCloseVideoCB(int id)
{
	try
	{
		//如果id是LinkageSetting动态库传回来的，直接return掉，不走下面流程，因为下面流程是给主程序用的
		if (id == g_nLinkageSettingVideoId)
		{
			return 0;
		}

		//如果id是ACSSystem动态库传回来的，直接return掉，不走下面流程，因为下面流程是给主程序用的
		if (id == g_nACSSystemVideoId)
		{
			return 0;
		}

		g_VMLog.WriteVmLog("VideoDirectDeviceCloseVideoCB关闭设备%d成功",id);

		bool bFlag = false;

		for (int i = 0;i < MAXVIEWCH;i++)
		{
			g_DlgVideoView[i].m_VideoViewOutInfo.Lock();
			if (g_DlgVideoView[i].m_VideoViewOutInfo.m_nVideoViewType == 2
				&&g_DlgVideoView[i].m_VideoViewOutInfo.m_nVideoSdkId == id)
			{
				bFlag = true;
			}
			else
			{
				bFlag = false;
			}
			g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();

			if (bFlag != false)
			{
				g_VMLog.WriteVmLog("VideoDirectDeviceCloseVideoCB在窗口%d中关闭设备%d成功",i,id);
	
				//清除信息显示窗口
				ClearVideoPresetLineAndTagInfoDlg(i);

				//停止气象信息显示
				StopVideoWinCurrentWeather(i);

				//停止录像
				CloseVideoRecordByWinRecordFlag(i);

				//停止音频
				CloseVideoAudioByWinRecordFlag(i);

				//清除信息,刷新窗口
				g_DlgVideoView[i].CloseViewChannel();
			}
		}
		return 0;
	}
	catch(...)
	{

	}
	return 0;
}

// CVEMCUCtlDlg 对话框

CVEMCUCtlDlg::CVEMCUCtlDlg(CWnd* pParent /*=NULL*/)
: CDialog(CVEMCUCtlDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAIN);

	m_FormsNum = 4;					//初始画面的个数
	m_pDlgMenu = NULL;
	g_pMainDlg = NULL;
	m_pDlgPageServer = NULL;
	g_pDlgReplay = NULL;
	m_pDlgHistoryLog = NULL;
	m_pDlgSelectDevice = NULL;
	m_pDlgShowControl = NULL;
	m_pDlgShowPageServer = NULL;
	m_pDlgLinkageServer = NULL;
	m_pDlgShowLinkageServer = NULL;
	m_pDlgPageViewInfo = NULL;
	m_pDlgStationSelect = NULL;
	m_pDlgAlarmSubscribe = NULL;
	m_pDlgAlarmPreview = NULL;
	m_pDlgColumn1 = NULL;
	m_pDlgColumn2 = NULL;

	m_iFocuseWindowID = -1;
	m_bFullScreen = FALSE;
	m_nFullScreen = 0;
	m_bMultiFullScreen = FALSE;
	m_bLinkageServerFlag = FALSE;
	m_bMenuFlag = TRUE;
	m_bPageServerFlag = TRUE;
	m_bPageViewInfoFlag = FALSE;
	m_iPreviewNum = 9;
	g_ua_realtalk_direction = E_RT_DIRECTION_OUT;
	g_ua_realtalk_state = E_RT_CALL_STATE_TERMINATED;
	m_bShowTempFavorite = false;
	m_bShowRealTalk = false;
	m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_MULTIVIEW;
	g_hWeatherCurrentInfoEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	g_hWeatherHistoryInfoEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	g_hWeatherHistoryWarningInfoEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	g_hWeatherTyphoonWarningInfoEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	g_hMicroWeatherCurrentInfoEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	g_hMicroWeatherHistoryInfoEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	
	memset(g_CurrentWeatherDataWinInfo,0,sizeof(g_CurrentWeatherDataWinInfo));
	memset(g_CurrentMicroWeatherDataWinInfo,0,sizeof(g_CurrentMicroWeatherDataWinInfo));

	memset(&g_user_config_info,0,sizeof(g_user_config_info));

	memset(&g_userpower,0,sizeof(g_userpower));
	g_userpower.userID = -1;

	m_nYtCommand = 0;
	memset(m_szYtCommandName,0,sizeof(m_szYtCommandName));

	m_hVideoExcelReportPipe = NULL;
	m_bVideoExcelReportPipeInitFlag = false;

	m_hVideoExcelRecordReportPipe = NULL;
	m_bVideoExcelRecordReportPipeInitFlag = false;

	for (int i = 0;i < MAXVIEWCH;i++)
	{
		g_CurrentWeatherDataWinInfo[i].nIndex = i;
		g_CurrentMicroWeatherDataWinInfo[i].nIndex = i;
		g_HistoryWeatherDataWinInfo[i].nIndex = i;
	}
}

CVEMCUCtlDlg::~CVEMCUCtlDlg()
{
	if (g_hWeatherCurrentInfoEvent != NULL)
	{
		CloseHandle(g_hWeatherCurrentInfoEvent);
		g_hWeatherCurrentInfoEvent = NULL;
	}

	if (g_hWeatherHistoryInfoEvent != NULL)
	{
		CloseHandle(g_hWeatherHistoryInfoEvent);
		g_hWeatherHistoryInfoEvent = NULL;
	}

	if (g_hWeatherHistoryWarningInfoEvent != NULL)
	{
		CloseHandle(g_hWeatherHistoryWarningInfoEvent);
		g_hWeatherHistoryWarningInfoEvent = NULL;
	}

	if (g_hWeatherTyphoonWarningInfoEvent != NULL)
	{
		CloseHandle(g_hWeatherTyphoonWarningInfoEvent);
		g_hWeatherTyphoonWarningInfoEvent = NULL;
	}

	if (g_hMicroWeatherCurrentInfoEvent != NULL)
	{
		CloseHandle(g_hMicroWeatherCurrentInfoEvent);
		g_hMicroWeatherCurrentInfoEvent = NULL;
	}

	if (g_hMicroWeatherHistoryInfoEvent != NULL)
	{
		CloseHandle(g_hMicroWeatherHistoryInfoEvent);
		g_hMicroWeatherHistoryInfoEvent = NULL;
	}
}

void CVEMCUCtlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVEMCUCtlDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_CLOSE()
	ON_MESSAGE(OM_CONTROLBUTTON, OnControlButton)
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_COMMAND(ID_MENUITEMEXITVIDEO, OnMenuItemExitVideo)
	ON_COMMAND(ID_MENU_STOP_ALL_VIDEO,OnMenuStopAllVideo)
	ON_COMMAND(ID_MENUITEMSTARTREC, OnMenuitemStartRec)
	ON_COMMAND(ID_MENUITEMSTOPREC, OnMenuitemStopRec)
	ON_COMMAND(ID_MENUITEMCAPTURE, OnMenuitemCapture)
	ON_COMMAND(ID_MENUITEM_MULTIFULLS, OnMenuitemMultifullScreen)
	ON_COMMAND(ID_MENUITEM_EXITMULTIFULLS, OnMenuitemExitmultifullScreen)
	ON_MESSAGE(OM_CHANNLECHANGE, &CVEMCUCtlDlg::OnChannelChange)
	ON_MESSAGE(OM_DBLCHANNEL, &CVEMCUCtlDlg::OnDBLChannel)
	ON_MESSAGE(OM_RIGHTCLICKCHANNEL, &CVEMCUCtlDlg::OnRightClickChannel)
	ON_MESSAGE(OM_CONTROLBUTTON, OnControlButton)
	ON_MESSAGE(OM_YTCONTROL, StartYtControl)
	ON_MESSAGE(WM_INIT_USB_VIDEO, &CVEMCUCtlDlg::OnInitUsbVideoMSGHandler)
	ON_MESSAGE(WM_EXIT_USB_VIDEO, &CVEMCUCtlDlg::OnExitUsbVideoMSGHandler)
	ON_MESSAGE(WM_SUB_OK, &CVEMCUCtlDlg::OnSubOK)
	ON_MESSAGE(WM_SUB_FAIL, &CVEMCUCtlDlg::OnSubFail)
	ON_MESSAGE(WM_VIDEO_LINKAGE_MANUAL_MESSAGE, &CVEMCUCtlDlg::OnVideoLinkageManualMessageHandler)
	ON_MESSAGE(WM_VIDEO_LINKAGE_ALARM_MESSAGE, &CVEMCUCtlDlg::OnVideoLinkageAlarmMessageHandler)
	ON_MESSAGE(WM_VIDEO_LINKAGE_STATE_MESSAGE, &CVEMCUCtlDlg::OnVideoLinkageStateMessageHandler)
	ON_MESSAGE(WM_VIDEO_CAMERA_CONTROL_NOTIFY_MESSAGE, &CVEMCUCtlDlg::OnVideoCameraControlNotifyMessageHandler)
	ON_MESSAGE(WM_VIDEO_WEATHER_WARNING_LINKAGE_MESSAGE, &CVEMCUCtlDlg::OnVideoLinkageWeatherWarningMessageHandler)
	ON_MESSAGE(WM_VIDEO_WEATHER_FORECAST_LINKAGE_MESSAGE, &CVEMCUCtlDlg::OnVideoLinkageWeatherForecastMessageHandler)
	ON_MESSAGE(WM_DEVICE_VIDEO_ERROR_REASON_NOTIFY_MESSAGE, &CVEMCUCtlDlg::OnDeviceVideoErrorReasonNotifyMessageHandler)
	ON_MESSAGE(WM_DEVICE_VIDEO_TALK_NOTIFY_MESSAGE, &CVEMCUCtlDlg::OnDeviceVideoTalkNotifyMessageHandler)
	ON_MESSAGE(WM_DEVICE_VIDEO_CLOSE_MESSAGE, &CVEMCUCtlDlg::OnDeviceVideoCloseMessageHandler)
	ON_MESSAGE(WM_DEVICE_VIDEO_QUICK_RESTART_MESSAGE, &CVEMCUCtlDlg::OnDeviceVideoQuickRestartMessageHandler)
	ON_MESSAGE(WM_DEVICE_VIDEO_STATUS_ONLINE_NOTIFY_MESSAGE, &CVEMCUCtlDlg::OnDeviceVideoStatusOnlineNotifyMessageHandler)
	ON_MESSAGE(WM_UPDATE_USER_VIEW_CAMERA_STATUS_MESSAGE, &CVEMCUCtlDlg::OnUpdateUserViewCameraStatusMessageHandler)
	ON_MESSAGE(WM_ELECMAP_OPEN_CAMERA_MESSAGE, &CVEMCUCtlDlg::OnElecMapOpenCameraMessageHandler)
	ON_COMMAND(ID_MENU_ITEM_SHOW, &CVEMCUCtlDlg::OnMenuItemShow)
	ON_COMMAND(ID_MENU_ITEM_EXIT, &CVEMCUCtlDlg::OnMenuItemExit)
	ON_WM_SYSCOMMAND()
	ON_COMMAND(ID_MENUITEM_TAGCAMERA, &CVEMCUCtlDlg::OnMenuitemTagCamera)
	ON_COMMAND(ID_MENUITEM_CANCELCAMERA, &CVEMCUCtlDlg::OnMenuitemCancelcamera)
	ON_WM_HOTKEY()
END_MESSAGE_MAP()


// CVEMCUCtlDlg 消息处理程序

void CVEMCUCtlDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CVEMCUCtlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CVEMCUCtlDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//////////////////////////////////////////////////////////////////////////

	try
	{
		g_pMainDlg = this;

		SetWindowTextA("热点区域监视");

		m_iFocuseWindowID = 0;

		m_nControlWidth = CONTROLWIDTH;
		m_nLinkageWidth = 0;
		m_nMenuHight = MENUHIGHT;
		m_nPageViewInfoHeight = PAGEVIEWHEIGHT;
		m_nAlarmHeight = ALARMHIGHT;

		LoadSkin();

		//初始化
		DialogInit();

		//注册热键
		::RegisterHotKey(m_hWnd,VM_HOT_KEY_PRIOR_PAGE,MOD_ALT,VK_PRIOR); 
		::RegisterHotKey(m_hWnd,VM_HOT_KEY_NEXT_PAGE,MOD_ALT,VK_NEXT); 
		::RegisterHotKey(m_hWnd,VM_HOT_KEY_EXPORT_REPORT,MOD_ALT,VK_HOME); 
		
		return true;
	}
	catch(...)
	{

	}

	MessageBox("程序初始化异常，程序将退出!","视频监视");
	
	exit(-3);

	return false;
}

BOOL CVEMCUCtlDlg::DialogInit()
{
	//日志库初始化
	gLog.createLog("VEMCUCtl.log");
	DEBUG_LOG("日志初始化成功！");

	//得到程序所在目录
	GetProgramAppPath();

	//得到程序的版本信息
	GetProgramAppVersionInfo();

	// 初始化SIP参数
	AppInitSipChannel();

	// 读配置文件
	if (!ReadIniFile())
	{
		MessageBox("配置文件错误，程序将退出！","视频监视");
		exit(-3);
	}

	//初始化解码器
	Init_DevSdk();

	//设置回调
	SetCallbackGetDevState(VideoDirectDeviceGetDeviceStateCB);
	SetCallbackOpenVideo(VideoDirectDeviceGetOpenVideoCB);
	SetCallbackCloseVideo(VideoDirectDeviceCloseVideoCB);

	//国网视频解码
	InitGWSDK();

	// 连接数据库
	if (!ConnectMySql())
	{
		MessageBox("连接数据库失败，程序将退出！","视频监视");
		exit(-3);
	}

	//给video_camera表增加一个anfang_flag字段
	if (!AddAnFangFlagToVideoCamera())
	{
		MessageBox("数据库初始化失败，程序将退出！","视频监视");
		exit(-3);
	}

	//检测并更新系统软件
	CheckAndUpdateSystemSoftware();

	//登录对话框
	CDlgLogin DlgLogin;

	// 登陆系统，判断权限。。。。。
	if (DlgLogin.DoModal()==IDOK)
	{
		if (g_userpower.userID < 0)
		{
			exit(-3);
			return false;
		}
	}
	else
	{
		exit(-3);
		return false;
	}

#if VM_SJ_CLIENT_VERSION
	{
		if (g_userpower.logintype != 1)
		{
			MessageBox("本版本为检修版本，其它类型用户请从服务器\"172.17.33.140\"下载相应版本。","变电站视频监控系统",MB_OK);
		}
	}
#else
	{
		if (g_userpower.logintype == 1)
		{
			MessageBox("本版本为调度版本，检修用户请从服务器\"172.17.33.140\"下载检修版本。","热点视频监视",MB_OK);
		}
	}
#endif

	//记录用户名
	char szPathName[256] = {0};
	memset(szPathName, 0, sizeof(szPathName));
	
	if (strlen(g_szAppPath) > 0)
	{
		strcpy_s(szPathName,sizeof(szPathName)-1,g_szAppPath);
	}
	else
	{
		GetCurrentDirectory(sizeof(szPathName)-1, szPathName);
	}

	strcat_s(szPathName, "\\VEMCUCtl.ini");
	WritePrivateProfileString("USER", "UserName", g_userpower.username,szPathName);

	//创建用户巡视权限表
	CreateUserStationViewTable();

	//获取用户巡视权限
	GetUserStationViewPower2();

	//巡检报表
	VideoExcelReportPipeInit();
	//CheckAndOpenVideoExcelReport();

	//录像报表
	VideoExcelRecordReportPipeInit();
	//CheckAndOpenVideoExcelRecordReport();

	//用户配置信息
	strcpy_s(g_user_config_info.szUserName,sizeof(g_user_config_info.szUserName),g_userpower.username);
	strcpy_s(g_user_config_info.szUserPassword,sizeof(g_user_config_info.szUserPassword),g_userpower.password);
	g_user_config_info.nUserId = g_userpower.userID;

	//客户端联动日志
	g_VMHistoryLog.SetVMHistoryLogInfo(g_userpower.username,g_DBServerIP,g_nDBServerPort,g_DBName,"mdcs","mdcs2008");
	g_VMHistoryLog.ConnectMySql();

	//客户端视频错误记录
	g_VideoErrorInfo.SetVideoErrorInfo(g_user_config_info.szLocalUdpIp,g_DBServerIP,g_nDBServerPort,g_DBName,"mdcs","mdcs2008");
	g_VideoErrorInfo.StartVideoErrorInfo();

	//客户端视频操作记录
	g_ClientVideoInfo.SetClientVideoInfo(g_user_config_info.szLocalUdpIp,g_DBServerIP,g_nDBServerPort,g_DBName,"mdcs","mdcs2008");
	g_ClientVideoInfo.StartClientVideoInfo();

	//摄像头状态更新
	g_CameraStatusInfo.SetCameraStatusInfo(g_user_config_info.szLocalUdpIp,g_DBServerIP,g_nDBServerPort,g_DBName,"mdcs","mdcs2008");
	g_CameraStatusInfo.StartCameraStatusInfo();

	// 初始化栈引擎
	int nRet = AppStartSipStack();
	if (nRet < 0)
	{
		MessageBox("可能原因:\n\n1.程序上一次运行没有退出\n2. 配置文件本地UDP/TCP地址错误\n3. 配置文件RTP端口号错误","视频监视");
		exit(-3);// 初始化失败退出
	}

	//设置系统托盘
	if(g_SystemTray.Create(NULL, WM_SYSTEM_TRAY_ICON_NOTIFY,_T("热点监控"),m_hIcon,IDR_MENU_SYSTEM_TRAY))
	{
		g_SystemTray.SetMenuDefaultItem(0,true);
		g_SystemTray.SetNotificationWnd(&g_SystemTray);
	}

	// 界面部分结构调整
	SetFullScreen();

	//加载窗口各个组成部分
	InitScreen();

	//语音对讲初始化
	g_ua_realtalk_callid = -1;

	//创建抓图目录为c://capture
	CreateDirectory("C:\\capture",NULL);

	//秒定时器
	SetTimer(TIMER_MAIN_EVENTID, 500, NULL);

	//气象刷新
	SetTimer(TIMER_WEATHER_EVENTID,2*1000,NULL);

	//云台锁定
	SetTimer(TIMER_YT_LOCK_CHECK_EVENTID,30*1000,NULL);

	//流量检测
	SetTimer(TIMER_VIDEO_FLOW_EVENTID,1000,NULL);

	//订阅摄像头状态
	SetTimer(TIMER_SUBSCRIBE_CAMERA_STATUS_EVENTID,30*1000,NULL);

	//检测并更新软件
	SetTimer(TIMER_CHECK_AND_UPDATE_SYSTEM_SOFTWARE_EVENTID,24*60*60*1000,NULL);

	m_iFocuseWindowID = 0;

	//启动线程池
	InitThreadPool();

	//启动TCP服务
	InitTcpServerClient();

	//启动气象
	InitVemAllWeatherInfo();
	

#if VM_SERVER_VERSION
	{
		StartSearchCameraPresenceInfo();//开始查询摄像头状态
	}
#endif
	///////////////管理dll的一些设置///////////////////////////////////////
	SystemLinkageSetting_SetAPI((void *)g_pCallAPI,(void *)g_pTransactionAPI,(void*)&g_user_config_info);

	///////////////////辅助系统相关//////////////////////////////////////////////////
	SetTimer(TIMER_ACSSYSTEM_LOAD_DLL,2000,NULL);

	return TRUE;
}

//辅助设备监控dll的配置
void CVEMCUCtlDlg::ACSSystemConfig()
{
	//将数据库和登录用户的信息传给dll
	ACSSystem_SetAPI((void*)&g_user_config_info);
	//设置回调函数取video的id，目的使dll调用devsdk打开视频时，不使用主程序中的回调
	ACSSystem_SetCallbackGetOpenVideoId(GetACSSystemOpenDVideoIdCB);
	ACSSystem_ShowWindow(m_hWnd,FALSE);
}

//辅助联动配置管理dll的配置
void CVEMCUCtlDlg::LinkageSettingConfig()
{
	//将数据库和登录用户的信息传给dll
	LinkageSetting_SetAPI((void*)&g_user_config_info);
	//设置回调函数取video的id，目的使dll调用devsdk打开视频时，不使用主程序中的回调
	LinkageSetting_SetCallbackGetOpenVideoId(GetLinkageSettingOpenDVideoIdCB);
	LinkageSetting_ShowWindow(m_hWnd,FALSE);
}

BOOL CVEMCUCtlDlg::AddAnFangFlagToVideoCamera()
{
	char sql_buf[1024]={0x0};
	MYSQL_RES *res;
	//判断是否已经添加该字段
	int rnum = 0;
	sprintf_s(sql_buf,"SELECT COLUMN_NAME from information_schema.`COLUMNS` where TABLE_NAME='video_camera' and COLUMN_NAME='anfang_flag'");
	if (!mysql_query(g_mySqlData, sql_buf))
	{
		res = mysql_store_result(g_mySqlData);
		rnum = mysql_num_rows(res);
		mysql_free_result( res ) ;
	}
	else
	{
		//数据库查询失败
		return FALSE;
	}

	if (rnum > 0)
	{
		//添加过，直接返回true
		return TRUE;
	}
	else
	{
		//没添加过，则先添加
		sprintf_s(sql_buf,"alter table video_camera add anfang_flag int(8) default 0 NOT NULL");
		//失败返回false;
		if (mysql_query(g_mySqlData, sql_buf))
		{
			return FALSE;
		}
	}

	return TRUE;
}

// 处理窗口系统消息
BOOL CVEMCUCtlDlg::PreTranslateMessage(MSG* pMsg)
{
	ACSSystem_PreTranslateMessage(pMsg);
	SystemLinkageSetting_PreTranslateMessage(pMsg);
	LinkageSetting_PreTranslateMessage(pMsg);
	ElecMap_TranslateMessage(pMsg);

	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_ESCAPE)//屏住ESC键
		return	TRUE;
	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_RETURN)//屏住ENTER键
		return	TRUE; 
	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_SPACE)//屏蔽空格键
		return	TRUE;

	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_PRIOR)//PgUp
	{
		if (m_pDlgPageViewInfo != NULL&&m_pDlgPageViewInfo->IsWindowVisible() != FALSE)
		{
			m_pDlgPageViewInfo->OnBnClickedBtnPrePage();
			return	TRUE;
		}
	}

	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_NEXT)//PgDn
	{
		if (m_pDlgPageViewInfo != NULL&&m_pDlgPageViewInfo->IsWindowVisible() != FALSE)
		{
			m_pDlgPageViewInfo->OnBnClickedBtnNextPage();
			return	TRUE; 
		}
	}

	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_HOME)//HOME
	{
		if (m_pDlgPageViewInfo != NULL&&m_pDlgPageViewInfo->IsWindowVisible() != FALSE)
		{
			m_pDlgPageViewInfo->OnBnClickedBtnExportReport();
			return	TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

//自动得到本端的IP地址
bool CVEMCUCtlDlg::GetLocalIpAddress(char *szLocalIpAddress)
{
	WORD wVersionRequested;
	WSADATA  wasData;
	int nResult;
	char szHostName[256] = {0};
	HOSTENT * pHost = NULL;

	try
	{
		wVersionRequested = MAKEWORD(2,2);
		nResult = WSAStartup(wVersionRequested,&wasData);
		if (nResult != 0)
			return false;

		memset(szHostName,0,sizeof(szHostName));

		nResult = gethostname(szHostName,sizeof(szHostName));
		if (nResult == SOCKET_ERROR)
			return false;

		pHost = gethostbyname(szHostName);
		if (pHost == NULL)
			return false;

		int nCount = 0;
		IN_ADDR* pInAddr = NULL;

		while(1)
		{
			if(pHost->h_addr_list[nCount] == 0)
				return false;

			pInAddr = (IN_ADDR*)pHost->h_addr_list[nCount];
			
			//if (pInAddr->S_un.S_un_b.s_b1 == 172||pInAddr->S_un.S_un_b.s_b1 == 192||pInAddr->S_un.S_un_b.s_b1 == 10)
			//{
			//	strcpy_s(szLocalIpAddress,32,inet_ntoa(*pInAddr));
			//	return true;
			//}

			//if (TestLinkServerIpAddress(pInAddr) != false)
			//{
			//	strcpy_s(szLocalIpAddress,32,inet_ntoa(*pInAddr));
			//	return true;
			//}
			nCount++;
		}

		return false;
	}
	catch(...)
	{

	}
	return false;
}

bool CVEMCUCtlDlg::TestLinkServerIpAddress(IN_ADDR* pInAddr)
{
	try
	{
		int nResult = -1;

		SOCKET  TestSocket = ::socket(AF_INET,SOCK_STREAM,0);
		if (TestSocket == INVALID_SOCKET)
			return false;

		struct sockaddr_in local_client_addr;
		struct sockaddr_in tcp_server_addr;

		memset(&local_client_addr,0,sizeof(local_client_addr));
		local_client_addr.sin_family = AF_INET;
		local_client_addr.sin_addr.s_addr = pInAddr->S_un.S_addr;
		local_client_addr.sin_port = 0;

		memset(&tcp_server_addr,0,sizeof(tcp_server_addr));
		tcp_server_addr.sin_family = AF_INET;
		tcp_server_addr.sin_addr.s_addr = inet_addr(g_LinkServerIP);
		tcp_server_addr.sin_port = htons(g_nLinkServerPort);

		nResult = bind(TestSocket,(sockaddr*)&local_client_addr,sizeof(sockaddr));
		if(nResult == SOCKET_ERROR)
		{
			closesocket(TestSocket);
			return false;
		}

		unsigned long ul = 1;
		nResult = ioctlsocket(TestSocket, FIONBIO, (unsigned long*)&ul);
		if(nResult == SOCKET_ERROR)
		{
			closesocket(TestSocket);
			return false;
		}

		nResult = ::connect(TestSocket,(sockaddr*)&tcp_server_addr,sizeof(sockaddr));

		//select 模型，即设置超时
		struct timeval timeout ;
		
		fd_set socket_set;
		FD_ZERO(&socket_set);
		FD_SET(TestSocket, &socket_set);
		
		timeout.tv_sec = 6; //连接超时6秒
		timeout.tv_usec =0;
	
		nResult = ::select(0, 0,&socket_set, 0, &timeout);
		if (nResult <= 0)
		{
			closesocket(TestSocket);
			return false;
		}
		else
		{
			closesocket(TestSocket);
			return true;
		}
	}
	catch(...)
	{

	}
	return false;
}

// 读取配置文件
bool CVEMCUCtlDlg::ReadIniFile(void)
{
	DWORD num = 0;
	char szPathName[MAX_PATH] = {0};
	memset(szPathName, 0, sizeof(szPathName));

	if (strlen(g_szAppPath) > 0)
	{
		strcpy_s(szPathName,sizeof(szPathName)-1,g_szAppPath);
	}
	else
	{
		GetCurrentDirectory(sizeof(szPathName)-1, szPathName);
	}
	
	strcat_s(szPathName, "\\VEMCUCtl.ini");

	//获取联动服务器的IP地址
	num = GetPrivateProfileString("LINKSERVER", "LinkServerIP", "192.168.1.13", g_LinkServerIP, IPADDRESS_LEN, szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：联动服务器的IP地址，错误！","视频监视");
		return false;
	}

	//获取联动服务器的端口
	g_nLinkServerPort= GetPrivateProfileInt("LINKSERVER","LinkServerPort",83,szPathName);
	if (g_nLinkServerPort <= 0)
	{
		MessageBox("获取配置文件：联动服务器端口，错误！","视频监视");
		return false;
	}

	// 获取本机UDP的IP地址
	num = GetPrivateProfileString("SipStack","LocalUdpAddress","192.168.1.13",app_StackCfg.szLocalUdpAddress,20,szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：本地UDP的IP地址，错误！","视频监视");
		return false;
	}

	char szLocalIpAddress[32] = {0};
	memset(szLocalIpAddress,0,sizeof(szLocalIpAddress));

	if (memcmp(app_StackCfg.szLocalUdpAddress,"127.0.0.1",9) == 0)
	{
		if(GetLocalIpAddress(szLocalIpAddress) != false)
			strcpy_s(app_StackCfg.szLocalUdpAddress,sizeof(app_StackCfg.szLocalUdpAddress),szLocalIpAddress);
	}

	// 获取本机UDP端口
	num = app_StackCfg.nLocalUdpPort = GetPrivateProfileInt("SipStack","LocalUdpPort",5082,szPathName);
	if (num < 0)
	{
		MessageBox("获取配置文件：本地UDP的端口，错误！","视频监视");
		return false;
	}

	// 是否使能TCP
	app_StackCfg.bTcpEnabled = false;

	// 获取本机的TCP地址
	sprintf(app_StackCfg.szLocalTcpAddress, "%s", app_StackCfg.szLocalUdpAddress);

	// 获取本机的TCP端口
	app_StackCfg.nLocalTcpPort = app_StackCfg.nLocalUdpPort;

	// 获取日志文件的名称
	sprintf(app_StackCfg.szLogBaseName, "%s", "c:\\VEMCUCtl");

	// 获取日志文件等级
	app_StackCfg.nLogLevel = RT_SIP_LOG_INFO|RT_SIP_LOG_WARN|RT_SIP_LOG_ERROR;

	// 获取最大注册数
	app_StackCfg.nMaxRegister = 10;

	// 获取最大呼叫数
	app_StackCfg.nMaxCall = 200;

	// 获取最大事务数
	app_StackCfg.nMaxTransactions = 1000;

	// 是否使用RTP模式的呼叫
	app_StackCfg.bUseJRtpCall = 1;

	// RTP端口范围最小值
	num = app_StackCfg.nMinRtpPort = GetPrivateProfileInt("SipStack","MinRtpPort",10000,szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：RTP端口范围最小值，错误！","视频监视");
		return false;
	}

	// RTP端口范围最大值
	num = app_StackCfg.nMaxRtpPort = GetPrivateProfileInt("SipStack","MaxRtpPort",20000,szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：RTP端口范围最大值，错误！","视频监视");
		return false;
	}

	// 取默认值
	app_StackCfg.nNatHoleMilliSecond = 1500;


	//获取cms的IP地址
	num = GetPrivateProfileString("SIPSERVER","SipServerIP","192.168.1.233",app_RegCh.reg_server,20,szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：SipServerIP，错误！","视频监视");
		return false;
	}

	app_StackCfg.szDnsServerList;
	app_StackCfg.szObdProxyAddress;

	//获取cms的端口
	app_RegCh.reg_serverport = GetPrivateProfileInt("SIPSERVER","SipServerPort",5060,szPathName);
	if (app_RegCh.reg_serverport <= 0)
	{
		MessageBox("获取配置文件：SipServerPort，错误！","视频监视");
		return false;
	}

	app_StackCfg.nObdProxyPort = app_RegCh.reg_serverport;

	//获取注册刷新时间
	app_RegCh.reg_expires = 3600;

	//是否使用快速rtp模式，默认为不使用
	app_StackCfg.bUseFastRtpProcess = 0;
	app_StackCfg.nDivBorder = 8000;
	app_StackCfg.nDivPacketMode = 1;

	sprintf(app_RegCh.reg_user, "%s", "100010000004000624");

	//获取HTTP服务器的IP地址
	num = GetPrivateProfileString("HTTPSERVER", "HttpServerIP", "192.168.1.13", g_HttpServerIP, IPADDRESS_LEN, szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：HTTP服务器的IP地址，错误！","视频监视");
		return false;
	}

	//获取HTTP服务器的端口
	g_nHttpServerPort= GetPrivateProfileInt("HTTPSERVER","HttpServerPort",83,szPathName);
	if (g_nHttpServerPort <= 0)
	{
		MessageBox("获取配置文件：HTTP服务器端口，错误！","视频监视");
		return false;
	}

	//获取用户名
	num = GetPrivateProfileString("USER", "UserName", "admin", g_userpower.username, sizeof(g_userpower.username), szPathName);
	if (num <= 0)
	{
		memset(g_userpower.username,0,sizeof(g_userpower.username));
	}

	//获取数据库的IP地址
	num = GetPrivateProfileString("DB", "DBServerIP", "192.168.1.13", g_DBServerIP, IPADDRESS_LEN, szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：数据库的IP地址，错误！","视频监视");
		return false;
	}

	
	//获取数据库端口
	g_nDBServerPort= GetPrivateProfileInt("DB","DBServerPort",3306,szPathName);
	if (g_nDBServerPort <= 0)
	{
		MessageBox("获取配置文件：数据库端口，错误！","视频监视");
		return false;
	}

	num = GetPrivateProfileString("DB", "DBName", "mdcs-d5000", g_DBName, 256, szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：数据库的名称，错误！","视频监视");
		return false;
	}

	//管理端预置位是否保存在前端DVR中
	g_user_config_info.nPreSetSaveEnable = GetPrivateProfileInt("PRESET","PreSetSaveEnalbe",0,szPathName);
	if (g_user_config_info.nPreSetSaveEnable < 0)
	{
		MessageBox("获取配置文件：预置位保存设置，错误！","视频监视");
		return false;
	}

	//获取客户端视频连接方式
	g_nClientVideoLinkType= GetPrivateProfileInt("CLIENTLINKTYPE","ClientVideoLinkType",0,szPathName);
	if (g_nClientVideoLinkType < 0)
	{
		MessageBox("获取配置文件：客户端视频连接方式，错误！","视频监视");
		return false;
	}

	//获取人工自动巡视间隔
	memset(&g_VideoPageViewInfo,0,sizeof(g_VideoPageViewInfo));
	g_VideoPageViewInfo.nTimeSpan= GetPrivateProfileInt("PAGEVIEWINFO","PageViewTimeSpan",10,szPathName);
	if (g_VideoPageViewInfo.nTimeSpan < 0)
	{
		MessageBox("获取配置文件：获取人工自动巡视间隔，错误！","视频监视");
		return false;
	}

	//获取人工巡视上一次站点
	num = GetPrivateProfileString("PAGEVIEWINFO","PageViewPreStation","500kV艾塘变", g_VideoPageViewInfo.szStationName, sizeof(g_VideoPageViewInfo.szStationName), szPathName);
	if (num < 0)
	{
		MessageBox("获取配置文件：获取人工自动巡视间隔，错误！","视频监视");
		return false;
	}

	//获取辅助系统服务器的IP地址
	num = GetPrivateProfileString("ACSSERVERINFO", "AcsServerIP", "127.0.0.1", g_AcsServerIp, IPADDRESS_LEN, szPathName);
	if (num <= 0)
	{
		MessageBox("获取配置文件：辅助系统服务器的IP地址，错误！","视频监视");
		return false;
	}

	//获取辅助系统端口
	g_nAcsServerPort= GetPrivateProfileInt("ACSSERVERINFO","AcsServerPort",0,szPathName);
	if (g_nAcsServerPort < 0)
	{
		MessageBox("获取配置文件：辅助系统服务器的端口，错误！","视频监视");
		return false;
	}

	//获取是否显示管理客户端
	g_nShowManagerClient = GetPrivateProfileInt("SHOWMANAGERCLIENT","ShowManagerClient",0,szPathName);
	if (g_nShowManagerClient < 0)
	{
		MessageBox("获取配置文件：辅助系统服务器的端口，错误！","视频监视");
		return false;
	}


	strcpy_s(g_user_config_info.szLocalUdpIp,sizeof(g_user_config_info.szLocalUdpIp),app_StackCfg.szLocalUdpAddress);
	g_user_config_info.nLocalUdpPort = app_StackCfg.nLocalUdpPort;
	g_user_config_info.nMinRtpPort = app_StackCfg.nMinRtpPort;
	g_user_config_info.nMaxRtpPort = app_StackCfg.nMaxRtpPort;
	strcpy_s(g_user_config_info.szSipServerIp,sizeof(g_user_config_info.szSipServerIp),app_RegCh.reg_server);
	g_user_config_info.nSipServerPort = app_RegCh.reg_serverport;
	strcpy_s(g_user_config_info.szHttpServerIp,sizeof(g_user_config_info.szHttpServerIp),g_HttpServerIP);
	g_user_config_info.nHttpServerPort = g_nHttpServerPort;
	strcpy_s(g_user_config_info.szDBServerIp,sizeof(g_user_config_info.szDBServerIp),g_DBServerIP);
	g_user_config_info.nDBServerPort = g_nDBServerPort;
	strcpy_s(g_user_config_info.szDBName,sizeof(g_user_config_info.szDBName),g_DBName);
	strcpy_s(g_user_config_info.szLinkServerIp,sizeof(g_user_config_info.szLinkServerIp),g_LinkServerIP);
	g_user_config_info.nLlinkServerPort = g_nLinkServerPort;
	strcpy_s(g_user_config_info.szAcsServerIp,sizeof(g_user_config_info.szAcsServerIp),g_AcsServerIp);
	g_user_config_info.nAcsServerPort = g_nAcsServerPort;

	return true;
}

//设置日志等级为整型
int CVEMCUCtlDlg::SetFilterToInt(char* strValue)
{
	char* token;
	int result = 0,temp = 0;

	token = strtok(strValue, " ,\t");
	while (token)
	{
		temp = atoi(token);
		if (temp == 1)
			result += 2;
		else if (temp == 2)
			result += 4;
		else if (temp == 3)
			result += 8;
		else if (temp == 4)
			result += 16;
		token = strtok(NULL, " ,\t");
	}

	return result;
}

// 连接数据库
bool CVEMCUCtlDlg::ConnectMySql(void)
{
	int times = 0;

	for (times = 0; times < 3; times ++)
	{
		if ((g_mySqlData = mysql_init((MYSQL*)0)) && mysql_real_connect(g_mySqlData, g_DBServerIP, "mdcs", "mdcs2008", g_DBName, g_nDBServerPort, NULL, times))
		{
			if (mysql_select_db(g_mySqlData, g_DBName) < 0)
			{
				TRACE("已成功连接mysql, 但选择数据库出错!\n");
				mysql_close(g_mySqlData);
				g_mySqlData = NULL;
				
				if (times >= 2)
					return false;
				else
					continue;
			}

			int nValue = 1;
			mysql_options(g_mySqlData,MYSQL_OPT_RECONNECT,(char *)&nValue);

			mysql_query(g_mySqlData,"SET NAMES 'GBK'");
			TRACE("已成功连接mysql!\n");

			return true;
		}
		else
		{
			mysql_close(g_mySqlData);
			g_mySqlData = NULL;

			TRACE("连接mysql失败!");

			if (times >= 2)
				return false;
			else
				continue;
		}
	}
	return true;
}

// 断开数据库
bool CVEMCUCtlDlg::DisConnectMySql(void)
{
	mysql_close(g_mySqlData);
	g_mySqlData = NULL;
	return true;
}

//创建用户巡视权限表
bool CVEMCUCtlDlg::CreateUserStationViewTable()
{

	if (g_mySqlData == NULL)
	{
		if (ConnectMySql() == false)
			return false;
	}

	char sql_buf[1024] = {0};
	int    nResult = 0;
	bool bResult = false;

	sprintf_s(sql_buf,"CREATE TABLE IF NOT EXISTS ct_user_station_view ("
		"id int(10) NOT NULL auto_increment,"
		"user_id int(10) NOT NULL default '0',"
		"station_id int(10) NOT NULL default '0',"
		"status int(10) NOT NULL default '1',"
		"PRIMARY KEY  (id))");

	nResult = mysql_query(g_mySqlData, sql_buf);

	bResult = ((nResult == 0)?true:false);

	return bResult;
}

bool CVEMCUCtlDlg::GetUserStationViewPower()
{
	g_nUserStationViewStationCount = 0;
	memset(g_nUserStationViewStationId,0,sizeof(g_nUserStationViewStationId));
	memset(g_szUserStationViewStationNum,0,sizeof(g_szUserStationViewStationNum));
	memset(g_szUserStationViewStationName,0,sizeof(g_szUserStationViewStationName));
	memset(g_nUserStationViewStationCameraCount,0,sizeof(g_nUserStationViewStationCameraCount));
	memset(g_nUserStationViewStationNoCheckCameraCount,0,sizeof(g_nUserStationViewStationNoCheckCameraCount));

	if (g_mySqlData == NULL)
	{
		if (ConnectMySql() == false) 
			return false;
	}

	MYSQL_RES * res = NULL ;
	MYSQL_ROW	row ;

	char buf[1024] = {0};
	g_nUserStationViewStationCount = 0;
	memset(g_nUserStationViewStationId,0,sizeof(g_nUserStationViewStationId));
	memset(g_szUserStationViewStationNum,0,sizeof(g_szUserStationViewStationNum));
	memset(g_szUserStationViewStationName,0,sizeof(g_szUserStationViewStationName));

	//////////////////////////////////////////////////////////////////////////
	sprintf_s(buf,sizeof(buf),"SELECT t1.station_id,t2.station_name_videoplant,t2.station_code_videoplant,t2.voltage_class FROM ct_user_station_view AS t1,ob_d5000_station AS t2 WHERE user_id=%d AND t1.station_id=t2.station_id AND t2.voltage_class=500 ORDER BY t2.order_num ASC",g_userpower.userID);

	int nResult = mysql_query(g_mySqlData,buf);
	if (nResult != 0)
		return FALSE;

	res = mysql_store_result(g_mySqlData);
	while(row = mysql_fetch_row( res ))
	{
		if (g_nUserStationViewStationCount >= MAX_STATION_USER_POWER_NUM)
			break;

		g_nUserStationViewStationId[g_nUserStationViewStationCount] = atoi(row[0]);
		strcpy_s(g_szUserStationViewStationName[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationName[g_nUserStationViewStationCount]),row[1]);
		strcpy_s(g_szUserStationViewStationNum[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationNum[g_nUserStationViewStationCount]),row[2]);
		g_nUserStationViewStationVoltageClass[g_nUserStationViewStationCount] = atoi(row[3]);
		g_nUserStationViewStationCount++;
	}
	mysql_free_result(res) ;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	sprintf_s(buf,sizeof(buf),"SELECT t1.station_id,t2.station_name_videoplant,t2.station_code_videoplant,t2.voltage_class FROM ct_user_station_view AS t1,ob_d5000_station AS t2 WHERE user_id=%d AND t1.station_id=t2.station_id AND t2.voltage_class=220 ORDER BY t2.order_num ASC",g_userpower.userID);

	nResult = mysql_query(g_mySqlData,buf);
	if (nResult != 0)
		return FALSE;

	res = mysql_store_result(g_mySqlData);
	while(row = mysql_fetch_row( res ))
	{
		if (g_nUserStationViewStationCount >= MAX_STATION_USER_POWER_NUM)
			break;

		g_nUserStationViewStationId[g_nUserStationViewStationCount] = atoi(row[0]);
		strcpy_s(g_szUserStationViewStationName[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationName[g_nUserStationViewStationCount]),row[1]);
		strcpy_s(g_szUserStationViewStationNum[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationNum[g_nUserStationViewStationCount]),row[2]);
		g_nUserStationViewStationVoltageClass[g_nUserStationViewStationCount] = atoi(row[3]);
		g_nUserStationViewStationCount++;
	}
	mysql_free_result(res) ;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	sprintf_s(buf,sizeof(buf),"SELECT t1.station_id,t2.station_name_videoplant,t2.station_code_videoplant,t2.voltage_class FROM ct_user_station_view AS t1,ob_d5000_station AS t2 WHERE user_id=%d AND t1.station_id=t2.station_id AND t2.voltage_class!=500 AND t2.voltage_class!=220 ORDER BY t2.order_num ASC",g_userpower.userID);

	nResult = mysql_query(g_mySqlData,buf);
	if (nResult != 0)
		return FALSE;

	res = mysql_store_result(g_mySqlData);
	while(row = mysql_fetch_row( res ))
	{
		if (g_nUserStationViewStationCount >= MAX_STATION_USER_POWER_NUM)
			break;

		g_nUserStationViewStationId[g_nUserStationViewStationCount] = atoi(row[0]);
		strcpy_s(g_szUserStationViewStationName[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationName[g_nUserStationViewStationCount]),row[1]);
		strcpy_s(g_szUserStationViewStationNum[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationNum[g_nUserStationViewStationCount]),row[2]);
		g_nUserStationViewStationVoltageClass[g_nUserStationViewStationCount] = atoi(row[3]);
		g_nUserStationViewStationCount++;
	}
	mysql_free_result(res) ;
	//////////////////////////////////////////////////////////////////////////

	char seps[32] = "-_. ";
	char szUserStationViewStationName[256] = {0};
	int    nUserStationViewStationNameLen = 0;
	char *pToken = NULL;
	
	for (int i = 0;i < g_nUserStationViewStationCount;i++)
	{
		pToken = strtok(g_szUserStationViewStationName[i],seps);
		
		if (pToken == NULL)
			continue;

		memset(szUserStationViewStationName,0,sizeof(szUserStationViewStationName));
		nUserStationViewStationNameLen = 0;

		while(pToken != NULL)
		{
			nUserStationViewStationNameLen = strlen(szUserStationViewStationName);
			sprintf(szUserStationViewStationName+nUserStationViewStationNameLen,"%s",pToken);
			pToken = strtok(NULL,seps);
		}

		strcpy_s(g_szUserStationViewStationName[i],sizeof(g_szUserStationViewStationName[i]),szUserStationViewStationName);
	}

	return true;
}

BOOL CVEMCUCtlDlg::GetUserStationViewPower2()
{
	g_nUserStationViewStationCount = 0;
	memset(g_nUserStationViewStationId,0,sizeof(g_nUserStationViewStationId));
	memset(g_szUserStationViewStationNum,0,sizeof(g_szUserStationViewStationNum));
	memset(g_szUserStationViewStationName,0,sizeof(g_szUserStationViewStationName));
	memset(g_nUserStationViewStationCameraCount,0,sizeof(g_nUserStationViewStationCameraCount));
	memset(g_nUserStationViewStationNoCheckCameraCount,0,sizeof(g_nUserStationViewStationNoCheckCameraCount));

	if (g_mySqlData == NULL)
	{
		if (ConnectMySql() == false) 
			return false;
	}

	int i = 0;
	int nResult = 0;
	MYSQL_RES * res = NULL ;
	MYSQL_ROW	row ;

	char buf[1024] = {0};
	g_nUserStationViewStationCount = 0;
	memset(g_nUserStationViewStationId,0,sizeof(g_nUserStationViewStationId));
	memset(g_szUserStationViewStationNum,0,sizeof(g_szUserStationViewStationNum));
	memset(g_szUserStationViewStationName,0,sizeof(g_szUserStationViewStationName));

	g_nUserStationViewYwbCount = 0;
	memset(g_nUserStationViewYwbId,0,sizeof(g_nUserStationViewYwbId));


	//调度省调、检修省检、管理员
	if (g_userpower.subgroupno == 0||g_userpower.logintype == 2)
	{
		sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station ORDER BY order_num ASC");

		nResult = mysql_query(g_mySqlData,buf);
		if (nResult != 0)
			return FALSE;

		res = mysql_store_result(g_mySqlData);
		while(row = mysql_fetch_row( res ))
		{
			if (g_nUserStationViewStationCount >= MAX_STATION_USER_POWER_NUM)
				break;

			g_nUserStationViewStationId[g_nUserStationViewStationCount] = atoi(row[0]);
			strcpy_s(g_szUserStationViewStationName[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationName[g_nUserStationViewStationCount]),row[1]);
			strcpy_s(g_szUserStationViewStationNum[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationNum[g_nUserStationViewStationCount]),row[2]);
			g_nUserStationViewStationVoltageClass[g_nUserStationViewStationCount] = atoi(row[3]);
			g_nUserStationViewStationCount++;
		}
		mysql_free_result(res) ;
		//////////////////////////////////////////////////////////////////////////
	}
	else
	{
		if (g_userpower.logintype == 0)//调度
		{
			sprintf_s(buf,sizeof(buf),"SELECT sd_ywb_id FROM ct_user_ywb_relation WHERE user_id=%d ORDER BY sd_ywb_id ASC",g_userpower.userID);
		}
		else if (g_userpower.logintype == 1)//检修
		{
			sprintf_s(buf,sizeof(buf),"SELECT ywb_id FROM ct_user_ywb_relation WHERE user_id=%d ORDER BY ywb_id ASC",g_userpower.userID);
		}
		else
		{
			return FALSE;
		}

		nResult = mysql_query(g_mySqlData,buf);
		if (nResult != 0)
			return FALSE;

		res = mysql_store_result(g_mySqlData);
		while(row = mysql_fetch_row( res ))
		{
			if (g_nUserStationViewYwbCount >= MAX_STATION_USER_POWER_YWB_NUM)
				break;

			g_nUserStationViewYwbId[g_nUserStationViewYwbCount] = atoi(row[0]);
			g_nUserStationViewYwbCount++;
		}
		mysql_free_result(res) ;


		//////////////////////////////////////////////////////////////////////////

		for (i = 0;i < g_nUserStationViewYwbCount;i++)
		{
			if (g_userpower.logintype == 0)//调度
			{
				sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station WHERE node_sd_yw_id=%d AND voltage_class=500 ORDER BY order_num ASC",g_nUserStationViewYwbId[i]);
			}
			else if (g_userpower.logintype == 1)//检修
			{
				sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station WHERE node_yw_id=%d AND voltage_class=500 ORDER BY order_num ASC",g_nUserStationViewYwbId[i]);
			}
			else
			{
				return FALSE;
			}

			int nResult = mysql_query(g_mySqlData,buf);
			if (nResult != 0)
				return FALSE;

			res = mysql_store_result(g_mySqlData);
			while(row = mysql_fetch_row( res ))
			{
				if (g_nUserStationViewStationCount >= MAX_STATION_USER_POWER_NUM)
					break;

				g_nUserStationViewStationId[g_nUserStationViewStationCount] = atoi(row[0]);
				strcpy_s(g_szUserStationViewStationName[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationName[g_nUserStationViewStationCount]),row[1]);
				strcpy_s(g_szUserStationViewStationNum[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationNum[g_nUserStationViewStationCount]),row[2]);
				g_nUserStationViewStationVoltageClass[g_nUserStationViewStationCount] = atoi(row[3]);
				g_nUserStationViewStationCount++;
			}
			mysql_free_result(res) ;
		}

		//////////////////////////////////////////////////////////////////////////
		for (i = 0;i < g_nUserStationViewYwbCount;i++)
		{
			if (g_userpower.logintype == 0)//调度
			{
				sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station WHERE node_sd_yw_id=%d AND voltage_class=220 ORDER BY order_num ASC",g_nUserStationViewYwbId[i]);
			}
			else if (g_userpower.logintype == 1)//检修
			{
				sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station WHERE node_yw_id=%d AND voltage_class=220 ORDER BY order_num ASC",g_nUserStationViewYwbId[i]);
			}
			else
			{
				return FALSE;
			}

			nResult = mysql_query(g_mySqlData,buf);
			if (nResult != 0)
				return FALSE;

			res = mysql_store_result(g_mySqlData);
			while(row = mysql_fetch_row( res ))
			{
				if (g_nUserStationViewStationCount >= MAX_STATION_USER_POWER_NUM)
					break;

				g_nUserStationViewStationId[g_nUserStationViewStationCount] = atoi(row[0]);
				strcpy_s(g_szUserStationViewStationName[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationName[g_nUserStationViewStationCount]),row[1]);
				strcpy_s(g_szUserStationViewStationNum[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationNum[g_nUserStationViewStationCount]),row[2]);
				g_nUserStationViewStationVoltageClass[g_nUserStationViewStationCount] = atoi(row[3]);
				g_nUserStationViewStationCount++;
			}
			mysql_free_result(res) ;
			//////////////////////////////////////////////////////////////////////////
		}

		for (i = 0;i < g_nUserStationViewYwbCount;i++)
		{
			//////////////////////////////////////////////////////////////////////////
			if (g_userpower.logintype == 0)//调度
			{
				sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station WHERE node_sd_yw_id=%d AND voltage_class=110 ORDER BY order_num ASC",g_nUserStationViewYwbId[i]);
			}
			else if (g_userpower.logintype == 1)//检修
			{
				sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station WHERE node_yw_id=%d AND voltage_class=110 ORDER BY order_num ASC",g_nUserStationViewYwbId[i]);
			}
			else
			{
				return FALSE;
			}

			nResult = mysql_query(g_mySqlData,buf);
			if (nResult != 0)
				return FALSE;

			res = mysql_store_result(g_mySqlData);
			while(row = mysql_fetch_row( res ))
			{
				if (g_nUserStationViewStationCount >= MAX_STATION_USER_POWER_NUM)
					break;

				g_nUserStationViewStationId[g_nUserStationViewStationCount] = atoi(row[0]);
				strcpy_s(g_szUserStationViewStationName[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationName[g_nUserStationViewStationCount]),row[1]);
				strcpy_s(g_szUserStationViewStationNum[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationNum[g_nUserStationViewStationCount]),row[2]);
				g_nUserStationViewStationVoltageClass[g_nUserStationViewStationCount] = atoi(row[3]);
				g_nUserStationViewStationCount++;
			}
			mysql_free_result(res) ;
			//////////////////////////////////////////////////////////////////////////
		}

		for (i = 0;i < g_nUserStationViewYwbCount;i++)
		{
			//////////////////////////////////////////////////////////////////////////
			if (g_userpower.logintype == 0)//调度
			{
				sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station WHERE node_sd_yw_id=%d AND voltage_class=35 ORDER BY order_num ASC",g_nUserStationViewYwbId[i]);
			}
			else if (g_userpower.logintype == 1)//检修
			{
				sprintf_s(buf,sizeof(buf),"SELECT station_id,station_name_videoplant,station_code_videoplant,voltage_class FROM ob_d5000_station WHERE node_yw_id=%d AND voltage_class=35 ORDER BY order_num ASC",g_nUserStationViewYwbId[i]);
			}
			else
			{
				return FALSE;
			}

			nResult = mysql_query(g_mySqlData,buf);
			if (nResult != 0)
				return FALSE;

			res = mysql_store_result(g_mySqlData);
			while(row = mysql_fetch_row( res ))
			{
				if (g_nUserStationViewStationCount >= MAX_STATION_USER_POWER_NUM)
					break;

				g_nUserStationViewStationId[g_nUserStationViewStationCount] = atoi(row[0]);
				strcpy_s(g_szUserStationViewStationName[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationName[g_nUserStationViewStationCount]),row[1]);
				strcpy_s(g_szUserStationViewStationNum[g_nUserStationViewStationCount],sizeof(g_szUserStationViewStationNum[g_nUserStationViewStationCount]),row[2]);
				g_nUserStationViewStationVoltageClass[g_nUserStationViewStationCount] = atoi(row[3]);
				g_nUserStationViewStationCount++;
			}
			mysql_free_result(res) ;
			//////////////////////////////////////////////////////////////////////////
		}
	}

	char seps[32] = "-_. ";
	char szUserStationViewStationName[256] = {0};
	int    nUserStationViewStationNameLen = 0;
	char *pToken = NULL;

	for (int i = 0;i < g_nUserStationViewStationCount;i++)
	{
		pToken = strtok(g_szUserStationViewStationName[i],seps);

		if (pToken == NULL)
			continue;

		memset(szUserStationViewStationName,0,sizeof(szUserStationViewStationName));
		nUserStationViewStationNameLen = 0;

		while(pToken != NULL)
		{
			nUserStationViewStationNameLen = strlen(szUserStationViewStationName);
			sprintf(szUserStationViewStationName+nUserStationViewStationNameLen,"%s",pToken);
			pToken = strtok(NULL,seps);
		}

		strcpy_s(g_szUserStationViewStationName[i],sizeof(g_szUserStationViewStationName[i]),szUserStationViewStationName);
	}

	return TRUE;
}

bool CVEMCUCtlDlg::CheckUserYwbViewPower2(int nYwbId)
{
	for (int i = 0;i < g_nUserStationViewYwbCount;i++)
	{
		if (g_nUserStationViewYwbId[i] == nYwbId)
			return true;
	}
	return false;
}

bool CVEMCUCtlDlg::CheckUserStationViewPower(HTREEITEM hTreeItem,int nType)
{
	bool bResult = false;

	if (hTreeItem == NULL||(nType != 1&&nType != 2))
		return false;

	if (m_pDlgPageServer == NULL)
		return false;

	char szStationNum[64] = {0};
	HTREEITEM hParentTreeItem = NULL;

	_T_NODE_INFO *pNodeInfo = NULL;
	_T_NODE_PRESET_INFO *pNodePresetInfo = NULL;

	if (nType == 1)//人工巡视
	{
		hParentTreeItem = m_pDlgPageServer->m_trServer.GetParentItem(hTreeItem);
		while (hParentTreeItem != NULL)
		{
			pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hParentTreeItem);
			if (pNodeInfo != NULL&&pNodeInfo->node_type == 2)
			{
				strcpy_s(szStationNum,pNodeInfo->node_num);
				break;
			}
			hParentTreeItem = m_pDlgPageServer->m_trServer.GetParentItem(hParentTreeItem);
		}
	}
	else if (nType == 2)//设备组
	{
		hParentTreeItem = m_pDlgPageServer->m_trPreset.GetParentItem(hTreeItem);
		while (hParentTreeItem != NULL)
		{
			pNodePresetInfo = (_T_NODE_PRESET_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hParentTreeItem);
			if (pNodePresetInfo != NULL&&pNodePresetInfo->node_type == 2)
			{
				strcpy_s(szStationNum,pNodePresetInfo->node_num);
				break;
			}
			hParentTreeItem = m_pDlgPageServer->m_trPreset.GetParentItem(hParentTreeItem);
		}
	}

	if (strlen(szStationNum) == 0)
		return false;

	for (int i = 0;i < g_nUserStationViewStationCount;i++)
	{
		if (strcmp(g_szUserStationViewStationNum[i],szStationNum) == 0)
		{
			bResult = true;
			break;
		}
	}

	return bResult;
}

// 设置窗口全屏
void CVEMCUCtlDlg::SetFullScreen(void)
{
	RECT	rc;
	HWND hDesktopWnd = ::GetDesktopWindow();
	::GetClientRect(hDesktopWnd, &rc);

	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	wndpl.flags = 0;
	wndpl.showCmd = SW_SHOWNORMAL;
	wndpl.rcNormalPosition = rc;
	SetWindowPlacement(&wndpl);
}

// 加载窗口各个组成部分
void CVEMCUCtlDlg::InitScreen(void)
{
	try
	{
		int i = 0;
		CRect	winrc;
		GetClientRect(winrc);

		CRect  Menu_rc;
		CRect Page_rc;
		CRect Control_rc;
		CRect  Linkage_rc;
		CRect  PageViewInfo_rc;
		CRect ShowAlarm_rc;
		CRect AlarmInfo_rc;
		CRect CameraAndControlRc;

		// 菜单窗口
		Menu_rc.top = winrc.top;
		Menu_rc.bottom = MENUHIGHT;
		Menu_rc.left = winrc.left;
		Menu_rc.right = winrc.right;

		m_pDlgMenu = new CDlgMenu();
		if (m_pDlgMenu != NULL)
		{
			m_pDlgMenu->Create(IDD_DIALOG_MENU, this);

			m_pDlgMenu->MoveWindow(Menu_rc);
			m_pDlgMenu->ShowWindow(SW_SHOW);

			char szUserInfo[256] = {0};
			sprintf_s(szUserInfo,sizeof(szUserInfo),"当前用户:\r\n   %s",g_userpower.username);
			m_pDlgMenu->m_staticUserInfo.SetText(szUserInfo,6,RGB(255,255,255));

			char szVersionInfo[256] = {0};
			sprintf_s(szVersionInfo,"版本: %s",g_szCurrentVersion);
			m_pDlgMenu->m_staticVersionInfo.SetText(szVersionInfo,6,RGB(255,255,255));
		}

		/******************************两个列****************************************/
		m_pDlgColumn1 = new CDlgColumn1();
		if (m_pDlgColumn1 != NULL)
		{
			m_pDlgColumn1->Create(IDD_DIALOG_COLUMN1, this);
			m_pDlgColumn1->MoveWindow(AlarmInfo_rc);
			m_pDlgColumn1->ShowWindow(SW_HIDE);
		}
		m_pDlgColumn2 = new CDlgColumn2();
		if (m_pDlgColumn2 != NULL)
		{
			m_pDlgColumn2->Create(IDD_DIALOG_COLUMN2, this);
			m_pDlgColumn2->MoveWindow(AlarmInfo_rc);
			m_pDlgColumn2->ShowWindow(SW_HIDE);
		}

		/******************************告警展示区域**********************************/
		AlarmInfo_rc.top = winrc.bottom - m_nAlarmHeight;
		AlarmInfo_rc.bottom = winrc.bottom;
		AlarmInfo_rc.left = winrc.left + m_nControlWidth;
		AlarmInfo_rc.right = winrc.right - m_nLinkageWidth;

		m_pDlgAlarmInfo =  new CDlgAlarmInfo();
		if (m_pDlgAlarmInfo != NULL)
		{
			m_pDlgAlarmInfo->Create(IDD_DIALOG_ALARM_INFO, this);
			m_pDlgAlarmInfo->MoveWindow(AlarmInfo_rc);
			m_pDlgAlarmInfo->ShowWindow(SW_SHOW);
		}

		/******************************视频区域**********************************/

		//这个for循环会导致界面加载时间很长
		//多画面区域
		for (i = 0; i < MAXVIEWCH; i ++)
		{
			g_DlgVideoView[i].m_bShowCapture = true;
			g_DlgVideoView[i].m_bShowRecord = true;
			g_DlgVideoView[i].m_bShowVoice = true;
			g_DlgVideoView[i].Create(IDD_DIALOG_VIEWOUT, this);
			g_DlgVideoView[i].m_iWinID = i;
			g_DlgVideoView[i].m_Pic.m_iWinID = i;
		}

		//设置视频信息
		RECT rect_video;
		for (i = 0;i < MAXVIEWCH;i++)
		{
			g_DlgVideoView[i].m_Pic.GetWindowRect(&rect_video);
			g_VideoInfoDlg[i].SetFontInfo(24,18,1000);
			g_VideoInfoDlg[i].SetBrushColor(RGB(255,0,255));
			g_VideoInfoDlg[i].Create(IDD_VIDEO_INFO_DIALOG,this);
			g_VideoInfoDlg[i].MoveWindow(&rect_video);
			g_VideoInfoDlg[i].ShowWindow(SW_HIDE);

			//////////////////////////////////////////////////////////////////////////
			g_VideoTagInfoDlg[i].SetFontInfo(40,30,1000);
			g_VideoTagInfoDlg[i].SetBrushColor(RGB(255,10,10));
			g_VideoTagInfoDlg[i].Create(IDD_VIDEO_TAG_INFO_DIALOG,this);
			g_VideoTagInfoDlg[i].MoveWindow(&rect_video);
			g_VideoTagInfoDlg[i].ShowWindow(SW_HIDE);
			g_VideoTagInfoDlg[i].SetVideoInfo("通信中断",0);
			g_VideoTagInfoDlg[i].SetVideoInfo("正常",1);
			g_VideoTagInfoDlg[i].SetVideoInfo("已标注缺陷",2);
		}

		//默认画面数
		SetForms(g_nPageViewOnePageMaxCameraNum,TRUE);

		// 左侧控制窗口
		Page_rc.top = Menu_rc.bottom;
		Page_rc.bottom = winrc.bottom;
		Page_rc.left = winrc.left;
		Page_rc.right = Page_rc.left + m_nControlWidth;

		m_pDlgPageServer = new CDlgPageServer();
		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->Create(IDD_DIALOG_PAGESERVER, this);
			m_pDlgPageServer->MoveWindow(Page_rc);
			m_pDlgPageServer->ShowWindow(SW_SHOW);
		}

		//右侧控制窗口-联动窗口
		Linkage_rc.top = Menu_rc.bottom;
		Linkage_rc.bottom = winrc.bottom;
		Linkage_rc.left = winrc.right - m_nLinkageWidth;
		Linkage_rc.right = winrc.right;

		m_pDlgLinkageServer = new CDlgLinkageServer();
		if (m_pDlgLinkageServer != NULL)
		{
			m_pDlgLinkageServer->Create(IDD_DIALOG_LINKAGE_SERVER,this);
			m_pDlgLinkageServer->MoveWindow(Linkage_rc);
			m_pDlgLinkageServer->ShowWindow(SW_HIDE);
		}

		//******************************手动轮巡导航*****************************/
		PageViewInfo_rc.top = Menu_rc.bottom;
		PageViewInfo_rc.bottom = PageViewInfo_rc.top + PAGEVIEWHEIGHT;
		PageViewInfo_rc.left = winrc.left + m_nControlWidth;
		PageViewInfo_rc.right = winrc.right - m_nLinkageWidth;

		m_pDlgPageViewInfo= new CDlgPageViewInfo();
		if (m_pDlgPageViewInfo != NULL)
		{
			m_pDlgPageViewInfo->Create(IDD_DIALOG_PAGE_VIEW_INFO, this);

			m_pDlgPageViewInfo->MoveWindow(PageViewInfo_rc);
			m_pDlgPageViewInfo->SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			m_pDlgPageViewInfo->ShowWindow(SW_SHOW);

			m_pDlgPageViewInfo->SetPageViewInfo(g_user_config_info.szLocalUdpIp,g_DBServerIP,g_nDBServerPort,g_DBName,"mdcs","mdcs2008");
			m_pDlgPageViewInfo->ConnectMySql();
		}

		//******************************显示控制*****************************/
		Control_rc.top = winrc.top;
		Control_rc.left = (winrc.left+winrc.right)/2-40;
		Control_rc.right = Control_rc.left + 90;
		Control_rc.bottom = winrc.top + 20;

		m_pDlgShowControl = new CDlgShowControl();
		if (m_pDlgShowControl != NULL)
		{
			m_pDlgShowControl->Create(IDD_DIALOG_SHOW_CONTROL,this);
			m_pDlgShowControl->MoveWindow(Control_rc);
			m_pDlgShowControl->ShowWindow(SW_HIDE);
		}

		//******************************显示隐藏告警窗口按钮*****************************/
		ShowAlarm_rc.top = winrc.bottom - SHOWALARMHIGHT;
		ShowAlarm_rc.left = (winrc.left+winrc.right)/2-40;
		ShowAlarm_rc.right = Control_rc.left + 90;
		ShowAlarm_rc.bottom = winrc.bottom;

		m_pDlgShowAlarm = new CDlgShowAlarm();
		if (m_pDlgShowAlarm != NULL)
		{
			m_pDlgShowAlarm->Create(IDD_DIALOG_SHOW_ALARM,this);
			m_pDlgShowAlarm->MoveWindow(ShowAlarm_rc);
			m_pDlgShowAlarm->ShowWindow(SW_HIDE);
		}

		//******************************显示云台、列表树*****************************/
		Control_rc.top = (winrc.top+winrc.bottom)/2-40;
		Control_rc.left = winrc.left;
		Control_rc.right = Control_rc.left + 14;
		Control_rc.bottom = Control_rc.top + 90;

		m_pDlgShowPageServer = new CDlgShowPageServer();
		if (m_pDlgShowPageServer != NULL)
		{
			m_pDlgShowPageServer->Create(IDD_DIALOG_SHOW_PAGE_SERVER,this);
			m_pDlgShowPageServer->MoveWindow(Control_rc);
			m_pDlgShowPageServer->ShowWindow(SW_HIDE);
		}

		//******************************显示联动*****************************/
		Linkage_rc.left = winrc.right - 8;
		Linkage_rc.top = (winrc.top+winrc.bottom)/2-40;
		Linkage_rc.right = winrc.right;
		Linkage_rc.bottom = Linkage_rc.top + 90;

		m_pDlgShowLinkageServer = new CDlgShowLinkageServer();
		if (m_pDlgShowLinkageServer != NULL)
		{
			m_pDlgShowLinkageServer->Create(IDD_DIALOG_SHOW_LINKAGE_SERVER,this);
			m_pDlgShowLinkageServer->MoveWindow(Linkage_rc);

#if VM_SJ_CLIENT_VERSION
			{
				m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);
			}
#else
			{
				m_pDlgShowLinkageServer->ShowWindow(SW_SHOW);
			}
#endif
		}

		//路径对话框
		m_pDlgRecFilePath = new CDlgRecFilePath();
		m_pDlgPicFilePath = new CDlgPicFilePath();

		//add by wx 2015.11.16 for elecmap tree dialog
		m_pDlgTreeElecMap = new CDlgTreeElecMap;
		if (m_pDlgTreeElecMap != NULL)
		{
			m_pDlgTreeElecMap->Create(IDD_DIALOG_TREE_ELECMAP, this);
			m_pDlgTreeElecMap->MoveWindow(Page_rc);
			m_pDlgTreeElecMap->ShowWindow(SW_HIDE);
		}

		//add for elec map : initial dialog.
		ElecMapWindow_Show(m_hWnd,FALSE);

		//add by wp 2015.12.3 for camera and control dialog
		CameraAndControlRc.top = Menu_rc.bottom;
		CameraAndControlRc.bottom = winrc.bottom;
		CameraAndControlRc.left = winrc.right - 500;
		CameraAndControlRc.right = winrc.right;
		m_pDlgCameraAndControl = new CDlgCameraAndControl;
		if (m_pDlgCameraAndControl !=  NULL)
		{
			m_pDlgCameraAndControl->Create(IDD_DIALOG_CAMERA_AND_CONTROL, this);
			m_pDlgCameraAndControl->MoveWindow(CameraAndControlRc);
			m_pDlgCameraAndControl->ShowWindow(SW_HIDE);
		}
	}
	catch(...)
	{

	}
}

// 设置窗口数目
void CVEMCUCtlDlg::SetForms(int num,BOOL bFlag)
{
	int i = 0;

	if (bFlag == FALSE&&m_FormsNum == num)
		return;

	m_FormsNum = num;

	//如果当前设置的窗口数目小于之前的窗口设置数目，则关闭多出的窗口图像
	if (num < m_iPreviewNum) 
	{
		for (i = num; i < m_iPreviewNum; i++)
		{
			g_DlgVideoView[i].ShowWindow(SW_HIDE);
		}
	}

	//重新设置窗口数目
	InitPreviewWnd(num);

	for (i = 0;i < num;i++ )
	{
		ModifyVideoInfoDlgWithView(i);
		ShowVideoInfoDlgByFlag(i);
	}

	if (num < m_iPreviewNum) 
	{
		for (i = num; i < m_iPreviewNum; i++)
		{
			g_DlgVideoView[i].ShowWindow(SW_HIDE);
			g_VideoInfoDlg[i].ShowWindow(SW_HIDE);
			g_VideoTagInfoDlg[i].ShowWindow(SW_HIDE);
		}
	}

	m_iPreviewNum = num;
}

void	CVEMCUCtlDlg::SetLinkageForms(int num)
{
	if (num <= m_FormsNum)
		return;

	if (num <= 4)
	{
		SetForms(4,TRUE);
	}
	else if(num <= 6)
	{
		SetForms(6,TRUE);
	}
	else if(num <= 9)
	{
		SetForms(9,TRUE);
	}
	else
	{
		SetForms(9,TRUE);
	}
}

// 重新设置窗口数目
void CVEMCUCtlDlg::InitPreviewWnd(int iPreviewNum)
{
	CRect rect;
	int nRows, nCols, i;

	switch (iPreviewNum)
	{
	case 0:
	case 1:
		nRows = 1;
		nCols   = 1;
		break;
	case 2:
	case 3:
	case 4:
		nRows = 2;
		nCols   = 2;
		break;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		nRows = 3;
		nCols = 3;
		break;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		nCols = 4;
		nRows = 4;
		break;
	case 25:
		nRows = 5;
		nCols = 5;
		break;
	case 36:
		nRows = 6;
		nCols = 6;
		break;
	default:
		nRows = 4;
		nCols = 4;
		break;
	}

	//设置　　nRows * nCols 显示方格
	RECT rc;
	GetWindowRect(&rc);
	ScreenToClient(&rc);

#ifdef SHOW_IN_ACTIVEX
	rc.bottom = rc.top + g_nWin_Height;
	rc.right = rc.left + g_nWin_Width;
#endif

	int iFrameWidth = 2;	//外界边框

	rc.left += 0;
	rc.top += 0;
	rc.bottom -= 1;
	rc.right -= 0;

	//先设置CDlgPageViewInfo的位置
	if (m_bPageViewInfoFlag != FALSE&&m_pDlgPageViewInfo != NULL&&m_nPageViewInfoHeight > 0)
	{
		RECT tmpRect;

		tmpRect.left = rc.left+m_nControlWidth;
		tmpRect.top = rc.top+m_nMenuHight;
		tmpRect.right = rc.right-m_nLinkageWidth;
		tmpRect.bottom = tmpRect.top+m_nPageViewInfoHeight;

		m_pDlgPageViewInfo->MoveWindow(&tmpRect);
		m_pDlgPageViewInfo->SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		m_pDlgPageViewInfo->ShowWindow(SW_SHOW);
	}
	else if (m_pDlgPageViewInfo != NULL)
	{
		m_pDlgPageViewInfo->ShowWindow(SW_HIDE);
	}

	//设置6分屏，由于6分屏比较特殊，所以单独分出来
	if (iPreviewNum == 6)
	{
		RECT tmpRect;
		if (m_bMultiFullScreen == FALSE)
		{
			tmpRect.left = rc.left+m_nControlWidth;
			tmpRect.top = rc.top+m_nPageViewInfoHeight;
			tmpRect.right = rc.right-m_nLinkageWidth;
			tmpRect.bottom = rc.bottom - m_nAlarmHeight;
			SetVideoPreviewWndSix(tmpRect,FALSE);
		}
		else
		{
			tmpRect.left = rc.left;
			tmpRect.top = rc.top;
			tmpRect.right = rc.right;
			tmpRect.bottom = rc.bottom;
			SetVideoPreviewWndSix(tmpRect,TRUE);
		}
		return;
	}

	//这个是设置其他分屏
	if (m_bMultiFullScreen == FALSE)
	{
		//显示宽度
		int iWinWidth = (rc.right - rc.left - ((nCols) * iFrameWidth) - m_nControlWidth-m_nLinkageWidth) / nCols;

		//显示高度
		int iWinHeight = (rc.bottom - rc.top - ((nRows) * iFrameWidth) - m_nMenuHight - m_nPageViewInfoHeight - ACCEHIGHT - m_nAlarmHeight) / nRows;

		for (i = 0; i < iPreviewNum; i++)
		{
			int iMyRow = i / nRows;
			int iMyCol = i % nCols;

			rect.top = iMyRow * iWinHeight + ((iMyRow + 1)* iFrameWidth) + m_nMenuHight+m_nPageViewInfoHeight;
			rect.bottom = rect.top + iWinHeight;
			rect.left = m_nControlWidth + iMyCol * iWinWidth + ((iMyCol + 1) * iFrameWidth);
			rect.right = rect.left + iWinWidth;

			g_DlgVideoView[i].MoveWindow(rect);
			g_DlgVideoView[i].ShowWindow(SW_SHOW);
			g_DlgVideoView[i].Invalidate(TRUE);
		}
	}
	else if (m_bMultiFullScreen == TRUE)
	{
		//显示宽度
		int iWinWidth = (rc.right - rc.left - ((nCols) * iFrameWidth)) / nCols;
		
		//显示高度
		int iWinHeight = (rc.bottom - rc.top - ((nRows) * iFrameWidth)) / nRows;

		for (i = 0; i < iPreviewNum; i++)
		{
			int iMyRow = i / nRows;
			int iMyCol = i % nCols;

			rect.top = iMyRow * iWinHeight + ((iMyRow + 1)* iFrameWidth);
			rect.bottom = rect.top + iWinHeight;
			rect.left = iMyCol * iWinWidth + ((iMyCol + 1) * iFrameWidth);
			rect.right = rect.left + iWinWidth;

			g_DlgVideoView[i].MoveWindow(rect);
			g_DlgVideoView[i].ShowWindow(SW_SHOW);
			g_DlgVideoView[i].Invalidate(TRUE);
		}
	}
}

void CVEMCUCtlDlg::ExitDlg()
{
	if(MessageBox("确定退出热点视频联动软件？","提示",MB_YESNO|MB_ICONQUESTION) != IDYES)
		return;

	try
	{
		//提示当前正在录像
		CString str_channel;
		CString	c_channel;
		BOOL bRecordFlag = FALSE;

		str_channel.Format("当前有通道正在录像，请先关闭录像再退出!\n 通道为:");

		int i = 0;
		for (i = 0 ; i < MAXVIEWCH; i ++)
		{
			if (g_DlgVideoView[i].m_bRecordFlag)
			{
				bRecordFlag = TRUE;
				c_channel.Format("%d ", i+1);
				str_channel += c_channel;
			}
		}

		if (bRecordFlag)
		{
			MessageBox(str_channel,"视频监视");
			return;
		}

		//系统热键
		UnregisterHotKey(m_hWnd,VM_HOT_KEY_PRIOR_PAGE); 
		UnregisterHotKey(m_hWnd,VM_HOT_KEY_NEXT_PAGE); 
		UnregisterHotKey(m_hWnd,VM_HOT_KEY_EXPORT_REPORT);

		//关闭定时器
		KillTimer(TIMER_CYCLE_EVENTID);
		KillTimer(TIMER_MAIN_EVENTID);
		KillTimer(TIMER_WEATHER_EVENTID);
		KillTimer(TIMER_YT_LOCK_CHECK_EVENTID);
		KillTimer(TIMER_VIDEO_FLOW_EVENTID);

		int nVideoViewType = 0;
		int nCallId = -1;
		int nVideoSdkId = -1;

		//停止视频
		for (i = 0 ; i < MAXVIEWCH; i ++)
		{
			g_DlgVideoView[i].m_VideoViewOutInfo.Lock();
			nVideoViewType = g_DlgVideoView[i].m_VideoViewOutInfo.m_nVideoViewType;
			nCallId = g_DlgVideoView[i].m_VideoViewOutInfo.m_nCallid;
			nVideoSdkId = g_DlgVideoView[i].m_VideoViewOutInfo.m_nVideoSdkId;
			g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();

			if (nVideoViewType == 1)
			{
				if (nCallId >= 0 && nCallId < 65535)
				{
					AppHangupCallIdByPlatform(nCallId);
					Sleep(200);
				}
			}
			else if (nVideoViewType == 2)
			{
				if (nVideoSdkId > 0)
				{
					AppHangupCallByDirectDevice(nVideoSdkId);
					Sleep(200);
				}
			}
		}

		//// 退出注册
		//	AppCancelReg();
		//	Sleep(100);

		////停止SIP协议栈
		//AppStopSipStack();
		//Sleep(500);
		if (m_pDlgAlarmInfo != NULL)
		{
			m_pDlgAlarmInfo->DestroyWindow();
			delete m_pDlgAlarmInfo;
			m_pDlgAlarmInfo = NULL;
		}

		if (m_pDlgShowAlarm != NULL)
		{
			m_pDlgShowAlarm->DestroyWindow();
			delete m_pDlgShowAlarm;
			m_pDlgShowAlarm = NULL;
		}

		if (m_pDlgMenu != NULL)
		{
			m_pDlgMenu->DestroyWindow();
			delete m_pDlgMenu;
			m_pDlgMenu = NULL;
		}

		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->DeleteAllTreeList();//删除全部树
			m_pDlgPageServer->DestroyWindow();
			delete m_pDlgPageServer;
			m_pDlgPageServer = NULL;
		}

		if (m_pDlgPageViewInfo != NULL)
		{
			m_pDlgPageViewInfo->DestroyWindow();
			delete m_pDlgPageViewInfo;
			m_pDlgPageViewInfo = NULL;
		}

		for (int i = 0; i < MAXVIEWCH; i ++)
		{
			g_DlgVideoView[i].DestroyWindow();
		}

		if (g_pDlgReplay != NULL)
		{
			g_pDlgReplay->DestroyWindow();
			delete g_pDlgReplay;
			g_pDlgReplay = NULL;
		}

		if (m_pDlgHistoryLog != NULL)
		{
			m_pDlgHistoryLog->DestroyWindow();
			delete m_pDlgHistoryLog;
			m_pDlgHistoryLog = NULL;
		}

		if (m_pDlgSelectDevice != NULL)
		{
			m_pDlgSelectDevice->DeleteTreeDevice();//删除树
			m_pDlgSelectDevice->DestroyWindow();
			delete m_pDlgSelectDevice;
			m_pDlgSelectDevice = NULL;
		}

		if (m_pDlgShowControl != NULL)
		{
			m_pDlgShowControl->DestroyWindow();
			delete m_pDlgShowControl;
			m_pDlgShowControl = NULL;
		}

		if (m_pDlgShowPageServer != NULL)
		{
			m_pDlgShowPageServer->DestroyWindow();
			delete m_pDlgShowPageServer;
			m_pDlgShowPageServer = NULL;
		}

		if (m_pDlgLinkageServer != NULL )
		{
			delete m_pDlgLinkageServer;
			m_pDlgLinkageServer = NULL;
		}

		if (m_pDlgShowLinkageServer != NULL)
		{
			delete m_pDlgShowLinkageServer;
			m_pDlgShowLinkageServer = NULL;
		}

		if (g_pDlgDeviceOnlineInfo != NULL)
		{
			delete g_pDlgDeviceOnlineInfo;
			g_pDlgDeviceOnlineInfo = NULL;
		}

		if (m_pDlgRecFilePath != NULL)
		{
			delete m_pDlgRecFilePath;
			m_pDlgRecFilePath = NULL;
		}
		
		if (m_pDlgPicFilePath != NULL)
		{
			delete m_pDlgPicFilePath;
			m_pDlgPicFilePath = NULL;
		}

		if (m_pDlgColumn1 != NULL)
		{
			delete m_pDlgColumn1;
			m_pDlgColumn1 = NULL;
		}

		if (m_pDlgColumn2 != NULL)
		{
			delete m_pDlgColumn2;
			m_pDlgColumn2 = NULL;
		}

		if (m_pDlgAlarmPreview != NULL)
		{
			delete m_pDlgAlarmPreview;
			m_pDlgAlarmPreview = NULL;
		}


		

		//停止查询摄像头状态
		StopSearchCameraPresenceInfo();

		//反初始化气象
		UnInitVemAllWeatherInfo();

		//退出TCP
		UnInitTcpServerClient();

		////释放线程池
		//FreeThreadPool();

		// 断开数据库连接
		DisConnectMySql();

		//退出解码器
		Exit_DevSdk();

		//退出视频巡检报表
		VideoExcelReportExitReport();
		VideoExcelReportPipeUnInit();

		//退出视频录像报表
		VideoExcelRecordReportExitReport();
		VideoExcelRecordReportPipeUnInit();
	}
	catch(...)
	{

	}

	//调用默认关闭
	CDialog::OnCancel();
}

// 窗口关闭
void CVEMCUCtlDlg::OnClose()
{
	ShowWindow(SW_HIDE);

	for(int i = 0;i < MAXVIEWCH;i++)
	{
		g_VideoInfoDlg[i].ShowWindow(SW_HIDE);
		g_VideoTagInfoDlg[i].ShowWindow(SW_HIDE);
	}

	if (m_pDlgShowControl != NULL)
	{
		m_pDlgShowControl->ShowWindow(SW_HIDE);
	}

	if (m_pDlgShowPageServer != NULL)
	{
		m_pDlgShowPageServer->ShowWindow(SW_HIDE);
	}

	if (m_pDlgShowLinkageServer != NULL)
	{
		m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);
	}

	HideAllVideoInfoDlg();
}

LRESULT CVEMCUCtlDlg::OnDefaultControlButton(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

LRESULT CVEMCUCtlDlg::OnMainControlButton(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

LRESULT CVEMCUCtlDlg::OnAlarmControlButton(WPARAM wParam, LPARAM lParam)
{
	//隐藏告警展示窗口
	if (wParam == IDC_BTN_SHOW_ALARM)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)//轮巡状态
			return 0;

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)//人工自动巡视状态
			return 0;

		//if (m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
		//	return 0;

		if (m_pDlgShowAlarm != NULL)
		{
			m_pDlgShowAlarm->ShowWindow(SW_SHOW);
		}

		if (m_pDlgAlarmInfo != NULL)
		{
			m_pDlgAlarmInfo->ShowWindow(SW_HIDE);
		}

		m_nAlarmHeight = 0;
		m_bMenuFlag = FALSE;

		switch (m_iPreviewNum)
		{
		case 1:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM1,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 4:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM4,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 6:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM6,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 9:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM9,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case  16:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM16,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		default:
			break;
		}

		//CRect PageServer_rc;
		//CRect VemCuCtl_rc;
		//CRect LinkageServer_rc;

		//GetClientRect(VemCuCtl_rc);

		//PageServer_rc.top = VemCuCtl_rc.top;
		//PageServer_rc.bottom = VemCuCtl_rc.bottom;
		//PageServer_rc.left = VemCuCtl_rc.left;
		//PageServer_rc.right = PageServer_rc.left + m_nControlWidth;

		//if (m_pDlgPageServer != NULL)
		//{
		//	m_pDlgPageServer->MoveWindow(PageServer_rc);
		//	m_pDlgPageServer->Invalidate(TRUE);
		//}

		//LinkageServer_rc.top = VemCuCtl_rc.top;
		//LinkageServer_rc.bottom = VemCuCtl_rc.bottom;
		//LinkageServer_rc.left = VemCuCtl_rc.right - m_nLinkageWidth;
		//LinkageServer_rc.right = VemCuCtl_rc.right;

		//if (m_pDlgLinkageServer != NULL)
		//{
		//	m_pDlgLinkageServer->MoveWindow(LinkageServer_rc);
		//	m_pDlgLinkageServer->Invalidate(TRUE);
		//}

		return 0;
	}
	return 0;
}

void CVEMCUCtlDlg::HideAllDialog()
{
	int i = 0;
	if (m_pDlgShowControl != NULL)
	{
		m_pDlgShowControl->ShowWindow(SW_HIDE);
	}

	//////////////////////////////////////////////////////////////////////////
	SystemLinkageSetting_ShowWindow(m_hWnd,FALSE);
	ACSSystem_ShowWindow(m_hWnd,FALSE);
	LinkageSetting_ShowWindow(m_hWnd,FALSE);
	ElecMapWindow_Show(m_hWnd,FALSE);
	//////////////////////////////////////////////////////////////////////////
	//add by wx 2015.11.16 for elecmap  tree dialog
	if (m_pDlgTreeElecMap !=NULL){
		m_pDlgTreeElecMap->ShowWindow(SW_HIDE);
	}
	//////////////////////////////////////////////////////////////////////////

	if (g_pDlgReplay != NULL)
	{
		g_pDlgReplay->ShowWindow(SW_HIDE);
	}

	if (g_pDlgDeviceLedger != NULL)
	{
		g_pDlgDeviceLedger->ShowWindow(SW_HIDE);
	}

	if (m_pDlgPageServer != NULL)
	{
		m_pDlgPageServer->ShowWindow(SW_HIDE);
	}

	if (m_pDlgShowPageServer != NULL)
	{
		m_pDlgShowPageServer->ShowWindow(SW_HIDE);
	}

	if (m_pDlgLinkageServer != NULL)
	{
		m_pDlgLinkageServer->ShowWindow(SW_HIDE);
	}

	if (m_pDlgColumn1 != NULL)
	{
		m_pDlgColumn1->ShowWindow(SW_HIDE);
	}

	if (m_pDlgColumn2 != NULL)
	{
		m_pDlgColumn2->ShowWindow(SW_HIDE);
	}

	if (m_pDlgAlarmPreview != NULL)
	{
		m_pDlgAlarmPreview->ShowWindow(SW_HIDE);
	}

	if (m_pDlgShowLinkageServer != NULL)
	{
		m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);
	}

	if (m_pDlgPageViewInfo != NULL)
	{
		m_pDlgPageViewInfo->ShowWindow(SW_HIDE);
	}

	for (i = 0; i < m_FormsNum; i ++)
	{
		g_DlgVideoView[i].ShowWindow(SW_HIDE);
	}

	if (m_pDlgHistoryLog != NULL)
	{
		m_pDlgHistoryLog->ShowWindow(SW_HIDE);
	}

	if (m_pDlgShowAlarm!=NULL)
	{
		m_pDlgShowAlarm->ShowWindow(SW_HIDE);
	}

	if (m_pDlgAlarmInfo!=NULL)
	{
		m_pDlgAlarmInfo->ShowWindow(SW_HIDE);
	}

	if (m_pDlgCameraAndControl != NULL)
	{
		m_pDlgCameraAndControl->ShowWindow(SW_HIDE);
	}

	HideAllVideoInfoDlg();//隐藏全部画线
}

LRESULT CVEMCUCtlDlg::OnMenuControlButton(WPARAM wParam, LPARAM lParam)
{
	int i = 0;

	CRect	winrc;
	GetClientRect(winrc);

	CRect  subwinrc;

	// 菜单窗口
	subwinrc.top = MENUHIGHT;
	subwinrc.bottom = winrc.bottom;
	subwinrc.left = winrc.left;
	subwinrc.right = winrc.right;

	// 最小化
	if (wParam == IDC_MENU_MIN)
	{
		if (m_pDlgShowControl != NULL)
		{
			m_pDlgShowControl->ShowWindow(SW_HIDE);
		}

		if (m_pDlgShowPageServer != NULL)
		{
			m_pDlgShowPageServer->ShowWindow(SW_HIDE);
		}

		HideAllVideoInfoDlg();

		ShowWindow(SW_MINIMIZE);
		return 0;
	}

	//托盘
	if (wParam == IDC_MENU_TRAY)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}

	//程序退出
	if (wParam == IDC_MENU_EXIT)
	{
		ExitDlg();
		return 0;
	}

	// 显示配置画面
	if (wParam == IDC_MENU_MANAGE)
	{
		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_MANAGE;

		HideAllDialog();

		SystemLinkageSetting_ShowWindow(m_hWnd,TRUE);

		return 0;
	}

	//如果是联动配置事件
	if (wParam == VEM_CONTROL_BUTTON_MESSAGE_TYPE_LINKAGEEVENT)
	{
		HideAllDialog();

		ElecMapWindow_Show(m_hWnd,TRUE);
	}

	//显示辅助系统
	if (wParam == IDC_MENU_ASSISTANCE)
	{
		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_ASSISTANCE;

		HideAllDialog();

		ACSSystem_ShowWindow(m_hWnd,TRUE);
		ElecMapWindow_Show(m_hWnd, TRUE);
		
		CRect rect;
		int nHeight = winrc.bottom - winrc.top - MENUHIGHT;
		int nWidth = winrc.right - winrc.left - 7*2;
		rect.top = winrc.top + MENUHIGHT + 0.4*nHeight + 30 *4;
		rect.bottom = winrc.bottom;
		rect.left = winrc.right - 0.3*nWidth;
		rect.right = winrc.right;
		SetElecMapDlgPosition(rect);

		return 0;
	}

	//显示辅助联动配置
	if (wParam == IDC_MENU_LINKAGE_SETTING)
	{
		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_LINKAGE_SETTING;

		HideAllDialog();

		LinkageSetting_ShowWindow(m_hWnd,TRUE);

		return 0;
	}

	//显示告警预览窗口
	if (wParam == IDC_BUTTON_ALARM_PREVIEW)
	{
		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_ALARM_PREVIEW;

		HideAllDialog();
		
		if (m_pDlgAlarmPreview != NULL)
		{
			m_pDlgAlarmPreview->ShowWindow(SW_SHOW);
		}
		else
		{
			m_pDlgAlarmPreview = new CDlgAlarmPreview();
			if (m_pDlgAlarmPreview != NULL)
			{
				m_pDlgAlarmPreview->Create(IDD_DIALOG_ALARM_PREVIEW, g_pMainDlg);
				m_pDlgAlarmPreview->MoveWindow(subwinrc);
				m_pDlgAlarmPreview->ShowWindow(SW_SHOW);
			}
		}
	}

	//显示设备台账窗口
	if (wParam == IDC_MENU_DEVICE_LEDGER)
	{
		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_DEVICE_LEDGER;

		HideAllDialog();

		if (g_pDlgDeviceLedger != NULL)
		{
			g_pDlgDeviceLedger->ShowWindow(SW_SHOW);
		}
		else
		{
			g_pDlgDeviceLedger = new CDlgDeviceLedger();
			if (g_pDlgDeviceLedger != NULL)
			{
				g_pDlgDeviceLedger->Create(IDD_DIALOG_DEVICE_LEDGER, g_pMainDlg);
				g_pDlgDeviceLedger->MoveWindow(subwinrc);
				g_pDlgDeviceLedger->ShowWindow(SW_SHOW);
			}
		}

		return 0;
	}

	//显示电子地图窗口
	if (wParam == IDC_MENU_ELECMAP)
	{
		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_DEVICE_LEDGER;

		HideAllDialog();
		
		//add by wx 2015.11.16 for elecmap  tree dialog
		if (m_pDlgTreeElecMap !=NULL){
			m_pDlgTreeElecMap->ShowWindow(SW_SHOW);
		}
		ElecMapWindow_Show(m_hWnd,TRUE);

		if (m_pDlgCameraAndControl != NULL)
		{
			m_pDlgCameraAndControl->ShowWindow(SW_SHOW);
		}

		CRect rect1(CONTROLWIDTH, MENUHIGHT, CONTROLWIDTH+7, winrc.bottom);
		if (m_pDlgColumn1 != NULL)
		{
			m_pDlgColumn1->MoveWindow(rect1);
			m_pDlgColumn1->ShowWindow(SW_SHOW);
		}

		CRect rect(CONTROLWIDTH+7, MENUHIGHT,  winrc.right - LINKAGEWIDTH * 2 - 7, winrc.bottom - ALARMHIGHT);
		SetElecMapDlgPosition(rect);

		//实时告警窗口
		if (m_pDlgAlarmInfo!=NULL)
		{
			m_pDlgAlarmInfo->ShowWindow(SW_SHOW);
			CRect rectAlarm(CONTROLWIDTH+7, winrc.bottom - ALARMHIGHT, winrc.right - LINKAGEWIDTH * 2 - 7, winrc.bottom);
			m_pDlgAlarmInfo->MoveWindow(rectAlarm);
		}


		CRect rect2(winrc.right - LINKAGEWIDTH * 2 -7, MENUHIGHT, winrc.right - LINKAGEWIDTH * 2, winrc.bottom);
		if (m_pDlgColumn2 != NULL)
		{
			m_pDlgColumn2->MoveWindow(rect2);
			m_pDlgColumn2->ShowWindow(SW_SHOW);
		}

		rect.top = winrc.top + MENUHIGHT;
		rect.bottom = winrc.bottom;
		rect.left = winrc.right - LINKAGEWIDTH * 2;
		rect.right = winrc.right;
		m_pDlgCameraAndControl->MoveWindow(rect);



		return 0;
	}

	// 显示多画面
	if (wParam == IDC_MENU_VIEW)
	{
		if (g_userpower.userCP.videoview == 0)//权限判断
			return 0;

		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_MULTIVIEW;

		HideAllDialog();
		
		if (m_bPageServerFlag != FALSE)
		{
			if (m_pDlgPageServer != NULL)
			{
				m_pDlgPageServer->ShowWindow(SW_SHOW);
			}

			if (m_pDlgShowPageServer != NULL)
			{
				m_pDlgShowPageServer->ShowWindow(SW_HIDE);
			}
		}
		else
		{
			if (m_pDlgPageServer != NULL)
			{
				m_pDlgPageServer->ShowWindow(SW_HIDE);
			}

			if (m_pDlgShowPageServer != NULL)
			{
				m_pDlgShowPageServer->ShowWindow(SW_SHOW);
			}
		}

		DisplayLinkageServerRelationListByFlag(m_bLinkageServerFlag);

		if(m_pDlgPageViewInfo != NULL)
		{
			if (m_bPageViewInfoFlag != FALSE)
			{
				m_pDlgPageViewInfo->ShowWindow(SW_SHOW);
			}
			else
			{
				m_pDlgPageViewInfo->ShowWindow(SW_HIDE);
			}
		}

		for (i = 0; i < m_FormsNum; i ++)
		{
			g_DlgVideoView[i].ShowWindow(SW_SHOW);
			ModifyVideoInfoDlgWithView(i);
			ShowVideoInfoDlgByFlag(i);
		}

		if (m_pDlgAlarmInfo!=NULL)
		{
			m_pDlgAlarmInfo->ShowWindow(SW_SHOW);
		}
		
		return 0;
	}

	// 显示轮巡
	if (wParam == IDC_MENU_TUNECYCLE)
	{
		HideAllDialog();

		return 0;
	}

	// 显示回放
	if (wParam == IDC_MENU_REPLAY)
	{
		if (g_userpower.userCP.replay == 0)//权限判断
			return 0;

		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_REPLAY;

		HideAllDialog();

		if (g_pDlgReplay != NULL)
		{
			g_pDlgReplay->ShowWindow(SW_SHOW);
		}
		else
		{
			g_pDlgReplay = new CDlgReplay();
			if (g_pDlgReplay != NULL)
			{
				g_pDlgReplay->Create(IDD_DIALOG_REPLAY, g_pMainDlg);
				g_pDlgReplay->MoveWindow(subwinrc);
				g_pDlgReplay->ShowWindow(SW_SHOW);
			}
		}

		return 0;
	}

	//事件日志
	if (wParam == IDC_MENU_EVENTLOG)
	{
		if (g_userpower.userCP.historylog == 0)//权限判断
			return 0;

		m_nMenuItemIndex = VEM_WIN_ITEM_TYPE_HISTORY_LOG;

		HideAllDialog();
		
		if (m_pDlgHistoryLog != NULL)
		{
			m_pDlgHistoryLog->ShowWindow(SW_SHOW);
		}
		else
		{
			m_pDlgHistoryLog = new CDlgHistoryLog();
			if (m_pDlgHistoryLog != NULL)
			{
				m_pDlgHistoryLog->Create(IDD_DIALOG_HISTORYLOG, g_pMainDlg);
				m_pDlgHistoryLog->MoveWindow(subwinrc);
				m_pDlgHistoryLog->ShowWindow(SW_SHOW);
			}
		}

		return 0;
	}

	//显示控制栏
	if (wParam == IDC_BTN_SHOW_CONTROL)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)//轮巡状态
			return 0;

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)//人工自动巡视状态
			return 0;

		if (m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
			return 0;

		HideAllDialog();

		if (m_pDlgShowControl != NULL)
		{
			m_pDlgShowControl->ShowWindow(SW_SHOW);
		}
		
		if (m_pDlgMenu != NULL)
		{
			m_pDlgMenu->ShowWindow(SW_HIDE);
		}
		
		m_nMenuHight = 0;
		m_bMenuFlag = FALSE;

		switch (m_iPreviewNum)
		{
		case 1:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM1,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 4:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM4,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 6:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM6,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 9:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM9,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case  16:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM16,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		default:
			break;
		}

		CRect PageServer_rc;
		CRect VemCuCtl_rc;
		CRect LinkageServer_rc;

		GetClientRect(VemCuCtl_rc);

		PageServer_rc.top = VemCuCtl_rc.top;
		PageServer_rc.bottom = VemCuCtl_rc.bottom;
		PageServer_rc.left = VemCuCtl_rc.left;
		PageServer_rc.right = PageServer_rc.left + m_nControlWidth;

		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->MoveWindow(PageServer_rc);
			m_pDlgPageServer->Invalidate(TRUE);
		}

		LinkageServer_rc.top = VemCuCtl_rc.top;
		LinkageServer_rc.bottom = VemCuCtl_rc.bottom;
		LinkageServer_rc.left = VemCuCtl_rc.right - m_nLinkageWidth;
		LinkageServer_rc.right = VemCuCtl_rc.right;

		if (m_pDlgLinkageServer != NULL)
		{
			m_pDlgLinkageServer->MoveWindow(LinkageServer_rc);
			m_pDlgLinkageServer->Invalidate(TRUE);
		}

		//调整告警展示窗口
		CRect AlarmInfo_rc;
		AlarmInfo_rc.top = LinkageServer_rc.bottom - g_pMainDlg->m_nAlarmHeight;
		AlarmInfo_rc.bottom = LinkageServer_rc.bottom;
		AlarmInfo_rc.left = VemCuCtl_rc.left + g_pMainDlg->m_nControlWidth;
		AlarmInfo_rc.right = VemCuCtl_rc.right - g_pMainDlg->m_nLinkageWidth;

		if (g_pMainDlg->m_pDlgAlarmInfo != NULL)
		{
			g_pMainDlg->m_pDlgAlarmInfo->MoveWindow(AlarmInfo_rc);
		}

		return 0;
	}

	//重新登录
	if (wParam == IDC_MENU_LOGIN)
	{
		CDlgLogin DlgLogin;

		if (DlgLogin.DoModal() == IDOK)
		{
			if (DlgLogin.m_nUserID < 0)
				return 0;

			//记录用户名
			char szPathName[256] = {0};
			memset(szPathName, 0, sizeof(szPathName));

			if (strlen(g_szAppPath) > 0)
			{
				strcpy_s(szPathName,sizeof(szPathName)-1,g_szAppPath);
			}
			else
			{
				GetCurrentDirectory(sizeof(szPathName)-1, szPathName);
			}
			
			strcat_s(szPathName, "\\VEMCUCtl.ini");
			WritePrivateProfileString("USER", "UserName", g_userpower.username,szPathName);

			//得到用户巡视权限
			GetUserStationViewPower2();

			char szUserInfo[256] = {0};
			sprintf_s(szUserInfo,sizeof(szUserInfo),"当前用户:\r\n   %s",g_userpower.username);

			if (m_pDlgMenu != NULL)
			{
				m_pDlgMenu->m_staticUserInfo.SetText(szUserInfo,6,RGB(255,255,255));
				CWnd *pWnd = m_pDlgMenu->GetDlgItem(IDC_USER_INFO_SHOW);
				if (pWnd != NULL)
				{
					RECT rect;
					pWnd->GetWindowRect(&rect);
					ScreenToClient(&rect);
					InvalidateRect(&rect,TRUE);
				}
			}

			strcpy_s(g_user_config_info.szUserName ,sizeof(g_user_config_info.szUserName),g_userpower.username);
			strcpy_s(g_user_config_info.szUserPassword,sizeof(g_user_config_info.szUserPassword),g_userpower.password);

			//刷新树列表
			if (m_pDlgPageServer != NULL)
			{
				m_pDlgPageServer->OnMenuitemReflashlist();
			}

			//刷新树列表
			if (m_pDlgSelectDevice != NULL)
			{
				m_pDlgSelectDevice->OnMenuitemSelectDeviceReflashlist();
			}

			//用户操作日志
			if (m_pDlgHistoryLog != NULL)
			{
				m_pDlgHistoryLog->m_DlgHlUser.m_ListCtrl_UserLog.DeleteAllItems();
			}

			//刷新报表导出用户信息
		    VideoExcelReportFreshUserInfo(g_userpower.userID);

			//设置重登录用户名
			SystemLinkageSetting_ReLoginUser(g_userpower.username);
		}
	}

	if (wParam == IDC_BTN_HELP)
	{
		if (MessageBox("是否打开帮助文件？","帮助提示",MB_YESNO) != IDNO)
		{
			ShellExecute(NULL,"open","变电站视频巡视使用说明书.doc",NULL,NULL,SW_MAXIMIZE);
		}
	}

	return 0;
}

LRESULT CVEMCUCtlDlg::OnPageServerControlButton(WPARAM wParam, LPARAM lParam)
{
	// 摄像头树
	if (wParam == IDC_BUTTON_PS_CAMERALIST)
	{
		m_pDlgPageServer->OnBnClickedButtonPsCameralist();
		return 0;
	}

	// 设备树
	if (wParam == IDC_BUTTON_PS_DEVICELIST)
	{
		m_pDlgPageServer->OnBnClickedButtonPsDevicelist();
		return 0;
	}

	//单画面
	if (wParam == IDC_BUTTON_PS_FORM1)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)
		{
			MessageBox("轮巡状态下不能进行多画面切换","视频监视");
			return 0;
		}

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
		{
			MessageBox("当前处于人工自动巡视状态","视频预览",MB_ICONWARNING);
			return 0;
		}

		m_nFullScreen = 0;
		m_bFullScreen = FALSE;

		if (m_iFocuseWindowID == 0)
		{
			DrawRect(m_iFocuseWindowID, 0);
		}

		if (m_pDlgPageViewInfo != NULL)
		{
			m_pDlgPageViewInfo->SetPageViewOnePageMaxCameraNum(1);
		}

		SetForms(1,TRUE);

		if (m_iFocuseWindowID == 0)
		{
			DrawRect(m_iFocuseWindowID, 1);
		}
		else
		{
			DrawRect(0, 0);
		}

		return 0;
	}

	//四画面
	if (wParam == IDC_BUTTON_PS_FORM4)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)
		{
			MessageBox(">>轮巡状态下不能进行多画面切换","视频监视");
			return 0;
		}

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
		{
			MessageBox("当前处于人工自动巡视状态","视频预览",MB_ICONWARNING);
			return 0;
		}

		m_nFullScreen = 0;
		m_bFullScreen = FALSE;

		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < m_FormsNum)
		{
			DrawRect(m_iFocuseWindowID, 0);
		}

		if (m_pDlgPageViewInfo != NULL)
		{
			m_pDlgPageViewInfo->SetPageViewOnePageMaxCameraNum(4);
		}

		SetForms(4,TRUE);
		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < 3)
		{
			DrawRect(m_iFocuseWindowID, 1);
		}
		else
		{
			DrawRect(0, 0);
		}

		return 0;
	}

	//六画面
	if (wParam == IDC_BUTTON_PS_FORM6)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)
		{
			MessageBox(">>轮巡状态下不能进行多画面切换","视频监视");
			return 0;
		}

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
		{
			MessageBox("当前处于人工自动巡视状态","视频预览",MB_ICONWARNING);
			return 0;
		}

		m_nFullScreen = 0;
		m_bFullScreen = FALSE;

		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < m_FormsNum)
		{
			DrawRect(m_iFocuseWindowID, 0);
		}

		SetForms(6,TRUE);
		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < 3)
		{
			DrawRect(m_iFocuseWindowID, 1);
		}
		else
		{
			DrawRect(0, 0);
		}

		return 0;
	}

	//九画面
	if (wParam == IDC_BUTTON_PS_FORM9)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)
		{
			MessageBox("轮巡状态下不能进行多画面切换","视频监视");
			return 0;
		}

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
		{
			MessageBox("当前处于人工自动巡视状态","视频预览",MB_ICONWARNING);
			return 0;
		}

		m_nFullScreen = 0;
		m_bFullScreen = FALSE;

		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < m_FormsNum)
		{
			DrawRect(m_iFocuseWindowID, 0);
		}

		if (m_pDlgPageViewInfo != NULL)
		{
			m_pDlgPageViewInfo->SetPageViewOnePageMaxCameraNum(9);
		}

		SetForms(9,TRUE);

		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < 8)
		{
			DrawRect(m_iFocuseWindowID, 1);
		}
		else
		{
			DrawRect(0, 0);
		}

		return 0;
	}

	//十六画面
	if (wParam == IDC_BUTTON_PS_FORM16)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)
		{
			MessageBox("轮巡状态下不能进行多画面切换","视频监视");
			return 0;
		}

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
		{
			MessageBox("当前处于人工自动巡视状态","视频预览",MB_ICONWARNING);
			return 0;
		}

		m_nFullScreen = 0;
		m_bFullScreen = FALSE;

		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < m_FormsNum)
		{
			DrawRect(m_iFocuseWindowID, 0);
		}
		SetForms(16,TRUE);
		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < 3)
		{
			DrawRect(m_iFocuseWindowID, 1);
		}
		else
		{
			DrawRect(0, 0);
		}

		return 0;
	}

	//设置预置点
	if (wParam == IDC_PRESET_SETMOVE)
	{
		SetPreSet();
		return 0;
	}

	//运行到预置点
	if (wParam == IDC_PRESET_MOVETO)
	{
		MoveToPreSet();
		return 0;
	}

	//获取视频参数
	if (wParam == IDC_GETVIEWSET)
	{
		DefaultViewSetting();
		return 0;
	}

	//设置视频参数
	if (wParam == IDC_SETVIEWSET)
	{
		SetViewSetting();
		return 0;
	}

	//显示隐藏侧边栏
	if (wParam == IDC_BTN_SHOW_HIDE)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)//轮巡状态
			return 0;

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)//人工自动巡视状态
			return 0;

		m_nControlWidth = 0;

		switch(m_iPreviewNum)
		{
		case 1:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM1,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 4:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM4,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 6:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM6,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 9:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM9,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case  16:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM16,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		default:
			break;
		}

		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->ShowWindow(SW_HIDE);
		}

		if (m_pDlgShowPageServer != NULL)
		{
			m_pDlgShowPageServer->ShowWindow(SW_SHOW);
		}

		m_bPageServerFlag = FALSE;

		//调整告警展示窗口
		CRect AlarmInfo_rc;
		CRect VemCuCtl_rc;

		GetClientRect(VemCuCtl_rc);

		AlarmInfo_rc.top = VemCuCtl_rc.bottom - m_nAlarmHeight;
		AlarmInfo_rc.bottom = VemCuCtl_rc.bottom;
		AlarmInfo_rc.left = VemCuCtl_rc.left;
		AlarmInfo_rc.right = VemCuCtl_rc.right;

		if (m_pDlgAlarmInfo != NULL)
		{
			m_pDlgAlarmInfo->MoveWindow(AlarmInfo_rc);
			//m_pDlgAlarmInfo->Invalidate(TRUE);
		}
		

		return 0;
	}

	return 0;
}

LRESULT CVEMCUCtlDlg::OnLinkageServerControlButton(WPARAM wParam, LPARAM lParam)
{
	//显示隐藏侧边栏
	if (wParam == IDC_BTN_HIDE_LINKAGE_SERVER)
	{
		if (g_TuneCycle_MehtodInfo.bState == true)//轮巡状态
			return 0;

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)//人工自动巡视状态
			return 0;

		m_nLinkageWidth = 0;

		switch(m_iPreviewNum)
		{
		case 1:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM1,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 4:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM4,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 6:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM6,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case 9:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM9,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		case  16:
			{
				SendMessage(OM_CONTROLBUTTON, IDC_BUTTON_PS_FORM16,VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER);
			}
			break;
		default:
			break;
		}

		m_bLinkageServerFlag = FALSE;

		DisplayLinkageServerRelationListByFlag(m_bLinkageServerFlag);

		return 0;
	}

	return 0;
}

void CVEMCUCtlDlg::CheckRelationLinkageListCount()
{
	if (m_pDlgLinkageServer == NULL)
		return;

	int nCount = m_pDlgLinkageServer->m_listRelation.GetItemCount();

	while(nCount > 300)
	{
		m_pDlgLinkageServer->m_listRelation.DeleteItem(nCount-1);
		nCount = m_pDlgLinkageServer->m_listRelation.GetItemCount();
	}
}

void CVEMCUCtlDlg::CheckAndSetYtLockState()
{
	if (m_iFocuseWindowID >= 0&&m_iFocuseWindowID < MAXVIEWCH&&m_pDlgPageServer != NULL)
	{
		if(g_DlgVideoView[m_iFocuseWindowID].IsYtLock() != FALSE)
		{
			m_pDlgPageServer->m_btnLock.SetIndex(1);
			m_pDlgPageServer->m_btnLock.Invalidate(true);
		}
		else
		{
			m_pDlgPageServer->m_btnLock.SetIndex(0);
			m_pDlgPageServer->m_btnLock.Invalidate(true);
		}
	}
}

BOOL CVEMCUCtlDlg::CheckMenuVideoTagIsDisplay(int iWinID)
{
	if(iWinID < 0||iWinID >= MAXVIEWCH)
		return FALSE;

	RECT rc;
	POINT pt;
	int nWidth = 0;
	int nHeight = 0;

	g_DlgVideoView[iWinID].GetWindowRect(&rc);

	nWidth = rc.right - rc.left;
	nWidth = nWidth/3;
	nHeight = rc.bottom - rc.top;
	nHeight = nHeight/3;

	GetCursorPos(&pt);

	if (pt.x >= rc.left+nWidth&&pt.x < rc.left+nWidth*2&&pt.y >= rc.top+nHeight&&pt.y <= rc.top+nHeight*2)
		return TRUE;

	return FALSE;
}

//在进程表中查找进程
BOOL CVEMCUCtlDlg::SearchProcessByName(char * pProcessName)
{
	HANDLE hSnapShot = INVALID_HANDLE_VALUE;

	PROCESSENTRY32 pe;

	char  szProcessName[MAX_PATH] = {0};
	char  szExeFileName[MAX_PATH] = {0};

	if (pProcessName == NULL)
		return FALSE;

	try
	{
		memset(szProcessName,0,sizeof(szProcessName));
		memset(szExeFileName,0,sizeof(szExeFileName));

		if (strlen(pProcessName) >= sizeof(szProcessName))
			return FALSE;

		strcpy(szProcessName,pProcessName);

		hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (hSnapShot == INVALID_HANDLE_VALUE)
			return FALSE;

		memset(&pe,0,sizeof(PROCESSENTRY32));
		pe.dwSize = sizeof(PROCESSENTRY32);

		if(!Process32First(hSnapShot,&pe))
		{
			CloseHandle(hSnapShot);
			hSnapShot = NULL;
			return FALSE;
		}

		while (Process32Next(hSnapShot,&pe))
		{
			strcpy(szExeFileName,pe.szExeFile);

			if(stricmp(szProcessName,szExeFileName) == 0)
			{
				CloseHandle(hSnapShot);
				hSnapShot = NULL;
				return TRUE;
			}
		}

		CloseHandle(hSnapShot);
		hSnapShot = NULL;
	}
	catch(...)
	{

	}

	return FALSE;
}

//检测并更新软件
BOOL CVEMCUCtlDlg::CheckAndUpdateSystemSoftware()
{
	STARTUPINFO si;
	memset(&si,0,sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;  
	memset(&pi,0,sizeof(pi));

	char szCommandLine[256] = {0};
	CString strExeFileName = "MyUpdate.exe";

#if VM_DKY_CLIENT_VERSION
	{
		sprintf_s(szCommandLine,"%s -v 1",strExeFileName);
	}
#endif

#if VM_SD_CLIENT_VERSION
	{
		sprintf_s(szCommandLine,"%s -v 2",strExeFileName);
	}
#endif

#if VM_SJ_CLIENT_VERSION
	{
		sprintf_s(szCommandLine,"%s -v 3",strExeFileName);
	}
#endif

#if VM_SERVER_VERSION
	{
		sprintf_s(szCommandLine,"%s -v 4",strExeFileName);
	}
#endif

#if VM_QABD_CLIENT_VERSION
	{
		sprintf_s(szCommandLine,"%s -v 5",strExeFileName);
	}
#endif

	if (strlen(szCommandLine) == 0)
	{
		sprintf_s(szCommandLine,"%s -v 0",strExeFileName);
	}

	if(SearchProcessByName((char *)(LPCSTR)strExeFileName) == TRUE)
		return TRUE;

	int nResult = CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi); 
	if (nResult != 0)
	{
		TRACE("创建系统更新程序失败!");
	}

	return TRUE;
}

 //人工自动视频巡视
BOOL  CVEMCUCtlDlg::VideoPageAutoView()
{
	if (m_pDlgPageViewInfo == NULL)
	{
		return FALSE;
	}

	m_pDlgPageViewInfo->OnBnClickedBtnNextPage();

	return TRUE;
}

// 消息控制按钮
LRESULT CVEMCUCtlDlg::OnControlButton(WPARAM wParam, LPARAM lParam)
{
	int nType = (int)lParam;

	switch (nType)
	{
	case VEM_CONTROL_BUTTON_MESSAGE_TYPE_DEFAULT:
		{
			return OnDefaultControlButton(wParam,lParam);
		}
		break;

	case VEM_CONTROL_BUTTON_MESSAGE_TYPE_MAIN:
		{
			return OnMainControlButton(wParam,lParam);
		}
		break;

	case VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU:
		{
			return OnMenuControlButton(wParam,lParam);
		}
		break;

	case VEM_CONTROL_BUTTON_MESSAGE_TYPE_PAGESERVER:
		{
			return OnPageServerControlButton(wParam,lParam);
		}
		break;

	case VEM_CONTROL_BUTTON_MESSAGE_TYPE_LINKAGESERVER:
		{
			return OnLinkageServerControlButton(wParam,lParam);
		}
		break;
	case VEM_CONTROL_BUTTON_MESSAGE_TYPE_ALARM:
		{
			return OnAlarmControlButton(wParam,lParam);
		}
		break;
	default:
		{
			return OnDefaultControlButton(wParam,lParam);
		}
		break;
	}

	return 0;
}

BOOL CVEMCUCtlDlg::OnEraseBkgnd(CDC* pDC)
{
	CRect   rect;
	GetWindowRect(&rect);
	CBitmap   m_pBmp;
	BITMAP   bm;
	CDC   dcMem;
	CBitmap*   pOldBitmap;

#ifdef SHOW_IN_ACTIVEX
	rect.bottom = rect.top + g_nWin_Height;
	rect.right = rect.left + g_nWin_Width;
#endif

	// 左上角
	m_pBmp.LoadBitmap(IDB_BITMAP_BG_LEFTUPCORNER);
	m_pBmp.GetObject(sizeof(BITMAP),(LPVOID)&bm);
	dcMem.CreateCompatibleDC(pDC);
	pOldBitmap = dcMem.SelectObject(&m_pBmp);

	pDC->StretchBlt(
		rect.left,
		rect.top,
		bm.bmWidth,
		bm.bmHeight,
		&dcMem,
		0,
		0,
		bm.bmWidth,
		bm.bmHeight,
		SRCCOPY);
	(dcMem.SelectObject(pOldBitmap))->DeleteObject();
	dcMem.DeleteDC();

	// 右上角
	m_pBmp.LoadBitmap(IDB_BITMAP_BG_RIGHTUPCORNER);
	m_pBmp.GetObject(sizeof(BITMAP),(LPVOID)&bm);
	dcMem.CreateCompatibleDC(pDC);
	pOldBitmap = dcMem.SelectObject(&m_pBmp);

	pDC->StretchBlt(
		rect.right-5,
		rect.top,
		bm.bmWidth,
		bm.bmHeight,
		&dcMem,
		0,
		0,
		bm.bmWidth,
		bm.bmHeight,
		SRCCOPY);
	(dcMem.SelectObject(pOldBitmap))->DeleteObject();
	dcMem.DeleteDC();

	// 上边
	m_pBmp.LoadBitmap(IDB_BITMAP_BG_UPCORNER);
	m_pBmp.GetObject(sizeof(BITMAP),(LPVOID)&bm);
	dcMem.CreateCompatibleDC(pDC);
	pOldBitmap = dcMem.SelectObject(&m_pBmp);

	pDC->StretchBlt(
		rect.left + 5,
		rect.top,
		rect.right - 10,
		bm.bmHeight,
		&dcMem,
		0,
		0,
		bm.bmWidth,
		bm.bmHeight,
		SRCCOPY);
	(dcMem.SelectObject(pOldBitmap))->DeleteObject();
	dcMem.DeleteDC();

	// 左边
	m_pBmp.LoadBitmap(IDB_BITMAP_BG_LEFTCORNER);
	m_pBmp.GetObject(sizeof(BITMAP),(LPVOID)&bm);
	dcMem.CreateCompatibleDC(pDC);
	pOldBitmap = dcMem.SelectObject(&m_pBmp);

	pDC->StretchBlt(
		rect.left,
		rect.top + 5,
		bm.bmWidth,
		rect.bottom,
		&dcMem,
		0,
		0,
		bm.bmWidth,
		bm.bmHeight,
		SRCCOPY);
	(dcMem.SelectObject(pOldBitmap))->DeleteObject();
	dcMem.DeleteDC();

	// 右边
	m_pBmp.LoadBitmap(IDB_BITMAP_BG_RIGHTCORNER);
	m_pBmp.GetObject(sizeof(BITMAP),(LPVOID)&bm);
	dcMem.CreateCompatibleDC(pDC);
	pOldBitmap = dcMem.SelectObject(&m_pBmp);

	pDC->StretchBlt(
		rect.right - 2,
		rect.top + 5,
		bm.bmWidth,
		rect.bottom - 5,
		&dcMem,
		0,
		0,
		bm.bmWidth,
		bm.bmHeight,
		SRCCOPY);
	(dcMem.SelectObject(pOldBitmap))->DeleteObject();
	dcMem.DeleteDC();

	// 中间
	m_pBmp.LoadBitmap(IDB_BITMAP_TOTAL_BG);
	m_pBmp.GetObject(sizeof(BITMAP),(LPVOID)&bm);
	dcMem.CreateCompatibleDC(pDC);
	pOldBitmap = dcMem.SelectObject(&m_pBmp);

	// 	pDC->StretchBlt(
	// 		rect.left + 2,
	// 		rect.top + 5,
	// 		rect.right - 4,
	// 		rect.bottom - 5,
	// 		&dcMem,
	// 		0,
	// 		0,
	// 		bm.bmWidth,
	// 		bm.bmHeight,
	// 		SRCCOPY);
	pDC->StretchBlt(
		rect.left,
		rect.top,
		rect.right,
		rect.bottom,
		&dcMem,
		0,
		0,
		bm.bmWidth,
		bm.bmHeight,
		SRCCOPY);
	(dcMem.SelectObject(pOldBitmap))->DeleteObject();
	dcMem.DeleteDC();

	return true;
}

/************************************************************************
* DrawFocusChannel
* 在指定通道划出焦点状态，在四周画框 / 红框，或黑框
*
************************************************************************/
void CVEMCUCtlDlg::DrawFocusChannel(int iChannel, BOOL bDraw)
{
	if (bDraw)
	{
		if (iChannel >= 0)
		{
			DrawRect(iChannel, 1);
		}
		//DrawRect(iChannel, 1);
	}
	else
	{
		if (iChannel >= 0)
		{
			DrawRect(iChannel, 0);
		}
	}
}

/************************************************************************
*  DrawRect
*  在指定视频通道窗口画方框 / 红框(选中)，或黑框(未选)
*
************************************************************************/
void CVEMCUCtlDlg::DrawRect(int iWinID, int nHighLight)
{
	COLORREF color;
	COLORREF color2;
	COLORREF color3;

	CDC *dc = GetDC();

	if (nHighLight == 0)
		color = RGB(37,137,127);
	else
		color = RGB(255,0,0);

	color2 = RGB(77, 188, 176);
	color3 = RGB(218,255,251);

	CRect Rect;
	g_DlgVideoView[iWinID].GetWindowRect(&Rect);
	ScreenToClient(&Rect);

	int nPenWidth = 1;

//	if (nHighLight != 0)
	{
		Rect.left--;
		Rect.right;
		Rect.top--;
		Rect.bottom;
	}
	//else
	//{
	//	Rect.left--;
	//	Rect.right++;
	//	Rect.top--;
	//	Rect.bottom++;
	//}

	CPen Pen(PS_SOLID, nPenWidth, color);
	CPen Pen2(PS_SOLID, nPenWidth, color2);
	CPen Pen3(PS_SOLID, nPenWidth, color3);
	
	CPen *pOldPen = dc->SelectObject(&Pen);

	if (nHighLight == 0)
	{
		dc->SelectObject(&Pen2);
	}

	dc->MoveTo(Rect.right,Rect.top);
	dc->LineTo(Rect.left,Rect.top);
	dc->LineTo(Rect.left,Rect.bottom);

	dc->MoveTo(Rect.right,Rect.top);
	dc->LineTo(Rect.right,Rect.bottom);

	if (nHighLight == 0)
	{
		dc->SelectObject(&Pen3);
	}

	dc->LineTo(Rect.left,Rect.bottom);

	if (nHighLight == 0&&g_DlgVideoView[iWinID].m_bSelectedFlag)
	{
		dc->MoveTo(Rect.right-1,Rect.top);
		dc->LineTo(Rect.right-1,Rect.bottom-1);
		dc->LineTo(Rect.left,Rect.bottom-1);
	}

	dc->SelectObject(pOldPen);

	ReleaseDC(dc);

	//if (nHighLight == 0)
	//{
	//	g_DlgVideoView[iWinID].ModifyControlSizeBySelectFlag(FALSE);
	//}
	//else
	//{
	//	g_DlgVideoView[iWinID].ModifyControlSizeBySelectFlag(TRUE);
	//}
}

BOOL CVEMCUCtlDlg::LoadSkin()
{
	m_ViewOutVideoBGBmp.LoadBitmap(IDB_BITMAP_VIEW_CHANNEL_BK);
	m_ViewOutVideoBGBmp.GetObject(sizeof(BITMAP),(LPVOID)&m_ViewOutVideoBGBm);
	return TRUE;
}

void CVEMCUCtlDlg::OnTimer(UINT_PTR nIDEvent)
{
	try
	{
		if (nIDEvent == TIMER_MAIN_EVENTID)
		{
			CTime nowtime = CTime::GetCurrentTime();

			//录像状态显示，录像状态时显示为
			int ch = 0;
			for (ch = 0; ch < MAXVIEWCH; ch ++)
			{
				if(m_pDlgPageServer->IsWindowVisible() != 0 && g_DlgVideoView[ch].IsWindowVisible() != 0)
					if (g_DlgVideoView[ch].m_bRecordFlag&& m_bFullScreen != 1 && ch < m_FormsNum)
					{
						DrawRect(ch, 2);
					}
					if (m_bFullScreen != 1 && m_iFocuseWindowID != ch && ch < m_FormsNum && g_DlgVideoView[ch].IsWindowVisible() != 0)
					{
						DrawRect(ch, 0);
					}
			}

			//选中框显示
			if(m_pDlgPageServer->IsWindowVisible() != 0 && g_DlgVideoView[0].IsWindowVisible() != 0)
			{
				if (m_bFullScreen != 1 && m_iFocuseWindowID >= 0 && m_iFocuseWindowID < m_FormsNum)
				{
					DrawRect(m_iFocuseWindowID, 1);
				}
			}
		}
		else if (nIDEvent == TIMER_CYCLE_EVENTID)//轮巡
		{
			TuneCycleThread();
		}
		else if (nIDEvent == TIMER_WEATHER_EVENTID)//气象
		{
			ShowVideoWinWeatherCurrentInfoOnTimerByFlag();//定时显示气象数据
		}
		else if (nIDEvent == TIMER_YT_LOCK_CHECK_EVENTID)//云台锁定
		{
			CheckAndSetYtLockState();
		}
		else if (nIDEvent == TIMER_VIDEO_FLOW_EVENTID)//视频流量
		{
			CheckShowVideoFlow();
		}
		else if (nIDEvent == TIMER_SUBSCRIBE_CAMERA_STATUS_EVENTID)//订阅摄像头状态
		{
			KillTimer(TIMER_SUBSCRIBE_CAMERA_STATUS_EVENTID);
			StartSubscribeCameraPresenceInfo();
		}
		else if (nIDEvent == TIMER_CHECK_AND_UPDATE_SYSTEM_SOFTWARE_EVENTID)//检测并自动更新系统
		{
			CheckAndUpdateSystemSoftware();
		}
		else if (nIDEvent == TIMER_VIDEO_PAGEVIEW_AUTOINFO)//人工自动巡视
		{
			KillTimer(TIMER_VIDEO_PAGEVIEW_AUTOINFO);
			
			VideoPageAutoView();

			if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
			{
				SetTimer(TIMER_VIDEO_PAGEVIEW_AUTOINFO,g_VideoPageViewInfo.nTimeSpan*1000,NULL);
			}
		}
		else if (nIDEvent >= TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER&&nIDEvent < TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER+MAXVIEWCH)
		{
			//删除错误信息提示
			KillTimer(nIDEvent);

			int nVideoChannelIndex = (int)nIDEvent - TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER;
			
			if (CheckViewVideoIsOpen(nVideoChannelIndex) <= 0)
				StartViewChannelPrevVideoByIndex(nVideoChannelIndex);
		}
		else if (nIDEvent >= TIMER_WEATHER_CHANNEL_0_TIMER&&nIDEvent < TIMER_WEATHER_CHANNEL_0_TIMER+MAXVIEWCH)
		{
			//定时请求气象数据
			KillTimer(nIDEvent);

			int nChannelIndex = nIDEvent - TIMER_WEATHER_CHANNEL_0_TIMER;

			ThreadPoolDispatchTask(RequestCurrentWeatherInfo,&nChannelIndex,sizeof(nChannelIndex),1);

			SetTimer(nIDEvent,1000*60*5,NULL);
		}
		else if (nIDEvent >= TIMER_MICRO_WEATHER_CHANNEL_0_TIMER&&nIDEvent < TIMER_MICRO_WEATHER_CHANNEL_0_TIMER+MAXVIEWCH)
		{
			//定时请求微气象数据
			KillTimer(nIDEvent);

			int nChannelIndex = nIDEvent - TIMER_MICRO_WEATHER_CHANNEL_0_TIMER;

			ThreadPoolDispatchTask(RequestCurentMicroWeatherInfo,&nChannelIndex,sizeof(nChannelIndex),1);

			SetTimer(nIDEvent,1000*60*5,NULL);
		}
		else if (nIDEvent >= TIMER_HISTORY_WEATHER_CHANNEL_0_TIMER&&nIDEvent < TIMER_HISTORY_WEATHER_CHANNEL_0_TIMER+MAXVIEWCH)
		{
			//定时请求气象数据
			KillTimer(nIDEvent);

			int nChannelIndex = nIDEvent - TIMER_HISTORY_WEATHER_CHANNEL_0_TIMER;

			ThreadPoolDispatchTask(RequestHistoryWeatherInfo,&nChannelIndex,sizeof(nChannelIndex),1);

			SetTimer(nIDEvent,1000*60*5,NULL);
		}
		else if (nIDEvent >= TIMER_VIDEO_INFO_SHOW_CHANNEL_0_TIMER&&nIDEvent < TIMER_VIDEO_INFO_SHOW_CHANNEL_0_TIMER+MAXVIEWCH)
		{
			//显示视频信息对话框
			KillTimer(nIDEvent);

			int nVideoInfoChannel = nIDEvent - TIMER_VIDEO_INFO_SHOW_CHANNEL_0_TIMER;

			ModifyVideoInfoDlgWithView(nVideoInfoChannel);
		}
		else if (nIDEvent == TIMER_ACSSYSTEM_LOAD_DLL)
		{
			//加载辅助系统的dll
			KillTimer(nIDEvent);

			//设置辅助设备监控dll的一些设置
			ACSSystemConfig();

			//辅助联动配置管理dll的一些设置
			LinkageSettingConfig();

			//启动辅助系统客户端
			InitAssClient();
			g_user_config_info.nAcsTcpHandle = g_pACSClient->m_hClientTcpHandle;
		}
	}
	catch(...)
	{

	}

	CDialog::OnTimer(nIDEvent);
}

void CVEMCUCtlDlg::OnMenuItemShow()
{
	ShowWindow(SW_SHOW);

	if (m_nMenuItemIndex == VEM_WIN_ITEM_TYPE_MULTIVIEW)
	{
		if(m_bPageServerFlag == FALSE)
		{
			if (m_pDlgShowPageServer != NULL)
				m_pDlgShowPageServer->ShowWindow(SW_SHOW);
		}

		DisplayShowLinkageServerByFlag(m_bLinkageServerFlag);
	}

	if (m_bMenuFlag == FALSE)
	{
		if (m_pDlgShowControl != NULL)
			m_pDlgShowControl->ShowWindow(SW_SHOW);
	}

	for(int i = 0;i < MAXVIEWCH;i++)
	{
		if (g_VideoInfoDlg[i].m_bVideoInfoShowFlag
			||g_VideoInfoDlg[i].m_OneArrowLine.m_bShowFlag
			||g_VideoInfoDlg[i].m_TwoArrowLine.m_bShowFlag)
		{
			g_VideoInfoDlg[i].ShowWindow(SW_SHOW);
		}

		if (g_VideoTagInfoDlg[i].m_bVideoInfoShowFlag)
		{
			g_VideoTagInfoDlg[i].ShowWindow(SW_SHOW);
		}
	}

	SendMessage(WM_SYSCOMMAND,SC_RESTORE);

	for (int i = 0;i < m_FormsNum;i++)
	{
		ShowVideoInfoDlgByFlag(i);
		DrawRect(i,g_DlgVideoView[i].m_bRecordFlag);
	}
}

void CVEMCUCtlDlg::OnMenuItemExit()
{
	ExitDlg();
}

//轮巡
void CVEMCUCtlDlg::TuneCycle()
{
	KillTimer(TIMER_CYCLE_EVENTID);

	int i = 0;
	int nResult = 0;

	for (i = 0; i < m_FormsNum; i++)
	{
		if (g_TuneCycle_MehtodInfo.bState == false)
			return;

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
			return;

		//取消断线二次重连
		KillTimer(TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER+i);

		// 挂断当前的视频
		nResult = CheckViewVideoIsOpen(i);
		if (nResult > 0)
		{
			CloseViewVideoByWinID(i,nResult);
		}

		// 查找当前轮巡列表中没有打开视频逐一打开
		if (g_TuneCycle_MehtodInfo.CurrentID == g_TuneCycle_MehtodInfo.TotalCameraNum)
		{
			// 已经是最后一个摄像头，播放第一个摄像头
			if (g_TuneCycle_MehtodInfo.TotalCameraNum <= m_FormsNum)
				break;

			g_TuneCycle_MehtodInfo.CurrentID = 0;
		}

		Sleep(300);

		char c_devtype[4]={0};
		int  i_devtype = 0;
		char showname[256] = {0};
		memset(showname, 0, sizeof(showname));

		c_devtype[0] = g_TuneCycle_MehtodInfo.CameraNum[g_TuneCycle_MehtodInfo.CurrentID][12];
		c_devtype[1] = g_TuneCycle_MehtodInfo.CameraNum[g_TuneCycle_MehtodInfo.CurrentID][13];

		i_devtype = atoi(c_devtype);

		if (i_devtype == 3 || i_devtype == 7 || i_devtype >= 8)
		{
			g_DlgVideoView[i].m_bYTControl = false;
			sprintf(showname, "{轮巡}-{%s}-{%s }(不可控)", g_TuneCycle_MehtodInfo.CameraName[g_TuneCycle_MehtodInfo.CurrentID],g_TuneCycle_MehtodInfo.MethodName);
		}
		else
		{
			g_DlgVideoView[i].m_bYTControl = true;
			sprintf(showname, "{轮巡}-{%s}-{%s}(可控)", g_TuneCycle_MehtodInfo.CameraName[g_TuneCycle_MehtodInfo.CurrentID],g_TuneCycle_MehtodInfo.MethodName);
		}

		g_DlgVideoView[i].m_VideoViewOutInfo.Lock();
		g_DlgVideoView[i].m_VideoViewOutInfo.m_nVideoViewType = 1;   // 通过南瑞平台呼叫
		strcpy(g_DlgVideoView[i].m_VideoViewOutInfo.m_szCameraCallNum, g_TuneCycle_MehtodInfo.CameraNum[g_TuneCycle_MehtodInfo.CurrentID]);
		strcpy(g_DlgVideoView[i].m_VideoViewOutInfo.m_szCameraName, g_TuneCycle_MehtodInfo.CameraName[g_TuneCycle_MehtodInfo.CurrentID]);
		g_DlgVideoView[i].m_VideoViewOutInfo.m_nDvrType = g_TuneCycle_MehtodInfo.CameraType[g_TuneCycle_MehtodInfo.CurrentID];
		g_DlgVideoView[i].m_VideoViewOutInfo.m_nCameraStatus = 1;	     // 摄像头状态
		g_DlgVideoView[i].m_VideoViewOutInfo.m_nCameraCallStatus  = VM_VIEW_OUT_CAMERA_CALL_STATUS_CALLING; //正在打开
		strcpy(g_DlgVideoView[i].m_VideoViewOutInfo.m_szNotes,showname);
		g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();

		if (AppVideoMakeCallByPlatform(i) < 0)
		{
			g_DlgVideoView[i].m_strStation.SetText("离线");
			g_DlgVideoView[i].m_VideoViewOutInfo.Clear();
			g_TuneCycle_MehtodInfo.CurrentID++;
			continue;
		}
		else
		{
			g_DlgVideoView[i].m_strStation.SetText("发起呼叫成功");
		}

		g_DlgVideoView[i].m_Pic.m_video_opened = true;
		g_DlgVideoView[i].m_strStation.SetTransparent(TRUE);					//设置背景透明色
		g_DlgVideoView[i].m_strStation.SetTextColor(RGB(23, 92, 85));		//设置字体颜色

		g_DlgVideoView[i].m_strStation.SetText(showname);

		g_TuneCycle_MehtodInfo.CurrentID++;
	}

	if (g_TuneCycle_MehtodInfo.bState == true)
		SetTimer(TIMER_CYCLE_EVENTID, g_TuneCycle_MehtodInfo.TimeInt*1000, NULL);
}

 //轮巡处理
void	CVEMCUCtlDlg::TuneCycleThread()
{
	KillTimer(TIMER_CYCLE_EVENTID);

	int i = 0;
	int nResult = 0;
	HTREEITEM hTreeItem = NULL;
	_T_NODE_INFO * pNodeInfo  = NULL;

	char szCameraNum[32] = {0};
	memset(szCameraNum,0,sizeof(szCameraNum));

	VIDEO_SIP_CALL_SERVER_INFO VideoSipCallServerInfo;
	memset(&VideoSipCallServerInfo,0,sizeof(VideoSipCallServerInfo));

	for (i = 0; i < m_FormsNum; i++)
	{
		if (g_TuneCycle_MehtodInfo.bState == false)
			return;

		if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
			return;

		// 查找当前轮巡列表中没有打开视频逐一打开
		if (g_TuneCycle_MehtodInfo.CurrentID == g_TuneCycle_MehtodInfo.TotalCameraNum)
		{
			// 已经是最后一个摄像头，播放第一个摄像头
			if (g_TuneCycle_MehtodInfo.TotalCameraNum <= m_FormsNum)
				break;

			g_TuneCycle_MehtodInfo.CurrentID = 0;
		}

		//////////////////////////////////////////////////////////////////////////
		memset(szCameraNum,0,sizeof(szCameraNum));
		memset(&VideoSipCallServerInfo,0,sizeof(VideoSipCallServerInfo));

		strcpy_s(szCameraNum,sizeof(szCameraNum)-1,g_TuneCycle_MehtodInfo.CameraNum[g_TuneCycle_MehtodInfo.CurrentID]);

		hTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szCameraNum,NULL,NULL);
		if (hTreeItem != NULL)
		{
			pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hTreeItem);
			if (pNodeInfo != NULL)
			{
				VideoSipCallServerInfo.nStatus = pNodeInfo->node_status;
				strcpy_s(VideoSipCallServerInfo.szCode,sizeof(VideoSipCallServerInfo.szCode),pNodeInfo->node_num);
				VideoSipCallServerInfo.nScreenId = i;
				strcpy_s(VideoSipCallServerInfo.szName,sizeof(VideoSipCallServerInfo.szName),pNodeInfo->node_name);
				VideoSipCallServerInfo.hItem = hTreeItem;
				strcpy_s(VideoSipCallServerInfo.szStation,sizeof(VideoSipCallServerInfo.szStation),pNodeInfo->node_station);
				sprintf_s(VideoSipCallServerInfo.szReason,sizeof(VideoSipCallServerInfo.szReason),"{自动轮巡}-{%s}-{%s}",g_TuneCycle_MehtodInfo.CameraName[g_TuneCycle_MehtodInfo.CurrentID],g_TuneCycle_MehtodInfo.MethodName);

				memcpy(&VideoSipCallServerInfo.server_node_info,pNodeInfo,sizeof(VideoSipCallServerInfo.server_node_info));

				if (IsCameraVideoLinkByVideoPlatform(g_nClientVideoLinkType,pNodeInfo->node_decodetag))
				{
					VideoSipCallServerInfo.nType = 1;//通过南瑞平台
					VideoSipCallServerInfo.nDecodeTag = pNodeInfo->node_decodetag;
				}
				else
				{
					VideoSipCallServerInfo.nType = 2;//直接连接前端设备
					VideoSipCallServerInfo.nDecodeTag = pNodeInfo->camera_info.dvr_info.dvr_type;
				}

				g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
				g_pMainDlg->ThreadPoolDispatchTask(ThreadMakeCallCameraServer,(void *)&VideoSipCallServerInfo,sizeof(VideoSipCallServerInfo),2);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		g_TuneCycle_MehtodInfo.CurrentID++;
	}

	if (g_TuneCycle_MehtodInfo.bState == true)
		SetTimer(TIMER_CYCLE_EVENTID, g_TuneCycle_MehtodInfo.TimeInt*1000, NULL);
}

//打开当前手动轮巡页视频
void CVEMCUCtlDlg::OpenCurPageVideo(HTREEITEM hTreeCameraItem)
{
	try
	{
		if (g_TuneCycle_MehtodInfo.bState == true)
		{
			MessageBox("当前处于轮巡状态","视频预览",MB_ICONWARNING);
			return;
		}

		int i = 0;
		int nResult = 0;
		BOOL bResult = FALSE;

		_T_NODE_INFO *pNodeInfo = NULL;
		HTREEITEM hTreeItem = hTreeCameraItem;

		if (m_pDlgPageServer == NULL)
			return;

		VIDEO_SIP_CALL_SERVER_INFO VideoSipCallServerInfo;

		for (i = 0; i < m_FormsNum; i++)
		{
			if (g_TuneCycle_MehtodInfo.bState == true)
				return;

			if (hTreeItem == NULL)
			{
				nResult = CheckViewVideoIsOpen(i);
				if (nResult > 0)
				{
					CloseViewVideoByWinID(i,nResult);
				}
				continue;
			}
				
			pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hTreeItem);
			if (pNodeInfo == NULL||pNodeInfo->node_type != 3)
			{
				hTreeItem = m_pDlgPageServer->m_trServer.GetNextSiblingItem(hTreeItem);
				continue;
			}

			//////////////////////////////////////////////////////////////////////////
			int nOperate_type = WM_DEVICE_VIDEO_OPERATE_RESULT_NOTAG_ONLINE_VIDEO;
			int nOperate_result = WM_DEVICE_VIDEO_OPERATE_RESULT_NOTAG_ONLINE_VIDEO;

			if (pNodeInfo->node_status == 0)
			{
				nOperate_type = WM_DEVICE_VIDEO_OPERATE_RESULT_NOTAG_OFFLINE_VIDEO;
				nOperate_result = WM_DEVICE_VIDEO_OPERATE_RESULT_NOTAG_OFFLINE_VIDEO;
			}
			else if (pNodeInfo->node_status == 2)
			{
				nOperate_type = WM_DEVICE_VIDEO_OPERATE_RESULT_TAG_ONLINE_VIDEO;
				nOperate_result = WM_DEVICE_VIDEO_OPERATE_RESULT_TAG_ONLINE_VIDEO;
			}
			else if (pNodeInfo->node_status == 3)
			{
				nOperate_type = WM_DEVICE_VIDEO_OPERATE_RESULT_TAG_OFFLINE_VIDEO;
				nOperate_result = WM_DEVICE_VIDEO_OPERATE_RESULT_TAG_OFFLINE_VIDEO;
			}

			m_pDlgPageServer->WriteUserClientVideoOperateInfo(pNodeInfo->node_num,pNodeInfo->node_name,pNodeInfo->node_station,nOperate_type,nOperate_result);
			//////////////////////////////////////////////////////////////////////////

			memset(&VideoSipCallServerInfo,0,sizeof(VideoSipCallServerInfo));

			if (IsCameraVideoLinkByVideoPlatform(g_nClientVideoLinkType,pNodeInfo->node_decodetag))//客户端视频连接方式
			{
				VideoSipCallServerInfo.nType = 1;//通过南瑞平台
				VideoSipCallServerInfo.nDecodeTag = pNodeInfo->node_decodetag;
			}
			else
			{
				VideoSipCallServerInfo.nType = 2;//直接连接前端设备
				VideoSipCallServerInfo.nDecodeTag = pNodeInfo->camera_info.dvr_info.dvr_type;
			}

			VideoSipCallServerInfo.nStatus = pNodeInfo->node_status;
			strcpy_s(VideoSipCallServerInfo.szCode,sizeof(VideoSipCallServerInfo.szCode),pNodeInfo->node_num);
			VideoSipCallServerInfo.nScreenId = i;
			strcpy_s(VideoSipCallServerInfo.szName,sizeof(VideoSipCallServerInfo.szName),pNodeInfo->node_name);
			VideoSipCallServerInfo.hItem = hTreeItem;
			strcpy_s(VideoSipCallServerInfo.szStation,sizeof(VideoSipCallServerInfo.szStation),pNodeInfo->node_station);
			sprintf(VideoSipCallServerInfo.szReason,"{人工巡视}-{%s}-{%s}", pNodeInfo->node_station,pNodeInfo->node_name);

			memcpy(&VideoSipCallServerInfo.server_node_info,pNodeInfo,sizeof(VideoSipCallServerInfo.server_node_info));

			g_VMLog.WriteVmLog("OpenCurPageVideo-----------VideoSipCallServerInfo.nType = %d,VideoSipCallServerInfo.nDecodeTag = %d",VideoSipCallServerInfo.nType,VideoSipCallServerInfo.nDecodeTag);

			g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
			g_pMainDlg->ThreadPoolDispatchTask(ThreadMakeCallCameraServer,(void *)&VideoSipCallServerInfo,sizeof(VideoSipCallServerInfo),2);
			
			hTreeItem = m_pDlgPageServer->m_trServer.GetNextSiblingItem(hTreeItem);
		}
	}
	catch(...)
	{

	}
}


/************************************************************************
*视频通道窗口变化
*鼠标点击某个窗口
*wParam / 0x1000 : 焦点窗口
*lParam : 焦点窗口
************************************************************************/
LRESULT CVEMCUCtlDlg::OnChannelChange(WPARAM wParam, LPARAM lParam)
{
	int iYTID = wParam % 0x1000;
	wParam = wParam / 0x1000;

	if (wParam > MAXVIEWCH || wParam < 0)
	{
		return 0;
	}

	if (m_iFocuseWindowID != (int)wParam)
	{
		CRect rc;
		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < m_FormsNum)
		{
			DrawRect(m_iFocuseWindowID, 0);
		}

		if ((wParam >= 0) && (wParam < MAXVIEWCH))
			m_iFocuseWindowID = wParam;
		else
			m_iFocuseWindowID = 0;

		DrawRect(m_iFocuseWindowID, 1);
	}

	CheckAndSetYtLockState();//检测并设置云台锁定状态

	return 1;
}


/************************************************************************
*   OnDBLChannel 对某个视频窗口双击操作, 1为界面全屏，2为显示屏全屏，0为退出所有全屏
************************************************************************/
LRESULT CVEMCUCtlDlg::OnDBLChannel(WPARAM wParam, LPARAM lParam)
{
	if (wParam > MAXVIEWCH || wParam < 0)
	{
		return 0;
	}

	if (m_bMultiFullScreen != FALSE)//多画面全屏
	{
		return 1;
	}

	if (m_nFullScreen < 2)
	{
		m_nFullScreen++;
	}
	else
	{
		m_nFullScreen = 0;
	}

	m_bFullScreen = !m_bFullScreen;
	
	int i = 0;

	if (m_nFullScreen == 0)//退出全屏
	{
		SetWindowPlacement(&m_OldWndpl);

		for ( i = 0; i < m_iPreviewNum; i++)
		{
			if (i == (int)wParam)
			{
				g_DlgVideoView[i].SetWindowPlacement(&m_chWndpl);
				ModifyVideoInfoDlgWithView(i);
				SetTimer(TIMER_VIDEO_INFO_SHOW_CHANNEL_0_TIMER+i,300,NULL);//延时显示视频信息
			}
			else
			{
				g_DlgVideoView[i].ShowWindow(SW_SHOW);
				ShowVideoInfoDlgByFlag(i);
			}
		}

		if (m_bMenuFlag != FALSE)
		{
			m_pDlgMenu->ShowWindow(SW_SHOW);
			m_pDlgShowControl->ShowWindow(SW_HIDE);
		}
		else
		{
			m_pDlgMenu->ShowWindow(SW_HIDE);
			m_pDlgShowControl->ShowWindow(SW_SHOW);
		}

		if (m_bPageServerFlag != FALSE)
		{
			m_pDlgPageServer->ShowWindow(SW_SHOW);
			m_pDlgShowPageServer->ShowWindow(SW_HIDE);
		}
		else
		{
			m_pDlgPageServer->ShowWindow(SW_HIDE);
			m_pDlgShowPageServer->ShowWindow(SW_SHOW);
		}

		DisplayLinkageServerRelationListByFlag(m_bLinkageServerFlag);

		if (m_bPageViewInfoFlag != FALSE)
		{
			m_pDlgPageViewInfo->ShowWindow(SW_SHOW);
		}
		else
		{
			m_pDlgPageViewInfo->ShowWindow(SW_HIDE);
		}

		DrawRect(m_iFocuseWindowID, 1);
	}
	else if(m_nFullScreen == 1)//放大显示当前窗口
	{
		GetWindowPlacement(&m_OldWndpl);
		g_DlgVideoView[wParam].GetWindowPlacement(&m_chWndpl);

		//设置nRows*nCols 显示方格
		RECT rc;
		GetClientRect(&rc);

		int iFrameWidth = 2;	//外界边框
		rc.left += iFrameWidth;
		rc.top += iFrameWidth;

		RECT	rect;
		rect.top = iFrameWidth + m_nMenuHight + m_nPageViewInfoHeight;
		rect.bottom = rc.bottom - ACCEHIGHT;
		rect.left = iFrameWidth + 2 +m_nControlWidth;
		rect.right = rc.right - 2 - m_nLinkageWidth;

		for ( i = 0; i < m_iPreviewNum; i++)
		{
			if (i == (int)wParam)
			{
				//移动窗口到指定位置
				g_DlgVideoView[i].MoveWindow(&rect, TRUE);
				ModifyVideoInfoDlgWithView(i);
			}
			else
			{
				//隐藏窗口
				g_DlgVideoView[i].ShowWindow(SW_HIDE);
				g_VideoInfoDlg[i].ShowWindow(SW_HIDE);
				g_VideoTagInfoDlg[i].ShowWindow(SW_HIDE);
			}
		}
	}
	else // 全屏
	{
		m_pDlgMenu->ShowWindow(SW_HIDE);
		m_pDlgShowControl->ShowWindow(SW_HIDE);
		m_pDlgPageServer->ShowWindow(SW_HIDE);
		m_pDlgShowPageServer->ShowWindow(SW_HIDE);
		m_pDlgLinkageServer->ShowWindow(SW_HIDE);
		m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);
		m_pDlgPageViewInfo->ShowWindow(SW_HIDE);

		//得到窗口大小
		RECT rc1;
		HWND hwnd1 = ::GetDesktopWindow();
		::GetClientRect(hwnd1, &rc1);

		MoveWindow(CRect(0, 0, rc1.right, rc1.bottom));
		RECT rc;
		GetClientRect(&rc);

		for ( i = 0; i < m_iPreviewNum; i++)
		{
			if (i == (int)wParam)
			{
				//移动窗口到指定位置
				g_DlgVideoView[wParam].MoveWindow(&rc1, TRUE);
				ModifyVideoInfoDlgWithView(i);
			}
			else
			{
				//隐藏窗口
				g_DlgVideoView[i].ShowWindow(SW_HIDE);
				g_VideoInfoDlg[i].ShowWindow(SW_HIDE);
				g_VideoTagInfoDlg[i].ShowWindow(SW_HIDE);
			}
		}
	}

	CheckAndSetYtLockState();//设置云台锁定状态

	return 1;
}


/************************************************************************
* 右键单击，弹出菜单
************************************************************************/
LRESULT CVEMCUCtlDlg::OnRightClickChannel(WPARAM wParam, LPARAM lParam)
{
	if (wParam > MAXVIEWCH || wParam < 0)
		return 0;

	int iWinID = (int)wParam;//窗口标识

	if (iWinID >= MAXVIEWCH || iWinID < 0)
		return 1;

	if (m_iFocuseWindowID != (int)iWinID)
	{
		CRect rc;
		if (m_iFocuseWindowID >= 0 && m_iFocuseWindowID < m_FormsNum)
		{
			DrawRect(m_iFocuseWindowID, 0);
		}

		if ((iWinID >= 0) && (iWinID < MAXVIEWCH))
			m_iFocuseWindowID = iWinID;
		else
			m_iFocuseWindowID = 0;

		DrawRect(m_iFocuseWindowID, 1);
	}

	CheckAndSetYtLockState();//设置云台锁定状态

	CMenu  popMenu;

	if(CheckMenuVideoTagIsDisplay(iWinID) != FALSE)
		popMenu.LoadMenu(IDR_MENU_VIDEO_TAG);
	else
		popMenu.LoadMenu(IDR_MENU_MAINPOP);

	CMenu *pMenu = popMenu.GetSubMenu(0); 

	if (pMenu == NULL)
		return 1;

	CPoint posMouse;
	GetCursorPos(&posMouse);

	if (g_TuneCycle_MehtodInfo.bState == true)
	{
		pMenu->EnableMenuItem(ID_MENUITEMEXITVIDEO,MF_BYCOMMAND|MF_GRAYED);
		pMenu->EnableMenuItem(ID_MENU_STOP_ALL_VIDEO,MF_BYCOMMAND|MF_GRAYED);
		pMenu->EnableMenuItem(ID_MENUITEMSTARTREC,MF_BYCOMMAND|MF_GRAYED);
		pMenu->EnableMenuItem(ID_MENUITEMSTOPREC,MF_BYCOMMAND|MF_GRAYED);
		pMenu->EnableMenuItem(ID_MENUITEMCAPTURE,MF_BYCOMMAND|MF_GRAYED);
	}

	pMenu->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RIGHTALIGN,  posMouse.x, posMouse.y, this); 

	return 1;
}

/***********************************************************************
*			通过视频区域菜单  退出视频
************************************************************************/
void CVEMCUCtlDlg::OnMenuItemExitVideo() 
{
	ThreadPoolDispatchTask(ThreadStopCurrentVideo,(void *)&m_iFocuseWindowID,sizeof(m_iFocuseWindowID),4);
	Sleep(100);

	if (m_iFocuseWindowID >= 0&&m_iFocuseWindowID < MAXVIEWCH)
		DrawRect(m_iFocuseWindowID,g_DlgVideoView[m_iFocuseWindowID].m_bRecordFlag);
}

/***********************************************************************
*			通过视频区域菜单  退出全部视频
************************************************************************/
void CVEMCUCtlDlg::OnMenuStopAllVideo()
{
	ThreadPoolDispatchTask(ThreadStopAllVideo,(void *)&m_iFocuseWindowID,sizeof(m_iFocuseWindowID),4);
	Sleep(100);

	for (int i = 0;i < m_FormsNum;i++)
	{
		DrawRect(i,g_DlgVideoView[i].m_bRecordFlag);
	}
}

/***********************************************************************
*云台操作
* 参数定义：
* WPARAM iCommandID, 
*  LPARAM iMove
*
iCommandID

1 -云台上
2 -云台左
3 -云台右
4 -云台下

5 -镜头拉远
6 -镜头拉近

11 - 调用预置点
12 - 保存预置点

255 -云台锁定

************************************************************************/
LRESULT CVEMCUCtlDlg::StartYtControl(WPARAM iCommandID, LPARAM iMove)
{
	if (g_TuneCycle_MehtodInfo.bState == true)
		return 0;

	if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
		return 0;

	int i = 0;

	if (m_iFocuseWindowID < 0||m_iFocuseWindowID >= MAXVIEWCH)
		return 0;

	char szCameraCallNum[64] = {0};
	char szCameraName[256] = {0};
	char szStationName[256] = {0};

	memset(szCameraCallNum,0,sizeof(szCameraCallNum));
	memset(szCameraName,0,sizeof(szCameraName));
	memset(szStationName,0,sizeof(szStationName));

	g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.Lock();
	strcpy_s(szCameraCallNum,sizeof(szCameraCallNum),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szCameraCallNum);
	strcpy_s(szCameraName,sizeof(szCameraName),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szCameraName);
	strcpy_s(szStationName,sizeof(szStationName),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szStationName);
	g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.UnLock();

	if (strlen(szCameraCallNum) == 0||strcmp(szCameraCallNum, "00000") == 0)
		return 0;

	int ispeed = 0;
	ispeed = m_pDlgPageServer->m_Combo_YTSpeed.GetCurSel() + 1;

	char szCommandName[128] = {0};

	if (strlen(szCameraCallNum) > 13)
	{
		//发送事务到GW
		int ytcommand = 0;
		char str_buf[1024] = {0};
		memset(str_buf, 0, sizeof(str_buf));

		TRACE("-----iMove=%d,iCommandID=%d------\n\n",iMove,iCommandID);
		if(iMove == 0)
		{
			switch (iCommandID)
			{
			case 1://云台控制上-结束
				{
					ytcommand = 0x0401;
					strcpy_s(szCommandName,"云台控制上-结束");
				}
				break;
			case 2://云台控制下-结束
				{
					ytcommand = 0x0403;
					strcpy_s(szCommandName,"云台控制下-结束");
				}
				break;
			case 3://云台控制左-结束
				{
					ytcommand = 0x0503;
					strcpy_s(szCommandName,"云台控制左-结束");
				}
				break;
			case 4://云台控制右-结束
				{
					ytcommand = 0x0501;
					strcpy_s(szCommandName,"云台控制右-结束");
				}
				break;
			case 5://云台控制调焦缩-结束
				{
					//ytcommand = 0x0201;
					//strcpy_s(szCommandName,"云台控制调焦缩-结束");
//					ytcommand = 0x0303;
//					ytcommand = 0x0301;
					ytcommand = 0x0401;
					strcpy_s(szCommandName,"云台控制焦距远-结束");
				}
				break;
			case 6://云台控制调焦伸-结束
				{
					//ytcommand = 0x0203;
					//strcpy_s(szCommandName,"云台控制调焦伸-结束");
//					ytcommand = 0x0301;
//					ytcommand = 0x0303;
					ytcommand = 0x0401;
					strcpy_s(szCommandName,"云台控制焦距近-结束");
				}
				break;
			case 7://云台控制焦距近-结束
				{
				//	ytcommand = 0x0303;
				//	strcpy_s(szCommandName,"云台控制焦距远-结束");
					ytcommand = 0x0301;
					strcpy_s(szCommandName,"云台控制焦距近-结束");
				}
				break;
			case 8://云台控制焦距远-结束
				{
					//ytcommand = 0x0301;
					//strcpy_s(szCommandName,"云台控制焦距近-结束");
					ytcommand = 0x0303;
					strcpy_s(szCommandName,"云台控制焦距远-结束");
				}
				break;
			case 9://云台控制光圈小-结束
				{
					ytcommand = 0x0101;
					strcpy_s(szCommandName,"云台控制光圈小-结束");
				}
				break;
			case 10://云台控制光圈大-结束
				{
					ytcommand = 0x0104;
					strcpy_s(szCommandName,"云台控制光圈大-结束");
				}
				break;
			case 11://红外开
				{
					ytcommand = 0x0d01;
					strcpy_s(szCommandName,"红外开");
				}
				break;
			case 12://红外关
				{
					ytcommand = 0x0d02;
					strcpy_s(szCommandName,"红外关");
				}
				break;
			case 13://加热开
				{
					ytcommand = 0x0c01;
					strcpy_s(szCommandName,"加热开");
				}
				break;
			case 14:// 加热关
				{
					ytcommand = 0x0c02;
					strcpy_s(szCommandName,"加热关");
				}
				break;
			case 15:// 灯光开
				{
					ytcommand = 0x0b01;
					strcpy_s(szCommandName,"灯光开");
				}
				break;
			case 16:// 灯光关
				{
					ytcommand = 0x0b02;
					strcpy_s(szCommandName,"灯光关");
				}
				break;
			case 17:// 雨刷开
				{
					ytcommand = 0x0a01;
					strcpy_s(szCommandName,"雨刷开");
				}
				break;
			case 18:// 雨刷关
				{
					ytcommand = 0x0a02;
					strcpy_s(szCommandName,"雨刷关");
				}
				break;
			case 19:
				{
					return 0;
					//ytcommand = 0x1102;
					//strcpy_s(szCommandName,"云台解锁");
				}
				break;
			}

			m_nYtCommand = 0;
			memset(m_szYtCommandName,0,sizeof(m_szYtCommandName));
		}
		else
		{
			switch (iCommandID)
			{
			case 1://云台控制上-开始
				{
					ytcommand = 0x0402;
					strcpy_s(szCommandName,"云台控制上-开始");
				}
				break;
			case 2://云台控制下-开始
				{
					ytcommand = 0x0404;
					strcpy_s(szCommandName,"云台控制下-开始");
				}
				break;
			case 3://云台控制左-开始
				{
					ytcommand = 0x0504;
					strcpy_s(szCommandName,"云台控制左-开始");
				}
				break;
			case 4://云台控制右-开始
				{
					ytcommand = 0x0502;
					strcpy_s(szCommandName,"云台控制右-开始");
				}
				break;
			case 5://云台控制调焦缩-开始
				{
					//ytcommand = 0x0202;
					//strcpy_s(szCommandName,"云台控制调焦缩-开始");
//					ytcommand = 0x0304;
					ytcommand = 0x0302;
					strcpy_s(szCommandName,"云台控制焦距远-开始");
					//ytcommand = 0x0302;
					//strcpy_s(szCommandName,"云台控制焦距近-开始");
				}
				break;
			case 6://云台控制调焦伸-开始
				{
					//ytcommand = 0x0204;
					//strcpy_s(szCommandName,"云台控制调焦伸-开始");
				/*	ytcommand = 0x0304;
					strcpy_s(szCommandName,"云台控制焦距远-开始");*/
//					ytcommand = 0x0302;
					ytcommand = 0x0304;
					strcpy_s(szCommandName,"云台控制焦距近-开始");
				}
				break;
			case 7://云台控制焦距近-开始
				{
					ytcommand = 0x0302;
					strcpy_s(szCommandName,"云台控制焦距近-开始");
				}
				break;
			case 8://云台控制焦距远-开始
				{
					ytcommand = 0x0304;
					strcpy_s(szCommandName,"云台控制焦距远-开始");
				}
				break;
			case 9://云台控制光圈小-开始
				{
					ytcommand = 0x0102;
					strcpy_s(szCommandName,"云台控制光圈小-开始");
				}
				break;
			case 10://云台控制光圈大-开始
				{
					ytcommand = 0x0103;
					strcpy_s(szCommandName,"云台控制光圈大-开始");
				}
				break;
			case 11:// 红外开
				{
					ytcommand = 0x0d01;
					strcpy_s(szCommandName,"红外开");
				}
				break;
			case 12:// 红外关
				{
					ytcommand = 0x0d02;
					strcpy_s(szCommandName,"红外关");
				}
				break;
			case 13:// 加热开
				{
					ytcommand = 0x0c01;
					strcpy_s(szCommandName,"加热开");
				}
				break;
			case 14:// 加热关
				{
					ytcommand = 0x0c02;
					strcpy_s(szCommandName,"加热关");
				}
				break;
			case 15:// 灯光开
				{
					ytcommand = 0x0b01;
//					strcpy_s(szCommandNasme,"灯光开");
				}
				break;
			case 16:// 灯光关
				{
					ytcommand = 0x0b02;
					strcpy_s(szCommandName,"灯光关");
				}
				break;
			case 17:// 雨刷开
				{
					ytcommand = 0x0a01;
					strcpy_s(szCommandName,"雨刷开");
				}
				break;
			case 18:// 雨刷关
				{
					ytcommand = 0x0a02;
					strcpy_s(szCommandName,"雨刷关");
				}
				break;
			case 19:
				{
					//ytcommand = 0x1101;
					//strcpy_s(szCommandName,"云台锁定");

					if (m_nYtCommand != 0)
					{
						ytcommand = m_nYtCommand;
						strcpy_s(szCommandName,sizeof(szCommandName),m_szYtCommandName);

						m_nYtCommand = 0;
						memset(m_szYtCommandName,0,sizeof(m_szYtCommandName));
					}
					else
					{
						ytcommand = 0x0401;
						strcpy_s(szCommandName,"云台控制上-结束");
					}
				}
				break;
			}

			if (iCommandID != 19)
			{
				m_nYtCommand = ytcommand;
				strcpy_s(m_szYtCommandName,sizeof(m_szYtCommandName),szCommandName);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		char szInfo[1024] = {0};
		memset(szInfo,0,sizeof(szInfo));

		if (iCommandID <= 10)
		{
			if ((iCommandID == 5||iCommandID == 6)&&iMove == 0)
			{
				sprintf(szInfo, "<ctrlcamera user=\"admin\" station=\"%s\" name=\"%s\" code=\"%s\" CommandName=\"%s\" command=\"%d\" CommandPara1=\"0\" CommandPara2=\"0\" CommandPara3=\"0\" srcip=\"%s\"/>",
					szStationName,szCameraName,szCameraCallNum,szCommandName,ytcommand,app_StackCfg.szLocalUdpAddress);
			}
			else
			{
				sprintf(szInfo, "<ctrlcamera user=\"admin\" station=\"%s\" name=\"%s\" code=\"%s\" CommandName=\"%s\" command=\"%d\" CommandPara1=\"%d\" CommandPara2=\"5\" CommandPara3=\"0\" srcip=\"%s\"/>",
					szStationName,szCameraName,szCameraCallNum,szCommandName,ytcommand,m_pDlgPageServer->m_Combo_YTSpeed.GetCurSel()+1,app_StackCfg.szLocalUdpAddress);
			}
		}
		else
		{
			sprintf(szInfo, "<ctrlcamera user=\"admin\" station=\"%s\" name=\"%s\" code=\"%s\" CommandName=\"%s\" command=\"%d\" CommandPara1=\"0\" CommandPara2=\"0\" CommandPara3=\"0\" srcip=\"%s\"/>",
				szStationName,szCameraName,szCameraCallNum,szCommandName,ytcommand,app_StackCfg.szLocalUdpAddress);
		}

		/*int nRet = DvrPtzControl_Devsdk(g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szDvrIP,g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_nDvrPort,g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szDvrUserName,
			g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szDvrPassword,g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_nDvrType,g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_nDvrId,
			g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_nCameraChannel,ytcommand,m_pDlgPageServer->m_Combo_YTSpeed.GetCurSel()+1);*/

		//TRACE("-----DvrPtzControl_Devsdk:command=%d,nRet=%d------\n\n",ytcommand,nRet);
		SendVideoYTControlInfo(szInfo,strlen(szInfo));
		
		//////////////////////////////////////////////////////////////////////////

		return 0;
	}

	return 0;
}

//运行到预置点
int CVEMCUCtlDlg::MoveToPreSet()
{
	int i = 0;

	//聚焦的串口
	if(m_iFocuseWindowID < 0 || m_iFocuseWindowID >= MAXVIEWCH)
		return 0;

	////当前聚焦视频的call_num为m_display_callnum[m_iFocuseWindowID]
	////根据call_num\iCommandID\iMove对视频云台进行控制
	//if(strcmp(g_DlgVideoView[m_iFocuseWindowID].m_CameraCallNum, "00000") == 0)
	//	return 0;

	//char pszXML[1024] = {0};
	//sprintf(pszXML, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	//	"<SIP_XML EventType=\"Control_Camera\">\n"
	//	"<Item Code=\"%s\" Command=\"1538\" CommandPara1=\"%d\" CommandPara2=\"5\" CommandPara3=\"\"/>\n"
	//	"</SIP_XML>"
	//	,g_DlgVideoView[m_iFocuseWindowID].m_CameraCallNum,g_pMainDlg->m_pDlgPageServer->m_DlgPreSet->m_Combo_PreSet.GetCurSel()+1);
	//TRACE("%s\n", pszXML);

	////发送云台控制事务信息
	//AppMakeTrans(g_DlgVideoView[m_iFocuseWindowID].m_CameraCallNum, E_RT_METHOD_TYPE_MESSAGE, pszXML, -1);

	return 0;
}

//设置预置点
void CVEMCUCtlDlg::SetPreSet()
{
	int i = 0;

	if (g_TuneCycle_MehtodInfo.bState == true)
		return;

	if (g_VideoPageViewInfo.nStatus == AUTOVIDEO_PAGEVIEW_STATUS_RUNNING)
		return;

	//聚焦的串口
	if(m_iFocuseWindowID == -1 || m_iFocuseWindowID > 25)
		return;

	////当前聚焦视频的call_num为m_display_callnum[m_iFocuseWindowID]
	////根据call_num\iCommandID\iMove对视频云台进行控制
	//if(strcmp(g_DlgVideoView[m_iFocuseWindowID].m_CameraCallNum, "00000") == 0)
	//	return;

	//char pszXML[1000];
	//sprintf(pszXML, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	//	"<SIP_XML EventType=\"Control_Camera\">\n"
	//	"<Item Code=\"%s\" Command=\"1537\" CommandPara1=\"%d\" CommandPara2=\"5\" CommandPara3=\"\"/>\n"
	//	"</SIP_XML>"
	//	,g_DlgVideoView[m_iFocuseWindowID].m_CameraCallNum ,g_pMainDlg->m_pDlgPageServer->m_DlgPreSet->m_Combo_PreSet.GetCurSel()+1);


	////发送云台控制事务信息
	//AppMakeTrans(g_DlgVideoView[m_iFocuseWindowID].m_CameraCallNum, E_RT_METHOD_TYPE_MESSAGE, pszXML, -1);
}

//设置视频参数设置
void CVEMCUCtlDlg::SetViewSetting()
{
	//聚焦的串口
	if(m_iFocuseWindowID == -1 || m_iFocuseWindowID > 25)
	{
		return;
	}

	char szCameraCallNum[32] = {0};
	memset(szCameraCallNum,0,sizeof(szCameraCallNum));

	g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.Lock();
	strcpy_s(szCameraCallNum,sizeof(szCameraCallNum),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szCameraCallNum);
	g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.UnLock();

	if(strlen(szCameraCallNum) == 0||strcmp(szCameraCallNum, "00000") == 0)
	{
		return;
	}

	int res = g_DlgVideoView[m_iFocuseWindowID].SetCurrentViewParameter(
		m_pDlgPageServer->m_DlgVideoParSet->m_sliderbrite.GetPos()/2,
		m_pDlgPageServer->m_DlgVideoParSet->m_sliderduibi.GetPos()/2,
		m_pDlgPageServer->m_DlgVideoParSet->m_sliderbaohe.GetPos()/2,
		m_pDlgPageServer->m_DlgVideoParSet->m_slidersedu.GetPos()/2);
}

//默认视频参数设置
void CVEMCUCtlDlg::DefaultViewSetting()
{
	if(m_iFocuseWindowID < 0 || m_iFocuseWindowID >= MAXVIEWCH)
	{
		return;
	}

	char szCameraCallNum[32] = {0};
	memset(szCameraCallNum,0,sizeof(szCameraCallNum));

	g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.Lock();
	strcpy_s(szCameraCallNum,sizeof(szCameraCallNum),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szCameraCallNum);
	g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.UnLock();

	if(strlen(szCameraCallNum) == 0||strcmp(szCameraCallNum, "00000") == 0)
	{
		return;
	}

	int res = g_DlgVideoView[m_iFocuseWindowID].SetDefaultViewParameter();

	m_pDlgPageServer->m_DlgVideoParSet->m_sliderbrite.SetPos(64*2);
	m_pDlgPageServer->m_DlgVideoParSet->m_sliderduibi.SetPos(64*2);
	m_pDlgPageServer->m_DlgVideoParSet->m_sliderbaohe.SetPos(64*2);
	m_pDlgPageServer->m_DlgVideoParSet->m_slidersedu.SetPos(64*2);
}

//轮巡开始
void CVEMCUCtlDlg::TuneCycleStart()
{
	int i = 0;
	int nResult = -1;
	
	//关闭多画面
	for (int i = 0;i < MAXVIEWCH;i++)
	{
		nResult = CheckViewVideoIsOpen(i);
		if (nResult > 0)
		{
			CloseViewVideoByWinID(i,nResult);
		}
	}

	SetForms(g_TuneCycle_MehtodInfo.ViewNum);		//设置多画面

	Sleep(300);

	SetTimer(TIMER_CYCLE_EVENTID, 100, NULL);
}

//轮巡结束
void CVEMCUCtlDlg::TuneCycleEnd()
{
	g_TuneCycle_MehtodInfo.bState = false;

	KillTimer(TIMER_CYCLE_EVENTID);
}

void CVEMCUCtlDlg::OnMenuitemStartRec() 
{
	if (g_DlgVideoView[m_iFocuseWindowID].m_bRecordFlag)//表示已经正在录像
		return;

	g_DlgVideoView[m_iFocuseWindowID].OnBnClickedButtonRecordVo();
}

void CVEMCUCtlDlg::OnMenuitemStopRec() 
{
	if (!g_DlgVideoView[m_iFocuseWindowID].m_bRecordFlag)//表示已经停止录像
		return;

	g_DlgVideoView[m_iFocuseWindowID].OnBnClickedButtonRecordVo();
}

void CVEMCUCtlDlg::OnMenuitemCapture() 
{
	g_DlgVideoView[m_iFocuseWindowID].OnBnClickedButtonCaptureVo();
}

void CVEMCUCtlDlg::OnMenuitemMultifullScreen() 
{
	if (m_bMultiFullScreen == FALSE)
	{
		m_bMultiFullScreen = TRUE;

		m_pDlgMenu->ShowWindow(SW_HIDE);
		m_pDlgPageServer->ShowWindow(SW_HIDE);
		m_pDlgLinkageServer->ShowWindow(SW_HIDE);

		m_pDlgShowControl->ShowWindow(SW_HIDE);
		m_pDlgShowPageServer->ShowWindow(SW_HIDE);
		m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);

		InitPreviewWnd(m_FormsNum);

		m_pDlgPageViewInfo->ShowWindow(SW_HIDE);

		for (int i = 0;i < m_FormsNum;i++)
		{
			ModifyVideoInfoDlgWithView(i);
			ShowVideoInfoDlgByFlag(i);
		}
	}
	else
	{
		return;
	}
}

void CVEMCUCtlDlg::OnMenuitemExitmultifullScreen() 
{
	if (m_bMultiFullScreen == TRUE)
	{
		m_bMultiFullScreen = FALSE;

		InitPreviewWnd(m_FormsNum);

		for (int i = 0;i < m_FormsNum;i++)
		{
			ModifyVideoInfoDlgWithView(i);
			ShowVideoInfoDlgByFlag(i);
		}

		Invalidate(FALSE);

		if (m_bMenuFlag != FALSE)
		{
			m_pDlgMenu->ShowWindow(SW_SHOW);
			m_pDlgShowControl->ShowWindow(SW_HIDE);
		}
		else
		{
			m_pDlgMenu->ShowWindow(SW_HIDE);
			m_pDlgShowControl->ShowWindow(SW_SHOW);
		}

		if (m_bPageServerFlag != FALSE)
		{
			m_pDlgPageServer->ShowWindow(SW_SHOW);
			m_pDlgShowPageServer->ShowWindow(SW_HIDE);
		}
		else
		{
			m_pDlgPageServer->ShowWindow(SW_HIDE);
			m_pDlgShowPageServer->ShowWindow(SW_SHOW);
		}

		DisplayLinkageServerRelationListByFlag(m_bLinkageServerFlag);

		if (m_bPageViewInfoFlag != FALSE)
		{
			m_pDlgPageViewInfo->ShowWindow(SW_SHOW);
		}
		else
		{
			m_pDlgPageViewInfo->ShowWindow(SW_HIDE);
		}
	}
	else
	{
		return;
	}
}

BOOL CVEMCUCtlDlg::ModifyVideoInfoDlg(int nIndex,RECT *pRect,BOOL bShowFlag,
									  int line1_from_x,int line1_from_y,int line1_to_x,int line1_to_y,
									  int line2_from_x,int line2_from_y,int line2_to_x,int line2_to_y)
{
	if (nIndex < 0||nIndex >= MAXVIEWCH||pRect == NULL)
		return FALSE;

	int nWidth = pRect->right - pRect->left;
	int nHeight = pRect->bottom - pRect->top;

	if (nWidth <= 0||nHeight <= 0)
		return FALSE;

	g_VideoInfoDlg[nIndex].ShowWindow(SW_HIDE);
	g_VideoInfoDlg[nIndex].MoveWindow(pRect,TRUE);

	int nStartX = 0;
	int nStartY = 0;
	int nStopX = 0;
	int nStopY = 0;

	nStartX = (line1_from_x*nWidth)/10000;
	nStartY = (line1_from_y*nHeight)/10000;
	nStopX = (line1_to_x*nWidth)/10000;
	nStopY = (line1_to_y*nHeight)/10000;

	if (nStartX != nStopX||nStartY != nStopY)
	{
		g_VideoInfoDlg[nIndex].SetArrowInfo(0,nStartX,nStartY,nStopX,nStopY);
		g_VideoInfoDlg[nIndex].m_OneArrowLine.ShowArrowLine(TRUE);
	}

	nStartX = (line2_from_x*nWidth)/10000;
	nStartY = (line2_from_y*nHeight)/10000;
	nStopX = (line2_to_x*nWidth)/10000;
	nStopY = (line2_to_y*nHeight)/10000;

	if (nStartX != nStopX||nStartY != nStopY)
	{
		g_VideoInfoDlg[nIndex].SetArrowInfo(1,nStartX,nStartY,nStopX,nStopY);
		g_VideoInfoDlg[nIndex].m_TwoArrowLine.ShowArrowLine(TRUE);
	}

	if (bShowFlag)
	{
		g_VideoInfoDlg[nIndex].ShowWindow(SW_SHOW);
		g_VideoInfoDlg[nIndex].Invalidate(TRUE);
		g_VideoInfoDlg[nIndex].m_bVideoInfoShowFlag = TRUE;
		g_VideoInfoDlg[nIndex].m_bVideoArrowShowFlag = TRUE;
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::ModifyVideoTagInfoDlg(int nIndex,RECT *pRect,	BOOL bShowFlag)
{
	if (nIndex < 0||nIndex >= MAXVIEWCH||pRect == NULL)
		return FALSE;

	int nWidth = pRect->right - pRect->left;
	int nHeight = pRect->bottom - pRect->top;

	if (nWidth <= 0||nHeight <= 0)
		return FALSE;

	g_VideoTagInfoDlg[nIndex].ShowWindow(SW_HIDE);
	g_VideoTagInfoDlg[nIndex].MoveWindow(pRect,TRUE);

	if (bShowFlag)
	{
		g_VideoTagInfoDlg[nIndex].m_bVideoInfoShowFlag = TRUE;
		g_VideoTagInfoDlg[nIndex].ShowWindow(SW_SHOW);
		g_VideoTagInfoDlg[nIndex].Invalidate(TRUE);
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::ShowVideoInfoDlgByFlag(int nIndex)
{
	if (nIndex < 0||nIndex >= MAXVIEWCH)
		return FALSE;

	if (g_VideoInfoDlg[nIndex].m_bVideoInfoShowFlag == TRUE||g_VideoInfoDlg[nIndex].m_bVideoArrowShowFlag == TRUE)
	{
		g_VideoInfoDlg[nIndex].ShowWindow(SW_SHOW);
	}
	else
	{
		g_VideoInfoDlg[nIndex].ShowWindow(SW_HIDE);
	}

	if (g_VideoTagInfoDlg[nIndex].m_bVideoInfoShowFlag == FALSE)
	{
		g_VideoTagInfoDlg[nIndex].ShowWindow(SW_HIDE);
	}
	else
	{
		g_VideoTagInfoDlg[nIndex].ShowWindow(SW_SHOW);
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::ModifyVideoInfoDlgWithView(int nIndex)
{
	if (nIndex < 0||nIndex >= MAXVIEWCH)
		return FALSE;

	RECT VideoRect;

	g_DlgVideoView[nIndex].m_Pic.GetWindowRect(&VideoRect);

	ModifyVideoInfoDlg(nIndex,&VideoRect,FALSE,
		g_DlgVideoView[nIndex].m_nLine1_from_x,g_DlgVideoView[nIndex].m_nLine1_from_y,
		g_DlgVideoView[nIndex].m_nLine1_to_x,g_DlgVideoView[nIndex].m_nLine1_to_y,
		g_DlgVideoView[nIndex].m_nLine2_from_x,g_DlgVideoView[nIndex].m_nLine2_from_y,
		g_DlgVideoView[nIndex].m_nLine2_to_x,g_DlgVideoView[nIndex].m_nLine2_to_y);

	ModifyVideoTagInfoDlg(nIndex,&VideoRect,FALSE);

	ShowVideoInfoDlgByFlag(nIndex);

	return TRUE;
}

//查找呼叫
int   CVEMCUCtlDlg::SearchIndexCallByCID(int nCid)
{
	int nResult = -1;
	int i =0;

	for(i = 0;i < MAXVIEWCH;i++)
	{
		g_DlgVideoView[i].m_VideoViewOutInfo.Lock();
		if (g_DlgVideoView[i].m_VideoViewOutInfo.m_nCallid == nCid)
		{
			g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();
			break;
		}
		g_DlgVideoView[i].m_VideoViewOutInfo.UnLock();
	}

	if (i >= 0&&i < MAXVIEWCH)
		nResult = i;

	return nResult;
}

//设置6分屏
BOOL CVEMCUCtlDlg::SetVideoPreviewWndSix(RECT &rc,BOOL bFullScreenFlag)
{
	int iWinWidth = 0;
	int iWinHeight = 0;
	int iFrameWidth = 2;	//外界边框
	int iPreviewNum = 6;
	int nCols = 3;
	int nRows = 3;
	int iMyRow = 0;
	int iMyCol   = 0;
	int i = 0;
	int nTmpHight = 0;

	RECT rect;

	if (bFullScreenFlag)
	{
		iWinWidth = (rc.right - rc.left - ((nCols) * iFrameWidth) ) / nCols;
		iWinHeight = (rc.bottom - rc.top - ((nRows) * iFrameWidth) ) / nRows;
		nTmpHight = 0;
	}
	else
	{
		iWinWidth = (rc.right - rc.left - ((nCols) * iFrameWidth) ) / nCols;
		iWinHeight = (rc.bottom - rc.top - ((nRows) * iFrameWidth) - m_nMenuHight - ACCEHIGHT) / nRows;
		nTmpHight = m_nMenuHight;
	}

	//0
	rect.top = rc.top+iFrameWidth+nTmpHight;
	rect.left = rc.left+iFrameWidth;
	rect.right = rect.left+iWinWidth*2+iFrameWidth;
	rect.bottom = rect.top + iWinHeight*2+iFrameWidth;
	g_DlgVideoView[0].MoveWindow(&rect);
	g_DlgVideoView[0].ShowWindow(SW_SHOW);
	g_DlgVideoView[0].Invalidate(TRUE);

	//1
	iMyRow = 0;
	iMyCol = 2;
	rect.top =rc.top + iMyRow * iWinHeight + ((iMyRow + 1)* iFrameWidth) + nTmpHight;
	rect.bottom = rect.top + iWinHeight;
	rect.left = rc.left + iMyCol * iWinWidth + ((iMyCol + 1) * iFrameWidth);
	rect.right = rect.left + iWinWidth;
	g_DlgVideoView[1].MoveWindow(&rect);
	g_DlgVideoView[1].ShowWindow(SW_SHOW);
	g_DlgVideoView[1].Invalidate(TRUE);

	//2
	iMyRow = 1;
	iMyCol = 2;
	rect.top =rc.top + iMyRow * iWinHeight + ((iMyRow + 1)* iFrameWidth) + nTmpHight;
	rect.bottom = rect.top + iWinHeight;
	rect.left = rc.left + iMyCol * iWinWidth + ((iMyCol + 1) * iFrameWidth);
	rect.right = rect.left + iWinWidth;
	g_DlgVideoView[2].MoveWindow(&rect);
	g_DlgVideoView[2].ShowWindow(SW_SHOW);
	g_DlgVideoView[2].Invalidate(TRUE);

	//3
	iMyRow = 2;
	iMyCol = 0;
	rect.top =rc.top + iMyRow * iWinHeight + ((iMyRow + 1)* iFrameWidth) + nTmpHight;
	rect.bottom = rect.top + iWinHeight;
	rect.left = rc.left + iMyCol * iWinWidth + ((iMyCol + 1) * iFrameWidth);
	rect.right = rect.left + iWinWidth;
	g_DlgVideoView[3].MoveWindow(&rect);
	g_DlgVideoView[3].ShowWindow(SW_SHOW);
	g_DlgVideoView[3].Invalidate(TRUE);

	//4
	iMyRow = 2;
	iMyCol = 1;
	rect.top =rc.top + iMyRow * iWinHeight + ((iMyRow + 1)* iFrameWidth) + nTmpHight;
	rect.bottom = rect.top + iWinHeight;
	rect.left = rc.left + iMyCol * iWinWidth + ((iMyCol + 1) * iFrameWidth);
	rect.right = rect.left + iWinWidth;
	g_DlgVideoView[4].MoveWindow(&rect);
	g_DlgVideoView[4].ShowWindow(SW_SHOW);
	g_DlgVideoView[4].Invalidate(TRUE);

	//5
	iMyRow = 2;
	iMyCol = 2;
	rect.top =rc.top + iMyRow * iWinHeight + ((iMyRow + 1)* iFrameWidth) + nTmpHight;
	rect.bottom = rect.top + iWinHeight;
	rect.left = rc.left + iMyCol * iWinWidth + ((iMyCol + 1) * iFrameWidth);
	rect.right = rect.left + iWinWidth;
	g_DlgVideoView[5].MoveWindow(&rect);
	g_DlgVideoView[5].ShowWindow(SW_SHOW);
	g_DlgVideoView[5].Invalidate(TRUE);

	return TRUE;
}

//隐藏视频显示信息
BOOL CVEMCUCtlDlg::HideVideoInfoDlg(int nIndex)
{
	if (nIndex < 0||nIndex >= MAXVIEWCH)
		return FALSE;

	g_VideoInfoDlg[nIndex].ShowWindow(SW_HIDE);
	g_VideoTagInfoDlg[nIndex].ShowWindow(SW_HIDE);

	return TRUE;
}

//隐藏全部视频显示信息
BOOL CVEMCUCtlDlg::HideAllVideoInfoDlg()
{
	for (int i = 0;i < MAXVIEWCH;i++)
	{
		HideVideoInfoDlg(i);
	}
	return TRUE;
}

//清除视频显示信息
BOOL CVEMCUCtlDlg::ClearVideoInfoDlgInfo(int nIndex)
{
	try
	{
		if (nIndex < 0||nIndex >= MAXVIEWCH)
			return FALSE;

		g_VideoInfoDlg[nIndex].ClearVideoArrowTextInfo();
		g_VideoInfoDlg[nIndex].Invalidate(TRUE);
		g_VideoInfoDlg[nIndex].ShowWindow(SW_HIDE);

		g_VideoTagInfoDlg[nIndex].m_bVideoInfoShowFlag = FALSE;
		g_VideoTagInfoDlg[nIndex].Invalidate(TRUE);
		g_VideoTagInfoDlg[nIndex].ShowWindow(SW_HIDE);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CVEMCUCtlDlg::InitThreadPool()
{
	int nResult = ComInitThreadPool(&g_CommnThreadPool,SINGLEFIFOTHREADPOOL|COPYARG,-1,5,1000);
	return (nResult == 0?TRUE:FALSE);
}

BOOL CVEMCUCtlDlg::FreeThreadPool()
{
	ComFreeThreadPool(&g_CommnThreadPool,0);
	return TRUE;
}

BOOL CVEMCUCtlDlg::ThreadPoolDispatchTask(void (*pFun)(void *), void *arg,int argsize,int key)
{
	int nResult = ComDispatchTask(&g_CommnThreadPool,pFun,arg,argsize,key);
	return (nResult == 0?TRUE:FALSE);
}

//新建一个tcp客户端
BOOL CVEMCUCtlDlg::InitAssClient()
{
	g_pACSClient = new CACSSystemClient();
	
	if (g_pACSClient == NULL)
		return FALSE;

	//辅助系统客户端初始化
	if (!g_pACSClient->Init())
		return FALSE;

	//连接辅助系统服务器
	if (!g_pACSClient->ConnectServer(g_AcsServerIp, g_nAcsServerPort))
		return FALSE;

	//发送登录请求
	if(!g_pACSClient->SendLoginServerRequest(g_userpower.username,g_userpower.password,LOGIN))
		return FALSE;

	return TRUE;
}

//TCP服务
BOOL CVEMCUCtlDlg::InitTcpServerClient()
{
	BOOL bResult = FALSE;

	if (strlen(app_StackCfg.szLocalUdpAddress) == 0)
		return FALSE;

	bResult = g_TcpServerClient.SetLocalIp(app_StackCfg.szLocalUdpAddress);
	if (!bResult)
		return FALSE;

	bResult = g_TcpServerClient.SetLocalServerPort(TCP_LOCAL_SERVER_PORT);
	if (!bResult)
		return FALSE;

	bResult = g_TcpServerClient.SetRemoteIp(g_LinkServerIP);
	if (!bResult)
		return FALSE;

	bResult = g_TcpServerClient.SetRemotePort(g_nLinkServerPort);
	if (!bResult)
		return FALSE;

	bResult = g_TcpServerClient.InitNetTcp();
	if (!bResult)
		return FALSE;

	g_TcpServerClient.SetInfoCB((void *)XmlInfoCallBack,NULL);

	bResult = g_TcpServerClient.StartNetTcpServer();
	if (!bResult)
		return FALSE;

	return TRUE;
}

BOOL CVEMCUCtlDlg::UnInitTcpServerClient()
{
	g_TcpServerClient.StopNetTcpClient();
	g_TcpServerClient.StopNetTcpServer();
	g_TcpServerClient.UnInitNetTcp();

	return TRUE;
}

//得到已经安装的解码SDK
int CVEMCUCtlDlg::GetGWCode(int *pCode)
{
	FILE *pFile = NULL;
	int nCodeNum = 0;
	int nCount = 0;
	int nStrlen = 0;
	char Buf[2048] = {0};
	char strCode[16] = {0};
	memset(Buf,0,sizeof(Buf));
	memset(strCode,0,sizeof(strCode));

	pFile = fopen("GWDecCFG.dat","rb");
	if (pFile == NULL)
		return 0;

	nCount = fread(Buf,1,sizeof(Buf)-1,pFile);
	if (nCount <= 0)
		return 0;

	fclose(pFile);
	pFile = NULL;

	char *pTemp = Buf;
	char *pTempCode = pTemp;

	while(*pTemp != 0)
	{
		if (*pTemp == '\r'&&*(pTemp+1) == '\n')
		{
			nStrlen = pTemp-pTempCode;
			if (nStrlen > 0)
			{
				memcpy(strCode,pTempCode,nStrlen);
				strCode[nStrlen] = 0;
				pCode[nCodeNum] = atoi(strCode);
				nCodeNum++;
			}
			pTemp +=2 ;
			pTempCode = pTemp;
			continue;
		}
		pTemp++;
	}

	nStrlen = pTemp-pTempCode;
	if (nStrlen > 0)
	{
		memcpy(strCode,pTempCode,nStrlen);
		strCode[nStrlen] = 0;
		pCode[nCodeNum] = atoi(strCode);
		nCodeNum++;
	}

	return nCodeNum;
}

//初始化国网解码SDK
bool CVEMCUCtlDlg::InitGWSDK()
{
	BOOL bResult = FALSE;
	int Code[256] = {0};
	int nCount = GetGWCode(Code);
	if (nCount <= 0)
		return false;

	for (int i = 0;i < nCount;i++)
	{
		bResult = g_GWPlay.LoadGWDecode(Code[i]);
	}

	return true;
}

//反初始化国网解码SDK
bool CVEMCUCtlDlg::UninitGWSDK()
{
	return true;
}

void CVEMCUCtlDlg::PlayClose(int win)
{
	g_DlgVideoView[win].CloseViewChannel();
}

//自定义消息处理
LRESULT CVEMCUCtlDlg::OnInitUsbVideoMSGHandler(WPARAM wParam, LPARAM lParam)
{
	return 1;
}

//自定义消息处理
LRESULT CVEMCUCtlDlg::OnExitUsbVideoMSGHandler(WPARAM wParam, LPARAM lParam)
{
	return 1;
}

LRESULT CVEMCUCtlDlg::OnSubOK(WPARAM wParam, LPARAM lParam)
{

	return 1;
}

LRESULT CVEMCUCtlDlg::OnSubFail(WPARAM wParam, LPARAM lParam)
{
	return 1;
}

LRESULT CVEMCUCtlDlg::OnVideoLinkageManualMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		//if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
		//	SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		CheckRelationLinkageListCount();

		//if (m_bMultiFullScreen == false)
		//	ShowSmallRelationList();

		char	szStationName[LINKAGE_STATION_NAME_LEN] = {0};
		char	szDeviceName[LINKAGE_DEVICE_NAME_LEN] = {0};
		int	nType = -1;
		int    nScreenId = -1;
		char szTime[LINKAGE_TIME_LEN] = {0};
		char szVideoStationName[LINKAGE_STATION_NAME_LEN] = {0};
		int    nCameraNum = 0;
		char szCameraName[LINKAGE_DEVICE_NAME_LEN] = {0};
		char szCameraCode[LINKAGE_DEVICE_CODE_LEN] = {0};
		int    nDecodeTag = -1;
		int    nIndex = -1;
		char szReason[512] = {0};
		char szType[128] = {0};
		char szScreenId[16] = {0};
		char szCameraNum[16] = {0};
		char szDecodeTag[16] = {0};
		char szPresetId[64] = {0};
		char szXYValue[64] = {0};

		COLORREF BKColor = RGB(246,250,255);
		COLORREF TextColor = RGB(0,168,0);

		VIDEO_LINKAGE_INFO *pVideoManualLinkage = NULL;
		VIDEO_LINKAGE_CAMERA_INFO *pLinkageCameraInfo = NULL;
		_T_NODE_PRESET_INFO * pPresetNodeInfo = NULL;
		_T_NODE_INFO * pNodeInfo = NULL;
		HTREEITEM hInfoTreeItem = NULL;

		time_t time_ManualLinkage = time(NULL);
		BOOL bResult = FALSE;

		if (lParam != WM_MAGIC_VEMCUCTL)
			return 0;

		if (wParam == 0)
			return 0;

		g_nViewOutVideoTypeFlag = VIDEO_LINKAGE_INFO_TYPE;

		pVideoManualLinkage = (VIDEO_LINKAGE_INFO *)wParam;

		strcpy_s(szStationName,sizeof(szStationName),pVideoManualLinkage->szStationName);
		strcpy_s(szDeviceName,sizeof(szDeviceName),pVideoManualLinkage->szDeviceName);
		nType = pVideoManualLinkage->nType;
		nScreenId = pVideoManualLinkage->nScreenId;
		strcpy_s(szTime,sizeof(szTime),pVideoManualLinkage->szTime);
		strcpy_s(szVideoStationName,sizeof(szVideoStationName),pVideoManualLinkage->szLinkageStationName);
		nCameraNum = pVideoManualLinkage->nLinkNum;
		pLinkageCameraInfo = pVideoManualLinkage->pLinkageCameraInfo;

		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->CameraTreelistAddCameraByStastion(szVideoStationName);
			m_pDlgPageServer->PresetTreelistAddPresetByStastion(szVideoStationName);
		}

		if (m_pDlgLinkageServer != NULL)
		{
			if (nType == 1)
				strcpy_s(szType,sizeof(szType),"手动");
			else
				strcpy_s(szType,sizeof(szType),"巡检");

			nIndex = m_pDlgLinkageServer->m_listRelation.InsertItem(0,"热点设备联动");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,1,szDeviceName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,2,szStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,3,szTime);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,4,szType);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,5,"");
			sprintf_s(szScreenId,sizeof(szScreenId),"%d",nScreenId);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,6,szScreenId);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,7,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,8,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,9,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,10,szVideoStationName);
			sprintf_s(szCameraNum,sizeof(szCameraNum),"%d",nCameraNum);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,11,szCameraNum);

			m_pDlgLinkageServer->m_listRelation.SetRowColor(0,BKColor);
			m_pDlgLinkageServer->m_listRelation.SetRowTextColor(0,TextColor);

			m_pDlgLinkageServer->m_listCameraRelation.DeleteAllItems();
			for (int i = 0;i < nCameraNum;i++)
			{
				strcpy_s(szCameraName,sizeof(szCameraName),pLinkageCameraInfo[i].szName);
				strcpy_s(szCameraCode,sizeof(szCameraCode),pLinkageCameraInfo[i].szCode);
				nDecodeTag = pLinkageCameraInfo[i].nDecodeTag;
				sprintf_s(szDecodeTag,sizeof(szDecodeTag),"%d",nDecodeTag);
				nIndex = m_pDlgLinkageServer->m_listCameraRelation.InsertItem(0,szCameraName);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,1,szCameraCode);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,2,szDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,3,szVideoStationName);
				sprintf(szPresetId,"%d",pLinkageCameraInfo[i].nPresetId);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,4,szPresetId);		
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,5,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,6,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,7,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,8,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,9,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,10,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,11,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,12,szXYValue);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,13,pLinkageCameraInfo[i].szPresetName);
			}
		}

		if (m_pDlgLinkageServer != NULL)
		{
			if(m_pDlgLinkageServer->m_CheckRelateVideo.GetCheck() == 0)
				return 0;
		}

		//联动优先级判断
		bResult = g_VideoLinkagePriorityInfo.DecideAndSetVideoLinkagePriorityInfo(VIDEO_LINKAGE_INFO_TYPE,VIDEO_MANUAL_LINKAGE_PRIORITY,time_ManualLinkage);
		if (!bResult)
			return 0;

		//////////////////////////////////////////////////////////////////////////
		if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
			SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		if (m_bMultiFullScreen == false)
			ShowSmallRelationList();
		//////////////////////////////////////////////////////////////////////////

		if (nCameraNum > 0&&pLinkageCameraInfo != NULL)
		{
			SetLinkageForms(nScreenId+nCameraNum);

			if (nCameraNum > MAXVIEWCH)
				nCameraNum = MAXVIEWCH;

			if (nScreenId < 1||nScreenId > 10)
				nScreenId = 1;

			_T_NODE_PRESET_INFO PresetInfo;

			for (int i = 0;i < nCameraNum;i++)
			{
				strcpy_s(szCameraName,sizeof(szCameraName),pLinkageCameraInfo[i].szName);
				strcpy_s(szCameraCode,sizeof(szCameraCode),pLinkageCameraInfo[i].szCode);
				nDecodeTag = pLinkageCameraInfo[i].nDecodeTag;
				sprintf(szReason, "{热点区域监视}-{%s}-{%s}",szDeviceName, szStationName);

				memset(&PresetInfo,0,sizeof(_T_NODE_PRESET_INFO));

				strcpy_s(PresetInfo.node_station,sizeof(PresetInfo.node_station),szStationName);
				strcpy_s(PresetInfo.node_num,sizeof(PresetInfo.node_num),szCameraCode);
				strcpy_s(PresetInfo.preset_name,sizeof(PresetInfo.preset_name),pLinkageCameraInfo[i].szPresetName);

				PresetInfo.preset_id = pLinkageCameraInfo->nPresetId;
				PresetInfo.line1_from_x = pLinkageCameraInfo[i].nFromX1;
				PresetInfo.line1_from_y = pLinkageCameraInfo[i].nFromY1;
				PresetInfo.line1_to_x = pLinkageCameraInfo[i].nToX1;
				PresetInfo.line1_to_y = pLinkageCameraInfo[i].nToY1;

				PresetInfo.line2_from_x = pLinkageCameraInfo[i].nFromX2;
				PresetInfo.line2_from_y = pLinkageCameraInfo[i].nFromY2;
				PresetInfo.line2_to_x = pLinkageCameraInfo[i].nToX2;
				PresetInfo.line2_to_y = pLinkageCameraInfo[i].nToY2;

				if (m_pDlgPageServer != NULL)
				{
					hInfoTreeItem = m_pDlgPageServer->SearchPresetTreePresetHandleTreeItem(NULL,szCameraCode,pLinkageCameraInfo[i].szPresetName);
					if (hInfoTreeItem != NULL)
					{
						pPresetNodeInfo = (_T_NODE_PRESET_INFO *)m_pDlgPageServer->m_trPreset.GetItemData(hInfoTreeItem);
						if (pPresetNodeInfo != NULL)
						{
							memcpy(&PresetInfo.camera_info,&pPresetNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
						}
					}
					else
					{
						hInfoTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szCameraCode,NULL,NULL);
						if (hInfoTreeItem != NULL)
						{
							pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hInfoTreeItem);
							if (pNodeInfo != NULL)
							{
								memcpy(&PresetInfo.camera_info,&pNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
							}
						}
					}
				}

				VIDEO_SIP_CALL_PRESET_INFO VideoSipCallPresetInfo;
				memset(&VideoSipCallPresetInfo,0,sizeof(VideoSipCallPresetInfo));

				if (IsCameraVideoLinkByVideoPlatform(g_nClientVideoLinkType,nDecodeTag))
				{
					VideoSipCallPresetInfo.nType = 2;
					VideoSipCallPresetInfo.nDecodeTag = nDecodeTag;
				}
				else
				{
					VideoSipCallPresetInfo.nType = 4;
					VideoSipCallPresetInfo.nDecodeTag = PresetInfo.camera_info.dvr_info.dvr_type;
				}

				strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),szCameraCode);
				strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),szCameraName);
				VideoSipCallPresetInfo.nScreenId = nScreenId+i-1;
				strcpy_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),szReason);
				memcpy(&VideoSipCallPresetInfo.preset_node_info,&PresetInfo,sizeof(VideoSipCallPresetInfo.preset_node_info));

				g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
				ThreadPoolDispatchTask(ThreadVideoLinkageMakeCall,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);

				Sleep(100);
			}
		}

		return 1;
	}
	catch(...)
	{

	}
	return 0;
}

LRESULT CVEMCUCtlDlg::OnVideoLinkageAlarmMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		//if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
		//	SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		CheckRelationLinkageListCount();

		//if (m_bMultiFullScreen == false)
		//	ShowSmallRelationList();

		char	szStationName[LINKAGE_STATION_NAME_LEN] = {0};
		char	szDeviceName[LINKAGE_DEVICE_NAME_LEN] = {0};
		char szAlarmType[LINKAGE_TYPE_LEN] = {0};
		char szTime[LINKAGE_TIME_LEN] = {0};
		char szContent[LINKAGE_ALARM_CONTENT_LEN] = {0};
		char szVideoStationName[LINKAGE_STATION_NAME_LEN] = {0};
		int    nCameraNum = 0;
		char szCameraName[LINKAGE_DEVICE_NAME_LEN] = {0};
		char szCameraCode[LINKAGE_DEVICE_CODE_LEN] = {0};

		int    nDecodeTag = -1;
		int    nIndex = -1;
		char szReason[512] = {0};
		char szCameraNum[16] = {0};
		char szDecodeTag[16] = {0};
		char szPresetId[64] = {0};
		char szXYValue[64] = {0};

		COLORREF BKColor = RGB(207,235,250);
		COLORREF TextColor = RGB(168,0,0);

		VIDEO_ALARM_LINKAGE_INFO *pVideoAlarmLinkage = NULL;
		VIDEO_LINKAGE_CAMERA_INFO *pLinkageCameraInfo = NULL;
		_T_NODE_PRESET_INFO * pPresetNodeInfo = NULL;
		_T_NODE_INFO * pNodeInfo = NULL;
		HTREEITEM hInfoTreeItem = NULL;

		time_t time_AlarmLinkage = time(NULL);
		BOOL bResult = FALSE;

		if (lParam != WM_MAGIC_VEMCUCTL)
			return 0;

		if (wParam == 0)
			return 0;

		g_nViewOutVideoTypeFlag = VIDEO_ALARM_LINKAGE_INFO_TYPE;

		pVideoAlarmLinkage = (VIDEO_ALARM_LINKAGE_INFO *)wParam;

		strcpy_s(szStationName,sizeof(szStationName),pVideoAlarmLinkage->szStationName);
		strcpy_s(szDeviceName,sizeof(szDeviceName),pVideoAlarmLinkage->szDeviceName);
		strcpy_s(szAlarmType,sizeof(szAlarmType),pVideoAlarmLinkage->szAlarmType);
		strcpy_s(szTime,sizeof(szTime),pVideoAlarmLinkage->szTime);
		strcpy_s(szContent,sizeof(szContent),pVideoAlarmLinkage->szContent);
		strcpy_s(szVideoStationName,sizeof(szVideoStationName),pVideoAlarmLinkage->szLinkageStationName);
		nCameraNum = pVideoAlarmLinkage->nLinkNum;
		pLinkageCameraInfo = pVideoAlarmLinkage->pLinkageCameraInfo;
		sprintf(szCameraNum, "%d", nCameraNum);

		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->CameraTreelistAddCameraByStastion(szVideoStationName);
			m_pDlgPageServer->PresetTreelistAddPresetByStastion(szVideoStationName);
		}

		if (m_pDlgLinkageServer != NULL)
		{
			nIndex = m_pDlgLinkageServer->m_listRelation.InsertItem(0,"告警联动");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,1,szDeviceName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,2,szStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,3,szTime);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,4,szAlarmType);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,5,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,6,"0");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,7,szContent);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,8,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,9,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,10,szVideoStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,11,szCameraNum);

			m_pDlgLinkageServer->m_listRelation.SetRowColor(0,BKColor);
			m_pDlgLinkageServer->m_listRelation.SetRowTextColor(0,TextColor);

			// 摄像头列表
			m_pDlgLinkageServer->m_listCameraRelation.DeleteAllItems();
			for (int i = 0; i < nCameraNum; i ++)
			{
				m_pDlgLinkageServer->m_listCameraRelation.InsertItem(0, pLinkageCameraInfo[i].szName);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,1,pLinkageCameraInfo[i].szCode);
				sprintf(szDecodeTag, "%d", pLinkageCameraInfo[i].nDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,2,szDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,3,szVideoStationName);
				sprintf(szPresetId,"%d",pLinkageCameraInfo[i].nPresetId);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,4,szPresetId);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,5,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,6,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,7,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,8,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,9,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,10,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,11,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,12,szXYValue);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,13,pLinkageCameraInfo[i].szPresetName);
			}
		}

		if (m_pDlgLinkageServer != NULL)
		{
			if (strcmp(szAlarmType,"事故") == 0&&m_pDlgLinkageServer->m_CheckRelateAccident.GetCheck() == 0)
				return 0;

			if (strcmp(szAlarmType,"异常") == 0&&m_pDlgLinkageServer->m_CheckRelateAbnormal.GetCheck() == 0)
				return 0;

			if (strcmp(szAlarmType,"变位") == 0&&m_pDlgLinkageServer->m_CheckRelateState.GetCheck() == 0)
				return 0;

			if (strcmp(szAlarmType,"开关变位") == 0&&m_pDlgLinkageServer->m_CheckRelateSwitchState.GetCheck() == 0)
				return 0;

			if (strcmp(szAlarmType,"越限") == 0&&m_pDlgLinkageServer->m_CheckRelateLimit.GetCheck() == 0)
				return 0;

			if (strcmp(szAlarmType,"告知") == 0&&m_pDlgLinkageServer->m_CheckRelateNotify.GetCheck() == 0)
				return 0;
		}

		//联动优先级判断
		bResult = g_VideoLinkagePriorityInfo.DecideAndSetVideoLinkagePriorityInfo(VIDEO_ALARM_LINKAGE_INFO_TYPE,VIDEO_ALARM_LINKAGE_PRIORITY,time_AlarmLinkage);
		if (!bResult)
			return 0;

		//////////////////////////////////////////////////////////////////////////
		if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
			SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		if (m_bMultiFullScreen == false)
			ShowSmallRelationList();
		//////////////////////////////////////////////////////////////////////////

		if (nCameraNum > 0&&pLinkageCameraInfo != NULL)
		{
			SetLinkageForms(nCameraNum);

			if(nCameraNum > MAXVIEWCH)
				nCameraNum = MAXVIEWCH;

			_T_NODE_PRESET_INFO PresetInfo;

			for (int i = 0;i < nCameraNum;i++)
			{
				strcpy_s(szCameraName,sizeof(szCameraName),pLinkageCameraInfo[i].szName);
				strcpy_s(szCameraCode,sizeof(szCameraCode),pLinkageCameraInfo[i].szCode);
				nDecodeTag = pLinkageCameraInfo[i].nDecodeTag;
				sprintf(szReason, "{告警联动}-{%s}-{%s}-{%s}",szDeviceName, szContent, szStationName);

				memset(&PresetInfo,0,sizeof(_T_NODE_PRESET_INFO));

				strcpy_s(PresetInfo.node_station,sizeof(PresetInfo.node_station),szStationName);
				strcpy_s(PresetInfo.node_num,sizeof(PresetInfo.node_num),szCameraCode);

				strcpy_s(PresetInfo.preset_name,sizeof(PresetInfo.preset_name),pLinkageCameraInfo[i].szPresetName);
				PresetInfo.preset_id = pLinkageCameraInfo->nPresetId;
				PresetInfo.line1_from_x = pLinkageCameraInfo[i].nFromX1;
				PresetInfo.line1_from_y = pLinkageCameraInfo[i].nFromY1;
				PresetInfo.line1_to_x = pLinkageCameraInfo[i].nToX1;
				PresetInfo.line1_to_y = pLinkageCameraInfo[i].nToY1;

				PresetInfo.line2_from_x = pLinkageCameraInfo[i].nFromX2;
				PresetInfo.line2_from_y = pLinkageCameraInfo[i].nFromY2;
				PresetInfo.line2_to_x = pLinkageCameraInfo[i].nToX2;
				PresetInfo.line2_to_y = pLinkageCameraInfo[i].nToY2;

				if (m_pDlgPageServer != NULL)
				{
					hInfoTreeItem = m_pDlgPageServer->SearchPresetTreePresetHandleTreeItem(NULL,szCameraCode,pLinkageCameraInfo[i].szPresetName);
					if (hInfoTreeItem != NULL)
					{
						pPresetNodeInfo = (_T_NODE_PRESET_INFO *)m_pDlgPageServer->m_trPreset.GetItemData(hInfoTreeItem);
						if (pPresetNodeInfo != NULL)
						{
							memcpy(&PresetInfo.camera_info,&pPresetNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
						}
					}
					else
					{
						hInfoTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szCameraCode,NULL,NULL);
						if (hInfoTreeItem != NULL)
						{
							pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hInfoTreeItem);
							if (pNodeInfo != NULL)
							{
								memcpy(&PresetInfo.camera_info,&pNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
							}
						}
					}
				}
				
				VIDEO_SIP_CALL_PRESET_INFO VideoSipCallPresetInfo;
				memset(&VideoSipCallPresetInfo,0,sizeof(VideoSipCallPresetInfo));

				if (IsCameraVideoLinkByVideoPlatform(g_nClientVideoLinkType,nDecodeTag))
				{
					VideoSipCallPresetInfo.nType = 2;
					VideoSipCallPresetInfo.nDecodeTag = nDecodeTag;
				}
				else
				{
					VideoSipCallPresetInfo.nType = 4;
					VideoSipCallPresetInfo.nDecodeTag = PresetInfo.camera_info.dvr_info.dvr_type;
				}

				strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),szCameraCode);
				strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),szCameraName);
				VideoSipCallPresetInfo.nScreenId = i;
				strcpy_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),szReason);
				memcpy(&VideoSipCallPresetInfo.preset_node_info,&PresetInfo,sizeof(VideoSipCallPresetInfo.preset_node_info));

				g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
				ThreadPoolDispatchTask(ThreadVideoLinkageMakeCall,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);

				Sleep(100);
			}
		}

		return 1;
	}
	catch(...)
	{

	}
	return 0;
}

LRESULT CVEMCUCtlDlg::OnVideoLinkageStateMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		//if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
		//	SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		CheckRelationLinkageListCount();

		//if (m_bMultiFullScreen == false)
		//	ShowSmallRelationList();

		char	szStationName[LINKAGE_STATION_NAME_LEN] = {0};
		char	szDeviceName[LINKAGE_DEVICE_NAME_LEN] = {0};
		char	szDeviceType[LINKAGE_TYPE_LEN] = {0};
		char szState[LINKAGE_STATE_INFO_LEN] = {0};
		char szTime[LINKAGE_TIME_LEN] = {0};
		char szContent[LINKAGE_ALARM_CONTENT_LEN] = {0};
		char szValue1[LINKAGE_VALUE_INFO_LEN] = {0};
		char szValue2[LINKAGE_VALUE_INFO_LEN] = {0};
		char szVideoStationName[LINKAGE_STATION_NAME_LEN] = {0};
		int    nCameraNum = 0;
		char szCameraName[LINKAGE_DEVICE_NAME_LEN] = {0};
		char szCameraCode[LINKAGE_DEVICE_CODE_LEN] = {0};
		int    nDecodeTag = -1;
		int    nIndex = -1;
		char szReason[512] = {0};
		char szCameraNum[16] = {0};
		char szDecodeTag[16] = {0};
		char szPresetId[64] = {0};
		char szXYValue[64] = {0};

		COLORREF BKColor = RGB(197,228,249);
		COLORREF TextColor = RGB(64,128,128);

		VIDEO_STATE_LINKAGE_INFO *pVideoStateLinkage = NULL;
		VIDEO_LINKAGE_CAMERA_INFO *pLinkageCameraInfo = NULL;
		_T_NODE_PRESET_INFO * pPresetNodeInfo = NULL;
		_T_NODE_INFO * pNodeInfo = NULL;
		HTREEITEM hInfoTreeItem = NULL;

		time_t time_StateLinkage = time(NULL);
		BOOL bResult = FALSE;

		if (lParam != WM_MAGIC_VEMCUCTL)
			return 0;

		if (wParam == 0)
			return 0;

		pVideoStateLinkage = (VIDEO_STATE_LINKAGE_INFO *)wParam;

		strcpy_s(szStationName,sizeof(szStationName),pVideoStateLinkage->szStationName);
		strcpy_s(szDeviceName,sizeof(szDeviceName),pVideoStateLinkage->szDeviceName);
		strcpy_s(szDeviceType,sizeof(szDeviceType),pVideoStateLinkage->szDeviceType);
		strcpy_s(szState,sizeof(szState),pVideoStateLinkage->szState);
		strcpy_s(szTime,sizeof(szTime),pVideoStateLinkage->szTime);
		strcpy_s(szContent,sizeof(szContent),pVideoStateLinkage->szContent);
		strcpy_s(szValue1,sizeof(szValue1),pVideoStateLinkage->szValue1);
		strcpy_s(szValue2,sizeof(szValue2),pVideoStateLinkage->szValue2);
		strcpy_s(szVideoStationName,sizeof(szVideoStationName),pVideoStateLinkage->szLinkageStationName);
		nCameraNum = pVideoStateLinkage->nLinkNum;
		pLinkageCameraInfo = pVideoStateLinkage->pLinkageCameraInfo;

		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->CameraTreelistAddCameraByStastion(szVideoStationName);
			m_pDlgPageServer->PresetTreelistAddPresetByStastion(szVideoStationName);
		}

		if (m_pDlgLinkageServer != NULL)
		{
			nIndex = m_pDlgLinkageServer->m_listRelation.InsertItem(0,"状态联动");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,1,szDeviceName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,2,szStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,3,szTime);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,4,szDeviceType);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,5,szState);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,6,"0");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,7,szContent);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,8,szValue1);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,9,szValue2);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,10,szVideoStationName);
			sprintf_s(szCameraNum,sizeof(szCameraNum),"%d",nCameraNum);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,11,szCameraNum);

			m_pDlgLinkageServer->m_listRelation.SetRowColor(0,BKColor);
			m_pDlgLinkageServer->m_listRelation.SetRowTextColor(0,TextColor);

			// 摄像头列表
			m_pDlgLinkageServer->m_listCameraRelation.DeleteAllItems();
			for (int i = 0; i < nCameraNum; i ++)
			{
				m_pDlgLinkageServer->m_listCameraRelation.InsertItem(0, pLinkageCameraInfo[i].szName);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,1,pLinkageCameraInfo[i].szCode);
				sprintf_s(szDecodeTag,sizeof(szDecodeTag),"%d", pLinkageCameraInfo[i].nDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,2,szDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,3,szVideoStationName);
				sprintf(szPresetId,"%d",pLinkageCameraInfo[i].nPresetId);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,4,szPresetId);			
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,5,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,6,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,7,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,8,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,9,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,10,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,11,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,12,szXYValue);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,13,pLinkageCameraInfo[i].szPresetName);
			}
		}

		//联动优先级判断
		bResult = g_VideoLinkagePriorityInfo.DecideAndSetVideoLinkagePriorityInfo(VIDEO_STATE_LINKAGE_INFO_TYPE,VIDEO_STATE_LINKAGE_PRIORITY,time_StateLinkage);
		if (!bResult)
			return 0;

		//////////////////////////////////////////////////////////////////////////
		if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
			SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		if (m_bMultiFullScreen == false)
			ShowSmallRelationList();
		//////////////////////////////////////////////////////////////////////////

		if (nCameraNum > 0&&pLinkageCameraInfo != NULL)
		{
			SetLinkageForms(nCameraNum);

			if (nCameraNum > MAXVIEWCH)
				nCameraNum = MAXVIEWCH;

			_T_NODE_PRESET_INFO PresetInfo;

			for (int i = 0;i < nCameraNum;i++)
			{
				strcpy_s(szCameraName,sizeof(szCameraName),pLinkageCameraInfo[i].szName);
				strcpy_s(szCameraCode,sizeof(szCameraCode),pLinkageCameraInfo[i].szCode);
				nDecodeTag = pLinkageCameraInfo[i].nDecodeTag;
				sprintf(szReason, "{状态变位}-{%s}-{%s}-{%s}",szDeviceName, szContent, szStationName);

				memset(&PresetInfo,0,sizeof(_T_NODE_PRESET_INFO));

				strcpy_s(PresetInfo.node_station,sizeof(PresetInfo.node_station),szStationName);
				strcpy_s(PresetInfo.node_num,sizeof(PresetInfo.node_num),szCameraCode);
				strcpy_s(PresetInfo.preset_name,sizeof(PresetInfo.preset_name),pLinkageCameraInfo[i].szPresetName);

				PresetInfo.preset_id = pLinkageCameraInfo->nPresetId;
				PresetInfo.line1_from_x = pLinkageCameraInfo[i].nFromX1;
				PresetInfo.line1_from_y = pLinkageCameraInfo[i].nFromY1;
				PresetInfo.line1_to_x = pLinkageCameraInfo[i].nToX1;
				PresetInfo.line1_to_y = pLinkageCameraInfo[i].nToY1;

				PresetInfo.line2_from_x = pLinkageCameraInfo[i].nFromX2;
				PresetInfo.line2_from_y = pLinkageCameraInfo[i].nFromY2;
				PresetInfo.line2_to_x = pLinkageCameraInfo[i].nToX2;
				PresetInfo.line2_to_y = pLinkageCameraInfo[i].nToY2;

				if (m_pDlgPageServer != NULL)
				{
					hInfoTreeItem = m_pDlgPageServer->SearchPresetTreePresetHandleTreeItem(NULL,szCameraCode,pLinkageCameraInfo[i].szPresetName);
					if (hInfoTreeItem != NULL)
					{
						pPresetNodeInfo = (_T_NODE_PRESET_INFO *)m_pDlgPageServer->m_trPreset.GetItemData(hInfoTreeItem);
						if (pPresetNodeInfo != NULL)
						{
							memcpy(&PresetInfo.camera_info,&pPresetNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
						}
					}
					else
					{
						hInfoTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szCameraCode,NULL,NULL);
						if (hInfoTreeItem != NULL)
						{
							pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hInfoTreeItem);
							if (pNodeInfo != NULL)
							{
								memcpy(&PresetInfo.camera_info,&pNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
							}
						}
					}
				}

				VIDEO_SIP_CALL_PRESET_INFO VideoSipCallPresetInfo;
				memset(&VideoSipCallPresetInfo,0,sizeof(VideoSipCallPresetInfo));

				if (IsCameraVideoLinkByVideoPlatform(g_nClientVideoLinkType,nDecodeTag))
				{
					VideoSipCallPresetInfo.nType = 2;
					VideoSipCallPresetInfo.nDecodeTag = nDecodeTag;
				}
				else
				{
					VideoSipCallPresetInfo.nType = 4;
					VideoSipCallPresetInfo.nDecodeTag = PresetInfo.camera_info.dvr_info.dvr_type;
				}

				strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),szCameraCode);
				strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),szCameraName);
				VideoSipCallPresetInfo.nScreenId = i;
				strcpy_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),szReason);
				memcpy(&VideoSipCallPresetInfo.preset_node_info,&PresetInfo,sizeof(VideoSipCallPresetInfo.preset_node_info));

				g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
				g_pMainDlg->ThreadPoolDispatchTask(ThreadVideoLinkageMakeCall,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);

				Sleep(100);
			}
		}

		return 1;
	}
	catch(...)
	{

	}
	return 0;
}

LRESULT CVEMCUCtlDlg::OnVideoCameraControlNotifyMessageHandler(WPARAM wParam, LPARAM lParam)
{
	return 0;

	char	szStationName[VIDEO_STATION_NAME_LEN] = {0};
	char	szDeviceName[VIDEO_DEVICE_NAME_LEN] = {0};
	char	szDeviceCode[VIDEO_DEVICE_CODE_LEN] = {0};
	char szUserName[VIDEO_USER_NAME_LEN] = {0};
	int nIndex = -1;

	char szTime[64] = {0};

	VIDEO_CAMERA_CONTROL_NOTIFY_INFO *pVideoCameraControlNotifyInfo = NULL;

	if (lParam != WM_MAGIC_VEMCUCTL)
		return 0;

	if (wParam == 0)
		return 0;

	pVideoCameraControlNotifyInfo = (VIDEO_CAMERA_CONTROL_NOTIFY_INFO *)wParam;

	strcpy_s(szStationName,sizeof(szStationName),pVideoCameraControlNotifyInfo->szStationName);
	strcpy_s(szDeviceName,sizeof(szDeviceName),pVideoCameraControlNotifyInfo->szDeviceName);
	strcpy_s(szDeviceCode,sizeof(szDeviceCode),pVideoCameraControlNotifyInfo->szDeviceCode);
	strcpy_s(szUserName,sizeof(szUserName),pVideoCameraControlNotifyInfo->szUserName);

	SYSTEMTIME sysTm;
	::GetLocalTime(&sysTm);

	sprintf_s(szTime,sizeof(szTime),"%04d-%02d-%02dT%02d:%02d:%02dZ",sysTm.wYear,sysTm.wMonth,sysTm.wDay,sysTm.wHour,sysTm.wMinute,sysTm.wSecond);

	if (m_pDlgLinkageServer != NULL)
	{
		nIndex = m_pDlgLinkageServer->m_listRelation.InsertItem(0,"云镜控制通知");
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,1,szDeviceName);
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,2,szStationName);
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,3,szTime);
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,4,"云镜控制通知");
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,5,"");
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,6,"0");
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,7,"");
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,8,szDeviceCode);
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,9,szUserName);
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,10,szStationName);
		m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,11,"0");
	}

	return 1;
}

LRESULT CVEMCUCtlDlg::OnVideoLinkageWeatherWarningMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		//if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
		//	SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		CheckRelationLinkageListCount();

		//if (m_bMultiFullScreen == false)
		//	ShowSmallRelationList();

		char  szStationID[LINKAGE_ID_LEN] = {0};
		char szWeatherType[LINKAGE_TYPE_LEN] = {0};
		char szTypeName[LINKAGE_TYPE_NAME_LEN] = {0};
		char szWarnValue[LINKAGE_TYPE_VALUE_LEN] = {0};
		char szTime[LINKAGE_TIME_LEN] = {0};
		char szVideoStationName[LINKAGE_STATION_NAME_LEN] = {0};
		int    nCameraNum = 0;
		char szCameraName[LINKAGE_DEVICE_NAME_LEN] = {0};
		char szCameraCode[LINKAGE_DEVICE_CODE_LEN] = {0};

		int    nDecodeTag = -1;
		int    nIndex = -1;
		char szReason[512] = {0};
		char szCameraNum[16] = {0};
		char szDecodeTag[16] = {0};
		char szPresetId[64] = {0};
		char szXYValue[64] = {0};

		COLORREF BKColor = RGB(207,235,250);
		COLORREF TextColor = RGB(116,15,195);

		VIDEO_WEATHER_WARNING_INFO *pVideoWeatherWarningLinkage = NULL;
		VIDEO_LINKAGE_CAMERA_INFO *pLinkageCameraInfo = NULL;
		_T_NODE_PRESET_INFO * pPresetNodeInfo = NULL;
		_T_NODE_INFO * pNodeInfo = NULL;
		HTREEITEM hInfoTreeItem = NULL;

		BOOL bResult = FALSE;
		time_t time_Linkage = time(NULL);

		if (lParam != WM_MAGIC_VEMCUCTL)
			return 0;

		if (wParam == 0)
			return 0;

		g_nViewOutVideoTypeFlag = VIDEO_WEATHER_WARNING_INFO_TYPE;

		pVideoWeatherWarningLinkage = (VIDEO_WEATHER_WARNING_INFO *)wParam;

		strcpy_s(szStationID,sizeof(szStationID),pVideoWeatherWarningLinkage->szStationID);
		strcpy_s(szWeatherType,sizeof(szWeatherType),pVideoWeatherWarningLinkage->szWeatherType);
		strcpy_s(szTypeName,sizeof(szTypeName),pVideoWeatherWarningLinkage->szTypeName);
		strcpy_s(szWarnValue,sizeof(szWarnValue),pVideoWeatherWarningLinkage->szWarnValue);
		strcpy_s(szTime,sizeof(szTime),pVideoWeatherWarningLinkage->szTime);
		strcpy_s(szVideoStationName,sizeof(szVideoStationName),pVideoWeatherWarningLinkage->szLinkageStationName);
		nCameraNum = pVideoWeatherWarningLinkage->nLinkNum;
		pLinkageCameraInfo = pVideoWeatherWarningLinkage->pLinkageCameraInfo;
		sprintf(szCameraNum, "%d", nCameraNum);

		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->CameraTreelistAddCameraByStastion(szVideoStationName);
			m_pDlgPageServer->PresetTreelistAddPresetByStastion(szVideoStationName);
		}

		if (m_pDlgLinkageServer != NULL)
		{
			nIndex = m_pDlgLinkageServer->m_listRelation.InsertItem(0,"气象报警");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,1,szVideoStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,2,szVideoStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,3,szTime);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,4,szTypeName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,5,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,6,"0");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,7,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,8,szTypeName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,9,szWarnValue);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,10,szVideoStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,11,szCameraNum);

			m_pDlgLinkageServer->m_listRelation.SetRowColor(0,BKColor);
			m_pDlgLinkageServer->m_listRelation.SetRowTextColor(0,TextColor);

			// 摄像头列表
			m_pDlgLinkageServer->m_listCameraRelation.DeleteAllItems();
			for (int i = 0; i < nCameraNum; i ++)
			{
				m_pDlgLinkageServer->m_listCameraRelation.InsertItem(0, pLinkageCameraInfo[i].szName);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,1,pLinkageCameraInfo[i].szCode);
				sprintf(szDecodeTag, "%d", pLinkageCameraInfo[i].nDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,2,szDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,3,szVideoStationName);
				sprintf(szPresetId,"%d",pLinkageCameraInfo[i].nPresetId);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,4,szPresetId);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,5,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,6,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,7,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,8,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,9,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,10,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,11,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,12,szXYValue);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,13,pLinkageCameraInfo[i].szPresetName);
			}
		}

		if (m_pDlgLinkageServer != NULL)
		{
			//if(m_pDlgLinkageServer->m_CheckRelateMetroAlarm.GetCheck() == 0)
			//	return 0;

			//if (strcmp(szTypeName,"台风告警") == 0&&m_pDlgLinkageServer->m_CheckRelateTyphoonAlarm.GetCheck() == 0)
			//	return 0;

			//if (strcmp(szTypeName,"雷电告警") == 0&&m_pDlgLinkageServer->m_CheckRelateThunderAlarm.GetCheck() == 0)
			//	return 0;

			if (strcmp(szTypeName,"雷电告警") == 0)
			{
				if (m_pDlgLinkageServer->m_CheckRelateThunderAlarm.GetCheck() == 0)
					return 0;
			}
			else
			{
				if (m_pDlgLinkageServer->m_CheckRelateMetroAlarm.GetCheck() == 0)
					return 0;
			}
		}

		//联动优先级判断
		bResult = g_VideoLinkagePriorityInfo.DecideAndSetVideoLinkagePriorityInfo(VIDEO_WEATHER_WARNING_INFO_TYPE,VIDEO_WEATHER_WARNING_LINKAGE_PRIORITY,time_Linkage);
		if (!bResult)
			return 0;

		//////////////////////////////////////////////////////////////////////////
		if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
			SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		if (m_bMultiFullScreen == false)
			ShowSmallRelationList();
		//////////////////////////////////////////////////////////////////////////

		if (nCameraNum > 0&&pLinkageCameraInfo != NULL)
		{
			SetLinkageForms(nCameraNum);

			if(nCameraNum > MAXVIEWCH)
				nCameraNum = MAXVIEWCH;

			_T_NODE_PRESET_INFO PresetInfo;

			for (int i = 0;i < nCameraNum;i++)
			{
				strcpy_s(szCameraName,sizeof(szCameraName),pLinkageCameraInfo[i].szName);
				strcpy_s(szCameraCode,sizeof(szCameraCode),pLinkageCameraInfo[i].szCode);
				nDecodeTag = pLinkageCameraInfo[i].nDecodeTag;
				sprintf(szReason, "{气象报警}-{%s}-{%s}",szCameraName,szVideoStationName);

				memset(&PresetInfo,0,sizeof(_T_NODE_PRESET_INFO));

				strcpy_s(PresetInfo.node_station,sizeof(PresetInfo.node_station),szVideoStationName);
				strcpy_s(PresetInfo.node_num,sizeof(PresetInfo.node_num),szCameraCode);

				PresetInfo.preset_id = pLinkageCameraInfo->nPresetId;
				PresetInfo.line1_from_x = pLinkageCameraInfo[i].nFromX1;
				PresetInfo.line1_from_y = pLinkageCameraInfo[i].nFromY1;
				PresetInfo.line1_to_x = pLinkageCameraInfo[i].nToX1;
				PresetInfo.line1_to_y = pLinkageCameraInfo[i].nToY1;

				PresetInfo.line2_from_x = pLinkageCameraInfo[i].nFromX2;
				PresetInfo.line2_from_y = pLinkageCameraInfo[i].nFromY2;
				PresetInfo.line2_to_x = pLinkageCameraInfo[i].nToX2;
				PresetInfo.line2_to_y = pLinkageCameraInfo[i].nToY2;

				if (m_pDlgPageServer != NULL)
				{
					hInfoTreeItem = m_pDlgPageServer->SearchPresetTreePresetHandleTreeItem(NULL,szCameraCode,pLinkageCameraInfo[i].szPresetName);
					if (hInfoTreeItem != NULL)
					{
						pPresetNodeInfo = (_T_NODE_PRESET_INFO *)m_pDlgPageServer->m_trPreset.GetItemData(hInfoTreeItem);
						if (pPresetNodeInfo != NULL)
						{
							memcpy(&PresetInfo.camera_info,&pPresetNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
						}
					}
					else
					{
						hInfoTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szCameraCode,NULL,NULL);
						if (hInfoTreeItem != NULL)
						{
							pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hInfoTreeItem);
							if (pNodeInfo != NULL)
							{
								memcpy(&PresetInfo.camera_info,&pNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
							}
						}
					}
				}

				VIDEO_SIP_CALL_PRESET_INFO VideoSipCallPresetInfo;
				memset(&VideoSipCallPresetInfo,0,sizeof(VideoSipCallPresetInfo));

				if (IsCameraVideoLinkByVideoPlatform(g_nClientVideoLinkType,nDecodeTag))
				{
					VideoSipCallPresetInfo.nType = 2;
					VideoSipCallPresetInfo.nDecodeTag = nDecodeTag;
				}
				else
				{
					VideoSipCallPresetInfo.nType = 4;
					VideoSipCallPresetInfo.nDecodeTag = PresetInfo.camera_info.dvr_info.dvr_type;
				}

				strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),szCameraCode);
				strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),szCameraName);
				VideoSipCallPresetInfo.nScreenId = i;
				strcpy_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),szReason);
				memcpy(&VideoSipCallPresetInfo.preset_node_info,&PresetInfo,sizeof(VideoSipCallPresetInfo.preset_node_info));

				g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
				ThreadPoolDispatchTask(ThreadVideoLinkageMakeCall,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);

				Sleep(100);
			}
		}

		return 1;
	}
	catch(...)
	{

	}

	return 0;
}

LRESULT CVEMCUCtlDlg::OnVideoLinkageWeatherForecastMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		//if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
		//	SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		CheckRelationLinkageListCount();

		//if (m_bMultiFullScreen == false)
		//	ShowSmallRelationList();

		char  szID[LINKAGE_ID_LEN] = {0};
		char szWeatherType[LINKAGE_TYPE_LEN] = {0};
		char szTypeName[LINKAGE_TYPE_NAME_LEN] = {0};
		char szWeatherLevel[VIDEO_LINKAGE_INFO_LEN] = {0};
		char szAreaType[VIDEO_LINKAGE_INFO_LEN] = {0};
		char szCityID[VIDEO_LINKAGE_INFO_LEN] = {0};
		char szCountyID[VIDEO_LINKAGE_INFO_LEN] = {0};
		char szContent[LINKAGE_CONTENT_LEN] = {0};
		char szStartTime[LINKAGE_TIME_LEN] = {0};
		char szStopTime[LINKAGE_TIME_LEN] = {0};
		char szVideoStationName[LINKAGE_STATION_NAME_LEN] = {0};

		int    nCameraNum = 0;
		char szCameraName[LINKAGE_DEVICE_NAME_LEN] = {0};
		char szCameraCode[LINKAGE_DEVICE_CODE_LEN] = {0};

		int    nDecodeTag = -1;
		int    nIndex = -1;
		char szReason[512] = {0};
		char szCameraNum[16] = {0};
		char szDecodeTag[16] = {0};
		char szPresetId[64] = {0};
		char szXYValue[64] = {0};

		COLORREF BKColor = RGB(207,235,250);
		COLORREF TextColor = RGB(116,15,195);

		VIDEO_WEATHER_FORECAST_INFO *pVideoWeatherForecastLinkage = NULL;
		VIDEO_LINKAGE_CAMERA_INFO *pLinkageCameraInfo = NULL;
		_T_NODE_PRESET_INFO * pPresetNodeInfo = NULL;
		_T_NODE_INFO * pNodeInfo = NULL;
		HTREEITEM hInfoTreeItem = NULL;

		BOOL bResult = FALSE;
		time_t time_Linkage = time(NULL);

		if (lParam != WM_MAGIC_VEMCUCTL)
			return 0;

		if (wParam == 0)
			return 0;

		g_nViewOutVideoTypeFlag = VIDEO_WEATHER_FORECAST_INFO_TYPE;

		pVideoWeatherForecastLinkage = (VIDEO_WEATHER_FORECAST_INFO *)wParam;

		strcpy_s(szID,sizeof(szID),pVideoWeatherForecastLinkage->szID);
		strcpy_s(szWeatherType,sizeof(szWeatherType),pVideoWeatherForecastLinkage->szWeatherType);
		strcpy_s(szTypeName,sizeof(szTypeName),pVideoWeatherForecastLinkage->szTypeName);
		strcpy_s(szWeatherLevel,sizeof(szWeatherLevel),pVideoWeatherForecastLinkage->szWeatherLevel);
		strcpy_s(szAreaType,sizeof(szAreaType),pVideoWeatherForecastLinkage->szAreaType);
		strcpy_s(szCityID,sizeof(szCityID),pVideoWeatherForecastLinkage->szCityID);
		strcpy_s(szCountyID,sizeof(szCountyID),pVideoWeatherForecastLinkage->szCountyID);
		strcpy_s(szContent,sizeof(szContent),pVideoWeatherForecastLinkage->szContent);
		strcpy_s(szStartTime,sizeof(szStartTime),pVideoWeatherForecastLinkage->szStartTime);
		strcpy_s(szStopTime,sizeof(szStopTime),pVideoWeatherForecastLinkage->szStopTime);
		strcpy_s(szVideoStationName,sizeof(szVideoStationName),pVideoWeatherForecastLinkage->szLinkageStationName);
		nCameraNum = pVideoWeatherForecastLinkage->nLinkNum;
		pLinkageCameraInfo = pVideoWeatherForecastLinkage->pLinkageCameraInfo;
		sprintf(szCameraNum, "%d", nCameraNum);

		if (m_pDlgPageServer != NULL)
		{
			m_pDlgPageServer->CameraTreelistAddCameraByStastion(szVideoStationName);
			m_pDlgPageServer->PresetTreelistAddPresetByStastion(szVideoStationName);
		}

		if (m_pDlgLinkageServer != NULL)
		{
			nIndex = m_pDlgLinkageServer->m_listRelation.InsertItem(0,"气象预警");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,1,szVideoStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,2,szVideoStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,3,szStartTime);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,4,szTypeName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,5,"");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,6,"0");
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,7,szContent);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,8,szTypeName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,9,szWeatherLevel);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,10,szVideoStationName);
			m_pDlgLinkageServer->m_listRelation.SetItemText(nIndex,11,szCameraNum);

			m_pDlgLinkageServer->m_listRelation.SetRowColor(0,BKColor);
			m_pDlgLinkageServer->m_listRelation.SetRowTextColor(0,TextColor);

			// 摄像头列表
			m_pDlgLinkageServer->m_listCameraRelation.DeleteAllItems();
			for (int i = 0; i < nCameraNum; i ++)
			{
				m_pDlgLinkageServer->m_listCameraRelation.InsertItem(0, pLinkageCameraInfo[i].szName);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,1,pLinkageCameraInfo[i].szCode);
				sprintf(szDecodeTag, "%d", pLinkageCameraInfo[i].nDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,2,szDecodeTag);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,3,szVideoStationName);
				sprintf(szPresetId,"%d",pLinkageCameraInfo[i].nPresetId);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,4,szPresetId);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,5,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,6,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,7,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY1);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,8,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,9,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nFromY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,10,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToX2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,11,szXYValue);
				sprintf(szXYValue,"%d",pLinkageCameraInfo[i].nToY2);
				m_pDlgLinkageServer->m_listCameraRelation.SetItemText(nIndex,12,szXYValue);
			}
		}

		if (m_pDlgLinkageServer != NULL)
		{
			//if(m_pDlgLinkageServer->m_CheckRelateMetroPre.GetCheck() == 0)
			//	return 0;

			//if (strcmp(szTypeName,"台风预警") == 0&&m_pDlgLinkageServer->m_CheckRelateTyphoonPre.GetCheck() == 0)
			//	return 0;

			//if (strcmp(szTypeName,"雷电预警") == 0&&m_pDlgLinkageServer->m_CheckRelateThunderPre.GetCheck() == 0)
			//	return 0;

			if (strcmp(szTypeName,"雷电预警") == 0)
			{
				if(m_pDlgLinkageServer->m_CheckRelateThunderAlarm.GetCheck() == 0)
					return 0;
			}
			else
			{
				if (m_pDlgLinkageServer->m_CheckRelateMetroAlarm.GetCheck() == 0)
					return 0;
			}
		}

		//联动优先级判断
		bResult = g_VideoLinkagePriorityInfo.DecideAndSetVideoLinkagePriorityInfo(VIDEO_WEATHER_FORECAST_INFO_TYPE,VIDEO_WEATHER_FORECAST_LINKAGE_PRIORITY,time_Linkage);
		if (!bResult)
			return 0;

		//////////////////////////////////////////////////////////////////////////
		if(m_nMenuItemIndex != VEM_WIN_ITEM_TYPE_MULTIVIEW)
			SendMessage(OM_CONTROLBUTTON,IDC_MENU_VIEW,VEM_CONTROL_BUTTON_MESSAGE_TYPE_MENU);

		if (m_bMultiFullScreen == false)
			ShowSmallRelationList();
		//////////////////////////////////////////////////////////////////////////

		if (nCameraNum > 0&&pLinkageCameraInfo != NULL)
		{
			SetLinkageForms(nCameraNum);

			if(nCameraNum > MAXVIEWCH)
				nCameraNum = MAXVIEWCH;

			_T_NODE_PRESET_INFO PresetInfo;

			for (int i = 0;i < nCameraNum;i++)
			{
				strcpy_s(szCameraName,sizeof(szCameraName),pLinkageCameraInfo[i].szName);
				strcpy_s(szCameraCode,sizeof(szCameraCode),pLinkageCameraInfo[i].szCode);
				nDecodeTag = pLinkageCameraInfo[i].nDecodeTag;
				sprintf(szReason, "{气象预警}-{%s}-{%s}",szCameraName, szVideoStationName);

				memset(&PresetInfo,0,sizeof(_T_NODE_PRESET_INFO));

				strcpy_s(PresetInfo.node_station,sizeof(PresetInfo.node_station),szVideoStationName);
				strcpy_s(PresetInfo.node_num,sizeof(PresetInfo.node_num),szCameraCode);

				PresetInfo.preset_id = pLinkageCameraInfo->nPresetId;
				PresetInfo.line1_from_x = pLinkageCameraInfo[i].nFromX1;
				PresetInfo.line1_from_y = pLinkageCameraInfo[i].nFromY1;
				PresetInfo.line1_to_x = pLinkageCameraInfo[i].nToX1;
				PresetInfo.line1_to_y = pLinkageCameraInfo[i].nToY1;

				PresetInfo.line2_from_x = pLinkageCameraInfo[i].nFromX2;
				PresetInfo.line2_from_y = pLinkageCameraInfo[i].nFromY2;
				PresetInfo.line2_to_x = pLinkageCameraInfo[i].nToX2;
				PresetInfo.line2_to_y = pLinkageCameraInfo[i].nToY2;

				if (m_pDlgPageServer != NULL)
				{
					hInfoTreeItem = m_pDlgPageServer->SearchPresetTreePresetHandleTreeItem(NULL,szCameraCode,pLinkageCameraInfo[i].szPresetName);
					if (hInfoTreeItem != NULL)
					{
						pPresetNodeInfo = (_T_NODE_PRESET_INFO *)m_pDlgPageServer->m_trPreset.GetItemData(hInfoTreeItem);
						if (pPresetNodeInfo != NULL)
						{
							memcpy(&PresetInfo.camera_info,&pPresetNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
						}
					}
					else
					{
						hInfoTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szCameraCode,NULL,NULL);
						if (hInfoTreeItem != NULL)
						{
							pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hInfoTreeItem);
							if (pNodeInfo != NULL)
							{
								memcpy(&PresetInfo.camera_info,&pNodeInfo->camera_info,sizeof(PresetInfo.camera_info));
							}
						}
					}
				}

				VIDEO_SIP_CALL_PRESET_INFO VideoSipCallPresetInfo;
				memset(&VideoSipCallPresetInfo,0,sizeof(VideoSipCallPresetInfo));

				if (IsCameraVideoLinkByVideoPlatform(g_nClientVideoLinkType,nDecodeTag))
				{
					VideoSipCallPresetInfo.nType = 2;
					VideoSipCallPresetInfo.nDecodeTag = nDecodeTag;
				}
				else
				{
					VideoSipCallPresetInfo.nType = 4;
					VideoSipCallPresetInfo.nDecodeTag = PresetInfo.camera_info.dvr_info.dvr_type;
				}

				strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),szCameraCode);
				strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),szCameraName);
				VideoSipCallPresetInfo.nScreenId = i;
				strcpy_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),szReason);
				memcpy(&VideoSipCallPresetInfo.preset_node_info,&PresetInfo,sizeof(VideoSipCallPresetInfo.preset_node_info));

				g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
				ThreadPoolDispatchTask(ThreadVideoLinkageMakeCall,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);

				Sleep(100);
			}
		}

		return 1;
	}
	catch(...)
	{

	}

	return 0;
}

LRESULT CVEMCUCtlDlg::OnUpdateUserViewCameraStatusMessageHandler(WPARAM wParam, LPARAM lParam)
{
	UpdateStationCameraVideoInfo();
	return 1;
}

LRESULT CVEMCUCtlDlg::OnDeviceVideoErrorReasonNotifyMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		int nVideoChannelId = (int)wParam;
		if (nVideoChannelId < 0||nVideoChannelId >= MAXVIEWCH)
			return 0;

		E_RtCallStateReason eReason = (E_RtCallStateReason)lParam;

		char szReason[256] = {0};
		char szVideoTitleInfo[1024] = {0};
		bool bFlag = false;

		VIDEO_ERROR_INFO VideoErrorInfo;
		memset(&VideoErrorInfo,0,sizeof(VideoErrorInfo));

		VideoErrorInfo.state = (int)E_RT_CALL_STATE_TERMINATED;
		VideoErrorInfo.reason = (int)eReason;

		g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.Lock();
		strcpy_s(VideoErrorInfo.camera_name,sizeof(VideoErrorInfo.camera_name),g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.m_szCameraName);
		strcpy_s(VideoErrorInfo.camera_code,sizeof(VideoErrorInfo.camera_code),g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.m_szCameraCallNum);
		strcpy_s(VideoErrorInfo.station_name,sizeof(VideoErrorInfo.station_name),g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.m_szStationName);
		g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.UnLock();

		int nIndex = -1;

		nIndex = GetStationIndexByStationName(VideoErrorInfo.station_name);
		if (nIndex < 0)
		{
			nIndex = GetStationIndexByD5000StationName(VideoErrorInfo.station_name);
		}

		if (nIndex >= 0&&nIndex < MAX_STATION_NUM)
		{
			strcpy_s(VideoErrorInfo.station_code,sizeof(VideoErrorInfo.station_code),g_tStation_Info[nIndex].station_code_videoplant);

			nIndex = GetStationNodeInfoByNodeId(g_tStation_Info[nIndex].node_id);

			if (nIndex > 0)
			{
				strcpy_s(VideoErrorInfo.node_name,sizeof(VideoErrorInfo.node_name),g_tStationNode_Info[nIndex].node_name);
			}
		}

		if (strlen(VideoErrorInfo.camera_name) != 0&&strlen(VideoErrorInfo.camera_code) > 10
			&&strlen(VideoErrorInfo.node_name) != 0&&strlen(VideoErrorInfo.station_name) != 0)
		{
			g_VideoErrorInfo.WriteVideoErrorInfo(&VideoErrorInfo);
		}

		switch (eReason)
		{
		case E_RT_CALL_REASON_UNDEFINED://未知错误
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		case E_RT_CALL_REASON_REMOTE_HANGUP://远端挂机
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		case E_RT_CALL_REASON_FAIL_REJECT://4xx消息(主叫,即被拒绝)
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		case E_RT_CALL_REASON_FAIL_SERVER://5xx消息(主叫,即服务端故障)
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		case E_RT_CALL_REASON_FAIL_GLOBAL://6xx消息(主叫,即全局网络故障)
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		case E_RT_CALL_REASON_NETWORK_ERROR://网络故障
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		case E_RT_CALL_REASON_WAIT_PICKUP_TIMEOUT://等待被叫摘机超时(主叫)
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		case E_RT_CALL_REASON_LOCAL_TIMEOUT://本端处理超时
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		case E_RT_CALL_REASON_LOCAL_FAILURE://本端因为某些逻辑失败(内存不够,数据错误等)
			{
				strcpy_s(szReason,sizeof(szReason),"离线");
				bFlag = true;
			}
			break;
		default:
			break;
		}

		if (bFlag)
		{
			g_DlgVideoView[nVideoChannelId].m_strStation.SetText(szReason);
			SetTimer(TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER+nVideoChannelId,2*1000,NULL);
		}

		return 1;
	}
	catch(...)
	{

	}
	return 0;
}

LRESULT CVEMCUCtlDlg::OnDeviceVideoTalkNotifyMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		int nVideoChannelId = (int)wParam;
		if (nVideoChannelId < 0||nVideoChannelId >= MAXVIEWCH)
			return 0;

		int nReason = (int)lParam;

		char szVideoTitleInfo[1024] = {0};
		bool bFlag = false;

		CLIENT_VIDEO_INFO ClientVideoInfo;
		memset(&ClientVideoInfo,0,sizeof(ClientVideoInfo));

		ClientVideoInfo.operate_type = nReason;
		ClientVideoInfo.operate_result = nReason;

		strcpy_s(ClientVideoInfo.user_name,sizeof(ClientVideoInfo.user_name),g_user_config_info.szUserName);

		g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.Lock();
		strcpy_s(ClientVideoInfo.camera_name,sizeof(ClientVideoInfo.camera_name),g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.m_szCameraName);
		strcpy_s(ClientVideoInfo.camera_code,sizeof(ClientVideoInfo.camera_code),g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.m_szCameraCallNum);
		strcpy_s(ClientVideoInfo.station_name,sizeof(ClientVideoInfo.station_name),g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.m_szStationName);
		g_DlgVideoView[nVideoChannelId].m_PreVideoViewOutInfo.UnLock();

		int nIndex = -1;

		nIndex = GetStationIndexByStationName(ClientVideoInfo.station_name);
		if (nIndex < 0)
		{
			nIndex = GetStationIndexByD5000StationName(ClientVideoInfo.station_name);
		}

		if (nIndex >= 0&&nIndex < MAX_STATION_NUM)
		{
			strcpy_s(ClientVideoInfo.station_code,sizeof(ClientVideoInfo.station_code),g_tStation_Info[nIndex].station_code_videoplant);
			ClientVideoInfo.station_id = g_tStation_Info[nIndex].station_id;

			nIndex = GetStationNodeInfoByNodeId(g_tStation_Info[nIndex].node_id);

			if (nIndex > 0)
			{
				strcpy_s(ClientVideoInfo.node_name,sizeof(ClientVideoInfo.node_name),g_tStationNode_Info[nIndex].node_name);
			}
		}

		if (strlen(ClientVideoInfo.camera_name) != 0&&strlen(ClientVideoInfo.camera_code) > 10
			&&strlen(ClientVideoInfo.node_name) != 0&&strlen(ClientVideoInfo.station_name) != 0)
		{
			g_ClientVideoInfo.WriteClientVideoInfo(&ClientVideoInfo);
		}

		return 1;
	}
	catch(...)
	{

	}

	return 0;
}

LRESULT CVEMCUCtlDlg::OnDeviceVideoCloseMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		int nIndex = (int)wParam;

		if (nIndex < 0)
			return 0;

		if (nIndex >= 0&&nIndex < MAXVIEWCH)
		{
			g_DlgVideoView[nIndex].m_btnTypeFlag.ShowWindow(SW_HIDE);
			ClearVideoInfoDlgInfo(nIndex);
			g_DlgVideoView[nIndex].FlashChannel();
		}

		//设备视频错误通知
		PostMessage(WM_DEVICE_VIDEO_ERROR_REASON_NOTIFY_MESSAGE,wParam,lParam);

		return 1;
	}
	catch(...)
	{

	}
	return 0;
}

LRESULT CVEMCUCtlDlg::OnDeviceVideoQuickRestartMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		int nIndex = (int)wParam;

		if (nIndex < 0)
			return 0;

		//快速重连
		if(QuickStartViewChannelPrevVideoByIndex(nIndex) == FALSE)
		{
			StartViewChannelPrevVideoByIndex(nIndex);
		}
		return 0;
	}
	catch(...)
	{

	}
	return 1;
}

LRESULT CVEMCUCtlDlg::OnElecMapOpenCameraMessageHandler(WPARAM wParam, LPARAM lParam)
{
	char szCameraCode[32] = {0};
	memcpy(szCameraCode, (char*)wParam, 32);

	//dll postmessage传递过来的消息，要释放
	char *p = (char*)wParam;
	if (p != NULL)
	{
		delete[] p;
		p = NULL;
	}

	g_pMainDlg->m_pDlgCameraAndControl->OpenCamera(szCameraCode);
	return 0;
}

LRESULT CVEMCUCtlDlg::OnDeviceVideoStatusOnlineNotifyMessageHandler(WPARAM wParam, LPARAM lParam)
{
	try
	{
		int nIndex = (int)wParam;

		if (nIndex < 0)
			return 0;

		char szCameraCallNum[32] = {0};
		memset(szCameraCallNum,0,sizeof(szCameraCallNum));

		g_DlgVideoView[nIndex].m_VideoViewOutInfo.Lock();
		strcpy_s(szCameraCallNum,sizeof(szCameraCallNum),g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_szCameraCallNum);
		g_DlgVideoView[nIndex].m_VideoViewOutInfo.UnLock();

		if(g_pMainDlg != NULL&&g_pMainDlg->m_pDlgPageServer != NULL&&strlen(szCameraCallNum) > 10)
		{
			g_pMainDlg->m_pDlgPageServer->AutoSetTreeCameraHandleTreeItemAndDatabaseStatus(VM_CAMERA_STATUS_ONLINE_BY_STATUS,szCameraCallNum,NULL,NULL,FALSE);
		}
		return 0;
	}
	catch(...)
	{

	}
	return 1;
}

// 显示放大的联动列表
void CVEMCUCtlDlg::ShowLargeRelationList()
{
	try
	{
		if (IsIconic())
		{
			SendMessage(WM_SYSCOMMAND,SC_RESTORE);
		}

		if (m_bLinkageServerFlag == TRUE&&m_pDlgLinkageServer->m_bRelationListExtendFlag == TRUE)
			return;

		CRect	winrc;
		GetClientRect(winrc);

		m_nLinkageWidth = LINKAGEWIDTH + 300;

		CRect	LinkageServer_rc;

		// 右侧控制窗口
		m_pDlgLinkageServer->ShowWindow(SW_HIDE);
		LinkageServer_rc.top = winrc.top + m_nMenuHight;
		LinkageServer_rc.bottom = winrc.bottom;
		LinkageServer_rc.left = winrc.right-m_nLinkageWidth;
		LinkageServer_rc.right = winrc.right;

		m_pDlgLinkageServer->MoveWindow(LinkageServer_rc);
		m_pDlgLinkageServer->ShowWindow(SW_SHOW);

		if (m_pDlgShowLinkageServer != NULL)
			m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);

		//调整告警展示窗口
		CRect AlarmInfo_rc;
		AlarmInfo_rc.top = winrc.bottom - g_pMainDlg->m_nAlarmHeight;
		AlarmInfo_rc.bottom = winrc.bottom;
		AlarmInfo_rc.left = winrc.left + g_pMainDlg->m_nControlWidth;
		AlarmInfo_rc.right = winrc.right - g_pMainDlg->m_nLinkageWidth;

		if (g_pMainDlg->m_pDlgAlarmInfo != NULL)
		{
			g_pMainDlg->m_pDlgAlarmInfo->MoveWindow(AlarmInfo_rc);
		}

		//多画面区域
		SetForms(m_FormsNum,TRUE);

		m_bLinkageServerFlag = TRUE;
		m_pDlgLinkageServer->m_bRelationListExtendFlag = TRUE;
	}
	catch(...)
	{

	}
}

// 显示缩小的联动列表
void CVEMCUCtlDlg::ShowSmallRelationList()
{
	try
	{
		if (IsIconic())
		{
			SendMessage(WM_SYSCOMMAND,SC_RESTORE);
		}

		if (m_bLinkageServerFlag == TRUE&&m_pDlgLinkageServer->m_bRelationListExtendFlag == FALSE)
			return;

		CRect	winrc;
		GetClientRect(winrc);

		m_nLinkageWidth = LINKAGEWIDTH;

		CRect	LinkageServer_rc;

		// 右侧控制窗口
		m_pDlgLinkageServer->ShowWindow(SW_HIDE);
		LinkageServer_rc.top = winrc.top + m_nMenuHight;
		LinkageServer_rc.bottom = winrc.bottom;
		LinkageServer_rc.left = winrc.right-m_nLinkageWidth;
		LinkageServer_rc.right = winrc.right;

		m_pDlgLinkageServer->MoveWindow(LinkageServer_rc);
		m_pDlgLinkageServer->ShowWindow(SW_SHOW);

		if (m_pDlgShowLinkageServer != NULL)
			m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);

		//多画面区域
		SetForms(m_FormsNum,TRUE);

		m_bLinkageServerFlag = TRUE;
		m_pDlgLinkageServer->m_bRelationListExtendFlag = FALSE;
	}
	catch(...)
	{

	}
}

//根据标志显示联动列表
void CVEMCUCtlDlg::DisplayLinkageServerRelationListByFlag(BOOL bFlag)
{
	try
	{
		if(m_pDlgLinkageServer == NULL||m_pDlgShowLinkageServer == NULL)
			return;

#if VM_SJ_CLIENT_VERSION
		{
			m_pDlgLinkageServer->ShowWindow(SW_HIDE);
			m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);
		}
#else
		{
			if (bFlag != FALSE)
			{
				m_pDlgLinkageServer->ShowWindow(SW_SHOW);
				m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);
			}
			else
			{
				m_pDlgLinkageServer->ShowWindow(SW_HIDE);
				m_pDlgShowLinkageServer->ShowWindow(SW_SHOW);

				//调整告警展示窗口
				CRect winrc;
				GetClientRect(winrc);
				CRect AlarmInfo_rc;
				AlarmInfo_rc.top = winrc.bottom - g_pMainDlg->m_nAlarmHeight;
				AlarmInfo_rc.bottom = winrc.bottom;
				AlarmInfo_rc.left = winrc.left + g_pMainDlg->m_nControlWidth;
				AlarmInfo_rc.right = winrc.right - g_pMainDlg->m_nLinkageWidth;

				if (g_pMainDlg->m_pDlgAlarmInfo != NULL)
				{
					g_pMainDlg->m_pDlgAlarmInfo->MoveWindow(AlarmInfo_rc);
				}
			}
		}
#endif

	}
	catch(...)
	{

	}
}

//根据标志显示联动按钮
void CVEMCUCtlDlg::DisplayShowLinkageServerByFlag(BOOL bFlag)
{
	try
	{
		if(m_pDlgShowLinkageServer == NULL)
			return;

#if VM_SJ_CLIENT_VERSION
		{
			m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);
		}
#else
		{
			if (bFlag != FALSE)
			{
				m_pDlgShowLinkageServer->ShowWindow(SW_HIDE);
			}
			else
			{
				m_pDlgShowLinkageServer->ShowWindow(SW_SHOW);
			}
		}
#endif

	}
	catch(...)
	{

	}
}

BOOL CVEMCUCtlDlg::GetProgramAppPath()
{
	char Buffer[MAX_PATH] = {0};
	int nResult = 0;
	char *p = NULL;

	try
	{
		memset(Buffer,0,sizeof(Buffer));

		nResult = GetModuleFileName(NULL,Buffer,sizeof(Buffer)-1);
		if (nResult > 0)
		{
			p = strrchr(Buffer,'\\');
			if (p > Buffer)
			{
				memcpy(g_szAppPath,Buffer,p-Buffer);
				return TRUE;
			}
		}
		return FALSE;
	}
	catch(...)
	{

	}
	return FALSE;
}

//从配置文件中读取版本信息
BOOL CVEMCUCtlDlg::GetFileVersionInfoFromFile(char *szVersion,int nLen)
{
	try
	{
		if (nLen == 0)
			return FALSE;

		FILE *pFile = NULL;
		int nCount = 0;
		int nVersionCount = 0;

		char szConfigName[1024] = {0};
		char szInfoName[1024] = {0};
		char buf[256] = {0};

		memset(szConfigName,0,sizeof(szConfigName));
		memset(szInfoName,0,sizeof(szInfoName));

		sprintf_s(szConfigName,sizeof(szConfigName)-1,"%s\\MyUpdate.dat",g_szAppPath);

		pFile = fopen(szConfigName,"rb");
		if (pFile == NULL)
			return FALSE;

		nCount = fread(g_FileDataInfo,1,sizeof(g_FileDataInfo)-1,pFile);
		if (nCount <= 0)
			return FALSE;

		memset(g_szCurrentVersion,0,sizeof(g_szCurrentVersion));

		for (int i = 0;i < nCount;i++)
		{
			if (g_FileDataInfo[i] == '\r')
				break;

			if ((g_FileDataInfo[i] >= '0'&&g_FileDataInfo[i] <='9')||g_FileDataInfo[i] == '.')
			{
				if (nVersionCount >= sizeof(g_szCurrentVersion))
					break;

				g_szCurrentVersion[nVersionCount] = g_FileDataInfo[i];
				nVersionCount++;
			}
		}

		strcpy_s(szVersion,nLen,g_szCurrentVersion);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

//得到本程序的版本信息
BOOL CVEMCUCtlDlg::GetFileVersionInfoFromApp(char *szVersion,int nLen)
{
	try
	{
		BOOL bResult = FALSE;
		DWORD dwResult = 0;
		DWORD dwVersionHandle = NULL;
		VS_FIXEDFILEINFO *pFileInfo = NULL;
		unsigned int uFileInfoSize = 0;

		char szFullAppPath[MAX_PATH] = {0};
		memset(szFullAppPath,0,sizeof(szFullAppPath));

		dwResult = GetModuleFileName(NULL,szFullAppPath,sizeof(szFullAppPath)-1);
		if (dwResult == 0)
			return FALSE;

		dwResult = GetFileVersionInfoSize(szFullAppPath,&dwVersionHandle);
		if (dwResult == 0||dwResult >= sizeof(g_szCurrentVersion))
			return FALSE;

		memset(g_szCurrentVersion,0,sizeof(g_szCurrentVersion));

		bResult = GetFileVersionInfo(szFullAppPath,dwVersionHandle,dwResult,g_szCurrentVersion);
		if (bResult == FALSE)
			return FALSE;

		bResult = VerQueryValue(g_szCurrentVersion,"\\",(void**)&pFileInfo,&uFileInfoSize);
		if (bResult == FALSE||pFileInfo == NULL)
			return FALSE;

		sprintf_s(szVersion,nLen,"%d.%d.%d.%d",
			HIWORD(pFileInfo->dwProductVersionMS),
			LOWORD(pFileInfo->dwProductVersionMS),
			HIWORD(pFileInfo->dwProductVersionLS),
			LOWORD(pFileInfo->dwProductVersionLS)
			);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

//得到程序的版本信息
BOOL CVEMCUCtlDlg::GetProgramAppVersionInfo()
{
	BOOL bResult = FALSE;
	char szAppVersionInfo[64] = {0};
	memset(szAppVersionInfo,0,sizeof(szAppVersionInfo));

	bResult = GetFileVersionInfoFromFile(szAppVersionInfo,sizeof(szAppVersionInfo)-1);//从配置文件读取版本信息
	if (bResult == FALSE)
	{
		memset(szAppVersionInfo,0,sizeof(szAppVersionInfo));
		bResult = GetFileVersionInfoFromApp(szAppVersionInfo,sizeof(szAppVersionInfo)-1);//读取程序版本信息
		if (bResult == FALSE)
		{
			sprintf_s(szAppVersionInfo,sizeof(szAppVersionInfo)-1,"1.0.0.1");//最初版本
		}
	}

	memset(g_szCurrentVersion,0,sizeof(g_szCurrentVersion));
	strcpy_s(g_szCurrentVersion,sizeof(g_szCurrentVersion)-1,szAppVersionInfo);

	return TRUE;
}

void CVEMCUCtlDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	try
	{
		if (nID == SC_RESTORE)
		{
			if (m_nMenuItemIndex == VEM_WIN_ITEM_TYPE_MULTIVIEW)
			{
				if (m_bPageServerFlag == FALSE)
				{
					if (m_pDlgShowPageServer != NULL)
						m_pDlgShowPageServer->ShowWindow(SW_SHOW);
				}

				DisplayShowLinkageServerByFlag(m_bLinkageServerFlag);
			}

			if (m_bMenuFlag == FALSE)
			{
				if (m_pDlgShowControl != NULL)
					m_pDlgShowControl->ShowWindow(SW_SHOW);
			}

			for (int i = 0;i < m_FormsNum;i++)
			{
				ShowVideoInfoDlgByFlag(i);
				DrawRect(i,g_DlgVideoView[i].m_bRecordFlag);
			}
		}
	}
	catch(...)
	{

	}

	CDialog::OnSysCommand(nID, lParam);
}

BOOL CVEMCUCtlDlg::InitVemAllWeatherInfo()
{
	for(int i = 0;i < 6;i++)
	{
		InitVemWeatherInfo(i);
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::UnInitVemAllWeatherInfo()
{
	for (int i = 0;i < 6;i++)
	{
		UnInitVemWeatherInfo(i);
	}
	return TRUE;
}

BOOL CVEMCUCtlDlg::InitVemWeatherInfo(int nIndex)
{
	SetConfigInfo(nIndex,app_StackCfg.szLocalUdpAddress,0,"172.17.32.95",7001);
	SetWeatherInfoCallback(nIndex,VemcuctlWeatherDataDealWith,this);
	InitWeatherInfo(nIndex);
	return TRUE;
}

BOOL CVEMCUCtlDlg::StartVemWeatherInfo(int nIndex)
{
	StartWeatherInfo(nIndex);
	return TRUE;
}

BOOL CVEMCUCtlDlg::StopVemWeatherInfo(int nIndex)
{
	StopWeatherInfo(nIndex);
	return TRUE;
}

BOOL CVEMCUCtlDlg::UnInitVemWeatherInfo(int nIndex)
{
	UnInitWeatherInfo(nIndex);
	return TRUE;
}

BOOL CVEMCUCtlDlg::VemSendCurrentWeatherInfoRequest(int nIndex,char *szUserKey,int nStationID,int nTimeRange)
{
	try
	{
		if (g_nWeatherCurrentInfoType != GET_WEATHER_INFO_TYPE_MIN&&time(NULL) - g_WeatherCurrentInfoTime <  10)
			return FALSE;

		DWORD dwResult = 0;
		ResetEvent(g_hWeatherCurrentInfoEvent);

		g_nWeatherCurrentInfoType = GET_WEATHER_INFO_TYPE_CURRENT_STATION_WEATHER;
		g_WeatherCurrentInfoTime = time(NULL);
		g_nWeatherCurrentInfoIndex = nIndex;

		StartVemWeatherInfo(0);

		SendCurrentWeatherInfoRequest(0,szUserKey,nStationID,nTimeRange);
		dwResult = WaitForSingleObject(g_hWeatherCurrentInfoEvent,1000*15);
		if (dwResult != WAIT_OBJECT_0)
		{
			g_nWeatherCurrentInfoType = GET_WEATHER_INFO_TYPE_MIN;
		}

		StopVemWeatherInfo(0);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CVEMCUCtlDlg::VemSendHistoryWeatherInfoRequest(int nIndex,char *szUserKey,int nStationID,char *szStartTime,char *szStopTime,char *szSortColumn,int nPageRecords,int nPageIndex)
{
	try
	{
		if (g_nWeatherHistoryInfoType != GET_WEATHER_INFO_TYPE_MIN&&time(NULL) - g_WeatherHistoryInfoTime <  10)
			return FALSE;

		DWORD dwResult = 0;
		ResetEvent(g_hWeatherHistoryInfoEvent);

		g_nWeatherHistoryInfoType = GET_WEATHER_INFO_TYPE_HISTORY_STATION_WEATHER;
		g_WeatherHistoryInfoTime = time(NULL);
		g_nWeatherHistoryInfoIndex = nIndex;

		StartVemWeatherInfo(1);

		SendHistoryWeatherInfoRequest(1,szUserKey,nStationID,szStartTime,szStopTime,szSortColumn,nPageRecords,nPageIndex);

		dwResult = WaitForSingleObject(g_hWeatherHistoryInfoEvent,1000*15);
		if (dwResult != WAIT_OBJECT_0)
		{
			g_nWeatherHistoryInfoType = GET_WEATHER_INFO_TYPE_MIN;
		}

		StopVemWeatherInfo(1);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CVEMCUCtlDlg::VemSendHistoryWarningInfoRequest(int nIndex,char *szUserKey,int nStationID,char *szStartTime,char *szStopTime,char *szSortColumn,int nPageRecords,int nPageIndex)
{
	try
	{
		if (g_nWeatherHistoryWarningInfoType != GET_WEATHER_INFO_TYPE_MIN&&time(NULL) - g_WeatherHistoryWarningInfoTime <  10)
			return FALSE;

		DWORD dwResult = 0;
		ResetEvent(g_hWeatherHistoryWarningInfoEvent);

		g_nWeatherHistoryWarningInfoType = GET_WEATHER_INFO_TYPE_HISTORY_WARNING_WEATHER;
		g_WeatherHistoryWarningInfoTime = time(NULL);
		g_nWeatherHistoryWarningInfoIndex = nIndex;

		StartVemWeatherInfo(2);

		SendHistoryWarningInfoRequest(2,szUserKey,nStationID,szStartTime,szStopTime,szSortColumn,nPageRecords,nPageIndex);

		dwResult = WaitForSingleObject(g_hWeatherHistoryWarningInfoEvent,1000*15);
		if (dwResult != WAIT_OBJECT_0)
		{
			g_nWeatherHistoryWarningInfoType = GET_WEATHER_INFO_TYPE_MIN;
		}

		StopVemWeatherInfo(2);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CVEMCUCtlDlg::VemSendTyphoonInfoRequest(int nIndex,char *szUserKey,int nTFID,char *szStartTime,char *szStopTime,char *szSortColumn,int nPageRecords,int nPageIndex)
{
	try
	{
		if (g_nWeatherTyphoonWarningInfoType != GET_WEATHER_INFO_TYPE_MIN&&time(NULL) - g_WeatherTyphoonWarningInfoTime <  10)
			return FALSE;

		DWORD dwResult = 0;
		ResetEvent(g_hWeatherTyphoonWarningInfoEvent);

		g_nWeatherTyphoonWarningInfoType = GET_WEATHER_INFO_TYPE_TYPHOON_WARNING_WEATHER;
		g_WeatherTyphoonWarningInfoTime = time(NULL);
		g_nWeatherTyphoonWarningInfoIndex = nIndex;

		StartVemWeatherInfo(3);

		SendTyphoonInfoRequest(3,szUserKey,nTFID,szStartTime,szStopTime,szSortColumn,nPageRecords,nPageIndex);

		dwResult = WaitForSingleObject(g_hWeatherTyphoonWarningInfoEvent,1000*15);
		if (dwResult != WAIT_OBJECT_0)
		{
			g_nWeatherTyphoonWarningInfoType = GET_WEATHER_INFO_TYPE_MIN;
		}

		StopVemWeatherInfo(3);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CVEMCUCtlDlg::VemSendCurrentMicroWeatherInfoRequest(int nIndex,char *szUserKey,int nComCode,char *szComType,int nTimeRange)
{
	try
	{
		if (g_nMicroWeatherCurrentInfoType != GET_WEATHER_INFO_TYPE_MIN&&time(NULL) - g_MicroWeatherCurrentInfoTime <  10)
			return FALSE;

		DWORD dwResult = 0;
		ResetEvent(g_hMicroWeatherCurrentInfoEvent);

		g_nMicroWeatherCurrentInfoType = GET_WEATHER_INFO_TYPE_CURRENT_STATION_MICRO_WEATHER;
		g_MicroWeatherCurrentInfoTime = time(NULL);
		g_nMicroWeatherCurrentInfoIndex = nIndex;

		StartVemWeatherInfo(4);

		SendCurrentMicroWeatherInfoRequest(4,szUserKey,nComCode,szComType,nTimeRange);

		dwResult = WaitForSingleObject(g_hMicroWeatherCurrentInfoEvent,1000*15);
		if (dwResult != WAIT_OBJECT_0)
		{
			g_nMicroWeatherCurrentInfoType = GET_WEATHER_INFO_TYPE_MIN;
		}

		StopVemWeatherInfo(4);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CVEMCUCtlDlg::VemSendHistoryMicroWeatherInfoRequest(int nIndex,char *szUserKey,int nComCode,char *szComType,char *szStartTime,char *szStopTime,char *szSortColumn,int nPageRecords,int nPageIndex)
{
	try
	{
		if (g_nMicroWeatherHistoryInfoType != GET_WEATHER_INFO_TYPE_MIN&&time(NULL) - g_MicroWeatherHistoryInfoTime <  10)
			return FALSE;

		DWORD dwResult = 0;
		ResetEvent(g_hMicroWeatherHistoryInfoEvent);

		g_nMicroWeatherHistoryInfoType = GET_WEATHER_INFO_TYPE_HISTORY_STATION_MICRO_WEATHER;
		g_MicroWeatherHistoryInfoTime = time(NULL);
		g_nMicroWeatherHistoryInfoIndex = nIndex;

		StartVemWeatherInfo(5);

		SendHistoryMicroWeatherInfoRequest(5,szUserKey,nComCode,szComType,szStartTime,szStopTime,szSortColumn,nPageRecords,nPageIndex);

		dwResult = WaitForSingleObject(g_hMicroWeatherHistoryInfoEvent,1000*15);
		if (dwResult != WAIT_OBJECT_0)
		{
			g_nMicroWeatherHistoryInfoType = GET_WEATHER_INFO_TYPE_MIN;
		}

		StopVemWeatherInfo(5);

		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

void CVEMCUCtlDlg::ShowVideoWinWeatherCurrentInfoOnTimer()
{
	try
	{
		int nShowCount = 0;
		char szWeatherInfo[1024] = {0};
		CString  strOldWeatherInfo;
		int    nWeatherInfoLen = 0;

		for (int i = 0;i < MAXVIEWCH;i++)
		{
			if (g_CurrentWeatherDataWinInfo[i].nFlag == 1)
			{
				if (nShowCount > 5)
					break;

				nShowCount++;
				sprintf_s(szWeatherInfo+nWeatherInfoLen,sizeof(szWeatherInfo)-nWeatherInfoLen,"窗口%d实时气象->%.1f摄氏度,%.1f%%湿度,%.1f度风向,%.1f米/秒,%.1f毫米/小时,%.1f千帕\r\n",i,
					g_CurrentWeatherDataWinInfo[i].temperature,
					g_CurrentWeatherDataWinInfo[i].humidity,
					g_CurrentWeatherDataWinInfo[i].ave_wd_2min,
					g_CurrentWeatherDataWinInfo[i].ave_ws_2min,
					g_CurrentWeatherDataWinInfo[i].rain_10min*6,
					g_CurrentWeatherDataWinInfo[i].air_pressure);

				nWeatherInfoLen = strlen(szWeatherInfo);
			}
		}

		//显示历史气象
		nShowCount = 0;
		for (int i = 0;i < MAXVIEWCH;i++)
		{
			if (g_HistoryWeatherDataWinInfo[i].nFlag == 1)
			{
				if (nShowCount > 5)
					break;

				nShowCount++;
				sprintf_s(szWeatherInfo+nWeatherInfoLen,sizeof(szWeatherInfo)-nWeatherInfoLen,"回放窗口%d历史气象->%.1f摄氏度,%.1f%%湿度,%.1f度风向,%.1f米/秒,%.1f毫米/小时,%.1f千帕\r\n",i,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].temperature,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].humidity,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].ave_wd_2min,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].ave_ws_2min,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].rain_10min*6,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].air_pressure);

				nWeatherInfoLen = strlen(szWeatherInfo);
			}
		}


		if (m_pDlgMenu != NULL)
		{
			RECT rect;
			strOldWeatherInfo = m_pDlgMenu->m_staticShowWeather.GetText();

			if (strOldWeatherInfo.Compare(szWeatherInfo) != 0)
			{
				m_pDlgMenu->m_staticShowWeather.SetText(szWeatherInfo,10,RGB(0,106,110));

				CWnd *pWnd = m_pDlgMenu->GetDlgItem(IDC_WEATHER_SHOW);
				if (pWnd != NULL)
				{
					pWnd->GetWindowRect(&rect);
					ScreenToClient(&rect);
					InvalidateRect(&rect,TRUE);
				}
			}
		}
	}
	catch(...)
	{

	}
}

void CVEMCUCtlDlg::ShowVideoWinWeatherCurrentInfoOnTimer2()
{
	try
	{
		int nShowCount = 0;
		char szWeatherInfo[1024] = {0};
		CString  strOldWeatherInfo;
		int    nWeatherInfoLen = 0;

		for (int i = 0;i < MAXVIEWCH;i++)
		{
			if (g_CurrentWeatherDataWinInfo[i].nFlag == 1)
			{
				if (nShowCount > 9)
					break;

				nShowCount++;
				sprintf_s(szWeatherInfo,sizeof(szWeatherInfo),"%.1f摄氏度,%.1f%%湿度,%.1f度风向,%.1f米/秒,%.1f毫米/小时,%.1f千帕\r\n",
					g_CurrentWeatherDataWinInfo[i].temperature,
					g_CurrentWeatherDataWinInfo[i].humidity,
					g_CurrentWeatherDataWinInfo[i].ave_wd_2min,
					g_CurrentWeatherDataWinInfo[i].ave_ws_2min,
					g_CurrentWeatherDataWinInfo[i].rain_10min*6,
					g_CurrentWeatherDataWinInfo[i].air_pressure);

				nWeatherInfoLen = strlen(szWeatherInfo);

				g_DlgVideoView[i].m_strWeather.GetWindowText(strOldWeatherInfo);
				if (strOldWeatherInfo.Compare(szWeatherInfo) != 0)
				{
					g_DlgVideoView[i].m_strWeather.SetTransparent(TRUE);
					g_DlgVideoView[i].m_strWeather.SetTextColor(RGB(23, 92, 85));
					g_DlgVideoView[i].m_strWeather.SetText(szWeatherInfo);
				}
			}
			else
			{
				//			sprintf_s(szWeatherInfo,sizeof(szWeatherInfo),"无气象信息");
				sprintf_s(szWeatherInfo,sizeof(szWeatherInfo)," ");

				g_DlgVideoView[i].m_strWeather.GetWindowText(strOldWeatherInfo);
				if (strOldWeatherInfo.Compare(szWeatherInfo) != 0)
				{
					g_DlgVideoView[i].m_strWeather.SetTransparent(TRUE);
					g_DlgVideoView[i].m_strWeather.SetTextColor(RGB(23, 92, 85));
					g_DlgVideoView[i].m_strWeather.SetText(szWeatherInfo);
				}
			}
		}

		//显示历史气象
		nShowCount = 0;
		for (int i = 0;i < 1;i++)
		{
			if (g_HistoryWeatherDataWinInfo[i].nFlag == 1)
			{
				if (nShowCount > 10)
					break;

				nShowCount++;
				sprintf_s(szWeatherInfo,sizeof(szWeatherInfo)-nWeatherInfoLen,"%.1f摄氏度,%.1f%%湿度,%.1f度风向,%.1f米/秒,%.1f毫米/小时,%.1f千帕\r\n",
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].temperature,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].humidity,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].ave_wd_2min,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].ave_ws_2min,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].rain_10min*6,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].air_pressure);

				nWeatherInfoLen = strlen(szWeatherInfo);

				if (g_pDlgReplay != NULL)
				{
					g_pDlgReplay->m_ch.m_strWeather.GetWindowText(strOldWeatherInfo);
					if (strOldWeatherInfo.Compare(szWeatherInfo) != 0)
					{
						g_pDlgReplay->m_ch.m_strWeather.SetTransparent(TRUE);
						g_pDlgReplay->m_ch.m_strWeather.SetTextColor(RGB(23, 92, 85));
						g_pDlgReplay->m_ch.m_strWeather.SetText(szWeatherInfo);
					}
				}
			}
			else
			{
				//			sprintf_s(szWeatherInfo,sizeof(szWeatherInfo),"无气象信息");
				sprintf_s(szWeatherInfo,sizeof(szWeatherInfo)," ");

				g_pDlgReplay->m_ch.m_strWeather.GetWindowText(strOldWeatherInfo);
				if (strOldWeatherInfo.Compare(szWeatherInfo) != 0)
				{
					g_pDlgReplay->m_ch.m_strWeather.SetTransparent(TRUE);
					g_pDlgReplay->m_ch.m_strWeather.SetTextColor(RGB(23, 92, 85));
					g_pDlgReplay->m_ch.m_strWeather.SetText(szWeatherInfo);
				}
			}
		}
	}
	catch(...)
	{

	}
}

BOOL CVEMCUCtlDlg::StartVideoWinWeatherCurrentInfo(int nIndex,char *szUserKey,int nStationId,int nTimeRange)
{
	try
	{
		if (nIndex < 0||nIndex > MAXVIEWCH)
			return FALSE;

		if (szUserKey == NULL||strlen(szUserKey) == 0||nStationId < 0||nTimeRange < 0)
			return FALSE;

		strcpy_s(g_CurrentWeatherDataWinInfo[nIndex].szUserKey,sizeof(g_CurrentWeatherDataWinInfo[nIndex].szUserKey),szUserKey);
		g_CurrentWeatherDataWinInfo[nIndex].nStationID = nStationId;
		g_CurrentWeatherDataWinInfo[nIndex].nTimeRange = nTimeRange;
		g_CurrentWeatherDataWinInfo[nIndex].nFlag = 1;

		SetTimer(TIMER_WEATHER_CHANNEL_0_TIMER+nIndex,500,NULL);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CVEMCUCtlDlg::StopVideoWinWeatherCurrentInfo(int nIndex)
{
	if (nIndex < 0||nIndex > MAXVIEWCH)
		return FALSE;

	try
	{
		KillTimer(TIMER_WEATHER_CHANNEL_0_TIMER+nIndex);
		g_CurrentWeatherDataWinInfo[nIndex].nFlag = 0;
		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

void CVEMCUCtlDlg::ShowVideoWinMicroWeatherCurrentInfoOnTimer()
{
	try
	{
		int nShowCount = 0;
		char szWeatherInfo[1024] = {0};
		CString  strOldWeatherInfo;
		int    nWeatherInfoLen = 0;

		for (int i = 0;i < MAXVIEWCH;i++)
		{
			if (g_CurrentMicroWeatherDataWinInfo[i].nFlag == 1)
			{
				if (nShowCount > 5)
					break;

				nShowCount++;
				sprintf_s(szWeatherInfo+nWeatherInfoLen,sizeof(szWeatherInfo)-nWeatherInfoLen,"窗口%d微气象->%.1f摄氏度,%.1f%%湿度,%.1f度风向,%.1f米/秒,%.1f毫米/小时,%.1f千帕,%.1f,%s\r\n",i,
					g_CurrentMicroWeatherDataWinInfo[i].temperature,
					g_CurrentMicroWeatherDataWinInfo[i].humidity,
					g_CurrentMicroWeatherDataWinInfo[i].wind_direction,
					g_CurrentMicroWeatherDataWinInfo[i].wind_speed,
					g_CurrentMicroWeatherDataWinInfo[i].precipitation,
					g_CurrentMicroWeatherDataWinInfo[i].air_pressure,
					g_CurrentMicroWeatherDataWinInfo[i].radiation,
					g_CurrentMicroWeatherDataWinInfo[i].precipitation_type);

				nWeatherInfoLen = strlen(szWeatherInfo);
			}
		}

		//显示历史气象
		nShowCount = 0;
		for (int i = 0;i < MAXVIEWCH;i++)
		{
			if (g_HistoryWeatherDataWinInfo[i].nFlag == 1)
			{
				if (nShowCount > 5)
					break;

				nShowCount++;
				sprintf_s(szWeatherInfo+nWeatherInfoLen,sizeof(szWeatherInfo)-nWeatherInfoLen,"回放窗口%d历史气象->%.1f摄氏度,%.1f%%湿度,%.1f度风向,%.1f米/秒,%.1f毫米/小时,%.1f千帕\r\n",i,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].temperature,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].humidity,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].ave_wd_2min,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].ave_ws_2min,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].rain_10min*6,
					g_HistoryWeatherDataWinInfo[i].DataRecordInfo[0].air_pressure);

				nWeatherInfoLen = strlen(szWeatherInfo);
			}
		}

		if (m_pDlgMenu != NULL)
		{
			RECT rect;
			strOldWeatherInfo = m_pDlgMenu->m_staticShowWeather.GetText();

			if (strOldWeatherInfo.Compare(szWeatherInfo) != 0)
			{
				m_pDlgMenu->m_staticShowWeather.SetText(szWeatherInfo,10,RGB(0,106,110));

				CWnd *pWnd = m_pDlgMenu->GetDlgItem(IDC_WEATHER_SHOW);
				if (pWnd != NULL)
				{
					pWnd->GetWindowRect(&rect);
					ScreenToClient(&rect);
					InvalidateRect(&rect,TRUE);
				}
			}
		}
	}
	catch(...)
	{

	}
}

void CVEMCUCtlDlg::ShowVideoWinMicroWeatherCurrentInfoOnTimer2()
{

}

BOOL CVEMCUCtlDlg::StartVideoWinMicroWeatherCurrentInfo(int nIndex,char *szUserKey,int nComCode,char *szComType,int nTimeRange)
{
	try
	{

		if (nIndex < 0||nIndex > MAXVIEWCH)
			return FALSE;

		if (szUserKey == NULL||strlen(szUserKey) == 0||nComCode < 0||szComType == NULL||strlen(szComType) == 0||nTimeRange < 0)
			return FALSE;

		strcpy_s(g_CurrentMicroWeatherDataWinInfo[nIndex].szUserKey,sizeof(g_CurrentMicroWeatherDataWinInfo[nIndex].szUserKey),szUserKey);
		g_CurrentMicroWeatherDataWinInfo[nIndex].nComCode = nComCode;
		strcpy_s(g_CurrentMicroWeatherDataWinInfo[nIndex].szComType,sizeof(g_CurrentMicroWeatherDataWinInfo[nIndex].szComType),szComType);
		g_CurrentMicroWeatherDataWinInfo[nIndex].nTimeRange = nTimeRange;
		g_CurrentMicroWeatherDataWinInfo[nIndex].nFlag = 1;

		SetTimer(TIMER_MICRO_WEATHER_CHANNEL_0_TIMER+nIndex,500,NULL);

		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

BOOL CVEMCUCtlDlg::StopVideoWinMicroWeatherCurrentInfo(int nIndex)
{
	if (nIndex < 0||nIndex > MAXVIEWCH)
		return FALSE;

	try
	{
		KillTimer(TIMER_MICRO_WEATHER_CHANNEL_0_TIMER+nIndex);
		g_CurrentMicroWeatherDataWinInfo[nIndex].nFlag = 0;
		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

void CVEMCUCtlDlg::ShowVideoWinWeatherCurrentInfoOnTimerByFlag()
{

//省检
#if VM_SJ_CLIENT_VERSION
	{
		return;
	}
#endif

	try
	{
		if (g_nCurrentWeatherDataWinInfoTypeFlag != 0)
			ShowVideoWinWeatherCurrentInfoOnTimer2();
		else
			ShowVideoWinMicroWeatherCurrentInfoOnTimer();
	}
	catch(...)
	{

	}
}

BOOL CVEMCUCtlDlg::StopVideoWinWeatherCurrentInfoByFlag(int nIndex)
{
	try
	{
		if (g_nCurrentWeatherDataWinInfoTypeFlag != 0)
			return StopVideoWinWeatherCurrentInfo(nIndex);
		else
			return StopVideoWinMicroWeatherCurrentInfo(nIndex);
	}
	catch(...)
	{

	}
	return FALSE;
}

int CVEMCUCtlDlg::GetStationIndexByStationName(char *szStationName)
{
	try
	{
		if (szStationName == NULL||strlen(szStationName) == 0)
			return -1;

		for (int i = 0;i < g_nStation_Num;i++)
		{
			if(strcmp(g_tStation_Info[i].station_name_videoplant,szStationName) == 0)
				return i;
		}

		return -1;
	}
	catch(...)
	{

	}
	return -1;
}

BOOL CVEMCUCtlDlg::StartVideoWinWeatherHistoryInfo(int nIndex,char *szUserKey,int nStationId,char *szStartTime,char *szStopTime)
{
	try
	{
		if (nIndex < 0||nIndex > MAXVIEWCH)
			return FALSE;

		if (szUserKey == NULL||strlen(szUserKey) == 0||nStationId < 0||szStartTime == NULL
			||strlen(szStartTime) == 0||szStopTime == NULL||strlen(szStopTime) == 0)
			return FALSE;

		strcpy_s(g_HistoryWeatherDataWinInfo[nIndex].szUserKey,sizeof(g_HistoryWeatherDataWinInfo[nIndex].szUserKey),szUserKey);
		g_HistoryWeatherDataWinInfo[nIndex].nStationID = nStationId;
		strcpy_s(g_HistoryWeatherDataWinInfo[nIndex].szStartTime,sizeof(g_HistoryWeatherDataWinInfo[nIndex].szStartTime),szStartTime);
		strcpy_s(g_HistoryWeatherDataWinInfo[nIndex].szStopTime,sizeof(g_HistoryWeatherDataWinInfo[nIndex].szStopTime),szStopTime);
		strcpy_s(g_HistoryWeatherDataWinInfo[nIndex].szSortColumn,sizeof(g_HistoryWeatherDataWinInfo[nIndex].szSortColumn),"DATE_TIME DESC");
		g_HistoryWeatherDataWinInfo[nIndex].nPageRecords = 10;
		g_HistoryWeatherDataWinInfo[nIndex].nPageIndex = 1;
		g_HistoryWeatherDataWinInfo[nIndex].nRecordCount = 0;
		memset(g_HistoryWeatherDataWinInfo[nIndex].DataRecordInfo,0,sizeof(g_HistoryWeatherDataWinInfo[nIndex].DataRecordInfo));
		g_HistoryWeatherDataWinInfo[nIndex].nFlag = 1;

		SetTimer(TIMER_HISTORY_WEATHER_CHANNEL_0_TIMER+nIndex,500,NULL);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CVEMCUCtlDlg::StopVideoWinWeatherHistoryInfo(int nIndex)
{
	if (nIndex < 0||nIndex > MAXVIEWCH)
		return FALSE;

	try
	{
		KillTimer(TIMER_HISTORY_WEATHER_CHANNEL_0_TIMER+nIndex);
		g_HistoryWeatherDataWinInfo[nIndex].nFlag = 0;
		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

int CVEMCUCtlDlg::GetStationIndexByStationCode(char *szStationCode)
{
	try
	{
		if (szStationCode == NULL||strlen(szStationCode) == 0)
			return -1;

		for (int i = 0;i < g_nStation_Num;i++)
		{
			if(strcmp(g_tStation_Info[i].station_code_videoplant,szStationCode) == 0)
				return i;
		}
		return -1;
	}
	catch(...)
	{

	}
	return -1;
}

int CVEMCUCtlDlg::GetStationIndexByD5000StationName(char *szD5000StationName)
{
	try
	{
		if (szD5000StationName == NULL||strlen(szD5000StationName) == 0)
			return -1;

		for (int i = 0;i < g_nStation_Num;i++)
		{
			if(strcmp(g_tStation_Info[i].station_name_d5000,szD5000StationName) == 0)
				return i;
		}

		return -1;
	}
	catch(...)
	{

	}
	return -1;
}

int CVEMCUCtlDlg::GetStationNodeInfoByNodeId(int nNodeId)
{ 
	try
	{
		if (nNodeId < 0)
			return -1;

		for(int i = 0;i < g_nStationNode_Num;i++)
		{
			if (g_tStationNode_Info[i].node_id == nNodeId)
				return i;
		}

		return -1;
	}
	catch(...)
	{

	}
	return -1;
}

//关闭当前视频
void CVEMCUCtlDlg::VemStopCurrentVideo()
{
	try
	{
		int nWinID = m_iFocuseWindowID;
		int nResult = 0;

		if (nWinID >= 0&&nWinID < MAXVIEWCH)
		{
			//关闭重连功能
			KillTimer(TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER+nWinID);

			nResult = CheckViewVideoIsOpen(nWinID);
			if (nResult > 0)
			{
				CloseViewVideoByWinID(nWinID,nResult);
				ClearVideoTagAndOffLineInfoDlg(nWinID);
			}
			else
			{
				//清除显示信息
				ClearVideoPresetLineAndTagInfoDlg(nWinID);

				//停止气象信息显示
				StopVideoWinCurrentWeather(nWinID);

				//停止录像
				CloseVideoRecordByWinRecordFlag(nWinID);

				//停止音频
				CloseVideoAudioByWinRecordFlag(nWinID);

				g_DlgVideoView[nWinID].CloseViewChannel();
			}

			g_DlgVideoView[nWinID].m_btnTypeFlag.ShowWindow(SW_HIDE);
		}
	
		CheckAndSetYtLockState();//设置云台锁定状态
	}
	catch(...)
	{

	}
}

//关闭全部视频
void CVEMCUCtlDlg::VemStopAllVideo()
{
	try
	{
		DWORD dResult = 0;
		int  nResult = 0;

		for (int i = 0;i < MAXVIEWCH;i++)
		{
			//关闭重连功能
			KillTimer(TIMER_VIDEO_ERROR_REASON_VIDEO_CHANNEL_0_TIMER+i);

			nResult = CheckViewVideoIsOpen(i);
			if (nResult > 0)
			{
				CloseViewVideoByWinID(i,nResult);
				ClearVideoTagAndOffLineInfoDlg(i);
			}
			else
			{
				//清除显示信息
				ClearVideoPresetLineAndTagInfoDlg(i);

				//停止气象信息显示
				StopVideoWinCurrentWeather(i);

				//停止录像
				CloseVideoRecordByWinRecordFlag(i);

				//停止音频
				CloseVideoAudioByWinRecordFlag(i);

				g_DlgVideoView[i].CloseViewChannel();
			}

			g_DlgVideoView[i].m_btnTypeFlag.ShowWindow(SW_HIDE);
			g_DlgVideoView[i].StopYtLock();
		}

		CheckAndSetYtLockState();//设置云台锁定状态
	}
	catch (...)
	{
		
	}
}

//显示视频流量
void CVEMCUCtlDlg::CheckShowVideoFlow()
{
	try
	{
		for (int i = 0;i < MAXVIEWCH;i++)
		{
			g_DlgVideoView[i].ShowCurrentVideoFlowInfo();
		}
	}
	catch(...)
	{

	}
}

//标注有问题画面
void CVEMCUCtlDlg::OnMenuitemTagCamera()
{
	try
	{
		char szCameraNum[64] = {0};
		char szCameraName[260] = {0};
		char szStationName[260] = {0};
		BOOL bResult = FALSE;

		if (m_iFocuseWindowID < 0||m_iFocuseWindowID >= MAXVIEWCH)
			return;

		if(CheckViewVideoIsOpen(m_iFocuseWindowID) <= 0)
		{
			g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.Lock();
			strcpy_s(szCameraNum,sizeof(szCameraNum),g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.m_szCameraCallNum);
			strcpy_s(szCameraName,sizeof(szCameraName),g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.m_szCameraName);
			strcpy_s(szStationName,sizeof(szStationName),g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.m_szStationName);
			g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.UnLock();

			if (strlen(szCameraNum) < 10)
			{
				MessageBox("该通道没有摄像头信息","视频监视");
				return;
			}
		}
		else
		{
			g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.Lock();
			strcpy_s(szCameraNum,sizeof(szCameraNum),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szCameraCallNum);
			strcpy_s(szCameraName,sizeof(szCameraName),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szCameraName);
			strcpy_s(szStationName,sizeof(szStationName),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szStationName);
			g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.UnLock();
		}

		if (m_pDlgPageServer != NULL)
		{
			bResult = m_pDlgPageServer->SetTreeCameraHandleTreeItemAndDatabaseStatus(VM_CAMERA_STATUS_TAG_BY_STATUS,szCameraNum,szCameraName,szStationName);
			if (bResult != FALSE)
			{
				RECT VideoRect;
				g_DlgVideoView[m_iFocuseWindowID].m_Pic.GetWindowRect(&VideoRect);
				g_VideoTagInfoDlg[m_iFocuseWindowID].SetVideoInfoCurrentIndex(2);
				ModifyVideoTagInfoDlg(m_iFocuseWindowID,&VideoRect,TRUE);
			}
		}
	}
	catch(...)
	{

	}
}

//取消标注
void CVEMCUCtlDlg::OnMenuitemCancelcamera()
{
	try
	{
		char szCameraNum[64] = {0};
		char szCameraName[260] = {0};
		char szStationName[260] = {0};
		BOOL bResult = FALSE;

		if (m_iFocuseWindowID < 0||m_iFocuseWindowID > MAXVIEWCH)
			return;

		if(CheckViewVideoIsOpen(m_iFocuseWindowID) <= 0)
		{
			g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.Lock();
			strcpy_s(szCameraNum,sizeof(szCameraNum),g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.m_szCameraCallNum);
			strcpy_s(szCameraName,sizeof(szCameraName),g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.m_szCameraName);
			strcpy_s(szStationName,sizeof(szStationName),g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.m_szStationName);
			g_DlgVideoView[m_iFocuseWindowID].m_PreVideoViewOutInfo.UnLock();

			if (strlen(szCameraNum) < 10)
			{
				MessageBox("该通道没有摄像头信息","视频监视");
				return;
			}
		}
		else
		{
			g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.Lock();
			strcpy_s(szCameraNum,sizeof(szCameraNum),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szCameraCallNum);
			strcpy_s(szCameraName,sizeof(szCameraName),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szCameraName);
			strcpy_s(szStationName,sizeof(szStationName),g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.m_szStationName);
			g_DlgVideoView[m_iFocuseWindowID].m_VideoViewOutInfo.UnLock();
		}

		if (m_pDlgPageServer != NULL)
		{
			bResult = m_pDlgPageServer->SetTreeCameraHandleTreeItemAndDatabaseStatus(VM_CAMERA_STATUS_NO_TAG_BY_STATUS,szCameraNum,szCameraName,szStationName);
			if (bResult != FALSE)
			{
				RECT VideoRect;
				g_VideoTagInfoDlg[m_iFocuseWindowID].m_bVideoInfoShowFlag = FALSE;
				g_DlgVideoView[m_iFocuseWindowID].m_Pic.GetWindowRect(&VideoRect);
				g_VideoTagInfoDlg[m_iFocuseWindowID].SetVideoInfoCurrentIndex(1);
				ModifyVideoTagInfoDlg(m_iFocuseWindowID,&VideoRect,FALSE);
			}
		}
	}
	catch(...)
	{

	}
}

BOOL CVEMCUCtlDlg::StartSubscribeCameraPresenceInfo()
{
	return TRUE;

	int nStationId = 0;
	char szCallee[64] = {0};
	int SuerID = 0;
	char szSubPresenceXML[1024] = {0};
	char szUrl[256] = {0};
	char szFromHost[256] = {0};
	char szToHost[256] = {0};

	for (int i = 0;i < g_nStation_Num;i++)
	{
		nStationId = g_tStation_Info[i].station_id;
		strcpy_s(szCallee,sizeof(szCallee),g_tStation_Info[i].station_code_videoplant);

		sprintf(szSubPresenceXML, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<SIP_XML EventType=\"Subscribe_Status\">\n"
			"<Item Code=\"%s\"/>"
			"</SIP_XML>\n", szCallee);

		g_pEventAPI->ProduceSID(&SuerID);
		app_SubCh[SuerID].iSID = SuerID;
		sprintf(app_SubCh[SuerID].stationcode[0], "%s", szCallee);

		sprintf_s(szUrl,sizeof(szUrl),"%s@%s",szCallee,app_RegCh.reg_server);
		sprintf_s(szFromHost,sizeof(szFromHost),"%s:%d",app_StackCfg.szLocalUdpAddress,app_StackCfg.nLocalUdpPort);
		sprintf_s(szToHost,sizeof(szToHost),"%s:%d",app_RegCh.reg_server,app_RegCh.reg_serverport);

		int ret = g_pEventAPI->MakeSubs(SuerID,
			szUrl,
			szSubPresenceXML,
			strlen(szSubPresenceXML),
			app_RegCh.reg_user,
			3600,
			"presence",
			(void *)nStationId,
			"application/xml",
			szFromHost,
			szToHost
			);
	}
	
	return TRUE;
}

BOOL CVEMCUCtlDlg::StartSearchCameraPresenceInfo()
{
	g_bCameraStatusSearchThreadExitFlag = false;

	g_hCameraStatusSearchThread = (HANDLE)_beginthreadex(NULL, 0,_CameraStatusSearchThread,(void*)this, 0, &g_uCameraStatusSearchThreadID);
	if (g_hCameraStatusSearchThread == 0)
		return FALSE;

	return TRUE;
}

BOOL CVEMCUCtlDlg::StopSearchCameraPresenceInfo()
{
	DWORD dwResult = 0;
	g_bCameraStatusSearchThreadExitFlag = true;

	if (g_hCameraStatusSearchThread != NULL)
	{
		dwResult = WaitForSingleObject(g_hCameraStatusSearchThread,6000);
		if (dwResult != WAIT_OBJECT_0)
		{
			TerminateThread(g_hCameraStatusSearchThread,1);
		}

		g_hCameraStatusSearchThread = NULL;
		g_uCameraStatusSearchThreadID = 0;
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::SearchCameraPresenceInfo(int nStationId,char *szStationNum)
{
	if (nStationId <= 0||nStationId > 2000||szStationNum == NULL||strlen(szStationNum) == 0)
		return FALSE;

	void *inArg = NULL;
	void *outArg = NULL;
	int nRet = 0;
	int timeout = 2;

	PREQ_RESOURCE req_resource = NULL;
	PRES_RESOURCE res_resource = NULL;
	PRES_RESOURCE_ITEM item_ptr;
	int nStationStatus = 0;

	try
	{
		nRet = initStructArg(&inArg, HTTP_REQUEST_RESOURCE);
		if (nRet !=  0)
			return FALSE;

		nRet = initStructArg(&outArg, HTTP_RESPONSE_RESOURCE);
		if (nRet !=  0)
		{
			freeStructArg(&inArg, HTTP_REQUEST_RESOURCE);
			return FALSE;
		}

		req_resource = (PREQ_RESOURCE)inArg;

		sprintf_s(req_resource->code, "%s",szStationNum);
		sprintf_s(req_resource->UserCode, "%s", app_RegCh.reg_user);

		req_resource->FromIndex = 1;
		req_resource->ToIndex = 300;

		nRet = HttpClient(g_HttpServerIP,
			g_nHttpServerPort,
			HTTP_REQUEST_RESOURCE,
			inArg,
			&outArg,
			timeout);

		if (nRet !=  0)
		{
			freeStructArg(&inArg, HTTP_REQUEST_RESOURCE);
			freeStructArg(&outArg, HTTP_RESPONSE_RESOURCE);
			return FALSE;
		}

		res_resource = (PRES_RESOURCE)outArg;

		CAMERA_STATUS_INFO CameraStatusInfo;
		memset(&CameraStatusInfo,0,sizeof(CameraStatusInfo));
		CameraStatusInfo.station_id = nStationId;
		CameraStatusInfo.type = 0;

		for (item_ptr = res_resource->item; item_ptr != NULL; item_ptr = item_ptr->next)
		{
			strcpy_s(CameraStatusInfo.camera_code,sizeof(CameraStatusInfo.camera_code),item_ptr->code);
			CameraStatusInfo.camera_status = item_ptr->status;

			if (CameraStatusInfo.camera_status != 0)
				nStationStatus = 1;

			g_CameraStatusInfo.WriteCameraStatusInfo(&CameraStatusInfo);
		}

		freeStructArg(&inArg, HTTP_REQUEST_RESOURCE);
		freeStructArg(&outArg, HTTP_RESPONSE_RESOURCE);

		memset(&CameraStatusInfo,0,sizeof(CameraStatusInfo));
		CameraStatusInfo.station_id = nStationId;
		CameraStatusInfo.type = 1;
		CameraStatusInfo.camera_status = nStationStatus;
		
		g_CameraStatusInfo.WriteCameraStatusInfo(&CameraStatusInfo);

		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

BOOL CVEMCUCtlDlg::StartViewChannelPrevVideoByIndex(int nIndex)
{
	if(nIndex < 0||nIndex >= MAXVIEWCH)
		return FALSE;

	try
	{
		int nDeviceIpAddressType = 0;
		int nProblemRestartCount = 0;
		char szPreCameraCallNum[32] = {0};
		memset(szPreCameraCallNum,0,sizeof(szPreCameraCallNum));

		char szPreCameraName[256] = {0};
		memset(szPreCameraName,0,sizeof(szPreCameraName));

		char szPreStationName[256] = {0};
		memset(szPreStationName,0,sizeof(szPreStationName));

		char szPresetName[256] = {0};
		memset(szPresetName,0,sizeof(szPresetName));

		int preset_id = -1;
		int line1_from_x = 0;
		int line1_from_y = 0;
		int line1_to_x = 0;
		int line1_to_y = 0;
		int line2_from_x = 0;
		int line2_from_y = 0;
		int line2_to_x = 0;
		int line2_to_y = 0;

		g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.Lock();
		strcpy_s(szPreCameraCallNum,sizeof(szPreCameraCallNum),g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_szCameraCallNum);
		strcpy_s(szPreCameraName,sizeof(szPreCameraName),g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_szCameraName);
		strcpy_s(szPreStationName,sizeof(szPreStationName),g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_szStationName);
		strcpy_s(szPresetName,sizeof(szPresetName),g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_preset_name);
		preset_id = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_preset_id;
		line1_from_x = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line1_from_x;
		line1_from_y = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line1_from_y;
		line1_to_x = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line1_to_x;
		line1_to_y = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line1_to_y;
		line2_from_x = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line2_from_x;
		line2_from_y = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line2_from_y;
		line2_to_x = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line2_to_x;
		line2_to_y = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line2_to_y;
		g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.UnLock();

		if (strlen(szPreCameraCallNum) < 10)
			return FALSE;

		g_DlgVideoView[nIndex].m_VideoViewOutInfo.Lock();

		g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_bVideoFlowProblemFlag = FALSE;//重置流量检测标志

		//摄像头重试次数
		if (g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nProblemRestartCount > 0)
		{
			g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nProblemRestartCount--;
			nProblemRestartCount = g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nProblemRestartCount;
		}
		else
		{
			//自动设置摄像头状态有问题
			if (g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nSetProblemCameraFlag == 2)
			{
				g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nSetProblemCameraFlag = 0;

				g_DlgVideoView[nIndex].m_VideoViewOutInfo.UnLock();

				if(m_pDlgPageServer != NULL&&strlen(szPreCameraCallNum) > 10)
				{
					m_pDlgPageServer->AutoSetTreeCameraHandleTreeItemAndDatabaseStatus(VM_CAMERA_STATUS_OFFLINE_BY_STATUS,szPreCameraCallNum,NULL,NULL,FALSE);
				}
			}
			else
			{
				g_DlgVideoView[nIndex].m_VideoViewOutInfo.UnLock();
			}
			return FALSE;
		}

		if (g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nProblemRestartCount == 0
			&&g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nSetProblemCameraFlag == 1)
		{
			//设置设备有问题标志,下一次再打不开就会自动设置摄像头状态有问题
			g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nSetProblemCameraFlag = 2;
		}

		g_DlgVideoView[nIndex].m_VideoViewOutInfo.UnLock();

		HTREEITEM hTreeItem = NULL;
		_T_NODE_INFO *pNodeInfo = NULL;
		_T_NODE_PRESET_INFO *pPresetNodeInfo = NULL;
		VIDEO_SIP_CALL_SERVER_INFO VideoSipCallServerInfo;
		memset(&VideoSipCallServerInfo,0,sizeof(VideoSipCallServerInfo));

		VIDEO_SIP_CALL_PRESET_INFO VideoSipCallPresetInfo;
		memset(&VideoSipCallPresetInfo,0,sizeof(VideoSipCallPresetInfo));

		if (m_pDlgPageServer != NULL )
		{
			if (preset_id < 0)//人工巡视
			{
				hTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szPreCameraCallNum,NULL,NULL);
				if (hTreeItem != NULL)
				{
					pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hTreeItem);
					if (pNodeInfo != NULL)
					{
						VideoSipCallServerInfo.nStatus = pNodeInfo->node_status;
						strcpy_s(VideoSipCallServerInfo.szCode,sizeof(VideoSipCallServerInfo.szCode),pNodeInfo->node_num);
						VideoSipCallServerInfo.nScreenId = nIndex;
						strcpy_s(VideoSipCallServerInfo.szName,sizeof(VideoSipCallServerInfo.szName),pNodeInfo->node_name);
						VideoSipCallServerInfo.hItem = hTreeItem;
						strcpy_s(VideoSipCallServerInfo.szStation,sizeof(VideoSipCallServerInfo.szStation),pNodeInfo->node_station);
						sprintf_s(VideoSipCallServerInfo.szReason,sizeof(VideoSipCallServerInfo.szReason),"{重新打开}-{%s}-{%s}",pNodeInfo->node_station,szPreCameraName);

						memcpy(&VideoSipCallServerInfo.server_node_info,pNodeInfo,sizeof(VideoSipCallServerInfo.server_node_info));

						if (nProblemRestartCount > 0)
						{
							VideoSipCallServerInfo.nType = 3;//通过南瑞平台
							VideoSipCallServerInfo.nDecodeTag = pNodeInfo->node_decodetag;
						}
						else if (g_nClientVideoLinkType == 0)//只用南瑞平台
						{
							VideoSipCallServerInfo.nType = 3;//通过南瑞平台
							VideoSipCallServerInfo.nDecodeTag = pNodeInfo->node_decodetag;
						}
						else if (g_nClientVideoLinkType == 1)//直接访问前端设备
						{
							VideoSipCallServerInfo.nType = 3;//通过南瑞平台
							VideoSipCallServerInfo.nDecodeTag = pNodeInfo->node_decodetag;
						}
						else if (g_nClientVideoLinkType == 2)//先通过南瑞平台，无法访问后再直接访问前端设备
						{
							nDeviceIpAddressType = GetDeviceIpAddressType(&pNodeInfo->camera_info);
							if (nDeviceIpAddressType != 2)
							{
								VideoSipCallServerInfo.nType = 4;//直接连接前端设备
								VideoSipCallServerInfo.nDecodeTag = pNodeInfo->camera_info.dvr_info.dvr_type;
							}
							else
							{
								VideoSipCallServerInfo.nType = 3;//通过南瑞平台
								VideoSipCallServerInfo.nDecodeTag = pNodeInfo->node_decodetag;
							}
						}
						else if (g_nClientVideoLinkType == 3)//先两次通过南瑞平台，无法访问后再直接访问前端设备
						{
							nDeviceIpAddressType = GetDeviceIpAddressType(&pNodeInfo->camera_info);
							if (nDeviceIpAddressType != 2)
							{
								VideoSipCallServerInfo.nType = 4;//直接连接前端设备
								VideoSipCallServerInfo.nDecodeTag = pNodeInfo->camera_info.dvr_info.dvr_type;
							}
							else
							{
								VideoSipCallServerInfo.nType = 3;//通过南瑞平台
								VideoSipCallServerInfo.nDecodeTag = pNodeInfo->node_decodetag;
							}
						}

						g_VMLog.WriteVmLog("StartViewChannelPrevVideoByIndex-----------VideoSipCallServerInfo.nType = %d,VideoSipCallServerInfo.nDecodeTag = %d",VideoSipCallServerInfo.nType,VideoSipCallServerInfo.nDecodeTag);

						g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
						g_pMainDlg->ThreadPoolDispatchTask(ThreadMakeCallCameraServer,(void *)&VideoSipCallServerInfo,sizeof(VideoSipCallServerInfo),2);
					}
				}
			}
			else//预置位
			{
				hTreeItem = m_pDlgPageServer->SearchPresetTreePresetHandleTreeItem(NULL,szPreCameraCallNum,szPresetName);
				if (hTreeItem != NULL)
				{
					pPresetNodeInfo = (_T_NODE_PRESET_INFO *)m_pDlgPageServer->m_trPreset.GetItemData(hTreeItem);
					if (pPresetNodeInfo != NULL)
					{
						VideoSipCallPresetInfo.nStatus = pPresetNodeInfo->node_status;
						strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),pPresetNodeInfo->node_num);
						VideoSipCallPresetInfo.nScreenId = nIndex;
						strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),pPresetNodeInfo->node_name);
						VideoSipCallPresetInfo.hItem = hTreeItem;
						strcpy_s(VideoSipCallPresetInfo.szStation,sizeof(VideoSipCallPresetInfo.szStation),pPresetNodeInfo->node_station);
						sprintf_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),"{重新打开}-{%s}-{%s}",pPresetNodeInfo->node_station,szPreCameraName);
						memcpy(&VideoSipCallPresetInfo.preset_node_info,pPresetNodeInfo,sizeof(VideoSipCallPresetInfo.preset_node_info));

						if (nProblemRestartCount > 0)
						{
							VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
							VideoSipCallPresetInfo.nDecodeTag = pPresetNodeInfo->node_decodetag;
						}
						else if (g_nClientVideoLinkType == 0)
						{
							VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
							VideoSipCallPresetInfo.nDecodeTag = pPresetNodeInfo->node_decodetag;
						}
						else if (g_nClientVideoLinkType == 1)
						{
							VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
							VideoSipCallPresetInfo.nDecodeTag = pPresetNodeInfo->node_decodetag;
						}
						else if (g_nClientVideoLinkType == 2)
						{
							nDeviceIpAddressType = GetDeviceIpAddressType(&pNodeInfo->camera_info);
							if (nDeviceIpAddressType != 2)
							{
								VideoSipCallPresetInfo.nType = 4;//直接连接前端设备
								VideoSipCallPresetInfo.nDecodeTag = pPresetNodeInfo->camera_info.dvr_info.dvr_type;
							}
							else
							{
								VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
								VideoSipCallPresetInfo.nDecodeTag = pPresetNodeInfo->node_decodetag;
							}
						}
						else if (g_nClientVideoLinkType == 3)
						{
							nDeviceIpAddressType = GetDeviceIpAddressType(&pNodeInfo->camera_info);
							if (nDeviceIpAddressType != 2)
							{
								VideoSipCallPresetInfo.nType = 4;//直接连接前端设备
								VideoSipCallPresetInfo.nDecodeTag = pPresetNodeInfo->camera_info.dvr_info.dvr_type;
							}
							else
							{
								VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
								VideoSipCallPresetInfo.nDecodeTag = pPresetNodeInfo->node_decodetag;
							}
						}

						g_VMLog.WriteVmLog("StartViewChannelPrevVideoByIndex-----------VideoSipCallPresetInfo.nType = %d,VideoSipCallPresetInfo.nDecodeTag = %d",VideoSipCallPresetInfo.nType,VideoSipCallPresetInfo.nDecodeTag);

						g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
						g_pMainDlg->ThreadPoolDispatchTask(ThreadMakeCallCameraPreset,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);
					}
				}
				else
				{
					hTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szPreCameraCallNum,NULL,NULL);
					if (hTreeItem != NULL)
					{
						pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hTreeItem);
						if (pNodeInfo != NULL)
						{
							VideoSipCallPresetInfo.nStatus = pNodeInfo->node_status;
							strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),pNodeInfo->node_num);
							strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),pNodeInfo->node_name);
							strcpy_s(VideoSipCallPresetInfo.szStation,sizeof(VideoSipCallPresetInfo.szStation),pNodeInfo->node_station);
							sprintf_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),"{重新打开}-{%s}-{%s}",pNodeInfo->node_station,szPreCameraName);
							VideoSipCallPresetInfo.nScreenId = nIndex;
							VideoSipCallPresetInfo.hItem = NULL;

							VideoSipCallPresetInfo.preset_node_info.node_type = 5;//预置位
							VideoSipCallPresetInfo.preset_node_info.preset_id = preset_id;
							VideoSipCallPresetInfo.preset_node_info.line1_from_x = line1_from_x;
							VideoSipCallPresetInfo.preset_node_info.line1_from_y = line1_from_y;
							VideoSipCallPresetInfo.preset_node_info.line1_to_x = line1_to_x;
							VideoSipCallPresetInfo.preset_node_info.line1_to_y = line1_to_y;
							VideoSipCallPresetInfo.preset_node_info.line2_from_x = line2_from_x;
							VideoSipCallPresetInfo.preset_node_info.line2_from_y = line2_from_y;
							VideoSipCallPresetInfo.preset_node_info.line2_to_x = line2_to_x;
							VideoSipCallPresetInfo.preset_node_info.line2_to_y = line2_to_y;
							strcpy_s(VideoSipCallPresetInfo.preset_node_info.preset_name,sizeof(VideoSipCallPresetInfo.preset_node_info.preset_name),szPresetName);
							memcpy(&VideoSipCallPresetInfo.preset_node_info.camera_info,&pNodeInfo->camera_info,sizeof(VideoSipCallPresetInfo.preset_node_info.camera_info));

							if (nProblemRestartCount > 0)
							{
								VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
								VideoSipCallPresetInfo.nDecodeTag = pNodeInfo->node_decodetag;
							}
							else if (g_nClientVideoLinkType == 0)
							{
								VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
								VideoSipCallPresetInfo.nDecodeTag = pNodeInfo->node_decodetag;
							}
							else if (g_nClientVideoLinkType == 1)
							{
								VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
								VideoSipCallPresetInfo.nDecodeTag = pNodeInfo->node_decodetag;
							}
							else if (g_nClientVideoLinkType == 2)
							{
								VideoSipCallPresetInfo.nType = 4;//直接连接前端设备
								VideoSipCallPresetInfo.nDecodeTag = pNodeInfo->camera_info.dvr_info.dvr_type;
							}
							else if (g_nClientVideoLinkType == 3)
							{
								VideoSipCallPresetInfo.nType = 4;//直接连接前端设备
								VideoSipCallPresetInfo.nDecodeTag = pNodeInfo->camera_info.dvr_info.dvr_type;
							}

							g_VMLog.WriteVmLog("StartViewChannelPrevVideoByIndex-----------VideoSipCallPresetInfo.nType = %d,VideoSipCallPresetInfo.nDecodeTag = %d",VideoSipCallPresetInfo.nType,VideoSipCallPresetInfo.nDecodeTag);

							g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
							g_pMainDlg->ThreadPoolDispatchTask(ThreadMakeCallCameraPreset,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);
						}
					}
				}
			}
		}
	
		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

BOOL CVEMCUCtlDlg::QuickStartViewChannelPrevVideoByIndex(int nIndex)
{
	if(nIndex < 0||nIndex >= MAXVIEWCH)
		return FALSE;

	try
	{
		char szPreCameraCallNum[32] = {0};
		memset(szPreCameraCallNum,0,sizeof(szPreCameraCallNum));

		char szPreCameraName[256] = {0};
		memset(szPreCameraName,0,sizeof(szPreCameraName));

		char szPresetName[256] = {0};
		memset(szPresetName,0,sizeof(szPresetName));

		char szPreStationName[256] = {0};
		memset(szPreStationName,0,sizeof(szPreStationName));

		int preset_id = -1;
		int line1_from_x = 0;
		int line1_from_y = 0;
		int line1_to_x = 0;
		int line1_to_y = 0;
		int line2_from_x = 0;
		int line2_from_y = 0;
		int line2_to_x = 0;
		int line2_to_y = 0;

		g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.Lock();
		strcpy_s(szPreCameraCallNum,sizeof(szPreCameraCallNum),g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_szCameraCallNum);
		strcpy_s(szPreCameraName,sizeof(szPreCameraName),g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_szCameraName);
		strcpy_s(szPreStationName,sizeof(szPreStationName),g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_szStationName);
		strcpy_s(szPresetName,sizeof(szPresetName),g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_preset_name);
		preset_id = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_preset_id;
		line1_from_x = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line1_from_x;
		line1_from_y = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line1_from_y;
		line1_to_x = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line1_to_x;
		line1_to_y = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line1_to_y;
		line2_from_x = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line2_from_x;
		line2_from_y = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line2_from_y;
		line2_to_x = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line2_to_x;
		line2_to_y = g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.m_line2_to_y;
		g_DlgVideoView[nIndex].m_PreVideoViewOutInfo.UnLock();

		if (strlen(szPreCameraCallNum) < 10)
			return FALSE;

		g_DlgVideoView[nIndex].m_VideoViewOutInfo.Lock();

		if (g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_bSetNrDecodeProblemFlag != FALSE)
		{
			g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_bSetNrDecodeProblemFlag = FALSE;
		}
		else
		{
			g_DlgVideoView[nIndex].m_VideoViewOutInfo.UnLock();
			return FALSE;
		}

		if (g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nNrDecodeProblemRestartCount > 0)
		{
			g_DlgVideoView[nIndex].m_VideoViewOutInfo.m_nNrDecodeProblemRestartCount--;
		}
		else
		{
			g_DlgVideoView[nIndex].m_VideoViewOutInfo.UnLock();
			return FALSE;
		}

		g_DlgVideoView[nIndex].m_VideoViewOutInfo.UnLock();

		HTREEITEM hTreeItem = NULL;
		_T_NODE_INFO *pNodeInfo = NULL;
		_T_NODE_PRESET_INFO *pPresetNodeInfo = NULL;
		VIDEO_SIP_CALL_SERVER_INFO VideoSipCallServerInfo;
		memset(&VideoSipCallServerInfo,0,sizeof(VideoSipCallServerInfo));
		VIDEO_SIP_CALL_PRESET_INFO VideoSipCallPresetInfo;
		memset(&VideoSipCallPresetInfo,0,sizeof(VideoSipCallPresetInfo));

		if (m_pDlgPageServer != NULL)
		{
			if (preset_id < 0)//人工巡视
			{
				hTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szPreCameraCallNum,NULL,NULL);
				if (hTreeItem != NULL)
				{
					pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hTreeItem);
					if (pNodeInfo != NULL)
					{
						VideoSipCallServerInfo.nStatus = pNodeInfo->node_status;
						strcpy_s(VideoSipCallServerInfo.szCode,sizeof(VideoSipCallServerInfo.szCode),pNodeInfo->node_num);
						VideoSipCallServerInfo.nScreenId = nIndex;
						strcpy_s(VideoSipCallServerInfo.szName,sizeof(VideoSipCallServerInfo.szName),pNodeInfo->node_name);
						VideoSipCallServerInfo.hItem = hTreeItem;
						strcpy_s(VideoSipCallServerInfo.szStation,sizeof(VideoSipCallServerInfo.szStation),pNodeInfo->node_station);
						sprintf_s(VideoSipCallServerInfo.szReason,sizeof(VideoSipCallServerInfo.szReason),"{重新打开}-{%s}-{%s}",pNodeInfo->node_station,szPreCameraName);
						memcpy(&VideoSipCallServerInfo.server_node_info,pNodeInfo,sizeof(VideoSipCallServerInfo.server_node_info));

						VideoSipCallServerInfo.nType = 3;//通过南瑞平台
						VideoSipCallServerInfo.nDecodeTag = pNodeInfo->node_decodetag;

						g_VMLog.WriteVmLog("QuickStartViewChannelPrevVideoByIndex-----------VideoSipCallServerInfo.nType = %d,VideoSipCallServerInfo.nDecodeTag = %d",VideoSipCallServerInfo.nType,VideoSipCallServerInfo.nDecodeTag);

						g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
						g_pMainDlg->ThreadPoolDispatchTask(ThreadMakeCallCameraServer,(void *)&VideoSipCallServerInfo,sizeof(VideoSipCallServerInfo),2);
					}
				}
			}
			else//预置位
			{
				//查找预置位树
				hTreeItem = m_pDlgPageServer->SearchPresetTreePresetHandleTreeItem(NULL,szPreCameraCallNum,szPresetName);
				if (hTreeItem != NULL)
				{
					pPresetNodeInfo = (_T_NODE_PRESET_INFO *)m_pDlgPageServer->m_trPreset.GetItemData(hTreeItem);
					if (pPresetNodeInfo != NULL)
					{
						VideoSipCallPresetInfo.nStatus = pPresetNodeInfo->node_status;
						strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),pPresetNodeInfo->node_num);
						VideoSipCallPresetInfo.nScreenId = nIndex;
						strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),pPresetNodeInfo->node_name);
						VideoSipCallPresetInfo.hItem = hTreeItem;
						strcpy_s(VideoSipCallPresetInfo.szStation,sizeof(VideoSipCallPresetInfo.szStation),pPresetNodeInfo->node_station);
						sprintf_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),"{重新打开}-{%s}-{%s}",pPresetNodeInfo->node_station,szPreCameraName);
						memcpy(&VideoSipCallPresetInfo.preset_node_info,pPresetNodeInfo,sizeof(VideoSipCallPresetInfo.preset_node_info));

						VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
						VideoSipCallPresetInfo.nDecodeTag = pPresetNodeInfo->node_decodetag;

						g_VMLog.WriteVmLog("QuickStartViewChannelPrevVideoByIndex-----------VideoSipCallPresetInfo.nType = %d,VideoSipCallPresetInfo.nDecodeTag = %d",VideoSipCallPresetInfo.nType,VideoSipCallPresetInfo.nDecodeTag);

						g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
						g_pMainDlg->ThreadPoolDispatchTask(ThreadMakeCallCameraServer,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);
					}
				}
				else//查找摄像头树
				{
					hTreeItem = m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szPreCameraCallNum,NULL,NULL);
					if (hTreeItem != NULL)
					{
						pNodeInfo = (_T_NODE_INFO *)m_pDlgPageServer->m_trServer.GetItemData(hTreeItem);
						if (pNodeInfo != NULL)
						{
							VideoSipCallPresetInfo.nStatus = pNodeInfo->node_status;
							strcpy_s(VideoSipCallPresetInfo.szCode,sizeof(VideoSipCallPresetInfo.szCode),pNodeInfo->node_num);
							strcpy_s(VideoSipCallPresetInfo.szName,sizeof(VideoSipCallPresetInfo.szName),pNodeInfo->node_name);
							strcpy_s(VideoSipCallPresetInfo.szStation,sizeof(VideoSipCallPresetInfo.szStation),pNodeInfo->node_station);
							sprintf_s(VideoSipCallPresetInfo.szReason,sizeof(VideoSipCallPresetInfo.szReason),"{重新打开}-{%s}-{%s}",pNodeInfo->node_station,szPreCameraName);
							VideoSipCallPresetInfo.nScreenId = nIndex;
							VideoSipCallPresetInfo.hItem = NULL;

							VideoSipCallPresetInfo.preset_node_info.node_type = 5;//预置位
							VideoSipCallPresetInfo.preset_node_info.preset_id = preset_id;
							VideoSipCallPresetInfo.preset_node_info.line1_from_x = line1_from_x;
							VideoSipCallPresetInfo.preset_node_info.line1_from_y = line1_from_y;
							VideoSipCallPresetInfo.preset_node_info.line1_to_x = line1_to_x;
							VideoSipCallPresetInfo.preset_node_info.line1_to_y = line1_to_y;
							VideoSipCallPresetInfo.preset_node_info.line2_from_x = line2_from_x;
							VideoSipCallPresetInfo.preset_node_info.line2_from_y = line2_from_y;
							VideoSipCallPresetInfo.preset_node_info.line2_to_x = line2_to_x;
							VideoSipCallPresetInfo.preset_node_info.line2_to_y = line2_to_y;
							strcpy_s(VideoSipCallPresetInfo.preset_node_info.preset_name,sizeof(VideoSipCallPresetInfo.preset_node_info.preset_name),szPresetName);

							VideoSipCallPresetInfo.nType = 3;//通过南瑞平台
							VideoSipCallPresetInfo.nDecodeTag = pNodeInfo->node_decodetag;
	
							g_VMLog.WriteVmLog("StartViewChannelPrevVideoByIndex-----------VideoSipCallPresetInfo.nType = %d,VideoSipCallPresetInfo.nDecodeTag = %d",VideoSipCallPresetInfo.nType,VideoSipCallPresetInfo.nDecodeTag);

							g_ThreadVideoOperateNumberInfo.DeviceVideoInNumerAdd();
							g_pMainDlg->ThreadPoolDispatchTask(ThreadMakeCallCameraPreset,(void *)&VideoSipCallPresetInfo,sizeof(VideoSipCallPresetInfo),2);
						}
					}
				}
			}
		}

		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

void CVEMCUCtlDlg::OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2)
{
	try
	{
		switch (nHotKeyId)
		{
		case VM_HOT_KEY_PRIOR_PAGE:
			{
				if (m_pDlgPageViewInfo != NULL&&m_pDlgPageViewInfo->IsWindowVisible() != FALSE)
				{
					m_pDlgPageViewInfo->OnBnClickedBtnPrePage();
				}
			}
			break;

		case VM_HOT_KEY_NEXT_PAGE:
			{
				if (m_pDlgPageViewInfo != NULL&&m_pDlgPageViewInfo->IsWindowVisible() != FALSE)
				{
					m_pDlgPageViewInfo->OnBnClickedBtnNextPage();
				}
			}
			break;

		case VM_HOT_KEY_EXPORT_REPORT:
			{
				if (m_pDlgPageViewInfo != NULL&&m_pDlgPageViewInfo->IsWindowVisible() != FALSE)
				{
					m_pDlgPageViewInfo->OnBnClickedBtnExportReport();
				}
			}
			break;
		}
	}
	catch(...)
	{

	}

	CDialog::OnHotKey(nHotKeyId, nKey1, nKey2);
}

bool CVEMCUCtlDlg::CreateUserStationViewHistoryTable(char *szUserStationViewHistoryTableName)
{
	char sql_buf[1024] = {0};
	int    nResult = 0;

	try
	{
		if (g_mySqlData == NULL)
		{
			ConnectMySql();
		}

		if (g_mySqlData == NULL)
			return false;

		sprintf_s(sql_buf,sizeof(sql_buf),"CREATE TABLE IF NOT EXISTS %s ("
			"id int(10) NOT NULL auto_increment,"
			"station_id int(10) NOT NULL default '0',"
			"camera_code char(64) NOT NULL default '',"
			"day int(4) NOT NULL default '1',"
			"status int(4) NOT NULL default '10',"
			"time char(32) NOT NULL default '',"
			"PRIMARY KEY  (id))",
			szUserStationViewHistoryTableName
			);

		nResult = mysql_query(g_mySqlData, sql_buf);
		if (nResult != 0)
			return false;

		return true;
	}
	catch(...)
	{

	}

	return false;
}

bool CVEMCUCtlDlg::UpdateUserStationViewHistoryTable(char *szUserStationViewHistoryTableName)
{
	char sql_buf[1024] = {0};
	int    nResult = 0;

	MYSQL_RES * res = NULL ;
	MYSQL_ROW	row ;
	int nCount = 0;
	int nMaxDayCount = 0;

	int nStationCount = 0;

	try
	{
		if (g_mySqlData == NULL)
		{
			ConnectMySql();
		}

		if (g_mySqlData == NULL)
			return false;

		SYSTEMTIME sysTm;
		::GetLocalTime(&sysTm);

		int nYear = sysTm.wYear;
		int nMonth = sysTm.wMonth;
		int nDay = sysTm.wDay;

		memset(sql_buf,0,sizeof(sql_buf));

		sprintf_s(sql_buf,sizeof(sql_buf),"SELECT COUNT(*)  FROM %s WHERE day=%d",szUserStationViewHistoryTableName,nDay);
		nResult = mysql_query(g_mySqlData, sql_buf);
		if (nResult != 0)
			return false;

		res = mysql_store_result(g_mySqlData);
		if (row = mysql_fetch_row(res))
		{
			nCount = atoi(row[0]);
		}
		mysql_free_result(res) ;

		if (nCount > 0)
			return true;

		char szCameraCode[512][32];
		int    nCameraStatus[512];
		int    nCameraCount = 0;
		memset(szCameraCode,0,sizeof(szCameraCode));
		memset(nCameraStatus,0,sizeof(nCameraStatus));

		for (int j = 0;j < g_nUserStationViewStationCount;j++)//站点
		{
			sprintf_s(sql_buf,sizeof(sql_buf),"SELECT camera_code,camera_status FROM ob_d5000_camera_station_%d ",g_nUserStationViewStationId[j]);
			nResult = mysql_query(g_mySqlData, sql_buf);
			if (nResult != 0)
				continue;

			nCameraCount = 0;

			res = mysql_store_result(g_mySqlData);
			while(row = mysql_fetch_row(res))
			{
				if (nCameraCount >= 512)
					break;

				strcpy_s(szCameraCode[nCameraCount],sizeof(szCameraCode[nCameraCount]),row[0]);
				nCameraStatus[nCameraCount] = atoi(row[1]);
				nCameraCount++;
			}
			mysql_free_result(res);

			for (int k = 0;k < nCameraCount;k++)
			{
				sprintf_s(sql_buf,sizeof(sql_buf),"INSERT INTO %s (station_id,camera_code,day,status) VALUES (%d,'%s',%d,%d) ",szUserStationViewHistoryTableName,g_nUserStationViewStationId[j],szCameraCode[k],nDay,10);
				mysql_query(g_mySqlData, sql_buf);
			}
		}
		return true;
	}
	catch(...)
	{

	}

	return false;
}

//更新用户巡视站点历史信息
bool CVEMCUCtlDlg::UpdateStationCameraVideoInfo()
{
	try
	{
		if (g_mySqlData == NULL)
		{
			ConnectMySql();
		}

		if (g_mySqlData == NULL)
			return false;

		MYSQL_RES * res = NULL ;
		MYSQL_ROW	row ;

		char sql_buf[1024] = {0};
		char szCameraCode[512][32];
		int    nCameraStatus[512];
		int    nCameraCount = 0;
		char szTime[64] = {0};

		memset(sql_buf,0,sizeof(sql_buf));
		memset(szCameraCode,0,sizeof(szCameraCode));
		memset(nCameraStatus,0,sizeof(nCameraStatus));

		SYSTEMTIME sysTm;
		::GetLocalTime(&sysTm);

		int nYear = sysTm.wYear;
		int nMonth = sysTm.wMonth;
		int nDay = sysTm.wDay;

		char szUserStationViewHistoryTableName[256] = {0};

		sprintf_s(szUserStationViewHistoryTableName,sizeof(szUserStationViewHistoryTableName),"ct_user_station_view_history_%d_%04d%02d",g_userpower.userID,nYear,nMonth);
		CreateUserStationViewHistoryTable(szUserStationViewHistoryTableName);
		UpdateUserStationViewHistoryTable(szUserStationViewHistoryTableName);

		sprintf_s(szTime,sizeof(szTime),"%04d-%02d-%02d %02d:%02d:%02d",nYear,nMonth,nDay,sysTm.wHour,sysTm.wMinute,sysTm.wSecond);

		int nResult = 0;

		for (int i = 0;i < g_nUserStationViewStationCount;i++)//站点
		{
			sprintf_s(sql_buf,sizeof(sql_buf),"SELECT camera_code,camera_status FROM ob_d5000_camera_station_%d ",g_nUserStationViewStationId[i]);
			nResult = mysql_query(g_mySqlData, sql_buf);
			if (nResult != 0)
				continue;

			nCameraCount = 0;
			int nCount = 0;

			res = mysql_store_result(g_mySqlData);
			while(row = mysql_fetch_row(res))
			{
				if (nCameraCount >= 512)
					break;

				strcpy_s(szCameraCode[nCameraCount],sizeof(szCameraCode[nCameraCount]),row[0]);
				nCameraStatus[nCameraCount] = atoi(row[1]);
				nCameraCount++;
			}
			mysql_free_result(res);

			for (int k = 0;k < nCameraCount;k++)
			{
				sprintf_s(sql_buf,sizeof(sql_buf),"SELECT COUNT(camera_code) FROM %s WHERE day=%d AND camera_code='%s' ",
					szUserStationViewHistoryTableName,
					nDay,szCameraCode[k]);

				if (mysql_query(g_mySqlData, sql_buf))
				{
					sprintf_s(sql_buf,sizeof(sql_buf),"INSERT INTO %s (station_id,camera_code,day,status) VALUES (%d,'%s',%d,%d) ",szUserStationViewHistoryTableName,g_nUserStationViewStationId[i],szCameraCode[k],nDay,nCameraStatus[k]);
					if (mysql_query(g_mySqlData, sql_buf))
						return false;
				}
				else
				{
					res = mysql_store_result(g_mySqlData);
					if(row = mysql_fetch_row(res))
					{
						nCount = atoi(row[0]);
					}
					mysql_free_result( res ) ;

					if (nCount != 0)
					{
						sprintf_s(sql_buf,sizeof(sql_buf),"UPDATE %s SET status=%d,time='%s' "
							"WHERE camera_code='%s' AND day=%d ",
							szUserStationViewHistoryTableName,
							nCameraStatus[k],szTime,
							szCameraCode[k],
							nDay);
					}
					else
					{
						sprintf_s(sql_buf,sizeof(sql_buf),"INSERT INTO %s (station_id,camera_code,day,status) VALUES (%d,'%s',%d,%d) ",szUserStationViewHistoryTableName,g_nUserStationViewStationId[i],szCameraCode[k],nDay,nCameraStatus[k]);
						if (mysql_query(g_mySqlData, sql_buf))
							return false;
					}
				}
			}
		}
		return true;
	}
	catch (...)
	{
		
	}

	return false;
}

int CVEMCUCtlDlg::GetDeviceIpAddressType(_T_CAMERA_INFO *pCameraInfo)
{
	try
	{
		int nResult = 0;

		if (pCameraInfo == NULL)
			return 0;

		if (strlen(pCameraInfo->dvr_info.dvr_ip) == 0)
			return 0;

		int nCount = 0;
		char *p = NULL;
		char szDvrIp[64] = {0};
		memset(szDvrIp,0,sizeof(szDvrIp));

		strcpy_s(szDvrIp,sizeof(szDvrIp)-1,pCameraInfo->dvr_info.dvr_ip);

		p = strtok(szDvrIp,".");
	
		while(p != NULL&&nCount < 8)
		{
			nCount++;
			p = strtok(NULL,".");
		}

		//0:表示DVRIP有问题，1:表示IP4,2:表示IP6
		if (nCount == 4)
		{
			nResult = 1;
		}
		else if (nCount == 6)
		{
			nResult = 2;
		}

		return nResult;
	}
	catch(...)
	{

	}

	return 0;
}

//根据进程名关闭进程
BOOL CVEMCUCtlDlg::KillProcessByName(char * pProcessName)
{
	HANDLE hSnapShot = INVALID_HANDLE_VALUE;
	HANDLE hProcess  = NULL;

	PROCESSENTRY32 pe;
	DWORD dwProcessID = 0;

	char  szProcessName[MAX_PATH];
	char  szExeFileName[MAX_PATH];

	if (pProcessName == NULL)
		return FALSE;

	try
	{
		memset(szProcessName,0,sizeof(szProcessName));
		memset(szExeFileName,0,sizeof(szExeFileName));

		if (strlen(pProcessName) >= sizeof(szProcessName))
			return FALSE;

		strcpy(szProcessName,pProcessName);

		hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (hSnapShot == INVALID_HANDLE_VALUE)
			return FALSE;

		memset(&pe,0,sizeof(PROCESSENTRY32));
		pe.dwSize = sizeof(PROCESSENTRY32);

		if(!Process32First(hSnapShot,&pe))
		{
			CloseHandle(hSnapShot);
			hSnapShot = NULL;
			return FALSE;
		}

		while (Process32Next(hSnapShot,&pe))
		{
			strcpy(szExeFileName,pe.szExeFile);

			if(stricmp(szProcessName,szExeFileName) == 0)
			{
				dwProcessID = pe.th32ProcessID;
				hProcess = ::OpenProcess(PROCESS_TERMINATE,FALSE,dwProcessID);

				if (hProcess == NULL)
				{
					CloseHandle(hSnapShot);
					hSnapShot = NULL;
					return FALSE;
				}

				if(::TerminateProcess(hProcess,0) == 0)
				{
					CloseHandle(hProcess);
					hProcess = NULL;

					CloseHandle(hSnapShot);
					hSnapShot = NULL;

					return FALSE;
				}

				CloseHandle(hProcess);
				hProcess = NULL;
			}
		}

		CloseHandle(hSnapShot);
		hSnapShot = NULL;
	}
	catch(...)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::TerminateProcess(char *szFileName)
{
	return KillProcessByName(szFileName);
}

BOOL  CVEMCUCtlDlg::CheckAndOpenVideoExcelReport()
{
	STARTUPINFO si;
	memset(&si,0,sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;  
	memset(&pi,0,sizeof(pi));

	char szCommandLine[256] = {0};
	CString strExeFileName = "VideoExcelReport.exe";

	//关闭报表导出进程
	TerminateProcess((char *)(LPCTSTR)strExeFileName);

	sprintf_s(szCommandLine,"%s -l 127.0.0.1 -d %s -u %d",strExeFileName,g_DBServerIP,g_userpower.userID);
	int nResult = CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi); 
	if (nResult != 0)
	{
		TRACE("创建报表进程失败!");
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelReportPipeInit()
{
	try
	{
		if (m_bVideoExcelReportPipeInitFlag == false)
		{
			if (StartVideoExcelReportPipe() != FALSE)
			{
				m_bVideoExcelReportPipeInitFlag = true;
			}
		}
	}
	catch(...)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelReportPipeUnInit()
{
	try
	{
		if (m_bVideoExcelReportPipeInitFlag == true)
		{
			StopVideoExcelReportPipe();
			m_bVideoExcelReportPipeInitFlag = false;
		}
	}
	catch(...)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelReportFreshUserInfo(int nUserId)
{
	VidoeExcelReportCbData ReportData;
	memset(&ReportData,0,sizeof(ReportData));

	ReportData.nFlag = 1;
	sprintf_s(ReportData.strCmdReq,sizeof(ReportData.strCmdReq)-1,"%s","fresh_user_info");
	ReportData.nInBufLen = sizeof(ReportData.strInBuf);
	sprintf_s(ReportData.strInBuf,sizeof(ReportData.strInBuf)-1,"%d",nUserId);

	int nLen = sizeof(ReportData);

	VideoExcelReportPipeWriteData(&ReportData,nLen);

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelReportShowReport()
{
	VidoeExcelReportCbData ReportData;
	memset(&ReportData,0,sizeof(ReportData));

	ReportData.nFlag = 1;
	sprintf_s(ReportData.strCmdReq,sizeof(ReportData.strCmdReq)-1,"%s","report_show_info");
	ReportData.nInBufLen = sizeof(ReportData.strInBuf);
	sprintf_s(ReportData.strInBuf,sizeof(ReportData.strInBuf)-1,"%d",1);

	int nLen = sizeof(ReportData);

	VideoExcelReportPipeWriteData(&ReportData,nLen);

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelReportHideReport()
{
	VidoeExcelReportCbData ReportData;
	memset(&ReportData,0,sizeof(ReportData));

	ReportData.nFlag = 1;
	sprintf_s(ReportData.strCmdReq,sizeof(ReportData.strCmdReq)-1,"%s","report_hide_info");
	ReportData.nInBufLen = sizeof(ReportData.strInBuf);
	sprintf_s(ReportData.strInBuf,sizeof(ReportData.strInBuf)-1,"%d",1);

	int nLen = sizeof(ReportData);

	VideoExcelReportPipeWriteData(&ReportData,nLen);
	
	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelReportExitReport()
{
	VidoeExcelReportCbData ReportData;
	memset(&ReportData,0,sizeof(ReportData));

	ReportData.nFlag = 1;
	sprintf_s(ReportData.strCmdReq,sizeof(ReportData.strCmdReq)-1,"%s","report_exit_info");
	ReportData.nInBufLen = sizeof(ReportData.strInBuf);
	sprintf_s(ReportData.strInBuf,sizeof(ReportData.strInBuf)-1,"%d",1);

	int nLen = sizeof(ReportData);

	VideoExcelReportPipeWriteData(&ReportData,nLen);

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
BOOL  CVEMCUCtlDlg::CheckAndOpenVideoExcelRecordReport()
{
	STARTUPINFO si;
	memset(&si,0,sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;  
	memset(&pi,0,sizeof(pi));

	char szCommandLine[256] = {0};
	CString strExeFileName = "VideoExcelRecordReport.exe";

	//关闭报表导出进程
	TerminateProcess((char *)(LPCTSTR)strExeFileName);

	sprintf_s(szCommandLine,"%s -l 127.0.0.1 -d %s -u %d",strExeFileName,g_DBServerIP,g_userpower.userID);
	int nResult = CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi); 
	if (nResult != 0)
	{
		TRACE("创建报表进程失败!");
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelRecordReportPipeInit()
{
	try
	{
		if (m_bVideoExcelRecordReportPipeInitFlag == false)
		{
			if (StartVideoExcelRecordReportPipe() != FALSE)
			{
				m_bVideoExcelRecordReportPipeInitFlag = true;
			}
		}
	}
	catch(...)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelRecordReportPipeUnInit()
{
	try
	{
		if (m_bVideoExcelRecordReportPipeInitFlag == true)
		{
			StopVideoExcelRecordReportPipe();
			m_bVideoExcelRecordReportPipeInitFlag = false;
		}
	}
	catch(...)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelRecordReportFreshUserInfo(int nUserId)
{
	VidoeExcelRecordReportCbData ReportData;
	memset(&ReportData,0,sizeof(ReportData));

	ReportData.nFlag = 1;
	sprintf_s(ReportData.strCmdReq,sizeof(ReportData.strCmdReq)-1,"%s","fresh_user_info");
	ReportData.nInBufLen = sizeof(ReportData.strInBuf);
	sprintf_s(ReportData.strInBuf,sizeof(ReportData.strInBuf)-1,"%d",nUserId);

	int nLen = sizeof(ReportData);

	VideoExcelRecordReportPipeWriteData(&ReportData,nLen);

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelRecordReportShowReport()
{
	VidoeExcelRecordReportCbData ReportData;
	memset(&ReportData,0,sizeof(ReportData));

	ReportData.nFlag = 1;
	sprintf_s(ReportData.strCmdReq,sizeof(ReportData.strCmdReq)-1,"%s","report_show_info");
	ReportData.nInBufLen = sizeof(ReportData.strInBuf);
	sprintf_s(ReportData.strInBuf,sizeof(ReportData.strInBuf)-1,"%d",1);

	int nLen = sizeof(ReportData);

	VideoExcelRecordReportPipeWriteData(&ReportData,nLen);

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelRecordReportHideReport()
{
	VidoeExcelRecordReportCbData ReportData;
	memset(&ReportData,0,sizeof(ReportData));

	ReportData.nFlag = 1;
	sprintf_s(ReportData.strCmdReq,sizeof(ReportData.strCmdReq)-1,"%s","report_hide_info");
	ReportData.nInBufLen = sizeof(ReportData.strInBuf);
	sprintf_s(ReportData.strInBuf,sizeof(ReportData.strInBuf)-1,"%d",1);

	int nLen = sizeof(ReportData);

	VideoExcelRecordReportPipeWriteData(&ReportData,nLen);

	return TRUE;
}

BOOL CVEMCUCtlDlg::VideoExcelRecordReportExitReport()
{
	VidoeExcelRecordReportCbData ReportData;
	memset(&ReportData,0,sizeof(ReportData));

	ReportData.nFlag = 1;
	sprintf_s(ReportData.strCmdReq,sizeof(ReportData.strCmdReq)-1,"%s","report_exit_info");
	ReportData.nInBufLen = sizeof(ReportData.strInBuf);
	sprintf_s(ReportData.strInBuf,sizeof(ReportData.strInBuf)-1,"%d",1);

	int nLen = sizeof(ReportData);

	VideoExcelRecordReportPipeWriteData(&ReportData,nLen);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

unsigned __stdcall _CameraStatusSearchThread(void *arg)
{
	if (arg == NULL)
		return -1;

	CVEMCUCtlDlg *pVEMCUCtlDlg = (CVEMCUCtlDlg*)arg;

	DWORD dwOldTickCount = GetTickCount();
	DWORD dwTickCount = dwOldTickCount;

	try
	{
		while (1)
		{
			if (g_bCameraStatusSearchThreadExitFlag == true)
			{
				return 0;
			}

			//Sleep(10*60*1000);

			//dwTickCount = GetTickCount();

			//if (dwTickCount < dwOldTickCount)
			//{
			//	dwOldTickCount = dwTickCount;
			//	continue;
			//}
			//
			//if (dwTickCount - dwOldTickCount < 3*60*60*1000)
			//	continue;

			//GetLocalTime(&SysTime);
		
			//if (SysTime.wHour > 3)
			//	continue;

			//dwOldTickCount = dwTickCount;

			for (int i = 0;i < g_nStation_Num;i++)
			{
				pVEMCUCtlDlg->SearchCameraPresenceInfo(g_tStation_Info[i].station_id,g_tStation_Info[i].station_code_videoplant);
				Sleep(1000);
			}

			pVEMCUCtlDlg->SendMessage(WM_UPDATE_USER_VIEW_CAMERA_STATUS_MESSAGE);

			Sleep(10*60*1000);
		}
	}
	catch(...)
	{

	}
	return -1;
}

//测试检测摄像头状态-------临时test用
unsigned __stdcall _CameraStatusSearchThread2(void *arg)
{
	if (arg == NULL)
		return -1;

	CVEMCUCtlDlg *pVEMCUCtlDlg = (CVEMCUCtlDlg*)arg;

	try
	{
		Sleep(3*60*1000);

		for (int i = 0;i < g_nStation_Num;i++)
		{
			pVEMCUCtlDlg->SearchCameraPresenceInfo(g_tStation_Info[i].station_id,g_tStation_Info[i].station_code_videoplant);
			Sleep(1000);
		}

		pVEMCUCtlDlg->SendMessage(WM_UPDATE_USER_VIEW_CAMERA_STATUS_MESSAGE);

//		AfxMessageBox("完成更新摄像头状态");
	}
	catch(...)
	{

	}
	return -1;
}


