// DlgReplay.cpp : 实现文件
//

#include "stdafx.h"
#include "VEMCUCtl.h"
#include "DlgReplay.h"
#include "VEMCUCtlDlg.h"
#include "xappany.h"
#include "DlgDeviceRelationCamera.h"
#include "AssDBOperate.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define ALARM_REPLAY (5)

//////////////////////////////////////////////////////////////////////////
CDlgReplay	 *g_pDlgReplay = NULL;
int				g_nRtspID = -1;
long			    g_RecvBytes = 0;
long			    g_TotalBytes = 0;
int				g_DevType = 0;

//////////////////////////////////////////////////////////////////////////
bool g_VideoReplayDownloadMenuFlag = false;
VIDEO_REPLAY_DOWNLOAD_INFO g_VideoReplayDownloadInfo;
char   g_ReplaySaveFilepath[256] = "C:\\";

//////////////////////////////////////////////////////////////////////////
int SwitchSPS(char * strSps,BYTE *sps);

//前端设备视频回放,播放进度
int VideoReplayDirectDeviceGetReplayPosCB(unsigned short dvrid,unsigned short channel,int pos)
{
	try
	{
		if (g_pDlgReplay == NULL)
			return -1;

		if (g_pDlgReplay->m_nDvrId == dvrid&&g_pDlgReplay->m_nCameraChannel == channel)
		{
			if (pos >= 0&&pos <= 100)
			{
				g_pDlgReplay->m_SliderPlayPos.SetPos(pos);
				return 0;
			}
		}

		return -1;
	}
	catch(...)
	{

	}
	return -1;
}

//前端设备视频下载进度
int VideoReplayDirectDeviceGetDownloadPosCB(unsigned short dvrid,unsigned short channel,int pos)
{
	try
	{
		if (g_pDlgReplay == NULL)
			return -1;

		if (g_pDlgReplay->m_nDvrId == dvrid&&g_pDlgReplay->m_nCameraChannel == channel)
		{
			if (pos >= 0&&pos <= 100)
			{
				g_pDlgReplay->m_SliderPlayPos.SetPos(pos);
				return 0;
			}
		}

		return -1;
	}
	catch(...)
	{

	}
	return -1;
}

//前端设备视频本地文件播放进度
int VideoReplayDirectDevicePlayLocalFilePosCB(LONG id,int pos)
{
	try
	{
		if (g_pDlgReplay == NULL)
			return -1;

		if (g_pDlgReplay->m_ch.m_nReplayLocalFileIDByDirectDevice == id)
		{
			if (pos >= 0&&pos <= 100)
			{
				g_pDlgReplay->m_SliderPlayPos.SetPos(pos);
				return 0;
			}
		}
		return -1;
	}
	catch(...)
	{

	}
	return -1;
}


//////////////////////////////////////////////////////////////////////////
// CDlgReplay 对话框

IMPLEMENT_DYNAMIC(CDlgReplay, CDialog)

CDlgReplay::CDlgReplay(CWnd* pParent /*=NULL*/)
: CDialog(CDlgReplay::IDD, pParent)
, m_strPlayState(_T(""))
, m_SelectedPlant(0)
, m_SelectedName(_T(""))
{
	m_pDlgStationSelect = NULL;
	m_pRtspClientAPI = NULL;
	m_NowSpeed = REPLAY_NORMALSPEED;
	m_RemotePlayState = REPLAY_STOP;
	m_LocalPlayState = REPLAY_STOP;
	m_pNodeInfo = NULL;
	m_nRtspMsgState = REPLAY_RTSP_STATE_INIT;

	memset(m_szSelectedNum,0,sizeof(m_szSelectedNum));
	memset(m_szSelectedType,0,sizeof(m_szSelectedType));
	memset(&m_Replay_Query_Info,0,sizeof(m_Replay_Query_Info));

	memset(m_szWeatherStationName,0,sizeof(m_szWeatherStationName));
	memset(m_szWeatherStartTime,0,sizeof(m_szWeatherStartTime));
	memset(m_szWeatherStopTime,0,sizeof(m_szWeatherStopTime));

	memset(&g_VideoReplayDownloadInfo,0,sizeof(g_VideoReplayDownloadInfo));
	memset(g_ReplaySaveFilepath,0,sizeof(g_ReplaySaveFilepath));

	memset(m_RtspURL, 0, sizeof(m_RtspURL));

	m_Brush.CreateSolidBrush(RGB(210,224,237)); 
	m_BgBrush.CreateSolidBrush(RGB(250, 250, 250)); // 背景的颜色
	m_StaticBKBrush.CreateSolidBrush(RGB(248, 252, 253)); // 背景的颜色
	m_nTableSelectIndex = 0;
	m_nPlaySliderWidth = 0;

	//视频访问方式
	m_nSearchVideoAccessType = 0;
	m_nPlayVideoAccessType = 0;
	m_nSpecialVideoAccessFlag = 0;

	/////////////////////////////////////////////////////////////////////////
	m_nDvrPort = 0;
	m_nCameraChannel = 0;
	m_nDvrId = 0;
	m_nDvrType = 0;
	m_nRecordFileSize = 0;
	memset(m_szDvrIP,0,sizeof(m_szDvrIP));
	memset(m_szUserName,0,sizeof(m_szUserName));
	memset(m_szUserPassword,0,sizeof(m_szUserPassword));
	memset(m_szDvrRecordStartTime,0,sizeof(m_szDvrRecordStartTime));
	memset(m_szDvrRecordStopTime,0,sizeof(m_szDvrRecordStopTime));
}

CDlgReplay::~CDlgReplay()
{

}

void CDlgReplay::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_DateTimeCtrl(pDX, IDC_DATE_STARTDAY, m_StartDay);
	DDX_DateTimeCtrl(pDX, IDC_DATE_STARTTIME, m_StartTime);
	DDX_DateTimeCtrl(pDX, IDC_DATE_STOPDAY, m_StopDay);
	DDX_DateTimeCtrl(pDX, IDC_DATE_STOPTIME, m_StopTime);
	DDX_Control(pDX, IDC_LIST_REPLAYFILE, m_ListCtrl_ReplayFile);
	DDX_Text(pDX, IDC_PALYSTATE, m_strPlayState);
	DDX_Text(pDX, IDC_PALYSPEED, m_strPlaySpeed);
	DDX_Control(pDX, IDC_SLIDER_PLAYPOS, m_SliderPlayPos);
	DDX_Text(pDX, IDC_EDIT_SELECTEDID, m_SelectedName);
	DDX_Control(pDX, IDC_COMBO_RECORDTYPE, m_ComboRecordFileType);
	DDX_Control(pDX,IDC_BUTTON_QUERY,m_btnQuery);
	DDX_Control(pDX,IDC_BUTTON_PLAY,m_btnPlay);
	DDX_Control(pDX,IDC_BUTTON_STOP,m_btnStop);
	DDX_Control(pDX,IDC_BUTTON_FAST,m_btnFast);
	DDX_Control(pDX,IDC_BUTTON_SLOW,m_btnSlow);
	DDX_Control(pDX,IDC_BUTTON_RP_PLANT_SELECT, m_btnSelPlant);
	DDX_Control(pDX,IDC_BUTTON_ALARM_REPLAY, m_btnAlarmReplay);
	DDX_Control(pDX,IDC_BUTTON_RP_D5000_SELECT, m_btnSelD5000);
	DDX_Control(pDX,IDC_BUTTON_RP_OP_SELECT, m_btnSelOP);
	DDX_Control(pDX,IDC_BUTTON_RP_WEATHER_SELECT, m_btnSelWeather);
	DDX_Control(pDX, IDC_BUTTON_LOCAL_REPLAY, m_btnLocalReplay);
	DDX_Control(pDX, IDC_BUTTON_SELECTID, m_btnNodeSelect);
	DDX_Control(pDX, IDC_COMBO_D5000_STATION_SEL, m_ComboD5000StationSel);
	DDX_Control(pDX, IDC_COMBO_D5000_EVNETTYPE_SEL, m_ComboD500EventType);
	DDX_Control(pDX, IDC_COMBO_D5000_DEV_SEL, m_ComboD5000DevSel);
	DDX_Control(pDX, IDC_LIST_REPLAY_RELATION, m_listReplayRelation);
	DDX_Control(pDX, IDC_COMBO_WEATHER_TYPE_SEL, m_ComboWeatherType);
	DDX_Control(pDX, IDC_COMBO_VIDEO_ACCESS_TYPE, m_ComboVideoAccessType);
}

BEGIN_MESSAGE_MAP(CDlgReplay, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_QUERY, &CDlgReplay::OnBnClickedButtonQuery)
	ON_BN_CLICKED(IDC_BUTTON_LOCALFILE, &CDlgReplay::OnBnClickedButtonLocalfile)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REPLAYFILE, &CDlgReplay::OnNMDblclkListReplayfile)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CDlgReplay::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CDlgReplay::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_FAST, &CDlgReplay::OnBnClickedButtonFast)
	ON_BN_CLICKED(IDC_BUTTON_SLOW, &CDlgReplay::OnBnClickedButtonSlow)
	ON_BN_CLICKED(IDC_RADIO_LOCALPLANT, &CDlgReplay::OnBnClickedRadioLocalPlant)
	ON_BN_CLICKED(IDC_RADIO_REMOTEPLANT, &CDlgReplay::OnBnClickedRadioRemotePlant)
	ON_BN_CLICKED(IDC_BUTTON_RP_PLANT_SELECT, &CDlgReplay::OnBnClickedButtonRpPlantSelect)
	ON_BN_CLICKED(IDC_BUTTON_RP_D5000_SELECT, &CDlgReplay::OnBnClickedButtonRpD5000Select)
	ON_BN_CLICKED(IDC_BUTTON_RP_WEATHER_SELECT, &CDlgReplay::OnBnClickedButtonRpWeatherSelect)
	ON_BN_CLICKED(IDC_BUTTON_RP_OP_SELECT, &CDlgReplay::OnBnClickedButtonRpOpSelect)
	ON_BN_CLICKED(IDC_BUTTON_SELECTID, &CDlgReplay::OnBnClickedButtonSelectid)
	ON_MESSAGE(WM_BITMAPSLIDER_LBUTTONDOWN,&CDlgReplay::OnSliderLButtonDownHandler)
	ON_MESSAGE(WM_SELECT_DEVICE_NOTIFY, OnSelectDeviceNotifyMessageHandler)
	ON_CBN_SELCHANGE(IDC_COMBO_D5000_STATION_SEL, &CDlgReplay::OnCbnSelchangeComboD5000StationSel)
	ON_BN_CLICKED(IDC_BUTTON_LOCAL_REPLAY, &CDlgReplay::OnBnClickedButtonLocalReplay)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_REPLAYFILE, &CDlgReplay::OnNMRClickListReplayfile)
	ON_COMMAND(ID_MENU_VIDEO_REPLAY_DOWNLOAD, &CDlgReplay::OnMenuVideoReplayDownload)
	ON_COMMAND(ID_MENU_VIDEO_REPLAY_PLAY, &CDlgReplay::OnMenuVideoReplayPlay)
	ON_BN_CLICKED(IDC_BUTTON_ALARM_REPLAY, &CDlgReplay::OnBnClickedButtonAlarmReplay)
	ON_BN_CLICKED(IDC_BUTTON_ALARM_SELECT, &CDlgReplay::OnBnClickedButtonAlarmSelect)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_REPLAYFILE, &CDlgReplay::OnNMCustomdrawListReplayfile)
END_MESSAGE_MAP()

// CDlgReplay 消息处理程序

//接收播放数据
void CDlgReplay::RecvRTP(int nid, void *data, int datalen, void *context)
{
	if (datalen <= 0||context == NULL)
		return;

	try
	{
		g_RecvBytes += datalen;

		CDlgReplay *pDlgReplay = (CDlgReplay *)context;

		GW_PLAY_CHANNEL_INFO *pPlayChannelInfo = NULL;
		TStreamDataParams DataParam;

		pPlayChannelInfo = pDlgReplay->m_ch.m_pPlayChannelInfo;
		if (pPlayChannelInfo == NULL)
			return;

		pDlgReplay->SaveVideoFileDataByVideoPlatform(data,datalen);

		DataParam.pBuffer = (BYTE*)data;
		DataParam.nSize = datalen;

		pPlayChannelInfo->Lock();
		try
		{
			if ((int)pPlayChannelInfo->m_hDecodeHandle > 0)
			{
				g_GWPlay.StreamInputData(pPlayChannelInfo->m_nDecodeType,pPlayChannelInfo->m_hDecodeHandle,&DataParam);
			}
		}
		catch(...)
		{
		}
		pPlayChannelInfo->Unlock();
	}
	catch(...)
	{

	}
}

//发送和接收到RTSP信息
void CDlgReplay::SendRecvMsg(int msgtype, char* msg, int msglen, void* context)
{
	if (msglen <= 0||context == NULL)
		return;

	try
	{
		CDlgReplay *pDlgReplay = (CDlgReplay*)context;

		if (msgtype == 1)//发送
		{
			if (strncmp(msg,"OPTIONS",7) == 0)
			{
				pDlgReplay->m_nRtspMsgState = REPLAY_RTSP_STATE_OPTIONS;
			}
			else if (strncmp(msg,"DESCRIBE",8) == 0)
			{
				pDlgReplay->m_nRtspMsgState = REPLAY_RTSP_STATE_DESCRIBE;
			}
			else if (strncmp(msg,"SETUP",5) == 0)
			{
				pDlgReplay->m_nRtspMsgState = REPLAY_RTSP_STATE_SETUP;
			}
			else if (strncmp(msg,"PLAY",4) == 0)
			{
				pDlgReplay->m_nRtspMsgState = REPLAY_RTSP_STATE_PLAY;
			}
			else if (strncmp(msg,"PAUSE",5) == 0)
			{
				pDlgReplay->m_nRtspMsgState = REPLAY_RTSP_STATE_PAUSE;
			}
			else if (strncmp(msg,"TEARDOWN",8) == 0)
			{
				pDlgReplay->m_nRtspMsgState = REPLAY_RTSP_STATE_TEARDOWN;
			}
		}
		else if (msgtype == 2)//接收
		{
			if (pDlgReplay->m_nRtspMsgState == REPLAY_RTSP_STATE_DESCRIBE)
			{
				if(strncmp(msg,"RTSP",4) == 0)
				{
					char *pSPS = strstr(msg,"sprop-parameter-sets=");
					if (pSPS != NULL)
					{
						pSPS  += strlen("sprop-parameter-sets=");
						char newSps[512] = {0};
						int    newSpsLen = 0;

						//						pSPS = "0000000034484b48feb3d0d608030420000000000110011001101000803e0000600120011110000004000000";

						newSpsLen = SwitchSPS(pSPS,(BYTE *)newSps);

						GW_PLAY_CHANNEL_INFO  *pPlayChannelInfo = pDlgReplay->m_ch.m_pPlayChannelInfo;

						if (pPlayChannelInfo == NULL)
							return;

						pPlayChannelInfo->Lock();

						pPlayChannelInfo->StopPlay();

						pPlayChannelInfo->SetData(NULL,NULL,-1,150,newSps,newSpsLen,NULL,NULL,-1);

						pPlayChannelInfo->StartPlay();

						pPlayChannelInfo->m_nCid = -1;

						pPlayChannelInfo->Unlock();
					}
				}
			}
		}
	}
	catch(...)
	{

	}
}

BOOL CDlgReplay::OnInitDialog()
{
	CDialog::OnInitDialog();

	//加载皮肤
	LoadSkin();

	//初始化控件 
	InitControl();

	// 启动rtsp库
	StartRtspStack();

	//设置定时器
	SetTimer(0, 500, NULL);

	// 选择本地平台作为查询对象
	m_SelectedPlant = 0;

	//选择平台视频
	OnBnClickedButtonRpPlantSelect();

	//设置前端视频回放进度信息回调函数
	SetCallbackGetPlayBackPos(VideoReplayDirectDeviceGetReplayPosCB);

	//设置前端视频下载进度信息回调函数
	SetCallbackGetLocalFilePos(VideoReplayDirectDeviceGetDownloadPosCB);

	//设置前端视频下载进度信息回调函数
	SetCallbackPlayLocalFilePos(VideoReplayDirectDevicePlayLocalFilePosCB);

	return TRUE;
}

BOOL CDlgReplay::OnEraseBkgnd(CDC* pDC)
{
	CRect   rect;
	GetClientRect(&rect);
	CBitmap   Bmp;
	BITMAP   bm;
	CDC   dcMem;

	CBitmap* pOldBitmap = NULL;
	pDC->FillRect (rect, &m_Brush);

	//上面业务选择的背景色
	Bmp.LoadBitmap(IDB_BITMAP_RP_UP_BG);
	Bmp.GetObject(sizeof(BITMAP),(LPVOID)&bm);   
	dcMem.CreateCompatibleDC(pDC);   
	pOldBitmap   =   dcMem.SelectObject(&Bmp);   

	pDC->StretchBlt(
		rect.left,
		rect.top,
		rect.right,
		32,
		&dcMem,
		0,
		0,
		bm.bmWidth,
		bm.bmHeight,
		SRCCOPY);

	dcMem.SelectObject(pOldBitmap);

	//查询条件
	Bmp.DeleteObject();
	Bmp.LoadBitmap(IDB_BITMAP_QUERYCONDITION);
	Bmp.GetObject(sizeof(BITMAP),(LPVOID)&bm);   
	dcMem.SelectObject(&Bmp);   

	pDC->StretchBlt(
		rect.left ,
		rect.top + 32,
		bm.bmWidth,
		bm.bmHeight,
		&dcMem,
		0,
		0,
		bm.bmWidth,
		bm.bmHeight,
		SRCCOPY);

	dcMem.SelectObject(pOldBitmap);

	return true;
}

BOOL CDlgReplay::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_ESCAPE)//屏住ESC键
		return	TRUE;
	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_RETURN)//屏住ENTER键
		return	TRUE; 
	if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_SPACE)//屏蔽空格键
		return	TRUE;

	return CDialog::PreTranslateMessage(pMsg);
}

HBRUSH CDlgReplay::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	//////////////////////////////////////////////////////////////////////////
	if (pWnd->GetDlgCtrlID() == IDC_RESULT_RECT)
	{
		// 背景色透明
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB(14,84,76));

		// 返回背景色的画刷
		return m_BgBrush;
	}

	if (pWnd->GetDlgCtrlID() == IDC_CONDITION_RECT)
	{
		// 背景色透明
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB(14,84,76));

		// 返回背景色的画刷
		return m_BgBrush;
	}

	if (pWnd->GetDlgCtrlID() == IDC_SLIDER_PLAYPOS)
	{
		pDC->SetBkColor(TRANSPARENT);
		return m_Brush;
	}

	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);
		return m_StaticBKBrush;
	}

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_PT_NODESEL)
	{
		pDC->SetBkMode(TRANSPARENT);
		return m_StaticBKBrush;
	}

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_D5000_STATION_SEL)
	{
		pDC->SetBkMode(TRANSPARENT);
		return m_StaticBKBrush;
	}

	return hbr;
}

void CDlgReplay::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	//////////////////////////////////////////////////////////////////////////
	RECT rc;
	CRect rect;
	CWnd *pWnd = NULL;

	int nControlWidth = 568;

	GetClientRect(&rc);

	if (GetDlgItem(IDC_LIST_REPLAYFILE)->GetSafeHwnd())
	{
		pWnd = GetDlgItem(IDC_LIST_REPLAYFILE);
		if(pWnd)//判断是否为空，因为对话框创建时会调用此函数，而当时控件还未创建
		{
			rect.left = 4;
			rect.right = nControlWidth - 8;
			rect.top = 291;
			rect.bottom=rc.bottom - 4;

			pWnd->MoveWindow(rect);	
		}

		// **********************平台查询条件中的位置
		pWnd = GetDlgItem(IDC_STATIC_PT_NODESEL); 
		if(pWnd)
		{
			rect.left = 18;
			rect.right = rect.left + 70;
			rect.top = 94;
			rect.bottom = rect.top + 17;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_EDIT_SELECTEDID); 
		if(pWnd)
		{
			rect.left = 91;
			rect.right = rect.left + 152;
			rect.top = 93;
			rect.bottom = rect.top + 23;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_BUTTON_SELECTID);
		if(pWnd)
		{
			rect.left = 244;
			rect.right = rect.left + 20;
			rect.top = 93;
			rect.bottom = rect.top + 23;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_STATIC_VIDEO_ACCESS_TYPE);
		if(pWnd)
		{
			rect.left = 280;
			rect.right = rect.left + 80;
			rect.top = 93;
			rect.bottom = rect.top + 23;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_COMBO_VIDEO_ACCESS_TYPE);
		if(pWnd)
		{
			rect.left = 361;
			rect.right = rect.left + 172;
			rect.top = 93;
			rect.bottom = rect.top + 23+200;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_STATIC_PT_RECTYPE_SEL); 
		if(pWnd)
		{
			rect.left = 18;
			rect.right = rect.left + 70;
			rect.top = 121;
			rect.bottom = rect.top + 17;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_COMBO_RECORDTYPE); 
		if(pWnd)
		{
			rect.left = 91;
			rect.right = rect.left + 172;
			rect.top = 118;
			rect.bottom = rect.top + 23+200;

			pWnd->MoveWindow(rect);
		}

		// **********************D5000查询条件中的位置
		pWnd = GetDlgItem(IDC_STATIC_D5000_STATION_SEL); 
		if(pWnd)
		{
			rect.left = 18;
			rect.right = rect.left + 70;
			rect.top = 94;
			rect.bottom = rect.top + 17;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_COMBO_D5000_STATION_SEL); 
		if(pWnd)
		{
			rect.left = 91;
			rect.right = rect.left + 152 + 20;
			rect.top = 93;
			rect.bottom = rect.top + 23+200;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_STATIC_D5000_EVENTTYPE_SEL); 
		if(pWnd)
		{
			rect.left = 18;
			rect.right = rect.left + 70;
			rect.top = 121;
			rect.bottom = rect.top + 17;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_COMBO_D5000_EVNETTYPE_SEL);
		if(pWnd)
		{
			rect.left = 91;
			rect.right = rect.left + 172;
			rect.top = 118;
			rect.bottom = rect.top + 23+200;

			pWnd->MoveWindow(rect);
		}

		// 		pWnd = GetDlgItem(IDC_BUTTON_D5000_STATION_SEL); 
		// 		if(pWnd)
		// 		{
		// 			rect.left = 244;
		// 			rect.right = rect.left + 20;
		// 			rect.top = 93;
		// 			rect.bottom = rect.top + 23;
		// 
		// 			pWnd->MoveWindow(rect);
		// 		}

		//pWnd = GetDlgItem(IDC_STATIC_D5000_DEV_SEL); 
		//if(pWnd)
		//{
		//	rect.left = 18;
		//	rect.right = rect.left + 70;
		//	rect.top = 148;
		//	rect.bottom = rect.top + 17;

		//	pWnd->MoveWindow(rect);
		//}

		//pWnd = GetDlgItem(IDC_COMBO_D5000_DEV_SEL); 
		//if(pWnd)
		//{
		//	rect.left = 91;
		//	rect.right = rect.left + 172;
		//	rect.top = 143;
		//	rect.bottom = rect.top + 23+200;

		//	pWnd->MoveWindow(rect);
		//}

		//**********************点击告警回放时调整位置
		pWnd = GetDlgItem(IDC_STATIC_ALARM_STATION);
		if (pWnd)
		{
			rect.left = 18;
			rect.right = rect.left + 70;
			rect.top = 94;
			rect.bottom = rect.top + 17;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_EDIT_ALARM_REPLAY_STATION);
		if (pWnd)
		{
			rect.left = 91;
			rect.right = rect.left + 152;
			rect.top = 93;
			rect.bottom = rect.top + 23;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_BUTTON_ALARM_SELECT);
		if (pWnd)
		{
			rect.left = 244;
			rect.right = rect.left + 20;
			rect.top = 93;
			rect.bottom = rect.top + 23;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_STATIC_ALARM_DEVICE);
		if(pWnd)
		{
			rect.left = 280;
			rect.right = rect.left + 80;
			rect.top = 93;
			rect.bottom = rect.top + 23;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_COMBO_ALARM_DEVICE);
		if(pWnd)
		{
			rect.left = 361;
			rect.right = rect.left + 172;
			rect.top = 93;
			rect.bottom = rect.top + 23+200;

			pWnd->MoveWindow(rect);
		}


		// **********************气象查询条件中的位置
		pWnd = GetDlgItem(IDC_STATIC_WEATHER_TYPE_SEL); 
		if(pWnd)
		{
			rect.left = 18;
			rect.right = rect.left + 70;
			rect.top = 94;
			rect.bottom = rect.top + 17;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_COMBO_WEATHER_TYPE_SEL);
		if(pWnd)
		{
			rect.left = 91;
			rect.right = rect.left + 172;
			rect.top = 93;
			rect.bottom = rect.top + 223;

			pWnd->MoveWindow(rect);
		}

		// **********************操作票查询条件中的位置
		pWnd = GetDlgItem(IDC_STATIC_OP_STATION_SEL); 
		if(pWnd)
		{
			rect.left = 18;
			rect.right = rect.left + 70;
			rect.top = 94;
			rect.bottom = rect.top + 17;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_EDIT_OP_STATION_SEL);  
		if(pWnd)
		{
			rect.left = 91;
			rect.right = rect.left + 152 + 20;
			rect.top = 93;
			rect.bottom = rect.top + 23;

			pWnd->MoveWindow(rect);
		}

		pWnd = GetDlgItem(IDC_BUTTON_OP_STATION_SEL); 
		if(pWnd)
		{
			rect.left = 244;
			rect.right = rect.left + 20;
			rect.top = 93;
			rect.bottom = rect.top + 23;

			pWnd->MoveWindow(rect);
		}

		// *****************************************最上面的按钮
		rect.left = 0;
		rect.right = 0;
		rect.top = 1;
		rect.bottom = 27;

#if VM_SJ_CLIENT_VERSION
		{
			pWnd = GetDlgItem(IDC_BUTTON_RP_D5000_SELECT);  
			if(pWnd)
			{
				pWnd->ShowWindow(SW_HIDE);
			}
		}
#else
		{
			pWnd = GetDlgItem(IDC_BUTTON_RP_D5000_SELECT);  
			if(pWnd)
			{
				rect.left = rect.right+ 1;
				rect.right =rect.left + 90;
				rect.top = 1;
				rect.bottom = 27;

				pWnd->MoveWindow(rect);
			}
		}
#endif

		//pWnd = GetDlgItem(IDC_BUTTON_RP_WEATHER_SELECT); 
		//if(pWnd)
		//{
		//	rect.left = rect.right+ 1;
		//	rect.right =rect.left + 90;
		//	rect.top = 1;
		//	rect.bottom = 27;

		//	pWnd->MoveWindow(rect);
		//}

		//pWnd = GetDlgItem(IDC_BUTTON_RP_OP_SELECT); 
		//if(pWnd)
		//{
		//	rect.left = rect.right+ 1;
		//	rect.right =rect.left + 90;
		//	rect.top = 1;
		//	rect.bottom = 27;

		//	pWnd->MoveWindow(rect);
		//}

		pWnd = GetDlgItem(IDC_BUTTON_RP_PLANT_SELECT);   
		if(pWnd)
		{
			rect.left = rect.right+ 1;
			rect.right =rect.left + 90;
			rect.top = 1;
			rect.bottom = 27;

			pWnd->MoveWindow(rect);
		}

		//告警回放按钮位置
		pWnd = GetDlgItem(IDC_BUTTON_ALARM_REPLAY);
		if (pWnd)
		{
			rect.left = rect.right+ 1;
			rect.right =rect.left + 90;
			rect.top = 1;
			rect.bottom = 27;

			pWnd->MoveWindow(rect);
		}

		int nLeft = nControlWidth;

		//视频显示区域
		GetClientRect(&rc);
		rc.left += nControlWidth;
		rc.bottom -= 6;
		rc.top += 32;
		rc.right -= 4;

		CRect viewrect;
		if (pWnd)
		{
			viewrect.top = rc.top+2;
			viewrect.bottom = rc.bottom - 50 - 3;
			viewrect.left = rc.left;
			viewrect.right = rc.right;

			m_ch.MoveWindow(viewrect);
			viewrect.bottom += 3;
		}

		// 播放按钮
		pWnd = GetDlgItem(IDC_BUTTON_PLAY);
		if (pWnd)
		{
			rect.top = viewrect.bottom + 15;
			rect.bottom = viewrect.bottom + 45;
			rect.left = viewrect.left;
			rect.right = rect.left + 30;

			pWnd->MoveWindow(rect);
		}

		// 停止按钮
		pWnd = GetDlgItem(IDC_BUTTON_STOP);
		if (pWnd)
		{
			rect.top = viewrect.bottom + 15;
			rect.bottom = viewrect.bottom + 45;
			rect.left = rect.right+1;
			rect.right = rect.left+30;

			pWnd->MoveWindow(rect);
		}

		// 快进按钮
		pWnd = GetDlgItem(IDC_BUTTON_FAST);
		if (pWnd)
		{
			rect.top = viewrect.bottom + 15;
			rect.bottom = viewrect.bottom + 45;
			rect.left = rect.right+1;
			rect.right = rect.left+30;

			pWnd->MoveWindow(rect);
		}

		// 慢进按钮
		pWnd = GetDlgItem(IDC_BUTTON_SLOW);
		if (pWnd)
		{
			rect.top = viewrect.bottom + 15;
			rect.bottom = viewrect.bottom + 45;
			rect.left = rect.right+1;
			rect.right = rect.left+30;

			pWnd->MoveWindow(rect);
		}

		// 播放速度
		pWnd = GetDlgItem(IDC_PALYSPEED);
		if (pWnd)
		{
			rect.top = viewrect.bottom + 15;
			rect.bottom = viewrect.bottom + 45;
			rect.left = rect.right+3;
			rect.right = rect.left+100;

			pWnd->MoveWindow(rect);
		}

		// 状态栏
		pWnd = GetDlgItem(IDC_PALYSTATE);
		if (pWnd)
		{
			rect.top = viewrect.bottom + 15;
			rect.bottom = viewrect.bottom + 45;
			rect.left = rect.right+1;
			rect.right = viewrect.right-1;

			pWnd->MoveWindow(rect);
		}

		// 进度条
		pWnd = GetDlgItem(IDC_SLIDER_PLAYPOS);
		if (pWnd)
		{
			rect.top = viewrect.bottom;
			rect.bottom = viewrect.bottom + 15;
			rect.left = viewrect.left+3;
			rect.right = viewrect.right-3;

			pWnd->MoveWindow(rect);

			if (m_nPlaySliderWidth != rect.right - rect.left)
			{
				m_nPlaySliderWidth = rect.right - rect.left;

				m_SliderPlayPos.SetBitmapChannel(IDB_BITMAP_REPLAY_SLIDER_OFF, IDB_BITMAP_REPLAY_SLIDER_ON, TRUE);
				//	m_SliderPlayPos.SetBitmapThumb(IDB_BITMAP_REPLAY_SLIDER_THUMB_OFF, IDB_BITMAP_REPLAY_SLIDER_THUMB_ON, FALSE);
				m_SliderPlayPos.SetRange(0,100);
				m_SliderPlayPos.SetPos(0);
				m_SliderPlayPos.SetMarginLeft(0);
				m_SliderPlayPos.SetMarginRight(0);
				m_SliderPlayPos.SetPageSize(10);
				m_SliderPlayPos.DrawFocusRect( FALSE );
			}
		}
	}	
}

void CDlgReplay::OnBnClickedButtonQuery()
{
	//鼠标为等待状态
	AfxGetApp()->DoWaitCursor(1); 

	if (m_nTableSelectIndex == 1)
	{
		SearchD5000LinkageHistoryVideo();
	}
	else if (m_nTableSelectIndex == 2)
	{
		SearchWeatherLinkageHistoryVideo();
	}
	else if (m_nTableSelectIndex == 4)
	{
		SearchVideoPlatformHistoryVideo();
	}
	//告警回放查询
	else if (m_nTableSelectIndex == ALARM_REPLAY)
	{
		SearchAlarmReplayHistoryVideo();
	}

	//恢复鼠标为正常状态
	AfxGetApp()->DoWaitCursor(0);
}

void CDlgReplay::SearchAlarmReplayHistoryVideo()
{
	UpdateData(TRUE);

	char szStartLinkageTime[128] = {0};
	char szStopLinkageTime[128] = {0};

	CTime starttime(m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	CTime stoptime(m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	if (stoptime <= starttime)
	{
		MessageBox("时间选择错误：开始时间大于结束时间","视频监视");
		return ;
	}

	sprintf(szStartLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	sprintf(szStopLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	//取变电站id，设备名称，时间参数
	char szYear[8] = {0};
	szYear[0] = szStartLinkageTime[0];
	szYear[1] = szStartLinkageTime[1];
	szYear[2] = szStartLinkageTime[2];
	szYear[3] = szStartLinkageTime[3];
	szYear[4] = 0;

	int nStationId = m_tStationNode.node_id;//变电站id
	CString strDevName = "";
	GetDlgItem(IDC_COMBO_ALARM_DEVICE)->GetWindowText(strDevName);//设备名称

	//根据变电站id和设备名称获取告警信息并添加到listcontrol中
	SearchAndAddToListControl(nStationId,strDevName.GetBuffer(),szYear,szStartLinkageTime,szStopLinkageTime);
}

void CDlgReplay::SearchAndAddToListControl(int nStationId, char* strDevName, char* szYear,char* szStartTime,char* szEndTime)
{
	char sql_buf[4096] = {0};
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int nCount = 0;
	
	if (nStationId == 0)//表示全部变电站
	{
		sprintf_s(sql_buf, "select d.voltage_class,d.station_name_videoplant,c.`name`, "
			" a.type,a.start_time,a.process_time,c.relation_video_status,a.alarm_level,a.process_status,a.process_person from ass_alarm_%s a  "
			" left join ass_rvu b on a.rvu_id=b.rvu_id "
			" left join ass_rvu_sm c on a.dev_id=c.id    "
			" left join ob_d5000_station d on b.station_id=d.station_id " 
			" where a.start_time>'%s' and a.start_time<'%s' ",szYear,szStartTime,szEndTime);
	}
	else if (strcmp(strDevName,"所有设备") == 0 && nStationId != 0)
	{
		sprintf_s(sql_buf, "select d.station_name_videoplant,d.voltage_class,c.`name`, "
			" a.type,a.start_time,a.process_time,c.relation_video_status,a.alarm_level,a.process_status,a.process_person from ass_alarm_%s a  "
			" left join ass_rvu b on a.rvu_id=b.rvu_id "
			" left join ass_rvu_sm c on a.dev_id=c.id    "
			" left join ob_d5000_station d on b.station_id=d.station_id " 
			" where a.start_time>'%s' and a.start_time<'%s' and d.station_id=%d",szYear,szStartTime,szEndTime,nStationId);
	}
	else if (strcmp(strDevName,"所有设备") != 0 && nStationId != 0)
	{
		sprintf_s(sql_buf, "select d.station_name_videoplant,d.voltage_class,c.`name`, "
			" a.type,a.start_time,a.process_time,c.relation_video_status,a.alarm_level,a.process_status,a.process_person from ass_alarm_%s a  "
			" left join ass_rvu b on a.rvu_id=b.rvu_id "
			" left join ass_rvu_sm c on a.dev_id=c.id    "
			" left join ob_d5000_station d on b.station_id=d.station_id " 
			" where a.start_time>'%s' and a.start_time<'%s' and c.`name`='%s' and d.station_id=%d",szYear,szStartTime,szEndTime,strDevName,nStationId);
	}

	m_ListCtrl_ReplayFile.DeleteAllItems();

	if (!mysql_query(g_mySqlData, sql_buf))
	{
		res = mysql_store_result(g_mySqlData);

		while (row = mysql_fetch_row(res))
		{
			m_ListCtrl_ReplayFile.InsertItem(nCount,row[1]);
			m_ListCtrl_ReplayFile.SetItemText(nCount,1,row[0]);
			m_ListCtrl_ReplayFile.SetItemText(nCount,2,row[2]);
			m_ListCtrl_ReplayFile.SetItemText(nCount,3,row[3]);
			m_ListCtrl_ReplayFile.SetItemText(nCount,4,row[4]);
			m_ListCtrl_ReplayFile.SetItemText(nCount,5,row[5]);
			
			if (atoi(row[6])==0)
			{
				m_ListCtrl_ReplayFile.SetItemText(nCount,6,"未关联视频");
			}
			else
			{
				m_ListCtrl_ReplayFile.SetItemText(nCount,6,"已关联视频");
			}
			
			m_ListCtrl_ReplayFile.SetItemText(nCount,7,row[7]);
			m_ListCtrl_ReplayFile.SetItemText(nCount,8,row[8]);
			m_ListCtrl_ReplayFile.SetItemText(nCount,9,row[9]);

			nCount++;
		}
		mysql_free_result(res) ;
	}

	if (nCount == 0)
	{
		m_ListCtrl_ReplayFile.InsertItem(nCount,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,1,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,2,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,3,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,4,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,5,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,6,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,7,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,8,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,9,"无数据");
	}
}

// 打开本地文件进行播放
void CDlgReplay::OnBnClickedButtonLocalfile()
{
	CString strPlayFileName;

	if (!BrowseFile(&strPlayFileName))
	{
		MessageBox("打开文件失败","视频监视");
		return;
	}

	char FileName[256] = {0};
	sprintf(FileName, "%s", strPlayFileName);

	if (strstr(FileName,"dev_") != NULL)
	{
		m_nPlayVideoAccessType = 1;
		if(m_ch.ReplayOpenFileByDirectDevice(FileName) < 0)
			return;
	}
	else 
	{
		m_nPlayVideoAccessType = 0;
		if (m_ch.ReplayOpenFileByVideoPlatform(FileName) < 0)
		return;
	}

	m_bPlayRemoteFlag = FALSE;

	m_ch.m_strStation.SetText(strPlayFileName);

	PlayLocalORRemoteVideo();
}

bool CDlgReplay::BrowseFile(CString *strFileName)
{
	CFileDialog dlg(TRUE, 
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR,
		"Mpeg4 Files (*.mp4)|*.mp4|h264 Files (*.264)|*.264|h264 Files (*.h264)|*.h264|All files(*.*)|*.*||");

	if(dlg.DoModal() == IDCANCEL)
	{
		return FALSE;
	}

	*strFileName = dlg.GetPathName();
	return TRUE;
}

BOOL CDlgReplay::LoadSkin()
{
	//加载按钮图片
	m_btnQuery.LoadBitmap(IDB_BITMAP_QUERY,0);
	m_btnQuery.LoadBitmap(IDB_BITMAP_QUERY,1);

	m_btnPlay.LoadBitmap(IDB_BITMAP_VIDEOPLAY,0);
	m_btnPlay.LoadBitmap(IDB_BITMAP_VIDEOPAUSE,1);
	m_btnPlay.SetIndex(0);
	m_btnPlay.Invalidate(TRUE);

	m_btnStop.LoadBitmap(IDB_BITMAP_VIDEOSTOP,0);
	m_btnStop.LoadBitmap(IDB_BITMAP_VIDEOSTOP,1);
	m_btnStop.SetIndex(0);
	m_btnStop.Invalidate(TRUE);

	m_btnFast.LoadBitmap(IDB_BITMAP_VIDEOFAST,0);
	m_btnFast.LoadBitmap(IDB_BITMAP_VIDEOFAST,1);
	m_btnFast.SetIndex(0);
	m_btnFast.Invalidate(TRUE);

	m_btnSlow.LoadBitmap(IDB_BITMAP_VIDEOSLOW,0);
	m_btnSlow.LoadBitmap(IDB_BITMAP_VIDEOSLOW,1);
	m_btnSlow.SetIndex(0);
	m_btnSlow.Invalidate(TRUE);

	m_btnSelPlant.LoadBitmap(IDB_BITMAP_RP_SEL_PT_ON, 0);
	m_btnSelPlant.LoadBitmap(IDB_BITMAP_RP_SEL_PT_OFF, 1);
	m_btnSelPlant.SetIndex(0);
	m_btnSelPlant.Invalidate(TRUE);

	m_btnAlarmReplay.LoadBitmap(IDB_BITMAP_ALARM_REPLAY_ON, 0);
	m_btnAlarmReplay.LoadBitmap(IDB_BITMAP_ALARM_REPLAY_OFF, 1);
	m_btnAlarmReplay.SetIndex(1);
	m_btnAlarmReplay.Invalidate(TRUE);

	m_btnSelD5000.LoadBitmap(IDB_BITMAP_RP_SEL_D5000_ON, 0);
	m_btnSelD5000.LoadBitmap(IDB_BITMAP_RP_SEL_D5000_OFF, 1);
	m_btnSelD5000.SetIndex(1);
	m_btnSelD5000.Invalidate(TRUE);

	m_btnSelOP.LoadBitmap(IDB_BITMAP_RP_SEL_OP_ON, 0);
	m_btnSelOP.LoadBitmap(IDB_BITMAP_RP_SEL_OP_OFF, 1);
	m_btnSelOP.SetIndex(1);
	m_btnSelOP.Invalidate(TRUE);

	m_btnSelWeather.LoadBitmap(IDB_BITMAP_RP_SEL_WEATHER_ON, 0);
	m_btnSelWeather.LoadBitmap(IDB_BITMAP_RP_SEL_WEATHER_OFF, 1);
	m_btnSelWeather.SetIndex(1);
	m_btnSelWeather.Invalidate(TRUE);

	m_btnLocalReplay.LoadBitmap(IDB_BITMAP_LOCAL_REPLAYER);
	m_btnLocalReplay.Invalidate(TRUE);

	m_btnNodeSelect.LoadBitmap(IDB_BITMAP_NODE_SELECT);
	m_btnNodeSelect.Invalidate(TRUE);

	m_SliderPlayPos.SetBitmapChannel(IDB_BITMAP_REPLAY_SLIDER_OFF, IDB_BITMAP_REPLAY_SLIDER_ON, TRUE);
	//	m_SliderPlayPos.SetBitmapThumb(IDB_BITMAP_REPLAY_SLIDER_THUMB_OFF, IDB_BITMAP_REPLAY_SLIDER_THUMB_ON, FALSE);
	m_SliderPlayPos.SetRange(0,100);
	m_SliderPlayPos.SetPos(0);
	m_SliderPlayPos.SetMarginLeft(0);
	m_SliderPlayPos.SetMarginRight(0);
	m_SliderPlayPos.SetPageSize(10);
	m_SliderPlayPos.DrawFocusRect( FALSE );

	RECT rc;
	m_SliderPlayPos.GetWindowRect(&rc);
	m_nPlaySliderWidth = rc.right - rc.left;

	return TRUE;
}

BOOL CDlgReplay::InitControl()
{
	// 查询结果列表中需要显示的列包括了：开始时间、结束时间、摄像头名称、
	DWORD dwStyle;
	dwStyle = m_ListCtrl_ReplayFile.GetStyle();
	dwStyle = LVS_EX_GRIDLINES |LVS_EX_FULLROWSELECT ;

	m_ListCtrl_ReplayFile.SetExtendedStyle(dwStyle);
	m_ListCtrl_ReplayFile.SetBkColor(RGB(239,246,253));
	m_ListCtrl_ReplayFile.SetTextBkColor(RGB(0xfe,0xFF,0xc6));


	//////////////////////////////////////////////////////////////////////////
	m_listReplayRelation.InsertColumn(0,"联动类型");
	m_listReplayRelation.SetColumnWidth(0,80);
	m_listReplayRelation.InsertColumn(1,"设备名称");
	m_listReplayRelation.SetColumnWidth(1,120);
	m_listReplayRelation.InsertColumn(2,"站点名称");
	m_listReplayRelation.SetColumnWidth(2,120);
	m_listReplayRelation.InsertColumn(3,"联动时间");
	m_listReplayRelation.SetColumnWidth(3,100);
	m_listReplayRelation.InsertColumn(4,"分类型");
	m_listReplayRelation.SetColumnWidth(4,80);
	m_listReplayRelation.InsertColumn(5,"状态");
	m_listReplayRelation.SetColumnWidth(5,80);
	m_listReplayRelation.InsertColumn(6,"屏号");
	m_listReplayRelation.SetColumnWidth(6,60);
	m_listReplayRelation.InsertColumn(7,"备注");
	m_listReplayRelation.SetColumnWidth(7,100);
	m_listReplayRelation.InsertColumn(8,"Value1");
	m_listReplayRelation.SetColumnWidth(8,60);
	m_listReplayRelation.InsertColumn(9,"Value2");
	m_listReplayRelation.SetColumnWidth(9,60);
	m_listReplayRelation.InsertColumn(10,"联动站点");
	m_listReplayRelation.SetColumnWidth(10,100);
	m_listReplayRelation.InsertColumn(11,"联动数量");
	m_listReplayRelation.SetColumnWidth(11,80);
	m_listReplayRelation.InsertColumn(12,"NodeId");
	m_listReplayRelation.SetColumnWidth(12,80);

	m_ComboWeatherType.InsertString(0, "所有告警天气");
	m_ComboWeatherType.InsertString(1, "暴雪");
	m_ComboWeatherType.InsertString(2, "暴雨");
	m_ComboWeatherType.InsertString(3, "高温");
	m_ComboWeatherType.InsertString(4, "低温");
	m_ComboWeatherType.InsertString(5, "大雾");
	m_ComboWeatherType.InsertString(6, "大风");
	m_ComboWeatherType.InsertString(7, "台风");
	m_ComboWeatherType.InsertString(8, "雷电");
	m_ComboWeatherType.SetCurSel(0);

	m_ComboVideoAccessType.InsertString(0,"通过平台");
	m_ComboVideoAccessType.InsertString(1,"直连设备");
	m_ComboVideoAccessType.SetCurSel(1);

	m_ComboRecordFileType.InsertString(0, "所有录像内容");
	m_ComboRecordFileType.InsertString(1, "报警录像");
	m_ComboRecordFileType.InsertString(2, "定时录像");
	m_ComboRecordFileType.SetCurSel(0);

	m_strPlayState.Format("当前状态：无");
	m_strPlaySpeed.Format("播放速度：1");

	m_SliderPlayPos.SetRange(0, 1000);
	m_SliderPlayPos.SetPos(0);

	//创建视频回放窗口
	m_ch.Create(IDD_DIALOG_VIEWOUT, this);
	m_ch.m_iWinID = REPLAYCH;
	m_ch.m_Pic.m_iWinID = REPLAYCH;
	m_ch.ShowWindow(SW_SHOW);

	//初始化控件
	CTime nowtime = CTime::GetCurrentTime();
	m_StartDay = nowtime;
	m_StopDay = nowtime;
	m_StartTime = CTime(2013,1,1,0,0,0);
	m_StopTime = CTime(2013,1,1,23,59,59);
	UpdateData(FALSE);

	return TRUE;
}

//启动rtsp栈
int CDlgReplay::StartRtspStack()
{
	E_RtspRes nRetCode = E_RTSP_OK;

	//1. Get Instance
	CRtspEngineer*	pStackMgr = NULL;
	pStackMgr = CRtspEngineer::Instance();
	if(pStackMgr == NULL)
	{
		TRACE("Failed to Get RTSP Stack\n\n");
		return -1;
	}

	//2. Init Stack
	T_RtspConfig cfg;
	memset(&cfg, 0, sizeof(T_RtspConfig));
	nRetCode = pStackMgr->InitEngineer(&cfg);
	if(nRetCode != E_RTSP_OK)
	{
		TRACE("Failed to Initialize Stack\n\n");
		return -1;
	}

	//3. Set Callback
	pStackMgr->SetCallback(&CDlgReplay::RecvRTP, this);
	pStackMgr->SetMsgCallback(&CDlgReplay::SendRecvMsg,this);

	//4. Start
	nRetCode = pStackMgr->StartEngineer();
	if(nRetCode != E_RTSP_OK)
	{
		TRACE("Failed to Start Engineer\n\n");
		return -1;
	}

	//5. Get Interface
	nRetCode = pStackMgr->GetInterface(&m_pRtspClientAPI);
	if(nRetCode!=E_RTSP_OK || m_pRtspClientAPI==NULL)
	{
		TRACE("Failed to Query Interface\n\n");
		return -1;
	}

	TRACE("The RTSP Engineer Start OK\n\n");
	return 0;
}

void CDlgReplay::OnNMDblclkListReplayfile(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	//////////////////////////////////////////////////////////////////////////
	POSITION pos = m_ListCtrl_ReplayFile.GetFirstSelectedItemPosition();

	if (pos == NULL)
		return;

	int index = m_ListCtrl_ReplayFile.GetNextSelectedItem(pos);

	if (index < 0)
		return;

	//关闭当前正在播放的视频
	OnBnClickedButtonStop();

	char szRemoteVideoShowTitle[512] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szCameraName[256] = {0};
	BOOL nRet = FALSE;

	if (m_RemotePlayState == REPLAY_STOP)
	{
		if (m_nTableSelectIndex == 1)//联动
		{

			if (m_nSearchVideoAccessType == 0)
			{
				nRet = GetAndSetVideoFileRtspInfo(index);
			}
			else
			{
				nRet = GetAndSetDeviceVideoFileInfo(index);
			}

			if (nRet == FALSE)
				return;

			m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));
			m_ListCtrl_ReplayFile.GetItemText(index, 2,szDeviceName,sizeof(szDeviceName));
			m_ListCtrl_ReplayFile.GetItemText(index, 3,szStationName,sizeof(szStationName));
			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{联动视频}-{%s}-{%s}-{%s}",szCameraName,szDeviceName,szStationName);

			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

			sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
			if (strlen(m_RtspURL) <= 4)
			{
				MessageBox("未查询到关联视频","视频监视");
				return;
			}

			m_ListCtrl_ReplayFile.GetItemText(index,11,m_szDvrRecordStartTime,sizeof(m_szDvrRecordStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,12,m_szDvrRecordStopTime,sizeof(m_szDvrRecordStopTime));

			m_nRecordFileSize = g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 14));
			m_nDvrType = g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

			m_ListCtrl_ReplayFile.GetItemText(index,11,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,12,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
			strcpy_s(m_szWeatherStationName,sizeof(m_szWeatherStationName),szStationName);
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

			SetLinkTimeLinePos(index);
		}
		else if (m_nTableSelectIndex == 4)//平台视频
		{
			m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));

			if (strcmp(szCameraName,"无数据") == 0)
				return;

			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{平台视频}-{%s}",szCameraName);

			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

			sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 3));
			g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 4));
			g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

			m_ListCtrl_ReplayFile.GetItemText(index,1,m_szDvrRecordStartTime,sizeof(m_szDvrRecordStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,2,m_szDvrRecordStopTime,sizeof(m_szDvrRecordStopTime));

			m_ListCtrl_ReplayFile.GetItemText(index,1,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,2,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象
		}
		else if (m_nTableSelectIndex == 2)//气象
		{
			if (m_nSearchVideoAccessType == 0)
			{
				nRet = GetAndSetVideoFileRtspInfo(index);
			}
			else
			{
				nRet = GetAndSetDeviceVideoFileInfo(index);
			}

			if (nRet == FALSE)
				return;

			m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));
			m_ListCtrl_ReplayFile.GetItemText(index, 2,szDeviceName,sizeof(szDeviceName));
			m_ListCtrl_ReplayFile.GetItemText(index, 3,szStationName,sizeof(szStationName));
			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{气象联动}-{%s}-{%s}",szCameraName,szStationName);

			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

			sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
			if (strlen(m_RtspURL) <= 4)
			{
				MessageBox("未查询到关联视频","视频监视");
				return;
			}

			g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 14));
			g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

			m_ListCtrl_ReplayFile.GetItemText(index,11,m_szDvrRecordStartTime,sizeof(m_szDvrRecordStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,12,m_szDvrRecordStopTime,sizeof(m_szDvrRecordStopTime));

			m_ListCtrl_ReplayFile.GetItemText(index,11,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,12,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
			strcpy_s(m_szWeatherStationName,sizeof(m_szWeatherStationName),szStationName);
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

			SetLinkTimeLinePos(index);
		}
		else if (m_nTableSelectIndex == 3)//操作票
		{
			return;
		}
		else
			return;

		m_bPlayRemoteFlag = TRUE;

		g_VideoReplayDownloadMenuFlag = false;//回放

		PlayLocalORRemoteVideo();
	}

	//////////////////////////////////////////////////////////////////////////
	*pResult = 0;
}

// 初始化播放窗口
void CDlgReplay::InitViewChannel()
{
	memset(m_ch.m_Headerbuf, 0, sizeof(m_ch.m_Headerbuf));
	m_ch.m_VideoViewOutInfo.Lock();
	m_ch.m_VideoViewOutInfo.m_nDecodeTag = g_DevType;
	m_ch.m_VideoViewOutInfo.UnLock();
	m_ch.m_nHeaderLen = 0;
	m_ch.InitViewChannel();
}

// 播放开始
void CDlgReplay::OnBnClickedButtonPlay()
{
	PlayLocalORRemoteVideo();
}

// 播放结束
void CDlgReplay::OnBnClickedButtonStop()
{
	if (m_bPlayRemoteFlag == FALSE)// 当前为播放本地，关闭
	{
		StopPlayLocalFile();
	}
	else // 当前为播放远端，关闭
	{
		StopPlayRemoteVideo();
		memset(m_RtspURL,0,sizeof(m_RtspURL));
		g_RecvBytes = 0;
		g_TotalBytes = 0;
	}

	m_btnPlay.SetIndex(0);
	m_btnPlay.Invalidate(TRUE);

	KillTimer(1);
	m_SliderPlayPos.SetPos(0);
	m_SliderPlayPos.SetDrawLinePointFlag(false);

	m_ch.m_strStation.SetText(" ");
}

void CDlgReplay::OnBnClickedButtonSlow()
{
	if (m_bPlayRemoteFlag == false)
	{
		SlowPlayLocalFile();
	}
	else
	{
		SlowPlayRemoteVideo();
	}
}

void CDlgReplay::OnBnClickedButtonFast()
{
	if (m_bPlayRemoteFlag == false)
	{
		FastPlayLocalFile();
	}
	else
	{
		FastPlayRemoteVideo();
	}
}

void CDlgReplay::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
	case 1:
		{
			ModifyVideoStatus();
		}
		break;
	}

	CDialog::OnTimer(nIDEvent);
}

void CDlgReplay::OnClose()
{
	//////////////////////////////////////////////////////////////////////////
	//	m_ch.DeInitSDK();

	CDialog::OnClose();
}

void CDlgReplay::OnBnClickedRadioLocalPlant()
{
	//////////////////////////////////////////////////////////////////////////
	m_SelectedPlant = 0;
}

void CDlgReplay::OnBnClickedRadioRemotePlant()
{
	//////////////////////////////////////////////////////////////////////////
	m_SelectedPlant = 1;
}

// 查询平台的条件
void CDlgReplay::OnBnClickedButtonRpPlantSelect()
{
	if (m_nTableSelectIndex == 4)
		return;

	if (m_RemotePlayState != REPLAY_STOP||m_LocalPlayState != REPLAY_STOP)
		return;

	m_nTableSelectIndex = 4;

	ShowAlarmReplayControl(FALSE);

	GetDlgItem(IDC_STATIC_PT_NODESEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_EDIT_SELECTEDID)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_BUTTON_SELECTID)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC_PT_RECTYPE_SEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_COMBO_RECORDTYPE)->ShowWindow(SW_SHOW);
	
	GetDlgItem(IDC_STATIC_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_D5000_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_D5000_DEV_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_DEV_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_D5000_EVENTTYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_EVNETTYPE_SEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_WEATHER_TYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_WEATHER_TYPE_SEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_OP_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT_OP_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_OP_STATION_SEL)->ShowWindow(SW_HIDE);

	m_btnSelPlant.SetIndex(0);
	m_btnSelPlant.Invalidate(true);
	m_btnSelD5000.SetIndex(1);
	m_btnSelD5000.Invalidate(true);
	m_btnSelOP.SetIndex(1);
	m_btnSelOP.Invalidate(true);
	m_btnSelWeather.SetIndex(1);
	m_btnSelWeather.Invalidate(true);
	m_btnAlarmReplay.SetIndex(1);
	m_btnAlarmReplay.Invalidate(true);

	m_ListCtrl_ReplayFile.DeleteAllItems();
	while(m_ListCtrl_ReplayFile.DeleteColumn(0));

	m_ListCtrl_ReplayFile.InsertColumn(0,"摄像头名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(0,150);
	m_ListCtrl_ReplayFile.InsertColumn(1,"开始时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(1,150);
	m_ListCtrl_ReplayFile.InsertColumn(2,"结束时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(2,150);
	m_ListCtrl_ReplayFile.InsertColumn(3,"URL");
	m_ListCtrl_ReplayFile.SetColumnWidth(3,0);
	m_ListCtrl_ReplayFile.InsertColumn(4,"文件大小");
	m_ListCtrl_ReplayFile.SetColumnWidth(4,150);
	m_ListCtrl_ReplayFile.InsertColumn(5,"摄像头号码");
	m_ListCtrl_ReplayFile.SetColumnWidth(5,0);
	m_ListCtrl_ReplayFile.InsertColumn(6,"站点名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(6,0);
	m_ListCtrl_ReplayFile.InsertColumn(7,"站点号码");
	m_ListCtrl_ReplayFile.SetColumnWidth(7,0);
}

// 查询D5000的条件
void CDlgReplay::OnBnClickedButtonRpD5000Select()
{
	if (m_nTableSelectIndex == 1)
		return;

	if (m_RemotePlayState != REPLAY_STOP||m_LocalPlayState != REPLAY_STOP)
		return;

	m_nTableSelectIndex = 1;

	ShowAlarmReplayControl(FALSE);

	GetDlgItem(IDC_STATIC_PT_NODESEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT_SELECTEDID)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_SELECTID)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_PT_RECTYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_RECORDTYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_D5000_STATION_SEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_COMBO_D5000_STATION_SEL)->ShowWindow(SW_SHOW);
	//	GetDlgItem(IDC_STATIC_D5000_DEV_SEL)->ShowWindow(SW_SHOW);
	//	GetDlgItem(IDC_COMBO_D5000_DEV_SEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC_D5000_EVENTTYPE_SEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_COMBO_D5000_EVNETTYPE_SEL)->ShowWindow(SW_SHOW);

	GetDlgItem(IDC_STATIC_WEATHER_TYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_WEATHER_TYPE_SEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_OP_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT_OP_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_OP_STATION_SEL)->ShowWindow(SW_HIDE);

	m_btnSelPlant.SetIndex(1);
	m_btnSelPlant.Invalidate(true);
	m_btnSelD5000.SetIndex(0);
	m_btnSelD5000.Invalidate(true);
	m_btnSelOP.SetIndex(1);
	m_btnSelOP.Invalidate(true);
	m_btnSelWeather.SetIndex(1);
	m_btnSelWeather.Invalidate(true);
	m_btnAlarmReplay.SetIndex(1);
	m_btnAlarmReplay.Invalidate(true);

	m_ComboD5000StationSel.ResetContent();
	m_ComboD5000StationSel.AddString("所有变电站");

	// 显示D5000系统变电站下拉列表
	for (int i = 0; i < g_nStation_Num; i ++)
	{
		if (strlen(g_tStation_Info[i].station_name_d5000) > 4&&strlen(g_tStation_Info[i].station_name_meteo_a) > 4)
		{
			m_ComboD5000StationSel.AddString(g_tStation_Info[i].station_name_d5000);
		}
	}
	m_ComboD5000StationSel.SetCurSel(0);

	m_ComboD5000DevSel.ResetContent();
	m_ComboD5000DevSel.AddString("所有设备");
	m_ComboD5000DevSel.SetCurSel(0);

	m_ComboD500EventType.ResetContent();
	m_ComboD500EventType.AddString("所有类型");
	m_ComboD500EventType.AddString("告警联动");
	m_ComboD500EventType.AddString("视频联动");
	m_ComboD500EventType.SetCurSel(0);

	m_ListCtrl_ReplayFile.DeleteAllItems();
	while(m_ListCtrl_ReplayFile.DeleteColumn(0));

	m_ListCtrl_ReplayFile.InsertColumn(0,"摄像头名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(0,0);
	m_ListCtrl_ReplayFile.InsertColumn(1,"摄像头号码");
	m_ListCtrl_ReplayFile.SetColumnWidth(1,0);
	m_ListCtrl_ReplayFile.InsertColumn(2,"设备名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(2,150);
	m_ListCtrl_ReplayFile.InsertColumn(3,"站点名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(3,100);
	m_ListCtrl_ReplayFile.InsertColumn(4,"联动类型");
	m_ListCtrl_ReplayFile.SetColumnWidth(4,100);
	m_ListCtrl_ReplayFile.InsertColumn(5,"联动时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(5,150);
	m_ListCtrl_ReplayFile.InsertColumn(6,"联动子类型");
	m_ListCtrl_ReplayFile.SetColumnWidth(6,150);
	m_ListCtrl_ReplayFile.InsertColumn(7,"联动状态");
	m_ListCtrl_ReplayFile.SetColumnWidth(7,150);
	m_ListCtrl_ReplayFile.InsertColumn(8,"ScreenId");
	m_ListCtrl_ReplayFile.SetColumnWidth(8,0);
	m_ListCtrl_ReplayFile.InsertColumn(9,"Value1");
	m_ListCtrl_ReplayFile.SetColumnWidth(9,0);
	m_ListCtrl_ReplayFile.InsertColumn(10,"Value2");
	m_ListCtrl_ReplayFile.SetColumnWidth(10,0);	
	m_ListCtrl_ReplayFile.InsertColumn(11,"开始时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(11,0);
	m_ListCtrl_ReplayFile.InsertColumn(12,"结束时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(12,0);
	m_ListCtrl_ReplayFile.InsertColumn(13,"URL");
	m_ListCtrl_ReplayFile.SetColumnWidth(13,0);
	m_ListCtrl_ReplayFile.InsertColumn(14,"文件大小");
	m_ListCtrl_ReplayFile.SetColumnWidth(14,0);
	m_ListCtrl_ReplayFile.InsertColumn(15,"摄像头类型");
	m_ListCtrl_ReplayFile.SetColumnWidth(15,0);

}

// 查询气象的条件
void CDlgReplay::OnBnClickedButtonRpWeatherSelect()
{
	if (m_nTableSelectIndex == 2)
		return;

	if (m_RemotePlayState != REPLAY_STOP||m_LocalPlayState != REPLAY_STOP)
		return;

	m_nTableSelectIndex = 2;

	GetDlgItem(IDC_STATIC_PT_NODESEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT_SELECTEDID)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_SELECTID)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_PT_RECTYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_RECORDTYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_D5000_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_D5000_DEV_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_DEV_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_D5000_EVENTTYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_EVNETTYPE_SEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_WEATHER_TYPE_SEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_COMBO_WEATHER_TYPE_SEL)->ShowWindow(SW_SHOW);

	GetDlgItem(IDC_STATIC_OP_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT_OP_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_OP_STATION_SEL)->ShowWindow(SW_HIDE);

	m_btnSelPlant.SetIndex(1);
	m_btnSelPlant.Invalidate(true);
	m_btnSelD5000.SetIndex(1);
	m_btnSelD5000.Invalidate(true);
	m_btnSelOP.SetIndex(1);
	m_btnSelOP.Invalidate(true);
	m_btnSelWeather.SetIndex(0);
	m_btnSelWeather.Invalidate(true);

	m_ListCtrl_ReplayFile.DeleteAllItems();
	while(m_ListCtrl_ReplayFile.DeleteColumn(0));

	m_ListCtrl_ReplayFile.InsertColumn(0,"摄像头名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(0,150);
	m_ListCtrl_ReplayFile.InsertColumn(1,"摄像头号码");
	m_ListCtrl_ReplayFile.SetColumnWidth(1,0);
	m_ListCtrl_ReplayFile.InsertColumn(2,"设备名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(2,0);
	m_ListCtrl_ReplayFile.InsertColumn(3,"站点名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(3,100);
	m_ListCtrl_ReplayFile.InsertColumn(4,"联动类型");
	m_ListCtrl_ReplayFile.SetColumnWidth(4,100);
	m_ListCtrl_ReplayFile.InsertColumn(5,"联动时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(5,150);
	m_ListCtrl_ReplayFile.InsertColumn(6,"联动子类型");
	m_ListCtrl_ReplayFile.SetColumnWidth(6,150);
	m_ListCtrl_ReplayFile.InsertColumn(7,"联动状态");
	m_ListCtrl_ReplayFile.SetColumnWidth(7,0);
	m_ListCtrl_ReplayFile.InsertColumn(8,"ScreenId");
	m_ListCtrl_ReplayFile.SetColumnWidth(8,0);
	m_ListCtrl_ReplayFile.InsertColumn(9,"Value1");
	m_ListCtrl_ReplayFile.SetColumnWidth(9,0);
	m_ListCtrl_ReplayFile.InsertColumn(10,"Value2");
	m_ListCtrl_ReplayFile.SetColumnWidth(10,0);	
	m_ListCtrl_ReplayFile.InsertColumn(11,"开始时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(11,0);
	m_ListCtrl_ReplayFile.InsertColumn(12,"结束时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(12,0);
	m_ListCtrl_ReplayFile.InsertColumn(13,"URL");
	m_ListCtrl_ReplayFile.SetColumnWidth(13,0);
	m_ListCtrl_ReplayFile.InsertColumn(14,"文件大小");
	m_ListCtrl_ReplayFile.SetColumnWidth(14,0);
	m_ListCtrl_ReplayFile.InsertColumn(15,"摄像头类型");
	m_ListCtrl_ReplayFile.SetColumnWidth(15,0);

}

// 查询操作票的条件
void CDlgReplay::OnBnClickedButtonRpOpSelect()
{
	if (m_nTableSelectIndex == 3)
		return;

	if (m_RemotePlayState != REPLAY_STOP||m_LocalPlayState != REPLAY_STOP)
		return;

	m_nTableSelectIndex = 3;

	GetDlgItem(IDC_STATIC_PT_NODESEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT_SELECTEDID)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_SELECTID)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_PT_RECTYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_RECORDTYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_D5000_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_D5000_DEV_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_DEV_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_D5000_EVENTTYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_EVNETTYPE_SEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_WEATHER_TYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_WEATHER_TYPE_SEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_OP_STATION_SEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_EDIT_OP_STATION_SEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_BUTTON_OP_STATION_SEL)->ShowWindow(SW_HIDE);

	m_btnSelPlant.SetIndex(1);
	m_btnSelPlant.Invalidate(true);
	m_btnSelD5000.SetIndex(1);
	m_btnSelD5000.Invalidate(true);
	m_btnSelOP.SetIndex(0);
	m_btnSelOP.Invalidate(true);
	m_btnSelWeather.SetIndex(1);
	m_btnSelWeather.Invalidate(true);
}

LRESULT CDlgReplay::OnSelectDeviceNotifyMessageHandler(WPARAM wParam, LPARAM lParam)
{

	if (lParam != WM_MAGIC_VEMCUCTL)
		return 0;

	if (wParam == 1)
	{
		if(g_pMainDlg == NULL||g_pMainDlg->m_pDlgSelectDevice == NULL)
			return 0;

		if(g_pMainDlg->m_pDlgSelectDevice->m_pNodeInfo == NULL)
			return 0;

		if (m_RemotePlayState != REPLAY_STOP||m_LocalPlayState != REPLAY_STOP)
		{
			//关闭当前正在播放的视频
			OnBnClickedButtonStop();

			m_ListCtrl_ReplayFile.DeleteAllItems();
			m_listReplayRelation.DeleteAllItems();
		}

		m_pNodeInfo = g_pMainDlg->m_pDlgSelectDevice->m_pNodeInfo;

		if (m_pNodeInfo->node_decodetag == 100)
		{
			m_nSpecialVideoAccessFlag = 1;
		}
		else
		{
			m_nSpecialVideoAccessFlag = 0;
		}

		m_SelectedName = m_pNodeInfo->node_name;
		strcpy(m_szSelectedNum,m_pNodeInfo->node_num);

		UpdateData(FALSE);
	}

	return 0;
}

LRESULT CDlgReplay::OnSliderLButtonDownHandler(WPARAM wParam,LPARAM lParam)
{
	if (wParam != IDC_SLIDER_PLAYPOS)
		return 0;

	int nSliderPos = lParam;

	float fPos = 0.0f;

	if (nSliderPos >= 0&&nSliderPos <= 100)
	{
		if(m_bPlayRemoteFlag  == false)//本地
		{
			if (m_LocalPlayState != REPLAY_STOP)
			{
				KillTimer(1);
				fPos = (float)nSliderPos/100;
				PlaySetLocalFilePos(fPos);
				SetTimer(1,1000,NULL);
			}
		}
		else//远程
		{
			if (m_RemotePlayState != REPLAY_STOP)
			{
				if (m_nPlayVideoAccessType == 0)
				{
					KillTimer(1);
					PlaySetRemoteVideoPos(nSliderPos);
					SetTimer(1,1000,NULL);
				}
				else
				{
					SetPosPlayBack_DevSdk(m_nDvrId,m_nCameraChannel,nSliderPos);
				}
			}
		}
	}

	return 0;
}

void CDlgReplay::OnBnClickedButtonSelectid()
{
	if (g_pMainDlg != NULL)
	{
		if (g_pMainDlg->m_pDlgSelectDevice == NULL)
		{
			g_pMainDlg->m_pDlgSelectDevice = new CDlgSelectDevice();
			if (g_pMainDlg->m_pDlgSelectDevice == NULL)
				return;

			g_pMainDlg->m_pDlgSelectDevice->Create(IDD_DIALOG_SELECT_DEVICE, g_pMainDlg);
			g_pMainDlg->m_pDlgSelectDevice->CenterWindow();
		}

		g_pMainDlg->m_pDlgSelectDevice->ShowWindow(SW_SHOW);
	}
}

void CDlgReplay::OnCbnSelchangeComboD5000StationSel()
{
	m_ComboD5000DevSel.ResetContent();
	m_ComboD5000DevSel.AddString("所有设备");

	m_ComboD5000DevSel.SetCurSel(0);
}

BOOL CDlgReplay::PlayRemoteVideo()
{
	// 关闭本地视频文件 
	if (m_LocalPlayState != REPLAY_STOP)
	{
		StopPlayLocalFile();
	}

	// 远程视频文件
	switch(m_RemotePlayState)
	{
	case REPLAY_PLAY:
		PausePlayRemoteVideo();
		break;
	case REPLAY_STOP:
		StartPlayRemoteVideo();
		break;
	case REPLAY_PAUSE:
		ResumePlayRemoteVideo();
		break;
	default:
		break;
	}

	return TRUE;
}

BOOL CDlgReplay::StartPlayRemoteVideo()
{
	m_nPlayVideoAccessType = m_nSearchVideoAccessType;

	if (m_nPlayVideoAccessType == 0)
	{
		return StartPlayRemoteVideoByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return StartPlayRemoteVideoByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::StopPlayRemoteVideo()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return StopPlayRemoteVideoByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		if (g_VideoReplayDownloadMenuFlag != FALSE)
		{
			return StopVideoReplayDownloadByDirectDevice();
		}
		else
		{
			return StopPlayRemoteVideoByDirectDevice();
		}
	}

	return FALSE;
}

BOOL CDlgReplay::PausePlayRemoteVideo()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return PausePlayRemoteVideoByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return PausePlayRemoteVideoByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::ResumePlayRemoteVideo()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return ResumePlayRemoteVideoByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return ResumePlayRemoteVideoByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::FastPlayRemoteVideo()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return FastPlayRemoteVideoByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return FastPlayRemoteVideoByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::SlowPlayRemoteVideo()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return SlowPlayRemoteVideoByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return SlowPlayRemoteVideoByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::PlaySetRemoteVideoPos(int nPos)
{
	if (m_nPlayVideoAccessType == 0)
	{
		return PlaySetRemoteVideoPosByVideoPlatform(nPos);
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return PlaySetRemoteVideoPosByDirectDevice(nPos);
	}

	return FALSE;
}

BOOL CDlgReplay::PlayRemoteVideoByVideoPlatform()
{
	try
	{
		// 关闭本地视频文件 
		if (m_LocalPlayState != REPLAY_STOP)
		{
			StopPlayLocalFile();
		}

		// 远程视频文件
		switch(m_RemotePlayState)
		{
		case REPLAY_PLAY:
			PausePlayRemoteVideoByVideoPlatform();
			break;
		case REPLAY_STOP:
			StartPlayRemoteVideoByVideoPlatform();
			break;
		case REPLAY_PAUSE:
			ResumePlayRemoteVideoByVideoPlatform();
			break;
		default:
			break;
		}

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::StartPlayRemoteVideoByVideoPlatform()
{
	try
	{
		int nRetCode = -1;
		g_RecvBytes = 0;

		if (m_pRtspClientAPI == NULL)
			return FALSE;

		m_SliderPlayPos.SetPos(0);

		if (strlen(m_RtspURL) < 10)
		{
			MessageBox("没有选择播放文件","视频监视");
			return FALSE;
		}

		//定时器
		SetTimer(1,1000,NULL);

		//初始化
		nRetCode = m_pRtspClientAPI->ProduceID(&g_nRtspID);
		m_strPlayState.Format("当前状态：正在打开......");
		UpdateData(false);

		char header[512] = {0};

		nRetCode = m_pRtspClientAPI->Play(g_nRtspID, m_RtspURL, header, 0);
		if(nRetCode != E_RTSP_OK)
		{
			m_strPlayState.Format("当前状态：打开失败，URL错误");
			UpdateData(false);
			g_nRtspID = -1;
			return FALSE;
		}

		InitViewChannel();

		sprintf_s(m_ch.m_Headerbuf,sizeof(m_ch.m_Headerbuf),"%s",header);
		m_ch.m_nHeaderLen = strlen(header);

		char newSps[512] = {0};
		int    newSpsLen = 0;

		newSpsLen = SwitchSPS(header,(BYTE *)newSps);

		//////////////////////////////////////////////////////////////////////////
		if (g_VideoReplayDownloadMenuFlag != false)
		{
			g_VideoReplayDownloadInfo.nRtspID = g_nRtspID;
			
			if (m_pNodeInfo != NULL&&m_pNodeInfo->node_decodetag == 100)
			{
				g_VideoReplayDownloadInfo.nDecodeTag = 100;
			}
			else
			{
				g_VideoReplayDownloadInfo.nDecodeTag = 150;
			}
			
			memcpy(g_VideoReplayDownloadInfo.newSps,newSps,newSpsLen);
			g_VideoReplayDownloadInfo.newSpsLen = newSpsLen;
			StartVideoReplayDownloadByVideoPlatform();
		}
		//////////////////////////////////////////////////////////////////////////

		if (m_pNodeInfo != NULL&&m_pNodeInfo->node_decodetag == 100)
		{
			m_ch.ReplayStartStreamByVideoPlatform(100,newSps,newSpsLen);
		}
		else
		{
			m_ch.ReplayStartStreamByVideoPlatform(150,newSps,newSpsLen);
		}
		
		m_strPlayState.Format("当前状态：播放中");
		m_NowSpeed = REPLAY_NORMALSPEED;
		m_strPlaySpeed.Format("播放速度：1");
		m_btnPlay.SetIndex(1);//暂停
		m_btnPlay.Invalidate(TRUE);
		m_RemotePlayState = REPLAY_PLAY;
		UpdateData(false);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::StopPlayRemoteVideoByVideoPlatform()
{
	try
	{
		if (g_nRtspID > 0||m_RemotePlayState != REPLAY_STOP)
		{
			m_NowSpeed = REPLAY_NORMALSPEED;
			m_RemotePlayState = REPLAY_STOP;
			m_strPlayState.Format("当前状态：停止");
			m_strPlaySpeed.Format("播放速度：1");

			m_btnPlay.SetIndex(1);//播放
			m_btnPlay.Invalidate(TRUE);

			g_RecvBytes = 0;
			UpdateData(false);

			if (m_pRtspClientAPI != NULL)
			{
				m_pRtspClientAPI->Stop(g_nRtspID);
			}

			m_ch.ReplayStopStreamByVideoPlatform();

			m_ch.CloseViewChannel();

			g_nRtspID = -1;

			StopRemoteVideoReplayHistoryWeatherInfo();//结束历史气象

			memset(m_RtspURL,0,sizeof(m_RtspURL));

			if (g_VideoReplayDownloadMenuFlag != false)
			{
				StopVideoReplayDownloadByVideoPlatform();
				g_VideoReplayDownloadMenuFlag = false;
			}
		}

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::PausePlayRemoteVideoByVideoPlatform()
{
	try
	{
		if (m_pRtspClientAPI != NULL)
		{
			E_RtspRes  nRetCode = m_pRtspClientAPI->Pause(g_nRtspID);
		}

		m_RemotePlayState = REPLAY_PAUSE;
		m_strPlayState.Format("当前状态：暂停");
		m_btnPlay.SetIndex(0);//暂停
		m_btnPlay.Invalidate(TRUE);
		UpdateData(false);

		m_ch.ReplayPauseByVideoPlatform();

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::ResumePlayRemoteVideoByVideoPlatform()
{
	try
	{
		if (m_pRtspClientAPI != NULL)
		{
			E_RtspRes nRetCode = m_pRtspClientAPI->Resume(g_nRtspID);
		}

		m_RemotePlayState = REPLAY_PLAY;
		m_strPlayState.Format("当前状态：播放中");
		m_btnPlay.SetIndex(1);//暂停
		m_btnPlay.Invalidate(TRUE);
		UpdateData(false);

		m_ch.ReplayResumeByVideoPlatform();

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::FastPlayRemoteVideoByVideoPlatform()
{
	try
	{
		float speed = 0.0f;

		switch(m_NowSpeed)
		{
		case REPLAY_NORMALSPEED:
			m_NowSpeed = REPLAY_2HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：2");
			speed = 2;
			break;
		case REPLAY_2HIGHSPEED:
			m_NowSpeed = REPLAY_4HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：4");
			speed = 4;
			break;
		case REPLAY_4HIGHSPEED:
			m_NowSpeed = REPLAY_8HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：8");
			speed = 8;
			break;
		case REPLAY_8HIGHSPEED:
			MessageBox("已经为最快速度","视频监视");
			return FALSE;
		case REPLAY_2LOWSPEED:
			m_NowSpeed = REPLAY_NORMALSPEED;
			m_strPlaySpeed.Format("播放速度：1");
			speed = 1;
			break;
		case REPLAY_4LOWSPEED:
			m_NowSpeed = REPLAY_2LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/2");
			speed =0.5f;
			break;
		case REPLAY_8LOWSPEED:
			m_NowSpeed = REPLAY_4LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/4");
			speed = 0.25f;
			break;
		}

		if (m_pRtspClientAPI != NULL)
		{
			E_RtspRes nRetCode = m_pRtspClientAPI->Speed(g_nRtspID,speed);
		}

		m_ch.ReplayFastByVideoPlatform();

		UpdateData(false);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::SlowPlayRemoteVideoByVideoPlatform()
{
	try
	{
		float speed = 0.0f;

		switch(m_NowSpeed)
		{
		case REPLAY_NORMALSPEED:
			m_NowSpeed = REPLAY_2LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/2");
			speed = 0.5;
			break;
		case REPLAY_2HIGHSPEED:
			m_NowSpeed = REPLAY_NORMALSPEED;
			m_strPlaySpeed.Format("播放速度：1");
			speed = 1;
			break;
		case REPLAY_4HIGHSPEED:
			m_NowSpeed = REPLAY_2HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：2");
			speed = 2;
			break;
		case REPLAY_8HIGHSPEED:
			m_NowSpeed = REPLAY_4HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：4");
			speed = 4;
			break;
		case REPLAY_2LOWSPEED:
			m_NowSpeed = REPLAY_4LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/4");
			speed = 0.25;
			break;
		case REPLAY_4LOWSPEED:
			m_NowSpeed = REPLAY_8LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/8");
			speed = 0.125;
			break;
		case REPLAY_8LOWSPEED:
			MessageBox("已经为最慢速度","视频监视");
			return FALSE;
		}

		if (m_pRtspClientAPI != NULL)
		{
			E_RtspRes nRetCode = m_pRtspClientAPI->Speed(g_nRtspID,speed);
		}

		m_ch.ReplaySlowByVideoPlatform();

		UpdateData(false);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::PlaySetRemoteVideoPosByVideoPlatform(int nPos)
{
	int nAdjustTime = 0;

	try
	{
		if (m_RemotePlayState != REPLAY_STOP)
		{
			if (m_pRtspClientAPI != NULL)
			{
				nAdjustTime = SwitchRemoteVideoTimeFromPosByVideoPlatform(nPos,m_szDvrRecordStartTime,m_szDvrRecordStopTime);
				E_RtspRes nRetCode = m_pRtspClientAPI->AdjustPos(g_nRtspID,nAdjustTime);
				if (nRetCode == E_RTSP_OK)
				{
					g_RecvBytes = (g_TotalBytes+99)/100*nPos;
					return TRUE;
				}
			}
		}
		return FALSE;
	}
	catch(...)
	{

	}
	return FALSE;
}

int CDlgReplay::SwitchRemoteVideoTimeFromPosByVideoPlatform(int nPos,char *szStartTime,char *szStopTime)
{
	int nAllTime = 0;
	int nReturnValue = 0;
	int nResult = 0;

	int nYear = 2014;
	int nMonth = 1;
	int nDay = 1;
	int nHour = 0;
	int nMinute = 0;
	int nSecond = 0;

	time_t start_time;
	time_t stop_time;

	tm temp_tm;

	try
	{
		do 
		{
			if (nPos < 0|| nPos > 100)
			{
				break;
			}

			if (szStartTime == NULL||strlen(szStartTime) == 0||szStopTime == NULL||strlen(szStopTime) == 0)
			{
				break;
			}

			nResult = sscanf(szStartTime,"%04d-%02d-%02dT%02d:%02d:%2dZ",&nYear,&nMonth,&nDay,&nHour,&nMinute,&nSecond);
			if (nResult < 6)
			{
				break;
			}

			temp_tm.tm_year = nYear-1900;
			temp_tm.tm_mon = nMonth-1;
			temp_tm.tm_mday = nDay;
			temp_tm.tm_hour = nHour;
			temp_tm.tm_min = nMinute;
			temp_tm.tm_sec = nSecond;

			start_time = mktime(&temp_tm);

			nResult = sscanf(szStopTime,"%04d-%02d-%02dT%02d:%02d:%2dZ",&nYear,&nMonth,&nDay,&nHour,&nMinute,&nSecond);
			if (nResult < 6)
			{
				break;
			}

			temp_tm.tm_year = nYear-1900;
			temp_tm.tm_mon = nMonth-1;
			temp_tm.tm_mday = nDay;
			temp_tm.tm_hour = nHour;
			temp_tm.tm_min = nMinute;
			temp_tm.tm_sec = nSecond;

			stop_time = mktime(&temp_tm);

			nAllTime = stop_time - start_time;

			if (nAllTime <= 0|| nAllTime > 24*60*60)
			{
				break;
			}

			nReturnValue = nAllTime*nPos/100;

			if (nReturnValue < 0)
			{
				nReturnValue = 0;
			}

			return nReturnValue;

		} while (FALSE);

	}
	catch(...)
	{

	}

	return 0;
}

BOOL CDlgReplay::PlayRemoteVideoByDirectDevice()
{
	try
	{
		// 关闭本地视频文件 
		if (m_LocalPlayState != REPLAY_STOP)
		{
			StopPlayLocalFile();
		}

		// 远程视频文件
		switch(m_RemotePlayState)
		{
		case REPLAY_PLAY:
			PausePlayRemoteVideoByDirectDevice();
			break;
		case REPLAY_STOP:
			StartPlayRemoteVideoByDirectDevice();
			break;
		case REPLAY_PAUSE:
			ResumePlayRemoteVideoByDirectDevice();
			break;
		default:
			break;
		}

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::StartPlayRemoteVideoByDirectDevice()
{
	try
	{
		int nRetCode = -1;

		m_SliderPlayPos.SetPos(0);

		if (strlen(m_RtspURL) < 3)
		{
			MessageBox("没有选择播放文件","视频监视");
			return FALSE;
		}

		//定时器
		SetTimer(1,1000,NULL);

		RECORDITEM record_item;
		strcpy_s(record_item.FileName,sizeof(record_item.FileName),m_RtspURL);
		strcpy_s(record_item.BeginTime,sizeof(record_item.BeginTime),m_szDvrRecordStartTime);
		strcpy_s(record_item.EndTime,sizeof(record_item.EndTime),m_szDvrRecordStopTime);
		record_item.size = m_nRecordFileSize;

		//初始化
		bool bResult = StartPlayBack_DevSdk(m_szDvrIP,m_nDvrPort,m_szUserName,m_szUserPassword,m_nDvrType,m_nDvrId,m_nCameraChannel,record_item,m_ch.m_Pic.GetSafeHwnd());
		if(bResult == false)
		{
			g_VMLog.WriteVmLog("--StartPlayRemoteVideoByDirectDevice-打开失败，URL错误-DvrIP=%s,DvrPort=%d,UserName=%s,UserPassword=%s,DvrID=%d,CameraChannel=%d",
				                                                                                                                            m_szDvrIP,m_nDvrPort,m_szUserName,m_szUserPassword,m_nDvrId,m_nCameraChannel);
			g_VMLog.WriteVmLog("--StartPlayRemoteVideoByDirectDevice-打开失败，URL错误-FileName=%s,BeginTime=%s,EndTime=%s,FileSize=%d",
				                                                                                                                            record_item.FileName,record_item.BeginTime,record_item.EndTime,record_item.size);
			m_strPlayState.Format("当前状态：打开失败，URL错误");
			UpdateData(false);
			return FALSE;
		}

		InitViewChannel();

		m_strPlayState.Format("当前状态：播放中");
		m_NowSpeed = REPLAY_NORMALSPEED;
		m_strPlaySpeed.Format("播放速度：1");
		m_btnPlay.SetIndex(1);//暂停
		m_btnPlay.Invalidate(TRUE);
		m_RemotePlayState = REPLAY_PLAY;
		UpdateData(false);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::StopPlayRemoteVideoByDirectDevice()
{
	try
	{
		if (m_RemotePlayState != REPLAY_STOP)
		{
			m_NowSpeed = REPLAY_NORMALSPEED;
			m_RemotePlayState = REPLAY_STOP;
			m_strPlayState.Format("当前状态：停止");
			m_strPlaySpeed.Format("播放速度：1");

			m_btnPlay.SetIndex(1);//播放
			m_btnPlay.Invalidate(TRUE);

			UpdateData(false);

			//停止回放
			StopPlayBack_DevSdk(m_nDvrId,m_nCameraChannel);

			StopRemoteVideoReplayHistoryWeatherInfo();//结束历史气象

			m_ch.CloseViewChannel();

			memset(m_RtspURL,0,sizeof(m_RtspURL));
		}

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::PausePlayRemoteVideoByDirectDevice()
{
	try
	{
		m_RemotePlayState = REPLAY_PAUSE;
		m_strPlayState.Format("当前状态：暂停");
		m_btnPlay.SetIndex(0);   //暂停
		m_btnPlay.Invalidate(TRUE);
		UpdateData(false);

		PausePlayBack_DevSdk(m_nDvrId,m_nCameraChannel);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::ResumePlayRemoteVideoByDirectDevice()
{
	try
	{
		m_RemotePlayState = REPLAY_PLAY;
		m_strPlayState.Format("当前状态：播放中");
		m_btnPlay.SetIndex(1);//暂停
		m_btnPlay.Invalidate(TRUE);
		UpdateData(FALSE);

		ReStartPlayBack_DevSdk(m_nDvrId,m_nCameraChannel);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::FastPlayRemoteVideoByDirectDevice()
{
	try
	{
		float speed = 0.0f;

		switch(m_NowSpeed)
		{
		case REPLAY_NORMALSPEED:
			m_NowSpeed = REPLAY_2HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：2");
			speed = 2;
			break;
		case REPLAY_2HIGHSPEED:
			m_NowSpeed = REPLAY_4HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：4");
			speed = 4;
			break;
		case REPLAY_4HIGHSPEED:
			m_NowSpeed = REPLAY_8HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：8");
			speed = 8;
			break;
		case REPLAY_8HIGHSPEED:
			MessageBox("已经为最快速度","视频监视");
			return FALSE;
		case REPLAY_2LOWSPEED:
			m_NowSpeed = REPLAY_NORMALSPEED;
			m_strPlaySpeed.Format("播放速度：1");
			speed = 1;
			break;
		case REPLAY_4LOWSPEED:
			m_NowSpeed = REPLAY_2LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/2");
			speed =0.5f;
			break;
		case REPLAY_8LOWSPEED:
			m_NowSpeed = REPLAY_4LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/4");
			speed = 0.25f;
			break;
		}

		FastPlayBack_DevSdk(m_nDvrId,m_nCameraChannel);

		UpdateData(FALSE);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::SlowPlayRemoteVideoByDirectDevice()
{
	try
	{
		float speed = 0.0f;

		switch(m_NowSpeed)
		{
		case REPLAY_NORMALSPEED:
			m_NowSpeed = REPLAY_2LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/2");
			speed = 0.5;
			break;
		case REPLAY_2HIGHSPEED:
			m_NowSpeed = REPLAY_NORMALSPEED;
			m_strPlaySpeed.Format("播放速度：1");
			speed = 1;
			break;
		case REPLAY_4HIGHSPEED:
			m_NowSpeed = REPLAY_2HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：2");
			speed = 2;
			break;
		case REPLAY_8HIGHSPEED:
			m_NowSpeed = REPLAY_4HIGHSPEED;
			m_strPlaySpeed.Format("播放速度：4");
			speed = 4;
			break;
		case REPLAY_2LOWSPEED:
			m_NowSpeed = REPLAY_4LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/4");
			speed = 0.25;
			break;
		case REPLAY_4LOWSPEED:
			m_NowSpeed = REPLAY_8LOWSPEED;
			m_strPlaySpeed.Format("播放速度：1/8");
			speed = 0.125;
			break;
		case REPLAY_8LOWSPEED:
			MessageBox("已经为最慢速度","视频监视");
			return FALSE;
		}

		SlowPlayBack_DevSdk(m_nDvrId,m_nCameraChannel);

		UpdateData(FALSE);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::PlaySetRemoteVideoPosByDirectDevice(int nPos)
{
	try
	{
		if (m_RemotePlayState != REPLAY_STOP)
		{
			SetPosPlayBack_DevSdk(m_nDvrId,m_nCameraChannel,nPos);
		}
		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::VideoReplayDownloadByVideoPlatform()
{
	try
	{
		POSITION pos = m_ListCtrl_ReplayFile.GetFirstSelectedItemPosition();

		if (pos == NULL)
			return FALSE;

		int index = m_ListCtrl_ReplayFile.GetNextSelectedItem(pos);

		if (index < 0)
			return FALSE;

		if (g_pMainDlg->m_pDlgRecFilePath == NULL||g_pMainDlg->m_pDlgRecFilePath->DoModal() == IDCANCEL)
			return FALSE;

		strcpy_s(g_ReplaySaveFilepath,sizeof(g_ReplaySaveFilepath),g_pMainDlg->m_pDlgRecFilePath->m_Filepath);

		char szRemoteVideoShowTitle[512] = {0};
		char szStationName[256] = {0};
		char szDeviceName[256] = {0};
		char szCameraName[256] = {0};
		char szCameraCode[64] = {0};
		char szLinkageTime[64] = {0};
		char szWeatherTime[64] = {0};
		BOOL nRet = FALSE;

		if (m_RemotePlayState == REPLAY_STOP)
		{
			if (m_nTableSelectIndex == 1)//联动
			{
				sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
				if (strlen(m_RtspURL) <= 4)
				{
					nRet = GetAndSetVideoFileRtspInfo(index);
					if (nRet == FALSE)
						return FALSE;
				}

				m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));
				m_ListCtrl_ReplayFile.GetItemText(index, 2,szDeviceName,sizeof(szDeviceName));
				m_ListCtrl_ReplayFile.GetItemText(index, 3,szStationName,sizeof(szStationName));
				sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{联动视频}-{%s}-{%s}-{%s}",szCameraName,szDeviceName,szStationName);

				m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

				sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
				if (strlen(m_RtspURL) <= 4)
				{
					MessageBox("未查询到关联视频","视频监视");
					return FALSE;
				}

				g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 14));
				g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

				m_ListCtrl_ReplayFile.GetItemText(index,11,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
				m_ListCtrl_ReplayFile.GetItemText(index,12,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
				strcpy_s(m_szWeatherStationName,sizeof(m_szWeatherStationName),szStationName);
				StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

				SetLinkTimeLinePos(index);

				g_VideoReplayDownloadInfo.nTotalBytes = g_TotalBytes;

				strcpy_s(g_VideoReplayDownloadInfo.szCameraName,sizeof(g_VideoReplayDownloadInfo.szCameraName),szCameraName);
				strcpy_s(g_VideoReplayDownloadInfo.szStationName,sizeof(g_VideoReplayDownloadInfo.szStationName),szStationName);
				memset(szCameraCode,0,sizeof(szCameraCode));

				if(GetVideoCameraCodeFromRtspUrlByVideoPlatform(m_RtspURL,szCameraCode) != FALSE)
				{
					strcpy_s(g_VideoReplayDownloadInfo.szCameraCode,sizeof(g_VideoReplayDownloadInfo.szCameraCode),szCameraCode);
				}

				strcpy_s(g_VideoReplayDownloadInfo.szReplayStartTime,sizeof(g_VideoReplayDownloadInfo.szReplayStartTime),m_szWeatherStartTime);
				strcpy_s(g_VideoReplayDownloadInfo.szReplayStopTime,sizeof(g_VideoReplayDownloadInfo.szReplayStopTime),m_szWeatherStopTime);

				m_ListCtrl_ReplayFile.GetItemText(index,5,szLinkageTime,sizeof(szLinkageTime));
				strcpy_s(g_VideoReplayDownloadInfo.szLinkageTime,sizeof(g_VideoReplayDownloadInfo.szLinkageTime),szLinkageTime);
			}
			else if (m_nTableSelectIndex == 4)//平台视频
			{
				m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));

				if (strcmp(szCameraName,"无数据") == 0)
					return FALSE;

				sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{平台视频}-{%s}",szCameraName);

				m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

				sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 3));
				g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 4));
				g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

				m_ListCtrl_ReplayFile.GetItemText(index,1,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
				m_ListCtrl_ReplayFile.GetItemText(index,2,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
				StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

				memset(szCameraCode,0,sizeof(szCameraCode));
				if(GetVideoCameraCodeFromRtspUrlByVideoPlatform(m_RtspURL,szCameraCode) != FALSE)
				{
					strcpy_s(g_VideoReplayDownloadInfo.szCameraCode,sizeof(g_VideoReplayDownloadInfo.szCameraCode),szCameraCode);
				}

				strcpy_s(g_VideoReplayDownloadInfo.szReplayStartTime,sizeof(g_VideoReplayDownloadInfo.szReplayStartTime),m_szWeatherStartTime);
				strcpy_s(g_VideoReplayDownloadInfo.szReplayStopTime,sizeof(g_VideoReplayDownloadInfo.szReplayStopTime),m_szWeatherStopTime);
			}
			else if (m_nTableSelectIndex == 2)//气象
			{
				sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
				if (strlen(m_RtspURL) <= 4)
				{
					nRet = GetAndSetVideoFileRtspInfo(index);
					if (nRet == FALSE)
						return FALSE;
				}

				m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));
				m_ListCtrl_ReplayFile.GetItemText(index, 2,szDeviceName,sizeof(szDeviceName));
				m_ListCtrl_ReplayFile.GetItemText(index, 3,szStationName,sizeof(szStationName));
				sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{气象联动}-{%s}-{%s}",szCameraName,szStationName);

				m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

				sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
				if (strlen(m_RtspURL) <= 4)
				{
					MessageBox("未查询到关联视频","视频监视");
					return FALSE;
				}

				g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 14));
				g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

				m_ListCtrl_ReplayFile.GetItemText(index,11,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
				m_ListCtrl_ReplayFile.GetItemText(index,12,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
				strcpy_s(m_szWeatherStationName,sizeof(m_szWeatherStationName),szStationName);
				StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

				SetLinkTimeLinePos(index);

				memset(szCameraCode,0,sizeof(szCameraCode));
				if(GetVideoCameraCodeFromRtspUrlByVideoPlatform(m_RtspURL,szCameraCode) != FALSE)
				{
					strcpy_s(g_VideoReplayDownloadInfo.szCameraCode,sizeof(g_VideoReplayDownloadInfo.szCameraCode),szCameraCode);
				}

				strcpy_s(g_VideoReplayDownloadInfo.szReplayStartTime,sizeof(g_VideoReplayDownloadInfo.szReplayStartTime),m_szWeatherStartTime);
				strcpy_s(g_VideoReplayDownloadInfo.szReplayStopTime,sizeof(g_VideoReplayDownloadInfo.szReplayStopTime),m_szWeatherStopTime);
				m_ListCtrl_ReplayFile.GetItemText(index,5,szWeatherTime,sizeof(szWeatherTime));
				strcpy_s(g_VideoReplayDownloadInfo.szWeatherTime,sizeof(g_VideoReplayDownloadInfo.szWeatherTime),szWeatherTime);
			}
			else if (m_nTableSelectIndex == 3)//操作票
			{
				return FALSE;
			}
			else
				return FALSE;

			m_bPlayRemoteFlag = TRUE;

			g_VideoReplayDownloadInfo.nDownloadType = m_nTableSelectIndex;

			g_VideoReplayDownloadMenuFlag = true;//标志视频下载

			PlayLocalORRemoteVideo();
		}

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::PlayLocalORRemoteVideo()
{
	try
	{
		// 播放本地
		if (m_bPlayRemoteFlag == FALSE)
		{
			return PlayLocalFile();
		}
		else
		{
			return PlayRemoteVideo();
		}
	}
	catch (...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::PlayLocalFile()
{
	// 关闭远程视频
	if (m_RemotePlayState != REPLAY_STOP)
	{
		StopPlayRemoteVideo();
	}

	switch(m_LocalPlayState)
	{
	case REPLAY_PLAY:
		PausePlayLocalFile();
		break;
	case REPLAY_STOP:
		StartPlayLocalFile();
		break;
	case REPLAY_PAUSE:
		ResumePlayLocalFile();
		break;
	default:
		break;
	}

	return TRUE;
}

BOOL CDlgReplay::StartPlayLocalFile()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return StartPlayLocalFileByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return StartPlayLocalFileByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::StopPlayLocalFile()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return StopPlayLocalFileByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return StopPlayLocalFileByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::PausePlayLocalFile()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return PausePlayLocalFileByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return PausePlayLocalFileByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::ResumePlayLocalFile()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return ResumePlayLocalFileByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return ResumePlayLocalFileByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::FastPlayLocalFile()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return FastPlayLocalFileByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return FastPlayLocalFileByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::SlowPlayLocalFile()
{
	if (m_nPlayVideoAccessType == 0)
	{
		return SlowPlayLocalFileByVideoPlatform();
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return SlowPlayLocalFileByDirectDevice();
	}

	return FALSE;
}

BOOL CDlgReplay::PlaySetLocalFilePos(float fPos)
{
	if (m_nPlayVideoAccessType == 0)
	{
		return PlaySetLocalFilePosByVideoPlatform(fPos);
	}
	else if (m_nPlayVideoAccessType == 1)
	{
		return PlaySetLocalFilePosByDirectDevice(fPos);
	}

	return FALSE;
}

BOOL CDlgReplay::ModifyVideoStatus()
{
	//进度条
	if (m_bPlayRemoteFlag == false)//本地
	{
		if (m_nPlayVideoAccessType == 0)
		{
			float fpos = m_ch.ReplayGetFilePosByVideoPlatform();
			m_SliderPlayPos.SetPos(fpos*100);
		}
		return TRUE;
	}
	else//远程播放
	{
		if (m_nPlayVideoAccessType == 0)
		{
			if (m_RemotePlayState == REPLAY_PLAY && g_TotalBytes != 0)
				m_SliderPlayPos.SetPos(((float)g_RecvBytes/g_TotalBytes)*100);

			if (m_RemotePlayState == REPLAY_STOP)
				m_SliderPlayPos.SetPos(0);
		}
	}

	return TRUE;
}

BOOL CDlgReplay::PlayLocalFileByVideoPlatform()
{
	// 关闭远程视频
	if (m_RemotePlayState != REPLAY_STOP)
	{
		StopPlayRemoteVideo();
	}

	switch(m_LocalPlayState)
	{
	case REPLAY_PLAY:
		PausePlayLocalFileByVideoPlatform();
		break;
	case REPLAY_STOP:
		StartPlayLocalFileByVideoPlatform();
		break;
	case REPLAY_PAUSE:
		ResumePlayLocalFileByVideoPlatform();
		break;
	default:
		break;
	}

	return TRUE;
}

BOOL CDlgReplay::StartPlayLocalFileByVideoPlatform()
{
	m_SliderPlayPos.SetPos(0);
	SetTimer(1,1000,NULL);

	BOOL bResult = m_ch.ReplayStartFileByVideoPlatform();
	if (bResult == FALSE)
	{
		MessageBox("请先选择文件","视频监视");
		return FALSE;
	}

	m_LocalPlayState = REPLAY_PLAY;
	m_btnPlay.SetIndex(1);//暂停
	m_btnPlay.Invalidate(TRUE);

	return TRUE;
}

BOOL CDlgReplay::StopPlayLocalFileByVideoPlatform()
{
	m_ch.ReplayStopFileByVideoPlatform();
	m_LocalPlayState = REPLAY_STOP;
	m_btnPlay.SetIndex(0);//播放
	m_btnPlay.Invalidate(TRUE);
	m_NowSpeed = REPLAY_NORMALSPEED;
	m_SliderPlayPos.SetPos(0);
	m_ch.CloseViewChannel();
	return TRUE;
}

BOOL CDlgReplay::PausePlayLocalFileByVideoPlatform()
{
	m_ch.ReplayPauseByVideoPlatform();
	m_LocalPlayState = REPLAY_PAUSE;
	m_btnPlay.SetIndex(0);//播放
	m_btnPlay.Invalidate(TRUE);
	return TRUE;
}

BOOL CDlgReplay::ResumePlayLocalFileByVideoPlatform()
{
	m_ch.ReplayResumeByVideoPlatform();
	m_LocalPlayState = REPLAY_PLAY;
	m_btnPlay.SetIndex(1);//暂停
	m_btnPlay.Invalidate(TRUE);
	return TRUE;
}

BOOL CDlgReplay::FastPlayLocalFileByVideoPlatform()
{
	float speed = 0.0f;

	switch(m_NowSpeed)
	{
	case REPLAY_NORMALSPEED:
		m_NowSpeed = REPLAY_2HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：2");
		speed = 2;
		break;
	case REPLAY_2HIGHSPEED:
		m_NowSpeed = REPLAY_4HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：4");
		speed = 4;
		break;
	case REPLAY_4HIGHSPEED:
		m_NowSpeed = REPLAY_8HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：8");
		speed = 8;
		break;
	case REPLAY_8HIGHSPEED:
		MessageBox("已经为最快速度","视频监视");
		return FALSE;
	case REPLAY_2LOWSPEED:
		m_NowSpeed = REPLAY_NORMALSPEED;
		m_strPlaySpeed.Format("播放速度：1");
		speed = 1;
		break;
	case REPLAY_4LOWSPEED:
		m_NowSpeed = REPLAY_2LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/2");
		speed =0.5f;
		break;
	case REPLAY_8LOWSPEED:
		m_NowSpeed = REPLAY_4LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/4");
		speed = 0.25f;
		break;
	}

	m_ch.ReplayFastByVideoPlatform();

	UpdateData(false);

	return TRUE;
}

BOOL CDlgReplay::SlowPlayLocalFileByVideoPlatform()
{
	float speed = 0.0f;

	switch(m_NowSpeed)
	{
	case REPLAY_NORMALSPEED:
		m_NowSpeed = REPLAY_2LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/2");
		speed = 0.5;
		break;
	case REPLAY_2HIGHSPEED:
		m_NowSpeed = REPLAY_NORMALSPEED;
		m_strPlaySpeed.Format("播放速度：1");
		speed = 1;
		break;
	case REPLAY_4HIGHSPEED:
		m_NowSpeed = REPLAY_2HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：2");
		speed = 2;
		break;
	case REPLAY_8HIGHSPEED:
		m_NowSpeed = REPLAY_4HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：4");
		speed = 4;
		break;
	case REPLAY_2LOWSPEED:
		m_NowSpeed = REPLAY_4LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/4");
		speed = 0.25;
		break;
	case REPLAY_4LOWSPEED:
		m_NowSpeed = REPLAY_8LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/8");
		speed = 0.125;
		break;
	case REPLAY_8LOWSPEED:
		MessageBox("已经为最慢速度","视频监视");
		return FALSE;
	}

	m_ch.ReplaySlowByVideoPlatform();

	UpdateData(FALSE);

	return TRUE;
}

BOOL CDlgReplay::PlaySetLocalFilePosByVideoPlatform(float fPos)
{
	if (m_LocalPlayState != REPLAY_STOP)
	{
		return	m_ch.ReplaySetFilePosByVideoPlatform(fPos);
	}

	return FALSE;
}

BOOL CDlgReplay::ModifyVideoStatusByVideoPlatform()
{
	//进度条
	if (m_bPlayRemoteFlag == false)
	{
		float fpos = m_ch.ReplayGetFilePosByVideoPlatform();
		m_SliderPlayPos.SetPos(fpos*100);
		return TRUE;
	}
	else
	{
		if (m_nPlayVideoAccessType == 0)
		{
			if (m_RemotePlayState == REPLAY_PLAY && g_TotalBytes != 0)
				m_SliderPlayPos.SetPos(((float)g_RecvBytes/g_TotalBytes)*100);

			if (m_RemotePlayState == REPLAY_STOP)
				m_SliderPlayPos.SetPos(0);
		}
	}

	return TRUE;
}

BOOL CDlgReplay::PlayLocalFileByDirectDevice()
{
	// 关闭远程视频
	if (m_RemotePlayState != REPLAY_STOP)
	{
		StopPlayRemoteVideo();
	}

	switch(m_LocalPlayState)
	{
	case REPLAY_PLAY:
		PausePlayLocalFileByDirectDevice();
		break;
	case REPLAY_STOP:
		StartPlayLocalFileByDirectDevice();
		break;
	case REPLAY_PAUSE:
		ResumePlayLocalFileByDirectDevice();
		break;
	default:
		break;
	}

	return TRUE;
}

BOOL CDlgReplay::StartPlayLocalFileByDirectDevice()
{
	m_SliderPlayPos.SetPos(0);
	SetTimer(1,1000,NULL);

	BOOL bResult = m_ch.ReplayStartFileByDirectDevice();
	if (bResult == FALSE)
	{
		MessageBox("请先选择文件","视频监视");
		return FALSE;
	}

	m_LocalPlayState = REPLAY_PLAY;
	m_btnPlay.SetIndex(1);//暂停
	m_btnPlay.Invalidate(TRUE);

	return TRUE;
}

BOOL CDlgReplay::StopPlayLocalFileByDirectDevice()
{
	m_ch.ReplayStopFileByDirectDevice();
	m_LocalPlayState = REPLAY_STOP;
	m_btnPlay.SetIndex(0);//播放
	m_btnPlay.Invalidate(TRUE);
	m_NowSpeed = REPLAY_NORMALSPEED;
	m_SliderPlayPos.SetPos(0);
	StopRemoteVideoReplayHistoryWeatherInfo();
	m_ch.CloseViewChannel();
	return TRUE;
}

BOOL CDlgReplay::PausePlayLocalFileByDirectDevice()
{
	m_ch.ReplayPauseByDirectDevice();
	m_LocalPlayState = REPLAY_PAUSE;
	m_btnPlay.SetIndex(0);//播放
	m_btnPlay.Invalidate(TRUE);
	return TRUE;
}

BOOL CDlgReplay::ResumePlayLocalFileByDirectDevice()
{
	m_ch.ReplayResumeByDirectDevice();
	m_LocalPlayState = REPLAY_PLAY;
	m_btnPlay.SetIndex(1);//暂停
	m_btnPlay.Invalidate(TRUE);
	return TRUE;
}

BOOL CDlgReplay::FastPlayLocalFileByDirectDevice()
{
	float speed = 0.0f;

	switch(m_NowSpeed)
	{
	case REPLAY_NORMALSPEED:
		m_NowSpeed = REPLAY_2HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：2");
		speed = 2;
		break;
	case REPLAY_2HIGHSPEED:
		m_NowSpeed = REPLAY_4HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：4");
		speed = 4;
		break;
	case REPLAY_4HIGHSPEED:
		m_NowSpeed = REPLAY_8HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：8");
		speed = 8;
		break;
	case REPLAY_8HIGHSPEED:
		MessageBox("已经为最快速度","视频监视");
		return FALSE;
	case REPLAY_2LOWSPEED:
		m_NowSpeed = REPLAY_NORMALSPEED;
		m_strPlaySpeed.Format("播放速度：1");
		speed = 1;
		break;
	case REPLAY_4LOWSPEED:
		m_NowSpeed = REPLAY_2LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/2");
		speed =0.5f;
		break;
	case REPLAY_8LOWSPEED:
		m_NowSpeed = REPLAY_4LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/4");
		speed = 0.25f;
		break;
	}

	m_ch.ReplayFastByDirectDevice();

	UpdateData(false);

	return TRUE;
}

BOOL CDlgReplay::SlowPlayLocalFileByDirectDevice()
{
	float speed = 0.0f;

	switch(m_NowSpeed)
	{
	case REPLAY_NORMALSPEED:
		m_NowSpeed = REPLAY_2LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/2");
		speed = 0.5;
		break;
	case REPLAY_2HIGHSPEED:
		m_NowSpeed = REPLAY_NORMALSPEED;
		m_strPlaySpeed.Format("播放速度：1");
		speed = 1;
		break;
	case REPLAY_4HIGHSPEED:
		m_NowSpeed = REPLAY_2HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：2");
		speed = 2;
		break;
	case REPLAY_8HIGHSPEED:
		m_NowSpeed = REPLAY_4HIGHSPEED;
		m_strPlaySpeed.Format("播放速度：4");
		speed = 4;
		break;
	case REPLAY_2LOWSPEED:
		m_NowSpeed = REPLAY_4LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/4");
		speed = 0.25;
		break;
	case REPLAY_4LOWSPEED:
		m_NowSpeed = REPLAY_8LOWSPEED;
		m_strPlaySpeed.Format("播放速度：1/8");
		speed = 0.125;
		break;
	case REPLAY_8LOWSPEED:
		MessageBox("已经为最慢速度","视频监视");
		return FALSE;
	}

	m_ch.ReplaySlowByDirectDevice();

	UpdateData(FALSE);

	return TRUE;
}

BOOL CDlgReplay::PlaySetLocalFilePosByDirectDevice(float fPos)
{
	if (m_LocalPlayState != REPLAY_STOP)
	{
		return	m_ch.ReplaySetFilePosByDirectDevice(100*fPos);
	}

	return FALSE;
}

BOOL CDlgReplay::ModifyVideoStatusByDirectDevice()
{
	//进度条
	if (m_bPlayRemoteFlag == false)
	{
		return TRUE;
	}
	else
	{
		if (m_nPlayVideoAccessType == 0)
		{
			if (m_RemotePlayState == REPLAY_PLAY && g_TotalBytes != 0)
				m_SliderPlayPos.SetPos(((float)g_RecvBytes/g_TotalBytes)*100);

			if (m_RemotePlayState == REPLAY_STOP)
				m_SliderPlayPos.SetPos(0);
		}
	}

	return TRUE;
}

void CDlgReplay::OnBnClickedButtonLocalReplay()
{
	OnBnClickedButtonLocalfile();
}

BOOL CDlgReplay::SearchVideoPlatformHistoryVideo()
{
	try
	{
		int nIndex = m_ComboVideoAccessType.GetCurSel();

		if (m_nSpecialVideoAccessFlag == 1)
		{
			nIndex = 0;
		}
		else if (m_nSpecialVideoAccessFlag == 2)
		{
			nIndex = 1;
		}

		if (nIndex == 0)
		{
			return SearchVideoPlatformHistoryVideoByVideoPlatform();
		}
		else if (nIndex == 1)
		{
			return SearchVideoPlatformHistoryVideoByDirectDevice();
		}

		return FALSE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::SearchVideoPlatformHistoryVideoByVideoPlatform()
{
	try
	{
		UpdateData(TRUE);

		//通过南瑞平台查询视频回放信息
		m_nSearchVideoAccessType = 0;

		if (m_pNodeInfo == NULL)
			return FALSE;

		int i = 0, j = 0;

		CTime starttime(m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
			m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

		CTime stoptime(m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
			m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

		if (stoptime <= starttime)
		{
			MessageBox("时间选择错误：开始时间大于结束时间","视频监视");
			return FALSE;
		}

		sprintf(m_Replay_Query_Info.start_time, "%04d-%02d-%02dT%02d:%02d:%02dZ",
			m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
			m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

		sprintf(m_Replay_Query_Info.stop_time, "%04d-%02d-%02dT%02d:%02d:%02dZ",
			m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
			m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

		m_ListCtrl_ReplayFile.DeleteAllItems();

		if (m_pNodeInfo != NULL)
		{
			strcpy_s(m_szWeatherStationName,sizeof(m_szWeatherStationName),m_pNodeInfo->node_station);
		}

		// 采用http请求资源
		PREQ_VIDEO	req_video = NULL;
		PRES_VIDEO	res_video = NULL;
		void	*inArg = NULL;
		void *outArg = NULL;

		int nRet = 0;
		int nRealNum = 0;//实际查询到的记录
		int nSubNum = 0;//这次返回的记录
		int nReceiveNum = 0;//累计得到的记录
		int nAllSearchResultNum = 0;//记录查询到的总数
		int nCount = 0;
		int nType = 0;

		if (m_ComboRecordFileType.GetCurSel() == 0)
		{
			nType = 0xFFFFFFFF;	 // 请求的类型值
		}
		else if (m_ComboRecordFileType.GetCurSel() == 1)
		{
			nType = 0x1;				// 请求的类型值
		}
		else
		{
			nType = 1048576;     // 请求的类型值
		}

		if (m_SelectedPlant == 0)
		{
			// 在本平台请求
			for (i = 0; i < g_TotalSysNum; i ++)
			{
				if (g_SystemInfo[i].localdomain == 1)
				{
					break;
				}
			}

			if (i == g_TotalSysNum)
			{
				MessageBox("系统不在线!","视频监视");
				return FALSE;
			}
		}
		else
		{
			// 构造所在系统号
			char	c_systemid[20]="000000000000000000";			//根据实际请求的节点号构造系统号
			memcpy(c_systemid, m_pNodeInfo->node_num, 5);

			// 判断所请求的系统是否在线，如果不在线则提示并退出
			for (i = 0; i < g_TotalSysNum; i ++)
			{
				if (strcmp(c_systemid, g_SystemInfo[i].sysid)== 0)
				{
					break;
				}
			}

			if (i == g_TotalSysNum)
			{
				MessageBox("系统不在线!","视频监视");
				return FALSE;
			}
		}

		m_ListCtrl_ReplayFile.DeleteAllItems();

		nRealNum = 100;
		nSubNum = 0;
		nReceiveNum = 0;

		while(1)
		{
			if (nReceiveNum >= nRealNum||nReceiveNum > 1000)
				return TRUE;

			nRet = initStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
			if (nRet < 0)
			{
				nCount = m_ListCtrl_ReplayFile.GetItemCount();
				if (nCount == 0)
				{
					MessageBox("构造请求失败","视频监视");
				}
				return FALSE;
			}

			req_video = (PREQ_VIDEO)inArg;
			strcpy(req_video->code, m_pNodeInfo->node_num);
			req_video->type = nType;
			strcpy(req_video->UserCode, app_RegCh.reg_user);
			req_video->FromIndex = nReceiveNum+1;
			req_video->ToIndex = nRealNum;
			strcpy(req_video->BeginTime, m_Replay_Query_Info.start_time);
			strcpy(req_video->EndTime, m_Replay_Query_Info.stop_time);
			req_video->PlantOrPU = 0;

			nRet = HttpClient(g_SystemInfo[i].http_ip,
				g_SystemInfo[i].http_port,
				HTTP_REQUEST_HISTORY_VIDEO,
				inArg,
				&outArg,
				10);

			if (nRet != 0)
			{
				nCount = m_ListCtrl_ReplayFile.GetItemCount();
				if (nCount == 0)
				{
					MessageBox("请求返回失败!","视频监视");
				}

				freeStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
				freeStructArg(&outArg, HTTP_RESPONSE_HISTORY_VIDEO);
				return FALSE;
			}

			res_video = (PRES_VIDEO)outArg;

			if (res_video->SubNum <= 0)
			{
				nCount = m_ListCtrl_ReplayFile.GetItemCount();
				if (nCount == 0)
				{
					MessageBox("查询无记录","视频监视");
				}

				freeStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
				freeStructArg(&outArg, HTTP_RESPONSE_HISTORY_VIDEO);
				return TRUE;
			}

			if (nAllSearchResultNum == 0)
			{
				nAllSearchResultNum = res_video->RealNum;
			}

			nRealNum = res_video->RealNum;
			nSubNum = res_video->SubNum;
			nReceiveNum += nSubNum;

			// 将查询结果存放到列表中
			PRES_VIDEO_ITEM res_ptr = NULL;
			for (res_ptr = res_video->item; res_ptr; res_ptr = res_ptr->next)
			{
				if (j >= 1000)
					break;

				m_ListCtrl_ReplayFile.InsertItem(j, m_pNodeInfo->node_name);
				m_ListCtrl_ReplayFile.SetItemText(j, 1, res_ptr->BeginTime);
				m_ListCtrl_ReplayFile.SetItemText(j, 2, res_ptr->EndTime);
				m_ListCtrl_ReplayFile.SetItemText(j, 3, res_ptr->FileUrl);

				char filesize[20];
				sprintf(filesize, "%d", res_ptr->size);
				m_ListCtrl_ReplayFile.SetItemText(j, 4, filesize);

				if (res_ptr->DecoderTag < 10)
				{
					m_ListCtrl_ReplayFile.SetItemData(j, m_pNodeInfo->node_decodetag);
				}
				else
				{
					m_ListCtrl_ReplayFile.SetItemData(j, res_ptr->DecoderTag);
				}
			}

			freeStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
			freeStructArg(&outArg, HTTP_RESPONSE_HISTORY_VIDEO);
		}

		nCount = m_ListCtrl_ReplayFile.GetItemCount();

		if (nCount == 0)
		{
			m_ListCtrl_ReplayFile.InsertItem(nCount,"无数据");
			m_ListCtrl_ReplayFile.SetItemText(nCount,1,"无数据");
			m_ListCtrl_ReplayFile.SetItemText(nCount,2,"无数据");
			m_ListCtrl_ReplayFile.SetItemText(nCount,3,"无数据");
			m_ListCtrl_ReplayFile.SetItemText(nCount,4,"无数据");
		}

		if (nAllSearchResultNum >= 1000)
		{
			MessageBox("当前最多显示1000条记录,用户可以缩小时间范围进行查询","视频监视");
		}

		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

BOOL CDlgReplay::SearchVideoPlatformHistoryVideoByDirectDevice()
{
	try
	{
		UpdateData(TRUE);

		//通过直连前端设备查询视频回放信息
		m_nSearchVideoAccessType = 1;

		if (m_pNodeInfo == NULL)
			return FALSE;

		int i = 0;
		int j = 0;

		CTime starttime(m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
			m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

		CTime stoptime(m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
			m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

		if (stoptime <= starttime)
		{
			MessageBox("时间选择错误：开始时间大于结束时间","视频监视");
			return FALSE;
		}

		m_ListCtrl_ReplayFile.DeleteAllItems();

		strcpy_s(m_szWeatherStationName,sizeof(m_szWeatherStationName),m_pNodeInfo->node_station);

		SDKTIME start_time;
		SDKTIME stop_time;

		start_time.year = m_StartDay.GetYear();
		start_time.month = m_StartDay.GetMonth();
		start_time.day = m_StartDay.GetDay();
		start_time.hour = m_StartTime.GetHour();
		start_time.minute = m_StartTime.GetMinute();
		start_time.second = m_StartTime.GetSecond();

		stop_time.year = m_StopDay.GetYear();
		stop_time.month = m_StopDay.GetMonth();
		stop_time.day = m_StopDay.GetDay();
		stop_time.hour = m_StopTime.GetHour();
		stop_time.minute = m_StopTime.GetMinute();
		stop_time.second = m_StopTime.GetSecond();

		void * record_info = NULL;
		int record_num = 0;

		char szFileSize[32] = {0};

		_T_CAMERA_INFO * pCameraInfo = NULL;
		_T_DVR_INFO * pDvrInfo = NULL;
		RECORDITEM *RecordItem = NULL;

		pCameraInfo = &m_pNodeInfo->camera_info;
		pDvrInfo = &pCameraInfo->dvr_info;

		strcpy_s(m_szDvrIP,sizeof(m_szDvrIP),pDvrInfo->dvr_ip);
		m_nDvrPort = pDvrInfo->dvr_port;
		m_nCameraChannel = pCameraInfo->channel;
		m_nDvrId = pDvrInfo->dvr_id;
		m_nDvrType = pDvrInfo->dvr_type;
		strcpy_s(m_szUserName,sizeof(m_szUserName),pDvrInfo->dvr_user);
		strcpy_s(m_szUserPassword,sizeof(m_szUserPassword),pDvrInfo->dvr_password);

		bool bResult = GetRecordFileInfo_DevSdk(m_szDvrIP,m_nDvrPort,m_szUserName,m_szUserPassword,m_nDvrType,m_nDvrId,m_nCameraChannel,start_time,stop_time,&record_info,&record_num);
		if (bResult&&record_info != NULL&&record_num > 0)
		{
			j = 0;

			RecordItem = (RECORDITEM *)record_info;

			for (i = 0;i < record_num;i++)
			{
				if (j >= 1000)
					break;

				m_ListCtrl_ReplayFile.InsertItem(j, m_pNodeInfo->node_name);
				m_ListCtrl_ReplayFile.SetItemText(j, 1, RecordItem[i].BeginTime);
				m_ListCtrl_ReplayFile.SetItemText(j, 2, RecordItem[i].EndTime);
				m_ListCtrl_ReplayFile.SetItemText(j, 3, RecordItem[i].FileName);

				sprintf(szFileSize, "%d", RecordItem[i].size);
				m_ListCtrl_ReplayFile.SetItemText(j, 4, szFileSize);
				m_ListCtrl_ReplayFile.SetItemText(j, 5, pCameraInfo->camera_num);
				m_ListCtrl_ReplayFile.SetItemText(j, 6, m_pNodeInfo->node_station);
				m_ListCtrl_ReplayFile.SetItemText(j, 7, m_pNodeInfo->node_stationnum);

				m_ListCtrl_ReplayFile.SetItemData(j,m_nDvrType);
			}
		}

		if (record_info != NULL)
		{
			FreeRecordFileInfo_DevSdk(m_nDvrType,record_info);
			record_info = NULL;
		}
		record_num = 0;

		int nCount = m_ListCtrl_ReplayFile.GetItemCount();

		if (nCount == 0)
		{
			m_ListCtrl_ReplayFile.InsertItem(nCount,"无数据");
			m_ListCtrl_ReplayFile.SetItemText(nCount,1,"无数据");
			m_ListCtrl_ReplayFile.SetItemText(nCount,2,"无数据");
			m_ListCtrl_ReplayFile.SetItemText(nCount,3,"无数据");
			m_ListCtrl_ReplayFile.SetItemText(nCount,4,"无数据");
		}

		if (j >= 1000)
		{
			MessageBox("当前最多显示1000条记录,用户可以缩小时间范围进行查询","视频监视");
		}

		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

BOOL CDlgReplay::SearchWeatherLinkageHistoryVideo()
{
	try
	{
		int nIndex = m_ComboVideoAccessType.GetCurSel();

		if (m_nSpecialVideoAccessFlag == 1)
		{
			nIndex = 0;
		}
		else if (m_nSpecialVideoAccessFlag == 2)
		{
			nIndex = 1;
		}

		if (nIndex == 0)
		{
			return SearchWeatherLinkageHistoryVideoByVideoPlatform();
		}
		else if (nIndex == 1)
		{
			return SearchWeatherLinkageHistoryVideoByDirectDevice();
		}

		return FALSE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::SearchWeatherLinkageHistoryVideoByVideoPlatform()
{
	UpdateData(TRUE);

	//通过南瑞平台查询视频回放信息
	m_nSearchVideoAccessType = 0;

	char szStartLinkageTime[128] = {0};
	char szStopLinkageTime[128] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szTypeName[64] = {0};

	int i = 0;
	int j = 0;
	int nIndex = 0;

	CTime starttime(m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	CTime stoptime(m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	if (stoptime <= starttime)
	{
		MessageBox("时间选择错误：开始时间大于结束时间","视频监视");
		return FALSE;
	}

	sprintf(szStartLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	sprintf(szStopLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	m_ListCtrl_ReplayFile.DeleteAllItems();

	nIndex = m_ComboWeatherType.GetCurSel();
	m_ComboWeatherType.GetLBText(nIndex,szTypeName);

	int nRet = ReadAndSetWeatherListInfo(szTypeName,szStartLinkageTime,szStopLinkageTime);

	int nCount = m_listReplayRelation.GetItemCount();

	char szNodeId[32] = {0};
	char szRealLinkageTime[128] = {0};
	int    nLen = 0;
	int    nNodeId = 0;

	for (i = 0;i < nCount;i++)
	{
		nLen = m_listReplayRelation.GetItemText(i,12,szNodeId,sizeof(szNodeId));
		if (nLen > 0)
		{
			nNodeId = atoi(szNodeId);
			nLen = m_listReplayRelation.GetItemText(i,3,szRealLinkageTime,sizeof(szRealLinkageTime));
			if (nLen > 0)
				ReadAndSetWeatherListCameraInfo(i,nNodeId,szRealLinkageTime);
		}
	}

	nCount = m_ListCtrl_ReplayFile.GetItemCount();

	if (nCount == 0)
	{
		m_ListCtrl_ReplayFile.InsertItem(nCount,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,1,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,2,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,3,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,4,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,5,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,6,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,7,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,8,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,9,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,10,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,11,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,12,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,13,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,14,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,15,"");
		m_ListCtrl_ReplayFile.SetItemData(nCount,150);
	}

	return TRUE;
}

BOOL CDlgReplay::SearchWeatherLinkageHistoryVideoByDirectDevice()
{
	UpdateData(TRUE);

	//通过直连前端设备查询视频回放信息
	m_nSearchVideoAccessType = 1;

	char szStartLinkageTime[128] = {0};
	char szStopLinkageTime[128] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szTypeName[64] = {0};

	int i = 0;
	int j = 0;
	int nIndex = 0;

	CTime starttime(m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	CTime stoptime(m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	if (stoptime <= starttime)
	{
		MessageBox("时间选择错误：开始时间大于结束时间","视频监视");
		return FALSE;
	}

	sprintf(szStartLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	sprintf(szStopLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	m_ListCtrl_ReplayFile.DeleteAllItems();

	nIndex = m_ComboWeatherType.GetCurSel();
	m_ComboWeatherType.GetLBText(nIndex,szTypeName);

	int nRet = ReadAndSetWeatherListInfo(szTypeName,szStartLinkageTime,szStopLinkageTime);

	int nCount = m_listReplayRelation.GetItemCount();

	char szNodeId[32] = {0};
	char szRealLinkageTime[128] = {0};
	int    nLen = 0;
	int    nNodeId = 0;

	for (i = 0;i < nCount;i++)
	{
		nLen = m_listReplayRelation.GetItemText(i,12,szNodeId,sizeof(szNodeId));
		if (nLen > 0)
		{
			nNodeId = atoi(szNodeId);
			nLen = m_listReplayRelation.GetItemText(i,3,szRealLinkageTime,sizeof(szRealLinkageTime));
			if (nLen > 0)
				ReadAndSetWeatherListCameraInfo(i,nNodeId,szRealLinkageTime);
		}
	}

	nCount = m_ListCtrl_ReplayFile.GetItemCount();

	if (nCount == 0)
	{
		m_ListCtrl_ReplayFile.InsertItem(nCount,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,1,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,2,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,3,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,4,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,5,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,6,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,7,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,8,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,9,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,10,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,11,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,12,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,13,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,14,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,15,"");
		m_ListCtrl_ReplayFile.SetItemData(nCount,150);
	}

	return TRUE;
}

BOOL CDlgReplay::SearchD5000LinkageHistoryVideo()
{
	try
	{
		int nIndex = m_ComboVideoAccessType.GetCurSel();

		if (m_nSpecialVideoAccessFlag == 1)
		{
			nIndex = 0;
		}
		else if (m_nSpecialVideoAccessFlag == 2)
		{
			nIndex = 1;
		}

		if (nIndex == 0)
		{
			return SearchD5000LinkageHistoryVideoByVideoPlatform();
		}
		else if (nIndex == 1)
		{
			return SearchD5000LinkageHistoryVideoByDirectDevice();
		}

		return FALSE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::SearchD5000LinkageHistoryVideoByVideoPlatform()
{
	UpdateData(TRUE);

	//通过南瑞平台查询视频回放信息
	m_nSearchVideoAccessType = 0;

	char szStartLinkageTime[128] = {0};
	char szStopLinkageTime[128] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szTypeName[64] = {0};

	int i = 0;
	int j = 0;
	int nIndex = 0;

	CTime starttime(m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	CTime stoptime(m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	if (stoptime <= starttime)
	{
		MessageBox("时间选择错误：开始时间大于结束时间","视频监视");
		return FALSE;
	}

	sprintf(szStartLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	sprintf(szStopLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	m_ListCtrl_ReplayFile.DeleteAllItems();

	nIndex = m_ComboD5000StationSel.GetCurSel();
	m_ComboD5000StationSel.GetLBText(nIndex,szStationName);

	nIndex = m_ComboD5000DevSel.GetCurSel();
	m_ComboD5000DevSel.GetLBText(nIndex,szDeviceName);

	nIndex = m_ComboD500EventType.GetCurSel();
	m_ComboD500EventType.GetLBText(nIndex,szTypeName);

	int nRet = ReadAndSetListInfo(szStationName,szDeviceName,szTypeName,szStartLinkageTime,szStopLinkageTime);

	int nCount = m_listReplayRelation.GetItemCount();

	char szNodeId[32] = {0};
	char szRealLinkageTime[128] = {0};
	int    nLen = 0;
	int    nNodeId = 0;

	for (i = 0;i < nCount;i++)
	{
		nLen = m_listReplayRelation.GetItemText(i,12,szNodeId,sizeof(szNodeId));
		if (nLen > 0)
		{
			nNodeId = atoi(szNodeId);
			nLen = m_listReplayRelation.GetItemText(i,3,szRealLinkageTime,sizeof(szRealLinkageTime));
			if (nLen > 0)
				ReadAndSetListCameraInfo(i,nNodeId,szRealLinkageTime);
		}
	}

	nCount = m_ListCtrl_ReplayFile.GetItemCount();

	if (nCount == 0)
	{
		m_ListCtrl_ReplayFile.InsertItem(nCount,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,1,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,2,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,3,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,4,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,5,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,6,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,7,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,8,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,9,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,10,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,11,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,12,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,13,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,14,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,15,"无数据");
		m_ListCtrl_ReplayFile.SetItemData(nCount,150);
	}

	return TRUE;
}

BOOL CDlgReplay::SearchD5000LinkageHistoryVideoByDirectDevice()
{
	UpdateData(TRUE);

	//通过直连前端设备查询视频回放信息
	m_nSearchVideoAccessType = 1;

	char szStartLinkageTime[128] = {0};
	char szStopLinkageTime[128] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szTypeName[64] = {0};

	int i = 0;
	int j = 0;
	int nIndex = 0;

	CTime starttime(m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	CTime stoptime(m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	if (stoptime <= starttime)
	{
		MessageBox("时间选择错误：开始时间大于结束时间","视频监视");
		return FALSE;
	}

	sprintf(szStartLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StartDay.GetYear(), m_StartDay.GetMonth(), m_StartDay.GetDay(),
		m_StartTime.GetHour(), m_StartTime.GetMinute(), m_StartTime.GetSecond());

	sprintf(szStopLinkageTime, "%04d-%02d-%02d %02d:%02d:%02d",
		m_StopDay.GetYear(), m_StopDay.GetMonth(), m_StopDay.GetDay(),
		m_StopTime.GetHour(), m_StopTime.GetMinute(), m_StopTime.GetSecond());

	m_ListCtrl_ReplayFile.DeleteAllItems();

	nIndex = m_ComboD5000StationSel.GetCurSel();
	m_ComboD5000StationSel.GetLBText(nIndex,szStationName);

	nIndex = m_ComboD5000DevSel.GetCurSel();
	m_ComboD5000DevSel.GetLBText(nIndex,szDeviceName);

	nIndex = m_ComboD500EventType.GetCurSel();
	m_ComboD500EventType.GetLBText(nIndex,szTypeName);

	int nRet = ReadAndSetListInfo(szStationName,szDeviceName,szTypeName,szStartLinkageTime,szStopLinkageTime);

	int nCount = m_listReplayRelation.GetItemCount();

	char szNodeId[32] = {0};
	char szRealLinkageTime[128] = {0};
	int    nLen = 0;
	int    nNodeId = 0;

	for (i = 0;i < nCount;i++)
	{
		nLen = m_listReplayRelation.GetItemText(i,12,szNodeId,sizeof(szNodeId));
		if (nLen > 0)
		{
			nNodeId = atoi(szNodeId);
			nLen = m_listReplayRelation.GetItemText(i,3,szRealLinkageTime,sizeof(szRealLinkageTime));
			if (nLen > 0)
				ReadAndSetListCameraInfo(i,nNodeId,szRealLinkageTime);
		}
	}

	nCount = m_ListCtrl_ReplayFile.GetItemCount();

	if (nCount == 0)
	{
		m_ListCtrl_ReplayFile.InsertItem(nCount,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,1,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,2,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,3,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,4,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,5,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,6,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,7,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,8,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,9,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,10,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,11,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,12,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,13,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,14,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,15,"无数据");
		m_ListCtrl_ReplayFile.SetItemData(nCount,150);
	}

	return TRUE;
}

BOOL CDlgReplay::ReadAndSetListInfo(char *szStation,char *szDevice,char *szType,char *szStartLinkageTime,char *szStopLinkageTime)
{
	if (szStation == NULL||szDevice == NULL||szType == NULL||
		szStartLinkageTime == NULL||szStopLinkageTime == NULL)
		return FALSE;

	if (strlen(szStation) == 0||strlen(szDevice) == 0||strlen(szType) == 0)
		return FALSE;

	if (strlen(szStartLinkageTime) == 0||strlen(szStopLinkageTime) == 0)
		return FALSE;

	m_listReplayRelation.DeleteAllItems();

	char sql_buf[512] = {0};
	bool result=false;

	MYSQL_RES * res = NULL ;
	MYSQL_ROW	row ;

	char szYear[8] = {0};
	int nYear = 2013;

	int nLinkageType = 0;
	char szLinkageType[64] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szSubType[64] = {0};
	char szState[64] = {0};
	char szScreenId[64] = {0};
	char szLinkageRealTime[64] = {0};
	char szContent[256] = {0};
	char szValue1[64] = {0};
	char szValue2[64] = {0};
	char szLinkageStationName[256] = {0};
	char szLinkageNum[16] = {0};
	char szNodeId[16] = {0};

	szYear[0] = szStartLinkageTime[0];
	szYear[1] = szStartLinkageTime[1];
	szYear[2] = szStartLinkageTime[2];
	szYear[3] = szStartLinkageTime[3];
	szYear[4] = 0;

	nYear = atoi(szYear);

	//读联动信息
	sprintf_s(sql_buf,sizeof(sql_buf),"SELECT event, station, device, type, state, screen_id, time,"
		"content, yx_value,yc_value,station,link_num,id FROM history_relation_%04d "
		"where time >= '%s' AND time <='%s' ",nYear,szStartLinkageTime,szStopLinkageTime);

	if (strcmp(szStation,"所有变电站") != 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND station='%s' ",szStation);
	}

	if (strcmp(szDevice,"所有设备") != 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND device='%s' ",szDevice);
	}

	if (strcmp(szType,"所有类型") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND (event='视频联动' OR event='告警联动') ");
	}
	else
	{
		sprintf(sql_buf+strlen(sql_buf),"AND event='%s' ",szType);
	}

	sprintf(sql_buf+strlen(sql_buf),"ORDER BY id");

	int nItemNum = 0;

	if (!mysql_query(g_mySqlData, sql_buf))
	{
		res = mysql_store_result(g_mySqlData);

		while ( row = mysql_fetch_row( res ) )
		{
			if (nItemNum > 300)
				break;

			sprintf_s(szLinkageType,"%s",row[0]);
			sprintf_s(szStationName, "%s", row[1]);
			sprintf_s(szDeviceName, "%s", row[2]);
			sprintf_s(szSubType,"%s",row[3]);
			sprintf_s(szState,"%s",row[4]);
			sprintf_s(szScreenId,"%s",row[5]);
			sprintf_s(szLinkageRealTime,"%s",row[6]);
			sprintf_s(szContent,"%s",row[7]);
			sprintf_s(szValue1, "%s", row[8]);
			sprintf_s(szValue2,"%s",row[9]);
			sprintf_s(szLinkageStationName,"%s",row[10]);
			sprintf_s(szLinkageNum,"%s",row[11]);
			sprintf_s(szNodeId,"%s",row[12]);

			m_listReplayRelation.InsertItem(nItemNum,szLinkageType);
			m_listReplayRelation.SetItemText(nItemNum,1,szDeviceName);
			m_listReplayRelation.SetItemText(nItemNum,2,szStationName);
			m_listReplayRelation.SetItemText(nItemNum,3,szLinkageRealTime);
			m_listReplayRelation.SetItemText(nItemNum,4,szSubType);
			m_listReplayRelation.SetItemText(nItemNum,5,szState);
			m_listReplayRelation.SetItemText(nItemNum,6,szScreenId);
			m_listReplayRelation.SetItemText(nItemNum,7,szContent);
			m_listReplayRelation.SetItemText(nItemNum,8,szValue1);
			m_listReplayRelation.SetItemText(nItemNum,9,szValue2);
			m_listReplayRelation.SetItemText(nItemNum,10,szLinkageStationName);
			m_listReplayRelation.SetItemText(nItemNum,11,szLinkageNum);
			m_listReplayRelation.SetItemText(nItemNum,12,szNodeId);

			nItemNum ++;
		}
		mysql_free_result( res ) ;
	}

	return TRUE;
}

BOOL CDlgReplay::ReadAndSetListCameraInfo(int nIndex,int nNodeId,char *szLinkageTime)
{
	if(nNodeId < 0)
		return FALSE;

	char sql_buf[512] = {0};
	bool result=false;

	MYSQL_RES * res = NULL ;
	MYSQL_ROW	row ;

	char szYear[8] = {0};
	int nYear = 2013;

	//////////////////////////////////////////////////////////////////////////
	char szCameraName[256] = {0};
	char szCameraCode[256] = {0};
	char szDecodeTag[16] = {0};
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	char szLinkageType[64] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szSubType[64] = {0};
	char szState[64] = {0};
	char szScreenId[64] = {0};
	char szLinkageRealTime[64] = {0};
	char szContent[256] = {0};
	char szValue1[64] = {0};
	char szValue2[64] = {0};
	char szLinkageStationName[256] = {0};
	char szLinkageNum[16] = {0};
	char szNodeId[16] = {0};

	int nDecodeTag = 0;
	int nCount = 0;

	//////////////////////////////////////////////////////////////////////////
	m_listReplayRelation.GetItemText(nIndex,0,szLinkageType,sizeof(szLinkageType));
	m_listReplayRelation.GetItemText(nIndex,1,szDeviceName,sizeof(szDeviceName));
	m_listReplayRelation.GetItemText(nIndex,2,szStationName,sizeof(szStationName));
	m_listReplayRelation.GetItemText(nIndex,3,szLinkageRealTime,sizeof(szLinkageRealTime));
	m_listReplayRelation.GetItemText(nIndex,4,szSubType,sizeof(szSubType));
	m_listReplayRelation.GetItemText(nIndex,5,szState,sizeof(szState));
	m_listReplayRelation.GetItemText(nIndex,6,szScreenId,sizeof(szScreenId));
	m_listReplayRelation.GetItemText(nIndex,7,szContent,sizeof(szContent));
	m_listReplayRelation.GetItemText(nIndex,8,szValue1,sizeof(szValue1));
	m_listReplayRelation.GetItemText(nIndex,9,szValue2,sizeof(szValue2));
	m_listReplayRelation.GetItemText(nIndex,10,szLinkageStationName,sizeof(szLinkageStationName));
	m_listReplayRelation.GetItemText(nIndex,11,szLinkageNum,sizeof(szLinkageNum));
	m_listReplayRelation.GetItemText(nIndex,12,szNodeId,sizeof(szNodeId));

	szYear[0] = szLinkageTime[0];
	szYear[1] = szLinkageTime[1];
	szYear[2] = szLinkageTime[2];
	szYear[3] = szLinkageTime[3];
	szYear[4] = 0;

	nYear = atoi(szYear);

	//读摄像头联动信息
	sprintf_s(sql_buf,sizeof(sql_buf),"SELECT device, code, decoder_tag, relate_id "
		"FROM history_relation_%04d "
		"where relate_id=%d AND event='控制云台' ORDER BY id",nYear,nNodeId);

	if (!mysql_query(g_mySqlData, sql_buf))
	{
		res = mysql_store_result(g_mySqlData);

		while ( row = mysql_fetch_row( res ) )
		{
			if (nCount > 1024)
				break;

			sprintf_s(szCameraName, "%s", row[0]);
			sprintf_s(szCameraCode, "%s", row[1]);
			sprintf_s(szDecodeTag,"%s",row[2]);
			sprintf_s(szNodeId,"%s",row[3]);

			nDecodeTag = atoi(szDecodeTag);

			m_ListCtrl_ReplayFile.InsertItem(nCount,szCameraName);
			m_ListCtrl_ReplayFile.SetItemText(nCount,1,szCameraCode);
			m_ListCtrl_ReplayFile.SetItemText(nCount,2,szDeviceName);
			m_ListCtrl_ReplayFile.SetItemText(nCount,3,szStationName);
			m_ListCtrl_ReplayFile.SetItemText(nCount,4,szLinkageType);
			m_ListCtrl_ReplayFile.SetItemText(nCount,5,szLinkageRealTime);
			m_ListCtrl_ReplayFile.SetItemText(nCount,6,szSubType);
			m_ListCtrl_ReplayFile.SetItemText(nCount,7,szState);
			m_ListCtrl_ReplayFile.SetItemText(nCount,8,szScreenId);
			m_ListCtrl_ReplayFile.SetItemText(nCount,9,szValue1);
			m_ListCtrl_ReplayFile.SetItemText(nCount,10,szValue2);
			m_ListCtrl_ReplayFile.SetItemText(nCount,11,"");
			m_ListCtrl_ReplayFile.SetItemText(nCount,12,"");
			m_ListCtrl_ReplayFile.SetItemText(nCount,13,"");
			m_ListCtrl_ReplayFile.SetItemText(nCount,14,"");
			m_ListCtrl_ReplayFile.SetItemText(nCount,15,"");
			m_ListCtrl_ReplayFile.SetItemData(0,nDecodeTag);

			nCount++;
		}
		mysql_free_result( res ) ;
	}

	return TRUE;
}

BOOL CDlgReplay::ReadAndSetWeatherListInfo(char *szType,char *szStartLinkageTime,char *szStopLinkageTime)
{
	if (strlen(szStartLinkageTime) == 0||strlen(szStopLinkageTime) == 0)
		return FALSE;

	m_listReplayRelation.DeleteAllItems();

	char sql_buf[512] = {0};
	bool result=false;

	MYSQL_RES * res = NULL ;
	MYSQL_ROW	row ;

	char szYear[8] = {0};
	int nYear = 2013;

	int nLinkageType = 0;
	char szLinkageType[64] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szSubType[64] = {0};
	char szState[64] = {0};
	char szScreenId[64] = {0};
	char szLinkageRealTime[64] = {0};
	char szContent[256] = {0};
	char szValue1[64] = {0};
	char szValue2[64] = {0};
	char szLinkageStationName[256] = {0};
	char szLinkageNum[16] = {0};
	char szNodeId[16] = {0};

	szYear[0] = szStartLinkageTime[0];
	szYear[1] = szStartLinkageTime[1];
	szYear[2] = szStartLinkageTime[2];
	szYear[3] = szStartLinkageTime[3];
	szYear[4] = 0;

	nYear = atoi(szYear);

	//读气象联动信息
	sprintf_s(sql_buf,sizeof(sql_buf),"SELECT event, station, device, weather_name, state, screen_id, time,"
		"content, yx_value,yc_value,station,link_num,id FROM history_relation_%04d "
		"where time >= '%s' AND time <='%s' AND (event='气象报警' OR event='气象预警') ",nYear,szStartLinkageTime,szStopLinkageTime);

	if (strcmp(szType,"暴雪") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND weather_type='BXYJ' ");
	}
	else if (strcmp(szType,"暴雨") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND weather_type='BYYJ' ");
	}
	else if (strcmp(szType,"高温") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND weather_type='GWYJ' ");
	}
	else if (strcmp(szType,"低温") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND weather_type='DWYJ' ");
	}
	else if (strcmp(szType,"大雾") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND weather_type='WUYJ' ");
	}
	else if (strcmp(szType,"大风") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND weather_type='DFYJ' ");
	}
	else if (strcmp(szType,"台风") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND weather_type='TAIFENG' ");
	}
	else if (strcmp(szType,"雷电") == 0)
	{
		sprintf(sql_buf+strlen(sql_buf),"AND weather_type='LEIDIAN' ");
	}

	sprintf(sql_buf+strlen(sql_buf),"ORDER BY id");

	int nItemNum = 0;

	if (!mysql_query(g_mySqlData, sql_buf))
	{
		res = mysql_store_result(g_mySqlData);

		while ( row = mysql_fetch_row( res ) )
		{
			if (nItemNum > 300)
				break;

			sprintf_s(szLinkageType,"%s",row[0]);
			sprintf_s(szStationName, "%s", row[1]);
			sprintf_s(szDeviceName, "%s", row[2]);
			sprintf_s(szSubType,"%s",row[3]);
			sprintf_s(szState,"%s",row[4]);
			sprintf_s(szScreenId,"%s",row[5]);
			sprintf_s(szLinkageRealTime,"%s",row[6]);
			sprintf_s(szContent,"%s",row[7]);
			sprintf_s(szValue1, "%s", row[8]);
			sprintf_s(szValue2,"%s",row[9]);
			sprintf_s(szLinkageStationName,"%s",row[10]);
			sprintf_s(szLinkageNum,"%s",row[11]);
			sprintf_s(szNodeId,"%s",row[12]);

			m_listReplayRelation.InsertItem(nItemNum,szLinkageType);
			m_listReplayRelation.SetItemText(nItemNum,1,szDeviceName);
			m_listReplayRelation.SetItemText(nItemNum,2,szStationName);
			m_listReplayRelation.SetItemText(nItemNum,3,szLinkageRealTime);
			m_listReplayRelation.SetItemText(nItemNum,4,szSubType);
			m_listReplayRelation.SetItemText(nItemNum,5,szState);
			m_listReplayRelation.SetItemText(nItemNum,6,szScreenId);
			m_listReplayRelation.SetItemText(nItemNum,7,szContent);
			m_listReplayRelation.SetItemText(nItemNum,8,szValue1);
			m_listReplayRelation.SetItemText(nItemNum,9,szValue2);
			m_listReplayRelation.SetItemText(nItemNum,10,szLinkageStationName);
			m_listReplayRelation.SetItemText(nItemNum,11,szLinkageNum);
			m_listReplayRelation.SetItemText(nItemNum,12,szNodeId);

			nItemNum ++;
		}
		mysql_free_result( res ) ;
	}

	return TRUE;
}

BOOL CDlgReplay::ReadAndSetWeatherListCameraInfo(int nIndex,int nNodeId,char *szLinkageTime)
{
	if(nNodeId < 0)
		return FALSE;

	char sql_buf[512] = {0};
	bool result=false;

	MYSQL_RES * res = NULL ;
	MYSQL_ROW	row ;

	char szYear[8] = {0};
	int nYear = 2013;

	//////////////////////////////////////////////////////////////////////////
	char szCameraName[256] = {0};
	char szCameraCode[256] = {0};
	char szDecodeTag[16] = {0};
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	char szLinkageType[64] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szSubType[64] = {0};
	char szState[64] = {0};
	char szScreenId[64] = {0};
	char szLinkageRealTime[64] = {0};
	char szContent[256] = {0};
	char szValue1[64] = {0};
	char szValue2[64] = {0};
	char szLinkageStationName[256] = {0};
	char szLinkageNum[16] = {0};
	char szNodeId[16] = {0};

	int nDecodeTag = 0;

	int nCount = 0;

	//////////////////////////////////////////////////////////////////////////
	m_listReplayRelation.GetItemText(nIndex,0,szLinkageType,sizeof(szLinkageType));
	m_listReplayRelation.GetItemText(nIndex,1,szDeviceName,sizeof(szDeviceName));
	m_listReplayRelation.GetItemText(nIndex,2,szStationName,sizeof(szStationName));
	m_listReplayRelation.GetItemText(nIndex,3,szLinkageRealTime,sizeof(szLinkageRealTime));
	m_listReplayRelation.GetItemText(nIndex,4,szSubType,sizeof(szSubType));
	m_listReplayRelation.GetItemText(nIndex,5,szState,sizeof(szState));
	m_listReplayRelation.GetItemText(nIndex,6,szScreenId,sizeof(szScreenId));
	m_listReplayRelation.GetItemText(nIndex,7,szContent,sizeof(szContent));
	m_listReplayRelation.GetItemText(nIndex,8,szValue1,sizeof(szValue1));
	m_listReplayRelation.GetItemText(nIndex,9,szValue2,sizeof(szValue2));
	m_listReplayRelation.GetItemText(nIndex,10,szLinkageStationName,sizeof(szLinkageStationName));
	m_listReplayRelation.GetItemText(nIndex,11,szLinkageNum,sizeof(szLinkageNum));
	m_listReplayRelation.GetItemText(nIndex,12,szNodeId,sizeof(szNodeId));

	szYear[0] = szLinkageTime[0];
	szYear[1] = szLinkageTime[1];
	szYear[2] = szLinkageTime[2];
	szYear[3] = szLinkageTime[3];
	szYear[4] = 0;

	nYear = atoi(szYear);

	//读摄像头联动信息
	sprintf_s(sql_buf,sizeof(sql_buf),"SELECT device, code, decoder_tag, relate_id "
		"FROM history_relation_%04d "
		"where relate_id=%d AND event='控制云台' ORDER BY id",nYear,nNodeId);

	if (!mysql_query(g_mySqlData, sql_buf))
	{
		res = mysql_store_result(g_mySqlData);

		while ( row = mysql_fetch_row( res ) )
		{
			if (nCount > 300)
				break;

			sprintf_s(szCameraName, "%s", row[0]);
			sprintf_s(szCameraCode, "%s", row[1]);
			sprintf_s(szDecodeTag,"%s",row[2]);
			sprintf_s(szNodeId,"%s",row[3]);

			nDecodeTag = atoi(szDecodeTag);

			m_ListCtrl_ReplayFile.InsertItem(nCount,szCameraName);
			m_ListCtrl_ReplayFile.SetItemText(nCount,1,szCameraCode);
			m_ListCtrl_ReplayFile.SetItemText(nCount,2,szDeviceName);
			m_ListCtrl_ReplayFile.SetItemText(nCount,3,szStationName);
			m_ListCtrl_ReplayFile.SetItemText(nCount,4,szLinkageType);
			m_ListCtrl_ReplayFile.SetItemText(nCount,5,szLinkageRealTime);
			m_ListCtrl_ReplayFile.SetItemText(nCount,6,szSubType);
			m_ListCtrl_ReplayFile.SetItemText(nCount,7,szState);
			m_ListCtrl_ReplayFile.SetItemText(nCount,8,szScreenId);
			m_ListCtrl_ReplayFile.SetItemText(nCount,9,szValue1);
			m_ListCtrl_ReplayFile.SetItemText(nCount,10,szValue2);
			m_ListCtrl_ReplayFile.SetItemText(nCount,11,"");
			m_ListCtrl_ReplayFile.SetItemText(nCount,12,"");
			m_ListCtrl_ReplayFile.SetItemText(nCount,13,"");
			m_ListCtrl_ReplayFile.SetItemText(nCount,14,"");
			m_ListCtrl_ReplayFile.SetItemText(nCount,15,"");
			m_ListCtrl_ReplayFile.SetItemData(nCount,nDecodeTag);

			nCount++;

		}
		mysql_free_result( res ) ;
	}

	if (nCount == 0)
	{
		m_ListCtrl_ReplayFile.InsertItem(nCount,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,1,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,2,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,3,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,4,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,5,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,6,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,7,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,8,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,9,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,10,"无数据");
		m_ListCtrl_ReplayFile.SetItemText(nCount,11,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,12,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,13,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,14,"");
		m_ListCtrl_ReplayFile.SetItemText(nCount,15,"");
		m_ListCtrl_ReplayFile.SetItemData(nCount,nDecodeTag);
	}

	return TRUE;
}

BOOL CDlgReplay::GetAndSetVideoFileRtspInfo(int nIndex)
{
	try
	{
		UpdateData(TRUE);

		char szNodeNum[128] = {0};
		char szLinkageTime[128] = {0};

		m_ListCtrl_ReplayFile.GetItemText(nIndex,1,szNodeNum,sizeof(szNodeNum));
		m_ListCtrl_ReplayFile.GetItemText(nIndex,5,szLinkageTime,sizeof(szLinkageTime));

		if (strlen(szNodeNum) == 0||strlen(szLinkageTime) == 0)
			return FALSE;

		if (strcmp(szNodeNum,"无数据") == 0||strcmp(szLinkageTime,"无数据") == 0)
			return FALSE;

		szLinkageTime[10] = 'T';
		szLinkageTime[19] = 'Z';
		szLinkageTime[20] = 0;


		time_t link_time;
		tm     link_tm;
		memset(&link_tm,0,sizeof(link_tm));

		int nYear = 0;
		int nMonth = 0;
		int nDay = 0;
		int nHour = 0;
		int nMinute = 0;
		int nSecond = 0;


		sscanf(szLinkageTime,"%04d-%02d-%02dT%02d:%02d:%2dZ",&nYear,&nMonth,&nDay,&nHour,&nMinute,&nSecond);

		link_tm.tm_year = nYear-1900;
		link_tm.tm_mon = nMonth-1;
		link_tm.tm_mday = nDay;
		link_tm.tm_hour = nHour;
		link_tm.tm_min = nMinute;
		link_tm.tm_sec = nSecond;

		link_time = mktime(&link_tm);

		time_t start_time = link_time + 8*60*60 - 12*60*60;
		time_t stop_time = link_time + 8*60*60 + 12*60*60;

		tm* start_tm = gmtime(&start_time);
		sprintf_s(m_Replay_Query_Info.start_time,"%04d-%02d-%02dT%02d:%02d:%02dZ",start_tm->tm_year+1900,start_tm->tm_mon+1,start_tm->tm_mday,start_tm->tm_hour,start_tm->tm_min,start_tm->tm_sec);

		tm* stop_tm = gmtime(&stop_time);
		sprintf_s(m_Replay_Query_Info.stop_time,"%04d-%02d-%02dT%02d:%02d:%02dZ",stop_tm->tm_year+1900,stop_tm->tm_mon+1,stop_tm->tm_mday,stop_tm->tm_hour,stop_tm->tm_min,stop_tm->tm_sec);


		// 采用http请求资源
		PREQ_VIDEO	req_video = NULL;
		PRES_VIDEO	res_video = NULL;
		void	*inArg = NULL, *outArg = NULL;
		int nRet = 0;
		int i = 0;

		nRet = initStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
		if (nRet < 0)
		{
			MessageBox("构造请求失败","视频监视");
			return FALSE;
		}

		req_video = (PREQ_VIDEO)inArg;

		strcpy(req_video->code,szNodeNum);
		strcpy(req_video->UserCode, app_RegCh.reg_user);
		req_video->FromIndex = 1;
		req_video->ToIndex = 100;
		strcpy(req_video->BeginTime, m_Replay_Query_Info.start_time);
		strcpy(req_video->EndTime, m_Replay_Query_Info.stop_time);

		req_video->type = 0xFFFFFFFF;	// 请求的类型值
		req_video->PlantOrPU = 0;

		if (m_SelectedPlant == 0)
		{
			// 在本平台请求
			for (i = 0; i < g_TotalSysNum; i ++)
			{
				if (g_SystemInfo[i].localdomain == 1)
				{
					break;
				}
			}

			if (i == g_TotalSysNum)
			{
				MessageBox("系统不在线!","视频监视");
				return FALSE;
			}
		}
		else
		{
			// 构造所在系统号
			char c_systemid[20]="000000000000000000";			//根据实际请求的节点号构造系统号
			memcpy(c_systemid, m_pNodeInfo->node_num, 5);
			// 判断所请求的系统是否在线，如果不在线则提示并退出
			for (i = 0; i < g_TotalSysNum; i ++)
			{
				if (strcmp(c_systemid, g_SystemInfo[i].sysid)== 0)
				{
					break;
				}
			}

			if (i == g_TotalSysNum)
			{
				MessageBox("系统不在线!","视频监视");
				freeStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
				freeStructArg(&outArg, HTTP_RESPONSE_HISTORY_VIDEO);
				return FALSE;
			}
		}

		nRet = HttpClient(g_SystemInfo[i].http_ip,
			g_SystemInfo[i].http_port,
			HTTP_REQUEST_HISTORY_VIDEO,
			inArg,
			&outArg,
			10);

		if (nRet != 0)
		{
			MessageBox("请求返回失败!","视频监视");
			freeStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
			freeStructArg(&outArg, HTTP_RESPONSE_HISTORY_VIDEO);
			return FALSE;
		}

		res_video = (PRES_VIDEO)outArg;

		if (res_video->SubNum <= 0)
		{
			MessageBox("未查询到关联视频","视频监视");
			freeStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
			freeStructArg(&outArg, HTTP_RESPONSE_HISTORY_VIDEO);
			return FALSE;
		}

		// 将查询结果存放到列表中
		char szFileStartTime[128] = {0};
		char szFileStopTime[128] = {0};

		PRES_VIDEO_ITEM res_ptr;
		for (res_ptr = res_video->item; res_ptr; res_ptr = res_ptr->next)
		{
			strcpy_s(szFileStartTime,sizeof(szFileStartTime),res_ptr->BeginTime);
			strcpy_s(szFileStopTime,sizeof(szFileStopTime),res_ptr->EndTime);

			if (strcmp(szFileStartTime,szLinkageTime) <= 0&&strcmp(szFileStopTime,szLinkageTime) >= 0)
			{
				m_ListCtrl_ReplayFile.SetItemText(nIndex, 11, szFileStartTime);
				m_ListCtrl_ReplayFile.SetItemText(nIndex, 12, szFileStopTime);
				m_ListCtrl_ReplayFile.SetItemText(nIndex, 13, res_ptr->FileUrl);
				char filesize[20];
				sprintf(filesize, "%d", res_ptr->size);
				m_ListCtrl_ReplayFile.SetItemText(nIndex, 14, filesize);

				if (res_ptr->DecoderTag < 10)
				{
					m_ListCtrl_ReplayFile.SetItemData(nIndex, m_pNodeInfo->node_decodetag);
				}
				else
				{
					m_ListCtrl_ReplayFile.SetItemData(nIndex, res_ptr->DecoderTag);
				}

				break;

			}
		}

		freeStructArg(&inArg, HTTP_REQUEST_HISTORY_VIDEO);
		freeStructArg(&outArg, HTTP_RESPONSE_HISTORY_VIDEO);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

//通过直连前端，得到并设置前端回放文件信息
BOOL CDlgReplay::GetAndSetDeviceVideoFileInfo(int nIndex)
{
	try
	{
		UpdateData(TRUE);

		char szCameraNum[32] = {0};
		char szStationName[128] = {0};
		char szVideoStationName[128] = {0};

		char szNodeNum[128] = {0};
		char szLinkageTime[128] = {0};

		m_ListCtrl_ReplayFile.GetItemText(nIndex,1,szNodeNum,sizeof(szNodeNum));
		m_ListCtrl_ReplayFile.GetItemText(nIndex,5,szLinkageTime,sizeof(szLinkageTime));
		
		if (strlen(szNodeNum) == 0||strlen(szLinkageTime) == 0)
			return FALSE;

		if (strcmp(szNodeNum,"无数据") == 0||strcmp(szLinkageTime,"无数据") == 0)
			return FALSE;

		szLinkageTime[10] = 'T';
		szLinkageTime[19] = 'Z';
		szLinkageTime[20] = 0;

		time_t link_time;
		tm     link_tm;
		memset(&link_tm,0,sizeof(link_tm));

		int nYear = 0;
		int nMonth = 0;
		int nDay = 0;
		int nHour = 0;
		int nMinute = 0;
		int nSecond = 0;

		sscanf(szLinkageTime,"%04d-%02d-%02dT%02d:%02d:%2dZ",&nYear,&nMonth,&nDay,&nHour,&nMinute,&nSecond);

		link_tm.tm_year = nYear-1900;
		link_tm.tm_mon = nMonth-1;
		link_tm.tm_mday = nDay;
		link_tm.tm_hour = nHour;
		link_tm.tm_min = nMinute;
		link_tm.tm_sec = nSecond;

		link_time = mktime(&link_tm);

		time_t start_time = link_time + 8*60*60 - 12*60*60;
		time_t stop_time = link_time + 8*60*60 + 12*60*60;

		tm* start_tm = gmtime(&start_time);
		SDKTIME start_time_sdk;
		start_time_sdk.year = start_tm->tm_year+1900;
		start_time_sdk.month = start_tm->tm_mon+1;
		start_time_sdk.day = start_tm->tm_mday;
		start_time_sdk.hour = start_tm->tm_hour;
		start_time_sdk.minute = start_tm->tm_min;
		start_time_sdk.second = start_tm->tm_sec;

		tm* stop_tm = gmtime(&stop_time);
		SDKTIME stop_time_sdk;
		stop_time_sdk.year = stop_tm->tm_year+1900;
		stop_time_sdk.month = stop_tm->tm_mon+1;
		stop_time_sdk.day = stop_tm->tm_mday;
		stop_time_sdk.hour = stop_tm->tm_hour;
		stop_time_sdk.minute = stop_tm->tm_min;
		stop_time_sdk.second = stop_tm->tm_sec;

		//////////////////////////////////////////////////////////////////////////
		void * record_info = NULL;
		int record_num = 0;

		char szFileSize[32] = {0};

		char szFileStartTime[128] = {0};
		char szFileStopTime[128] = {0};

		int i = 0;
		int j = 0;

		_T_CAMERA_INFO * pCameraInfo = NULL;
		_T_DVR_INFO * pDvrInfo = NULL;
		RECORDITEM *RecordItem = NULL;

		if (g_pMainDlg == NULL||g_pMainDlg->m_pDlgPageServer == NULL)
			return FALSE;

		m_ListCtrl_ReplayFile.GetItemText(nIndex,1,szCameraNum,sizeof(szCameraNum));
		m_ListCtrl_ReplayFile.GetItemText(nIndex,3,szStationName,sizeof(szStationName));
		memset(szVideoStationName,0,sizeof(szVideoStationName));

		for (i = 0;i < g_nStation_Num;i++)
		{
			if (strcmp(g_tStation_Info[i].station_name_d5000,szStationName) == 0)
			{
				strcpy_s(szVideoStationName,sizeof(szVideoStationName),g_tStation_Info[i].station_name_videoplant);
			}
		}

		if (strlen(szVideoStationName) == 0)
			return FALSE;

		g_pMainDlg->m_pDlgPageServer->CameraTreelistAddCameraByStastion(szVideoStationName);
		HTREEITEM hTreeItem =  g_pMainDlg->m_pDlgPageServer->SearchCameraTreeCameraHandleTreeItem(NULL,szCameraNum,NULL,NULL);

		if (hTreeItem == NULL)
			return FALSE;

		_T_NODE_INFO * pNodeInfo = (_T_NODE_INFO *)g_pMainDlg->m_pDlgPageServer->m_trServer.GetItemData(hTreeItem);
		if (pNodeInfo == NULL)
			return FALSE;

		m_pNodeInfo = pNodeInfo;
		pCameraInfo = &m_pNodeInfo->camera_info;
		pDvrInfo = &pCameraInfo->dvr_info;

		strcpy_s(m_szDvrIP,sizeof(m_szDvrIP),pDvrInfo->dvr_ip);
		m_nDvrPort = pDvrInfo->dvr_port;
		m_nCameraChannel = pCameraInfo->channel;
		m_nDvrId = pDvrInfo->dvr_id;
		m_nDvrType = pDvrInfo->dvr_type;
		strcpy_s(m_szUserName,sizeof(m_szUserName),pDvrInfo->dvr_user);
		strcpy_s(m_szUserPassword,sizeof(m_szUserPassword),pDvrInfo->dvr_password);

		g_VMLog.WriteVmLog("--GetAndSetDeviceVideoFileInfo-DvrIP=%s,DvrPort=%d,UserName=%s,UserPassword=%s,DvrType=%d,DvrID=%d,CameraChannel=%d",
			m_szDvrIP,m_nDvrPort,m_szUserName,m_szUserPassword,m_nDvrType,m_nDvrId,m_nCameraChannel);

		g_VMLog.WriteVmLog("--GetAndSetDeviceVideoFileInfo-Start_Time-%04d-%02d-%02d %02d:%02d:%02d--------StopTime--%04d-%02d-%02d %02d:%02d:%02d",
			start_time_sdk.year,start_time_sdk.month,start_time_sdk.day,start_time_sdk.hour,start_time_sdk.minute,start_time_sdk.second,
			stop_time_sdk.year,stop_time_sdk.month,stop_time_sdk.day,stop_time_sdk.hour,stop_time_sdk.minute,stop_time_sdk.second);

		bool bResult = GetRecordFileInfo_DevSdk(m_szDvrIP,m_nDvrPort,m_szUserName,m_szUserPassword,m_nDvrType,m_nDvrId,m_nCameraChannel,start_time_sdk,stop_time_sdk,&record_info,&record_num);
		if (bResult&&record_info != NULL&&record_num > 0)
		{
			j = 0;

			RecordItem = (RECORDITEM *)record_info;

			for (i = 0;i < record_num;i++)
			{
				if (j >= 1000)
					break;

				strcpy_s(szFileStartTime,sizeof(szFileStartTime),RecordItem[i].BeginTime);
				strcpy_s(szFileStopTime,sizeof(szFileStopTime),RecordItem[i].EndTime);

				if (strcmp(szFileStartTime,szLinkageTime) <= 0&&strcmp(szFileStopTime,szLinkageTime) >= 0)
				{
					m_ListCtrl_ReplayFile.SetItemText(nIndex, 11, RecordItem[i].BeginTime);
					m_ListCtrl_ReplayFile.SetItemText(nIndex, 12, RecordItem[i].EndTime);
					m_ListCtrl_ReplayFile.SetItemText(nIndex, 13, RecordItem[i].FileName);

					sprintf(szFileSize, "%d", RecordItem[i].size);
					m_ListCtrl_ReplayFile.SetItemText(nIndex, 14, szFileSize);

					m_ListCtrl_ReplayFile.SetItemData(nIndex,m_nDvrType);

					break;
				}
			}
		}

		if (record_info != NULL)
		{
			FreeRecordFileInfo_DevSdk(m_nDvrType,record_info);
			record_info = NULL;
		}
		record_num = 0;

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::SetLinkTimeLinePos(int nIndex)
{
	char szLinkageTime[64] = {0};
	char szFileStartTime[64] = {0};
	char szFileStopTime[64] = {0};

	m_ListCtrl_ReplayFile.GetItemText(nIndex,5,szLinkageTime,sizeof(szLinkageTime));
	m_ListCtrl_ReplayFile.GetItemText(nIndex,11,szFileStartTime,sizeof(szFileStartTime));
	m_ListCtrl_ReplayFile.GetItemText(nIndex,12,szFileStopTime,sizeof(szFileStopTime));

	if (strlen(szLinkageTime) == 0||strlen(szFileStartTime) == 0||strlen(szFileStopTime) == 0)
		return FALSE;

	time_t link_time;
	time_t start_time;
	time_t stop_time;

	tm     temp_tm;
	memset(&temp_tm,0,sizeof(temp_tm));

	int nYear = 0;
	int nMonth = 0;
	int nDay = 0;
	int nHour = 0;
	int nMinute = 0;
	int nSecond = 0;


	sscanf(szLinkageTime,"%04d-%02d-%02d %02d:%02d:%2d",&nYear,&nMonth,&nDay,&nHour,&nMinute,&nSecond);

	temp_tm.tm_year = nYear-1900;
	temp_tm.tm_mon = nMonth-1;
	temp_tm.tm_mday = nDay;
	temp_tm.tm_hour = nHour;
	temp_tm.tm_min = nMinute;
	temp_tm.tm_sec = nSecond;

	link_time = mktime(&temp_tm);

	sscanf(szFileStartTime,"%04d-%02d-%02dT%02d:%02d:%2dZ",&nYear,&nMonth,&nDay,&nHour,&nMinute,&nSecond);

	temp_tm.tm_year = nYear-1900;
	temp_tm.tm_mon = nMonth-1;
	temp_tm.tm_mday = nDay;
	temp_tm.tm_hour = nHour;
	temp_tm.tm_min = nMinute;
	temp_tm.tm_sec = nSecond;

	start_time = mktime(&temp_tm);

	sscanf(szFileStopTime,"%04d-%02d-%02dT%02d:%02d:%2dZ",&nYear,&nMonth,&nDay,&nHour,&nMinute,&nSecond);

	temp_tm.tm_year = nYear-1900;
	temp_tm.tm_mon = nMonth-1;
	temp_tm.tm_mday = nDay;
	temp_tm.tm_hour = nHour;
	temp_tm.tm_min = nMinute;
	temp_tm.tm_sec = nSecond;

	stop_time = mktime(&temp_tm);

	int nAllTimeSize = stop_time - start_time;
	int nLinkageTimeSize = link_time - start_time;

	if (nAllTimeSize <= 0||nLinkageTimeSize < 0||nAllTimeSize < nLinkageTimeSize)
		return FALSE;

	int nLinkTimePos = (nLinkageTimeSize*1000)/nAllTimeSize;

	m_SliderPlayPos.SetDrawLinePoint(nLinkTimePos);
	m_SliderPlayPos.SetDrawLinePointFlag(true);

	return TRUE;
}

int CDlgReplay::GetWeatherStationIdFromStationName(char *szStationName)
{
	if (szStationName == NULL||strlen(szStationName) == 0)
		return -1;

	int nStationId = -1;

	for (int i = 0;i < g_nStation_Num;i++)
	{
		if(strcmp(g_tStation_Info[i].station_name_videoplant,szStationName) == 0
			||strcmp(g_tStation_Info[i].station_name_d5000,szStationName) == 0
			||strcmp(g_tStation_Info[i].station_name_meteo_a,szStationName) == 0)
		{
			if (strlen(g_tStation_Info[i].station_code_weather) != 0)
				nStationId = atoi(g_tStation_Info[i].station_code_weather);
			break;
		}
	}

	return nStationId;
}

BOOL CDlgReplay::StartRemoteVideoReplayHistoryWeatherInfo()
{
	if (g_pMainDlg == NULL)
		return FALSE;

	int nStationId = GetWeatherStationIdFromStationName(m_szWeatherStationName);
	if (nStationId < 0)
		return FALSE;

	//////////////////////////////////////////////////////////////////////////
	time_t weather_time;
	tm       weather_tm;
	memset(&weather_tm,0,sizeof(weather_tm));

	int nYear = 0;
	int nMonth = 0;
	int nDay = 0;
	int nHour = 0;
	int nMinute = 0;
	int nSecond = 0;

	char szStartTime[64] = {0};
	char szStopTime[64] = {0};

	sscanf(m_szWeatherStartTime,"%04d-%02d-%02dT%02d:%02d:%2dZ",&nYear,&nMonth,&nDay,&nHour,&nMinute,&nSecond);

	weather_tm.tm_year = nYear-1900;
	weather_tm.tm_mon = nMonth-1;
	weather_tm.tm_mday = nDay;
	weather_tm.tm_hour = nHour;
	weather_tm.tm_min = nMinute;
	weather_tm.tm_sec = nSecond;

	weather_time = mktime(&weather_tm);

	time_t start_time = weather_time + 8*60*60 - 24*60*60;
	time_t stop_time = weather_time + 8*60*60 + 24*60*60;

	tm* start_tm = gmtime(&start_time);
	sprintf_s(szStartTime,"%04d-%02d-%02d %02d:%02d:%02d",start_tm->tm_year+1900,start_tm->tm_mon+1,start_tm->tm_mday,start_tm->tm_hour,start_tm->tm_min,start_tm->tm_sec);

	tm* stop_tm = gmtime(&stop_time);
	sprintf_s(szStopTime,"%04d-%02d-%02d %02d:%02d:%02d",stop_tm->tm_year+1900,stop_tm->tm_mon+1,stop_tm->tm_mday,stop_tm->tm_hour,stop_tm->tm_min,stop_tm->tm_sec);

	//////////////////////////////////////////////////////////////////////////

	g_pMainDlg->StartVideoWinWeatherHistoryInfo(0,"jsepc@dp",nStationId,szStartTime,szStopTime);

	//	g_pMainDlg->StartVideoWinWeatherHistoryInfo(0,"jsepc@dp",nStationId,m_szWeatherStartTime,m_szWeatherStopTime);

	return TRUE;
}

BOOL CDlgReplay::StopRemoteVideoReplayHistoryWeatherInfo()
{
	if (g_pMainDlg == NULL)
		return FALSE;

	g_pMainDlg->StopVideoWinWeatherHistoryInfo(0);

	return TRUE;
}

void CDlgReplay::OnNMRClickListReplayfile(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	//////////////////////////////////////////////////////////////////////////
	CMenu  popMenu;

	popMenu.LoadMenu(IDR_MENU_VIDEO_REPLAY);
	CMenu *pMenu = popMenu.GetSubMenu(0); 

	CPoint posMouse;
	GetCursorPos(&posMouse);

	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RIGHTALIGN,  posMouse.x, posMouse.y, this); 
	//////////////////////////////////////////////////////////////////////////

	*pResult = 0;
}

//回放下载
void CDlgReplay::OnMenuVideoReplayDownload()
{
	if (m_nTableSelectIndex == ALARM_REPLAY)
	{
		m_nSearchVideoAccessType = 1;
	}

	if(m_nSearchVideoAccessType == 0)
	{
		VideoReplayDownloadByVideoPlatform();
	}
	else if (m_nSearchVideoAccessType == 1)
	{
		VideoReplayDownloadByDirectDevice();
	}
}

//回放播放
void CDlgReplay::OnMenuVideoReplayPlay()
{
	POSITION pos = m_ListCtrl_ReplayFile.GetFirstSelectedItemPosition();

	if (pos == NULL)
		return;

	int index = m_ListCtrl_ReplayFile.GetNextSelectedItem(pos);

	if (index < 0)
		return;

	char szRemoteVideoShowTitle[512] = {0};
	char szStationName[256] = {0};
	char szDeviceName[256] = {0};
	char szCameraName[256] = {0};
	BOOL nRet = FALSE;

	if (m_RemotePlayState == REPLAY_STOP)
	{
		if (m_nTableSelectIndex == 1)//联动
		{
			sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
			if (strlen(m_RtspURL) <= 4)
			{
				nRet = GetAndSetVideoFileRtspInfo(index);
				if (nRet == FALSE)
					return;
			}

			m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));
			m_ListCtrl_ReplayFile.GetItemText(index, 2,szDeviceName,sizeof(szDeviceName));
			m_ListCtrl_ReplayFile.GetItemText(index, 3,szStationName,sizeof(szStationName));
			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{联动视频}-{%s}-{%s}-{%s}",szCameraName,szDeviceName,szStationName);

			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

			sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
			if (strlen(m_RtspURL) <= 4)
			{
				MessageBox("未查询到关联视频","视频监视");
				return;
			}

			m_ListCtrl_ReplayFile.GetItemText(index,11,m_szDvrRecordStartTime,sizeof(m_szDvrRecordStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,12,m_szDvrRecordStopTime,sizeof(m_szDvrRecordStopTime));
			m_ListCtrl_ReplayFile.GetItemText(index,13,m_RtspURL,sizeof(m_RtspURL));

			m_nRecordFileSize = g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 14));
			m_nDvrType = g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

			m_ListCtrl_ReplayFile.GetItemText(index,11,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,12,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
			strcpy_s(m_szWeatherStationName,sizeof(m_szWeatherStationName),szStationName);
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

			SetLinkTimeLinePos(index);
		}
		else if (m_nTableSelectIndex == 4)//平台视频
		{
			m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));

			if (strcmp(szCameraName,"无数据") == 0)
				return;

			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{平台视频}-{%s}",szCameraName);

			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

			sprintf_s(m_szDvrRecordStartTime,sizeof(m_szDvrRecordStartTime),"%s",m_ListCtrl_ReplayFile.GetItemText(index, 1));
			sprintf_s(m_szDvrRecordStopTime,sizeof(m_szDvrRecordStopTime),"%s",m_ListCtrl_ReplayFile.GetItemText(index, 2));
			sprintf_s(m_RtspURL,sizeof(m_RtspURL),"%s", m_ListCtrl_ReplayFile.GetItemText(index, 3));
			m_nRecordFileSize = g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 4));
			g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

			m_ListCtrl_ReplayFile.GetItemText(index,1,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,2,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象
		}
		else if (m_nTableSelectIndex == 2)//气象
		{
			sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
			if (strlen(m_RtspURL) <= 4)
			{
				nRet = GetAndSetVideoFileRtspInfo(index);
				if (nRet == FALSE)
					return;
			}

			m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));
			m_ListCtrl_ReplayFile.GetItemText(index, 2,szDeviceName,sizeof(szDeviceName));
			m_ListCtrl_ReplayFile.GetItemText(index, 3,szStationName,sizeof(szStationName));
			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{气象联动}-{%s}-{%s}",szCameraName,szStationName);

			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

			sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 13));
			if (strlen(m_RtspURL) <= 4)
			{
				MessageBox("未查询到关联视频","视频监视");
				return;
			}

			g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 14));
			g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

			m_ListCtrl_ReplayFile.GetItemText(index,11,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,12,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
			strcpy_s(m_szWeatherStationName,sizeof(m_szWeatherStationName),szStationName);
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

			SetLinkTimeLinePos(index);
		}
		else if (m_nTableSelectIndex == 3)//操作票
		{
			return;
		}
		else if (m_nTableSelectIndex == ALARM_REPLAY)
		{
			if (strcmp(m_ListCtrl_ReplayFile.GetItemText(index, 2).GetBuffer(),"无数据") == 0)
				return;

			//弹出选择关联的哪个摄像头播放的窗口
			char szDevName[64] = {0};
			CString strCameraName;
			m_ListCtrl_ReplayFile.GetItemText(index, 2, szDevName, 64);
			int nDevId = GetDevIdByDevName(szDevName);
			CDlgDeviceRelationCamera dlgDeviceRelationCamera(nDevId);
			if (dlgDeviceRelationCamera.DoModal()==IDOK)
			{
				strCameraName = dlgDeviceRelationCamera.m_strCameraName;
			}

			if (strCameraName.IsEmpty())
				return;

			//获取回放视频接口需要的参数
			char szCameraCode[32] = {0};
			GetCameraCodeByCameraName(szCameraCode, strCameraName.GetBuffer(0));

			ASS_CAMERA_INFO tAssCameraInfo;
			memset(&tAssCameraInfo, 0, sizeof(ASS_CAMERA_INFO));
			GetCameraInfoByCameraCode(&tAssCameraInfo, szCameraCode);

			char szAlarmHappenTime[32] = {0};
			m_ListCtrl_ReplayFile.GetItemText(index, 4, szAlarmHappenTime, 32);
			COleDateTime oleAlarmHappenTime;
			oleAlarmHappenTime.ParseDateTime(szAlarmHappenTime);
			SDKTIME start_time;
			SDKTIME stop_time;
			start_time.year = oleAlarmHappenTime.GetYear();
			start_time.month = oleAlarmHappenTime.GetMonth();
			start_time.day = oleAlarmHappenTime.GetDay();
			start_time.hour = oleAlarmHappenTime.GetHour();
			start_time.minute = oleAlarmHappenTime.GetMinute();
			start_time.second = oleAlarmHappenTime.GetSecond();
			stop_time.year = oleAlarmHappenTime.GetYear();
			stop_time.month = oleAlarmHappenTime.GetMonth();
			stop_time.day = oleAlarmHappenTime.GetDay();
			stop_time.hour = oleAlarmHappenTime.GetHour();
			stop_time.minute = oleAlarmHappenTime.GetMinute();
			stop_time.second = oleAlarmHappenTime.GetSecond() + 1;	//时间参数这里注意，比告警发生时间多一秒，即可取到当前这一小时的文件

			//获取回放视频文件
			void * record_info = NULL;
			RECORDITEM *pRecordItem = NULL;
			int nRecordNum = 0;
			bool bResult = GetRecordFileInfo_DevSdk(tAssCameraInfo.tDvrInfo.szDvrIp,
				tAssCameraInfo.tDvrInfo.nDvrPort,
				tAssCameraInfo.tDvrInfo.szDvrUserName,
				tAssCameraInfo.tDvrInfo.szDvrPassword,
				tAssCameraInfo.tDvrInfo.nDvrType,
				tAssCameraInfo.tDvrInfo.nDvrId,
				tAssCameraInfo.nChannel,start_time,stop_time,&record_info,&nRecordNum);
			if (bResult&&record_info != NULL&&nRecordNum > 0)
			{
				pRecordItem = (RECORDITEM *)record_info;
			}
			else
			{
				return;
			}

			//开始给成员变量赋值
			sprintf_s(m_szDvrIP, 32, tAssCameraInfo.tDvrInfo.szDvrIp);
			m_nDvrPort = tAssCameraInfo.tDvrInfo.nDvrPort;
			sprintf_s(m_szUserName, 32, tAssCameraInfo.tDvrInfo.szDvrUserName);
			sprintf_s(m_szUserPassword, 32, tAssCameraInfo.tDvrInfo.szDvrPassword);
			m_nDvrType = tAssCameraInfo.tDvrInfo.nDvrType;
			m_nDvrId = tAssCameraInfo.tDvrInfo.nDvrId;
			m_nCameraChannel = tAssCameraInfo.nChannel;
			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{平台视频}-{%s}",strCameraName.GetBuffer(0));
			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);
			sprintf_s(m_szDvrRecordStartTime,sizeof(m_szDvrRecordStartTime), "%s", pRecordItem[0].BeginTime);
			sprintf_s(m_szDvrRecordStopTime,sizeof(m_szDvrRecordStopTime), "%s", pRecordItem[0].EndTime);

			sprintf_s(m_RtspURL,sizeof(m_RtspURL),"%s", pRecordItem[0].FileName);
			m_nRecordFileSize = g_TotalBytes = pRecordItem[0].size;
			g_DevType = tAssCameraInfo.tDvrInfo.nDvrType;

			sprintf_s(m_szWeatherStartTime,sizeof(m_szWeatherStartTime), "%s", pRecordItem[0].BeginTime);
			sprintf_s(m_szWeatherStartTime,sizeof(m_szWeatherStartTime), "%s", pRecordItem[0].EndTime);
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象
			if (record_info != NULL)
			{
				FreeRecordFileInfo_DevSdk(tAssCameraInfo.tDvrInfo.nDvrType,record_info);
				record_info = NULL;
			}
			nRecordNum = 0;
			
			//使用直连方式查看历史视频
			m_nSearchVideoAccessType = 1;

		}
		else
			return;

		m_bPlayRemoteFlag = TRUE;

		g_VideoReplayDownloadMenuFlag = false;//回放服务

		PlayLocalORRemoteVideo();
	}
}

BOOL CDlgReplay::StartVideoReplayDownloadByVideoPlatform()
{
	if (g_VideoReplayDownloadInfo.bDownloadFlag != FALSE)
		return TRUE;

	if (strlen(g_ReplaySaveFilepath) == 0)
	{
		if (g_pMainDlg->m_pDlgRecFilePath == NULL||g_pMainDlg->m_pDlgRecFilePath->DoModal() == IDCANCEL)
			return FALSE;

		strcpy_s(g_ReplaySaveFilepath,sizeof(g_ReplaySaveFilepath),g_pMainDlg->m_pDlgRecFilePath->m_Filepath);
	}

	CTime nowtime = CTime::GetCurrentTime();

	char file_name[256] = {0};
	DWORD dwDataLen = 0;

	sprintf(g_VideoReplayDownloadInfo.szVideoFileName, "%s00%3d%04d%02d%02d%02d%02d%02d.mp4",
		g_VideoReplayDownloadInfo.szCameraCode,
		g_VideoReplayDownloadInfo.nDecodeTag,
		nowtime.GetYear(), nowtime.GetMonth(), nowtime.GetDay(),
		nowtime.GetHour(), nowtime.GetMinute(), nowtime.GetSecond());

	sprintf(file_name, "%s\\%s",g_ReplaySaveFilepath,g_VideoReplayDownloadInfo.szVideoFileName);

	g_VideoReplayDownloadInfo.hVideoSaveFile = CreateFile(file_name,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (g_VideoReplayDownloadInfo.hVideoSaveFile == INVALID_HANDLE_VALUE)
	{
		g_VideoReplayDownloadInfo.bDownloadFlag = FALSE;
		return FALSE;
	}

	if (m_ch.m_pPlayChannelInfo != NULL)
	{
		WriteFile(g_VideoReplayDownloadInfo.hVideoSaveFile,g_VideoReplayDownloadInfo.newSps,g_VideoReplayDownloadInfo.newSpsLen,&dwDataLen,NULL);
		FlushFileBuffers(g_VideoReplayDownloadInfo.hVideoSaveFile);
	}

	sprintf(g_VideoReplayDownloadInfo.szInfoFileName, "%s00%3d%04d%02d%02d%02d%02d%02d.info",
		g_VideoReplayDownloadInfo.szCameraCode,
		g_VideoReplayDownloadInfo.nDecodeTag,
		nowtime.GetYear(), nowtime.GetMonth(), nowtime.GetDay(),
		nowtime.GetHour(), nowtime.GetMinute(), nowtime.GetSecond());

	sprintf(file_name, "%s\\%s",g_ReplaySaveFilepath,g_VideoReplayDownloadInfo.szInfoFileName);

	g_VideoReplayDownloadInfo.hInfoSaveFile = CreateFile(file_name,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	g_VideoReplayDownloadInfo.bDownloadFlag = TRUE;

	return TRUE;
}

BOOL CDlgReplay::StopVideoReplayDownloadByVideoPlatform()
{
	if (g_VideoReplayDownloadInfo.bDownloadFlag == FALSE)
		return TRUE;

	g_VideoReplayDownloadInfo.bDownloadFlag = FALSE;

	if (g_VideoReplayDownloadInfo.hVideoSaveFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_VideoReplayDownloadInfo.hVideoSaveFile);
		g_VideoReplayDownloadInfo.hVideoSaveFile = INVALID_HANDLE_VALUE;
	}

	DWORD dwDataLen = 0;

	if (g_VideoReplayDownloadInfo.hInfoSaveFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(g_VideoReplayDownloadInfo.hInfoSaveFile,&g_VideoReplayDownloadInfo,sizeof(g_VideoReplayDownloadInfo),&dwDataLen,NULL);
		FlushFileBuffers(g_VideoReplayDownloadInfo.hInfoSaveFile);
		CloseHandle(g_VideoReplayDownloadInfo.hInfoSaveFile);
		g_VideoReplayDownloadInfo.hInfoSaveFile = INVALID_HANDLE_VALUE;
	}

	memset(&g_VideoReplayDownloadInfo,0,sizeof(g_VideoReplayDownloadInfo));

	g_VideoReplayDownloadInfo.bDownloadFlag = FALSE;
	g_VideoReplayDownloadInfo.hVideoSaveFile = INVALID_HANDLE_VALUE;
	g_VideoReplayDownloadInfo.hInfoSaveFile = INVALID_HANDLE_VALUE;

	return TRUE;
}

BOOL CDlgReplay::SaveVideoFileDataByVideoPlatform(void *pData,int nDataLen)
{
	DWORD dwDataLen = 0;

	try
	{
		if (g_VideoReplayDownloadInfo.bDownloadFlag != FALSE&&g_VideoReplayDownloadInfo.hVideoSaveFile != INVALID_HANDLE_VALUE)
		{
			g_VideoReplayDownloadInfo.nRecvBytes += nDataLen;
			WriteFile(g_VideoReplayDownloadInfo.hVideoSaveFile,pData,(DWORD)nDataLen,&dwDataLen,NULL);
		}
		return TRUE;
	}
	catch(...)
	{

	}

	return FALSE;
}

BOOL CDlgReplay::GetVideoCameraCodeFromRtspUrlByVideoPlatform(char *szRtspUrl,char *szCameraCode)
{
	if (szRtspUrl == NULL||strlen(szRtspUrl) == 0||szCameraCode == NULL)
		return FALSE;

	strcpy(szCameraCode,"000000000000000000");

	char *p = NULL;
	char *q = NULL;

	p = strstr(szRtspUrl,"rtsp://");
	if (p == NULL)
		return FALSE;

	p += strlen("rtsp://");
	p = strstr(p,"/");
	if (p == NULL)
		return FALSE;

	p++;
	p = strstr(p,"/");
	if (p == NULL)
		return FALSE;

	p++;
	q = p;
	q = strstr(p,"/");
	if (p == NULL)
		return FALSE;

	memcpy(szCameraCode,p,q-p);

	return TRUE;
}

BOOL CDlgReplay::VideoReplayDownloadByDirectDevice()
{
	try
	{
		if (g_VideoReplayDownloadInfo.bDownloadFlag != FALSE)
			return TRUE;

		m_SliderPlayPos.SetPos(0);
		g_TotalBytes = 0;
		g_DevType = 0;

		POSITION pos = m_ListCtrl_ReplayFile.GetFirstSelectedItemPosition();

		if (pos == NULL)
			return FALSE;

		int index = m_ListCtrl_ReplayFile.GetNextSelectedItem(pos);

		if (index < 0)
			return FALSE;

		if (g_pMainDlg->m_pDlgRecFilePath == NULL||g_pMainDlg->m_pDlgRecFilePath->DoModal() == IDCANCEL)
			return FALSE;

		strcpy_s(g_ReplaySaveFilepath,sizeof(g_ReplaySaveFilepath),g_pMainDlg->m_pDlgRecFilePath->m_Filepath);

		char szRemoteVideoShowTitle[512] = {0};
		char szStationName[256] = {0};
		char szStationCode[64] = {0};
		char szDeviceName[256] = {0};
		char szCameraName[256] = {0};
		char szCameraCode[64] = {0};
		char szLinkageTime[64] = {0};
		char szWeatherTime[64] = {0};
		BOOL nRet = FALSE;

		if (m_nTableSelectIndex == 4)//平台视频
		{
			m_ListCtrl_ReplayFile.GetItemText(index, 0,szCameraName,sizeof(szCameraName));

			if (strlen(szCameraName) == 0||strcmp(szCameraName,"无数据") == 0)
				return FALSE;

			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{平台视频}-{%s}",szCameraName);

			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);

			memset(szCameraCode,0,sizeof(szCameraCode));
			sprintf(m_RtspURL, "%s", m_ListCtrl_ReplayFile.GetItemText(index, 3));
			m_ListCtrl_ReplayFile.GetItemText(index,5,szCameraCode,sizeof(szCameraCode));
			m_ListCtrl_ReplayFile.GetItemText(index,6,szStationName,sizeof(szStationName));
			m_ListCtrl_ReplayFile.GetItemText(index,7,szStationCode,sizeof(szStationCode));

			g_TotalBytes = atoi(m_ListCtrl_ReplayFile.GetItemText(index, 4));
			g_VideoReplayDownloadInfo.nDecodeTag = g_DevType = m_ListCtrl_ReplayFile.GetItemData(index);

			m_ListCtrl_ReplayFile.GetItemText(index,1,m_szWeatherStartTime,sizeof(m_szWeatherStartTime));
			m_ListCtrl_ReplayFile.GetItemText(index,2,m_szWeatherStopTime,sizeof(m_szWeatherStopTime));
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

			strcpy_s(g_VideoReplayDownloadInfo.szCameraName,sizeof(g_VideoReplayDownloadInfo.szCameraName),szCameraName);
			strcpy_s(g_VideoReplayDownloadInfo.szCameraCode,sizeof(g_VideoReplayDownloadInfo.szCameraCode),szCameraCode);
			strcpy_s(g_VideoReplayDownloadInfo.szStationName,sizeof(g_VideoReplayDownloadInfo.szStationName),szStationName);
			strcpy_s(g_VideoReplayDownloadInfo.szStationCode,sizeof(g_VideoReplayDownloadInfo.szStationCode),szStationCode);
			strcpy_s(g_VideoReplayDownloadInfo.szReplayStartTime,sizeof(g_VideoReplayDownloadInfo.szReplayStartTime),m_szWeatherStartTime);
			strcpy_s(g_VideoReplayDownloadInfo.szReplayStopTime,sizeof(g_VideoReplayDownloadInfo.szReplayStopTime),m_szWeatherStopTime);
		}

		if (m_nTableSelectIndex == ALARM_REPLAY)
		{
			if (strcmp(m_ListCtrl_ReplayFile.GetItemText(index, 2).GetBuffer(),"无数据") == 0)
				return FALSE;

			//弹出选择关联的哪个摄像头播放的窗口
			char szDevName[64] = {0};
			CString strCameraName;
			m_ListCtrl_ReplayFile.GetItemText(index, 2, szDevName, 64);
			int nDevId = GetDevIdByDevName(szDevName);
			CDlgDeviceRelationCamera dlgDeviceRelationCamera(nDevId);
			if (dlgDeviceRelationCamera.DoModal()==IDOK)
			{
				strCameraName = dlgDeviceRelationCamera.m_strCameraName;
			}

			if (strCameraName.IsEmpty())
				return FALSE;

			sprintf_s(szRemoteVideoShowTitle,sizeof(szRemoteVideoShowTitle),"{平台视频}-{%s}",strCameraName.GetBuffer(0));
			m_ch.m_strStation.SetText(szRemoteVideoShowTitle);


			//获取回放视频接口需要的参数
			char szCameraCode[32] = {0};
			GetCameraCodeByCameraName(szCameraCode, strCameraName.GetBuffer(0));

			ASS_CAMERA_INFO tAssCameraInfo;
			memset(&tAssCameraInfo, 0, sizeof(ASS_CAMERA_INFO));
			GetCameraInfoByCameraCode(&tAssCameraInfo, szCameraCode);

			char szAlarmHappenTime[32] = {0};
			m_ListCtrl_ReplayFile.GetItemText(index, 4, szAlarmHappenTime, 32);
			COleDateTime oleAlarmHappenTime;
			oleAlarmHappenTime.ParseDateTime(szAlarmHappenTime);
			SDKTIME start_time;
			SDKTIME stop_time;
			start_time.year = oleAlarmHappenTime.GetYear();
			start_time.month = oleAlarmHappenTime.GetMonth();
			start_time.day = oleAlarmHappenTime.GetDay();
			start_time.hour = oleAlarmHappenTime.GetHour();
			start_time.minute = oleAlarmHappenTime.GetMinute();
			start_time.second = oleAlarmHappenTime.GetSecond();
			stop_time.year = oleAlarmHappenTime.GetYear();
			stop_time.month = oleAlarmHappenTime.GetMonth();
			stop_time.day = oleAlarmHappenTime.GetDay();
			stop_time.hour = oleAlarmHappenTime.GetHour();
			stop_time.minute = oleAlarmHappenTime.GetMinute();
			stop_time.second = oleAlarmHappenTime.GetSecond() + 1;	//时间参数这里注意，比告警发生时间多一秒，即可取到当前这一小时的文件

			//获取回放视频文件
			void * record_info = NULL;
			RECORDITEM *pRecordItem = NULL;
			int nRecordNum = 0;
			bool bResult = GetRecordFileInfo_DevSdk(tAssCameraInfo.tDvrInfo.szDvrIp,
				tAssCameraInfo.tDvrInfo.nDvrPort,
				tAssCameraInfo.tDvrInfo.szDvrUserName,
				tAssCameraInfo.tDvrInfo.szDvrPassword,
				tAssCameraInfo.tDvrInfo.nDvrType,
				tAssCameraInfo.tDvrInfo.nDvrId,
				tAssCameraInfo.nChannel,start_time,stop_time,&record_info,&nRecordNum);
			if (bResult&&record_info != NULL&&nRecordNum > 0)
			{
				pRecordItem = (RECORDITEM *)record_info;
			}
			else
			{
				return FALSE;
			}

			sprintf(m_RtspURL, "%s", pRecordItem[0].FileName);
			m_ListCtrl_ReplayFile.GetItemText(index,1,szStationName,sizeof(szStationName));
			char szStationCode[32] = {0};
			GetStationCodeByStationName(szStationCode, szStationName);

			g_TotalBytes = pRecordItem[0].size;
			g_VideoReplayDownloadInfo.nDecodeTag = g_DevType = tAssCameraInfo.tDvrInfo.nDvrType;

			sprintf(m_szWeatherStartTime, "%s", pRecordItem[0].BeginTime);
			sprintf(m_szWeatherStopTime, "%s", pRecordItem[0].EndTime);
			StartRemoteVideoReplayHistoryWeatherInfo();//历史气象

			strcpy_s(g_VideoReplayDownloadInfo.szCameraName,sizeof(g_VideoReplayDownloadInfo.szCameraName),szCameraName);
			strcpy_s(g_VideoReplayDownloadInfo.szCameraCode,sizeof(g_VideoReplayDownloadInfo.szCameraCode),szCameraCode);
			strcpy_s(g_VideoReplayDownloadInfo.szStationName,sizeof(g_VideoReplayDownloadInfo.szStationName),szStationName);
			strcpy_s(g_VideoReplayDownloadInfo.szStationCode,sizeof(g_VideoReplayDownloadInfo.szStationCode),szStationCode);
			strcpy_s(g_VideoReplayDownloadInfo.szReplayStartTime,sizeof(g_VideoReplayDownloadInfo.szReplayStartTime),m_szWeatherStartTime);
			strcpy_s(g_VideoReplayDownloadInfo.szReplayStopTime,sizeof(g_VideoReplayDownloadInfo.szReplayStopTime),m_szWeatherStopTime);
		}

		m_bPlayRemoteFlag = TRUE;

		g_VideoReplayDownloadInfo.nDownloadType = m_nTableSelectIndex;

		g_VideoReplayDownloadMenuFlag = true;//标志视频下载

		//////////////////////////////////////////////////////////////////////////
		if (g_VideoReplayDownloadMenuFlag != false)
		{
			StartVideoReplayDownloadByDirectDevice();
		}

		//////////////////////////////////////////////////////////////////////////
		m_strPlayState.Format("当前状态：下载中");
		m_NowSpeed = REPLAY_NORMALSPEED;
		m_strPlaySpeed.Format(" ");
		m_btnPlay.SetIndex(1);//暂停
		m_btnPlay.Invalidate(TRUE);
		m_RemotePlayState = REPLAY_PLAY;
		UpdateData(false);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::StartVideoReplayDownloadByDirectDevice()
{
	try
	{
		if (g_VideoReplayDownloadInfo.bDownloadFlag != FALSE)
			return TRUE;

		if (strlen(g_ReplaySaveFilepath) == 0)
		{
			if (g_pMainDlg->m_pDlgRecFilePath == NULL||g_pMainDlg->m_pDlgRecFilePath->DoModal() == IDCANCEL)
				return FALSE;

			strcpy_s(g_ReplaySaveFilepath,sizeof(g_ReplaySaveFilepath),g_pMainDlg->m_pDlgRecFilePath->m_Filepath);
		}

		CTime nowtime = CTime::GetCurrentTime();

		char file_name[256] = {0};
		DWORD dwDataLen = 0;

		sprintf(g_VideoReplayDownloadInfo.szInfoFileName, "dev_%s00%3d%04d%02d%02d%02d%02d%02d.info",
			g_VideoReplayDownloadInfo.szCameraCode,
			g_VideoReplayDownloadInfo.nDecodeTag,
			nowtime.GetYear(), nowtime.GetMonth(), nowtime.GetDay(),
			nowtime.GetHour(), nowtime.GetMinute(), nowtime.GetSecond());

		sprintf(file_name, "%s\\%s",g_ReplaySaveFilepath,g_VideoReplayDownloadInfo.szInfoFileName);

		g_VideoReplayDownloadInfo.hInfoSaveFile = CreateFile(file_name,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (g_VideoReplayDownloadInfo.hInfoSaveFile == INVALID_HANDLE_VALUE)
		{
			g_VideoReplayDownloadInfo.bDownloadFlag = FALSE;
			return FALSE;
		}

		sprintf(g_VideoReplayDownloadInfo.szVideoFileName, "dev_%s00%3d%04d%02d%02d%02d%02d%02d.mp4",
			g_VideoReplayDownloadInfo.szCameraCode,
			g_VideoReplayDownloadInfo.nDecodeTag,
			nowtime.GetYear(), nowtime.GetMonth(), nowtime.GetDay(),
			nowtime.GetHour(), nowtime.GetMinute(), nowtime.GetSecond());

		sprintf(file_name, "%s\\%s",g_ReplaySaveFilepath,g_VideoReplayDownloadInfo.szVideoFileName);

		g_VideoReplayDownloadInfo.hVideoSaveFile = INVALID_HANDLE_VALUE;

		RECORDITEM record_item;
		strcpy_s(record_item.FileName,sizeof(record_item.FileName),m_RtspURL);
		strcpy_s(record_item.BeginTime,sizeof(record_item.BeginTime),m_szDvrRecordStartTime);
		strcpy_s(record_item.EndTime,sizeof(record_item.EndTime),m_szDvrRecordStopTime);
		record_item.size = m_nRecordFileSize;

		//初始化
		bool bResult = StartDownLoadFile_DevSdk(m_szDvrIP,m_nDvrPort,m_szUserName,m_szUserPassword,m_nDvrType,m_nDvrId,m_nCameraChannel,record_item,file_name);
		if(bResult == false)
		{
			CloseHandle(g_VideoReplayDownloadInfo.hInfoSaveFile);
			g_VideoReplayDownloadInfo.hInfoSaveFile = INVALID_HANDLE_VALUE;

			m_strPlayState.Format("当前状态：打开失败，URL错误");
			UpdateData(FALSE);
			return FALSE;
		}

		//表示直连前端设备
		m_nPlayVideoAccessType = 1;

		m_strPlayState.Format("当前状态：下载中");
		m_NowSpeed = REPLAY_NORMALSPEED;
		m_strPlaySpeed.Format(" ");
		m_btnPlay.SetIndex(1);//暂停
		m_btnPlay.Invalidate(TRUE);
		m_RemotePlayState = REPLAY_PLAY;

		g_VideoReplayDownloadInfo.bDownloadFlag = TRUE;

		UpdateData(FALSE);

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

BOOL CDlgReplay::StopVideoReplayDownloadByDirectDevice()
{
	try
	{
		if (g_VideoReplayDownloadInfo.bDownloadFlag == FALSE)
			return TRUE;

		g_VideoReplayDownloadInfo.bDownloadFlag = FALSE;

		if (g_VideoReplayDownloadInfo.hVideoSaveFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(g_VideoReplayDownloadInfo.hVideoSaveFile);
			g_VideoReplayDownloadInfo.hVideoSaveFile = INVALID_HANDLE_VALUE;
		}

		DWORD dwDataLen = 0;

		if (g_VideoReplayDownloadInfo.hInfoSaveFile != INVALID_HANDLE_VALUE)
		{
			WriteFile(g_VideoReplayDownloadInfo.hInfoSaveFile,&g_VideoReplayDownloadInfo,sizeof(g_VideoReplayDownloadInfo),&dwDataLen,NULL);
			FlushFileBuffers(g_VideoReplayDownloadInfo.hInfoSaveFile);
			CloseHandle(g_VideoReplayDownloadInfo.hInfoSaveFile);
			g_VideoReplayDownloadInfo.hInfoSaveFile = INVALID_HANDLE_VALUE;
		}

		memset(&g_VideoReplayDownloadInfo,0,sizeof(g_VideoReplayDownloadInfo));

		g_VideoReplayDownloadInfo.bDownloadFlag = FALSE;
		g_VideoReplayDownloadInfo.hVideoSaveFile = INVALID_HANDLE_VALUE;
		g_VideoReplayDownloadInfo.hInfoSaveFile = INVALID_HANDLE_VALUE;

		bool bResult = StopDownLoadFile_DevSdk(m_nDvrId,m_nCameraChannel);

		m_RemotePlayState = REPLAY_STOP;

		return TRUE;
	}
	catch(...)
	{

	}
	return FALSE;
}

void CDlgReplay::ShowAlarmReplayControl(BOOL bFlag)
{
	GetDlgItem(IDC_EDIT_ALARM_REPLAY_STATION)->EnableWindow(FALSE);
	if (bFlag)
	{
		GetDlgItem(IDC_STATIC_ALARM_STATION)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_ALARM_DEVICE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT_ALARM_REPLAY_STATION)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_COMBO_ALARM_DEVICE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON_ALARM_SELECT)->ShowWindow(SW_SHOW);
	}
	else
	{
		GetDlgItem(IDC_STATIC_ALARM_STATION)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_ALARM_DEVICE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_ALARM_REPLAY_STATION)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_COMBO_ALARM_DEVICE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUTTON_ALARM_SELECT)->ShowWindow(SW_HIDE);
	}
}

void CDlgReplay::OnBnClickedButtonAlarmReplay()
{
	if (m_nTableSelectIndex == ALARM_REPLAY)
		return;

	if (m_RemotePlayState != REPLAY_STOP||m_LocalPlayState != REPLAY_STOP)
		return;

	m_nTableSelectIndex = ALARM_REPLAY;

	GetDlgItem(IDC_STATIC_PT_NODESEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT_SELECTEDID)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_SELECTID)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_PT_RECTYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_RECORDTYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_VIDEO_ACCESS_TYPE)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_D5000_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_STATION_SEL)->ShowWindow(SW_HIDE);
	//	GetDlgItem(IDC_STATIC_D5000_DEV_SEL)->ShowWindow(SW_SHOW);
	//	GetDlgItem(IDC_COMBO_D5000_DEV_SEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC_D5000_EVENTTYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_D5000_EVNETTYPE_SEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_WEATHER_TYPE_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMBO_WEATHER_TYPE_SEL)->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_OP_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT_OP_STATION_SEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_OP_STATION_SEL)->ShowWindow(SW_HIDE);

	//告警回放相关按钮
	ShowAlarmReplayControl(TRUE);

	int nWidth = 1024;
	m_ListCtrl_ReplayFile.DeleteAllItems();
	while(m_ListCtrl_ReplayFile.DeleteColumn(0));

	m_ListCtrl_ReplayFile.InsertColumn(0,"电压等级");
	m_ListCtrl_ReplayFile.SetColumnWidth(0,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(1,"变电站名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(1,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(2,"告警设备名称");
	m_ListCtrl_ReplayFile.SetColumnWidth(2,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(3,"告警类型");
	m_ListCtrl_ReplayFile.SetColumnWidth(3,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(4,"告警发生时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(4,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(5,"告警处理时间");
	m_ListCtrl_ReplayFile.SetColumnWidth(5,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(6,"是否关联视频");
	m_ListCtrl_ReplayFile.SetColumnWidth(6,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(7,"优先级");
	m_ListCtrl_ReplayFile.SetColumnWidth(7,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(8,"处理状态");
	m_ListCtrl_ReplayFile.SetColumnWidth(8,nWidth*0.1);
	m_ListCtrl_ReplayFile.InsertColumn(9,"处理人员");
	m_ListCtrl_ReplayFile.SetColumnWidth(9,nWidth*0.1);

	m_btnSelPlant.SetIndex(1);
	m_btnSelPlant.Invalidate(true);
	m_btnSelD5000.SetIndex(1);
	m_btnSelD5000.Invalidate(true);
	m_btnSelOP.SetIndex(1);
	m_btnSelOP.Invalidate(true);
	m_btnSelWeather.SetIndex(1);
	m_btnSelWeather.Invalidate(true);
	m_btnAlarmReplay.SetIndex(0);
	m_btnAlarmReplay.Invalidate(true);
}

void CDlgReplay::OnBnClickedButtonAlarmSelect()
{
	CDlgStationSelect dlgStationSelect;

	if (dlgStationSelect.DoModal() == IDOK)
	{
		memcpy(&m_tStationNode,&dlgStationSelect.m_tStationNode,sizeof(_T_NODE_INFO));
		GetDlgItem(IDC_EDIT_ALARM_REPLAY_STATION)->SetWindowText(m_tStationNode.node_station);

		//更新combox设备
		if (strcmp(m_tStationNode.node_station,"全部")==0)
		{
			//添加combox控件为所有设备
			CComboBox* pComboAlarmDevice = (CComboBox*)GetDlgItem(IDC_COMBO_ALARM_DEVICE);
			pComboAlarmDevice->ResetContent();
			pComboAlarmDevice->AddString("所有设备");
			pComboAlarmDevice->SetCurSel(0);
			return;
		}
		else
		{
			//设置edit文本并更新combox控件
			GetDlgItem(IDC_EDIT_ALARM_REPLAY_STATION)->SetWindowText(m_tStationNode.node_station);
			UpdateAlarmReplayDeviceCombox(m_tStationNode.node_id);
		}
	}
}

void CDlgReplay::UpdateAlarmReplayDeviceCombox(int nStationId)
{
	CComboBox* pComboAlarmDevice = (CComboBox*)GetDlgItem(IDC_COMBO_ALARM_DEVICE);
	pComboAlarmDevice->ResetContent();
	pComboAlarmDevice->AddString("所有设备");

	char sql_buf[1024] = {0};
	MYSQL_RES *res;
	MYSQL_ROW row ;
	sprintf_s(sql_buf, "select a.name from ass_rvu_sm a where a.rvu_id in (select b.rvu_id from ass_rvu b where b.station_id=%d)",nStationId);
	if (!mysql_query(g_mySqlData, sql_buf))
	{
		res = mysql_store_result(g_mySqlData);

		while (row = mysql_fetch_row(res))
		{
			pComboAlarmDevice->AddString(row[0]);
		}
		mysql_free_result(res) ;
	}
	pComboAlarmDevice->SetCurSel(0);
}

void CDlgReplay::OnNMCustomdrawListReplayfile(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVCUSTOMDRAW lpLVCustomDraw = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	switch(pNMCD->dwDrawStage)
	{
	case CDDS_PREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
		}
		break;
	case CDDS_ITEMPREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
		}
		break;
	case CDDS_ITEMPREPAINT|CDDS_SUBITEM:
		{
			//得到索引
			int nItem = static_cast<int>(pNMCD->dwItemSpec);

			//得到索引的记录文本
			char szLinkageVideo[64] = {0};
			m_ListCtrl_ReplayFile.GetItemText(nItem,6,szLinkageVideo,sizeof(szLinkageVideo));

			//判断
			if (strcmp(szLinkageVideo,"已关联视频") == 0)
			{
				lpLVCustomDraw->clrText = RGB(0,0,0);//设置字体颜色
				lpLVCustomDraw->clrTextBk = RGB(255,0,255);//设置背景色
			}
			else
			{	
				lpLVCustomDraw->clrText = RGB(0,0,0);//设置字体颜色
				lpLVCustomDraw->clrTextBk = RGB(255,128,128);//设置背景色
			}

			*pResult = CDRF_NOTIFYITEMDRAW;
		}
		break;
	}
}
