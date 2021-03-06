#pragma once

// CDlgPageServer 对话框
#include "SkinButton.h"
#include "Label.h"
#include "DlgPreSet.h"
#include "DlgVideoParSet.h"
#include "ViewChannel.h"
#include "DlgTagClass.h"
#include "BtnST.h"
#include "afxcmn.h"
#include "afxwin.h"

class CDlgPageServer : public CPropertyPage
{
	DECLARE_DYNAMIC(CDlgPageServer)

public:
	CDlgPageServer();
	virtual ~CDlgPageServer();

public:
	BOOL LoadSkin();
	BOOL InitControl();
	BOOL InitTreeList();
	void OnControlButton(UINT nID);// 发送消息到主窗口
	void InitCameraTreelist();// 初始化列表树
	void InitCameraTreelistQABD();// 初始化列表树--青奥保电
	void InitCameraTreelistByVoltage();// 初始化列表树
	void InitCameraTreelistByCity();// 初始化列表树
	void DeleteCameraTreeList();//删除列表树
	void DeleteCameraChildTreeListInfo(HTREEITEM hItem);//删除子列表树
	void DeleteCameraTreeListItemAndDataInfo(HTREEITEM hItem);//删除列表树
	void InitPresetList();// 初始化预置位列表树
	void InitPresetListByVoltage();// 初始化预置位列表树
	void InitPresetListByCity();// 初始化预置位列表树
	void DeletePresetTreeList();//删除预置列表树
	void DeletePresetChildTreeListInfo(HTREEITEM hItem);//删除预置子列表树
	void DeletePresetTreeListItemAndDataInfo(HTREEITEM hItem);//删除列表树
	void InitTuneCycleList();// 初始化轮巡列表树
	void DeleteTuneCycleTreeList();//删除轮巡列表树
	void DeleteTuneCycleChildTreeListInfo(HTREEITEM hItem);//删除轮巡子列表树
	void DeleteAllTreeList();//删除全部列表树
	void SetCameraNodeInfo(HTREEITEM hItem, 
		char* ItemName, char* Itemnum, int StationOrCameraID,int ItemStatus,int Streamless,
		float ItemLongitude, float ItemLatitude, 
		int ItemDecodeTag, int Itemtype, char* stationname);// 设置节点信息
	void SetPresetNodeInfo(HTREEITEM hItem, 
		char* ItemName, char* Itemnum, int StationOrCameraID, char* PresetName, int PresetID, int ItemStatus,int Streamless, 
		float ItemLongitude, float ItemLatitude, 
		int ItemDecodeTag, int Itemtype, char* stationname,
		int line1_from_x = 0,int line1_from_y = 0,int line1_to_x = 0,int line1_to_y = 0,
		int line2_from_x = 0,int line2_from_y = 0,int line2_to_x = 0,int line2_to_y = 0 );// 设置节点信息
	void TreeVisit(HTREEITEM hItem);// 遍历树
	void ReadDBSystemInfo();// 读取数据库信息，包括视频平台信息、变电站信息及节点信息
	BOOL ShowControl(int nFormNum);
	BOOL HideControl(int nFormNum);
	BOOL CameraTreelistAddCameraByStastion(char *szStationName);//在摄像头树形列表中加载指定站点下面的摄像头
	BOOL PresetTreelistAddPresetByStastion(char *szStationName);//在预置位树形列表中加载指定站点下面的摄像头
	BOOL CameraTreelistAddCameraByStastion(int nStationId);//在摄像头树形列表中加载指定站点下面的摄像头
	int    CheckStationCurrentStatus(int nStationId);
	int    CheckStationCurrentStatus2(int nStationId);

	BOOL ReadAndSetRelationListInfo(char *szLinkageTime);
	BOOL ReadAndSetCameraRelationListInfo(int nNodeId,char *szStationName,char *szLinkageTime);

	int  GetRelationListInfoId(char *szLinkageType,char *szStationName,char *szDeviceName,char *szSubType,char *szLinkageTime);

	HTREEITEM SearchCameraTreeCameraHandleTreeItem(HTREEITEM hTreeItem,char *szCameraNum,char *szCameraName,char *szStationName);
	HTREEITEM SearchPresetTreeCameraHandleTreeItem(HTREEITEM hTreeItem,char *szCameraNum,char *szCameraName,char *szStationName);
	HTREEITEM SearchPresetTreePresetHandleTreeItem(HTREEITEM hTreeItem,char *szCameraNum,char *szPresetName);

	HTREEITEM SearchCameraTreeStationHandleTreeItem(HTREEITEM hTreeItem,char *szStationNum,char *szStationName);
	HTREEITEM SearchPresetTreeStationHandleTreeItem(HTREEITEM hTreeItem,char *szStationNum,char *szStationName);

	//设置摄像头状态和图标，0:不可用,1:可用,2:手动标注
	BOOL SetTreeCameraHandleTreeItemAndDatabaseStatus(int nStatus,char *szCameraNum,char *szCameraName,char *szStationName,BOOL bDisplayInfo=TRUE);

	//设置摄像头状态和图标
	BOOL AutoSetTreeCameraHandleTreeItemAndDatabaseStatus(int nStatus,char *szCameraNum,char *szCameraName,char *szStationName,BOOL bDisplayInfo=TRUE);

	BOOL WriteUserClientVideoOperateInfo(char *szCameraNum,char *szCameraName,char *szStationName,int nOperateType,int nOperateResult);

	BOOL OnTreeServerStationFlashInfo(HTREEITEM hTreeItemStation);

	//////////////////////////////////////////////////////////////////////////
	//设置摄像头的DVR信息
	BOOL SetTreeServerCameraDvrInfo(HTREEITEM hTreeItem);
	BOOL SetTreePresetCameraDvrInfo(HTREEITEM hTreeItem);

	//设置站点摄像头DVR信息
	BOOL SetTreeServerStationCameraDvrInfo(HTREEITEM hTreeItem);
	BOOL SetTreePresetStationCameraDvrInfo(HTREEITEM hTreeItem);

	//设置自动轮巡摄像头DVR信息
	BOOL SetTreeTuneCycleStationCameraDvrInfo(HTREEITEM hTreeItem);
	int      GetTreeTuneCycleStationIdByCameraNum(char *szCameraNum);
	BOOL SetTreeTuneCycleStationCameraDvrInfo(HTREEITEM hTreeItem,int nStationId);

	//搜索树形列表
	HTREEITEM SearchTreeServerByInfo(char *szInfo);//搜索摄像机树形列表
	HTREEITEM SearchTreePresetByInfo(char *szInfo);//搜索预置位树形列表
	HTREEITEM SearchTreeTunecycleByInfo(char *szInfo);//搜索自动轮巡树形列表
	HTREEITEM SearchTreeCtrlByInfo(CTreeCtrl *pTreeCtrl,HTREEITEM hTreeItem,char *szInfo);//搜索树形列表
	HTREEITEM SearchTreeCtrlChildItemByInfo(CTreeCtrl *pTreeCtrl,HTREEITEM hTreeItem,char *szInfo);//搜索树形列表下面结点
	HTREEITEM SearchTreeCtrlSiblingItemByInfo(CTreeCtrl *pTreeCtrl,HTREEITEM hTreeItem,char *szInfo);//搜索树形列表下面结点

public:
	//////////////////////////////////////////////////////////////////////////
	CDlgPreSet	*m_DlgPreSet;// 加载的预置位界面
	CDlgVideoParSet	*m_DlgVideoParSet;// 加载的视频参数设置界面

public:
	//////////////////////////////////////////////////////////////////////////
	bool		    m_ytLock;		    // 是否锁定
	DWORD	m_ytLockTime;	// 锁定的时间
	int			m_nRelationListShow;	// 显示的列表是大的还是小的
	BOOL		m_bDragging;
	int			m_preCh;
	int			m_TotalItemNum;
	BOOL m_bOnlyShowAnFang;//是否仅显示安防摄像机，默认为false
	HTREEITEM m_hItemFirstStationNodeForTreeServer;//第一个变电站节点

public:
	_T_NODE_INFO *m_pCameraNodeInfo;			// 摄像头列表中的节点信息
	_T_NODE_PRESET_INFO	*m_pPresetNodeInfo;		// 预置位列表中的节点信息

public:
	CComboBox	m_Combo_YTSpeed;
	CTreeCtrl		m_trServer;
	CTreeCtrl		m_trPreset;
	CTreeCtrl		m_trTuneCycle;
	CToolTipCtrl	m_pPtool;

	CSkinButton   m_btnChoosePreset;
	CSkinButton	m_btnChooseYT;
	CSkinButton	m_btnChooseView;
	CSkinButton	m_btnUp;
	CSkinButton	m_btnDown;
	CSkinButton	m_btnRight;
	CSkinButton	m_btnLeft;
	CSkinButton	m_btnLock;
	CSkinButton	m_btn_ZOOMIN;
	CSkinButton	m_btn_ZOOMOUT;
	CSkinButton	m_btnCall;
	CSkinButton	m_btnSave;
	CSkinButton	m_btDiaphragmL;
	CSkinButton	m_btDiaphragmS;
	CSkinButton	m_btnResurve1;
	CSkinButton	m_btnResurve2;

	CSkinButton m_btnYSOpen;
	CSkinButton m_btnYSClose;
	CSkinButton m_btnJROpen;
	CSkinButton m_btnJRClose;
	CSkinButton m_btnHWOpen;
	CSkinButton m_btnHWClose;
	CSkinButton m_btnDGOpen;
	CSkinButton m_btnDGClose;

	CSkinButton m_btnRelationLargeShow;

	CSkinButton m_btnListDevice;// 一次设备列表
	CSkinButton m_btnListRelation;// 实时联动信息列表
	CSkinButton m_btnListCamera;// 摄像头列表
	CSkinButton m_btnListCycle;// 轮巡组
	CSkinButton m_btnShowHide;//显示隐藏按钮

	CSkinButton m_btnSearch;//搜索按钮
	CString m_strSearch;//搜索编辑框

	CButtonST		m_Form1;
	CButtonST		m_Form4;
	CButtonST        m_Form6;
	CButtonST		m_Form9;
	CButtonST        m_Form16;

	CLabel		m_titlespeed;
	CListCtrl	m_RealRelationList;
	CListCtrl   m_RealCameraRelationList;

public:
	HTREEITEM	m_hSelectItem;
	HTREEITEM	m_hSelectPageItem;
	HTREEITEM	m_hTotalItem[MAXNODENUM];

public:
	//////////////////////////////////////////////////////////////////////////
	CImageList	m_ilTreeIcons;		//树状列表
	CImageList* m_pDragImage;	//For creating and managing the drag-image

	CBitmap   m_PageServerBmp;
	CBitmap   m_PageServerBottomBmp;
	CBitmap   m_YTControlBGBmp;
	CBitmap   m_RightBmp;

	BITMAP    m_RightBm;

public:
// 对话框数据
	enum { IDD = IDD_DIALOG_PAGESERVER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMClickTreeServer(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkTreeServer(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickTreeServer(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkTreePreset(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonChooseyt();
	afx_msg void OnBnClickedButtonChooseview();
	afx_msg void OnBnClickedButtonChoosePreset();
	afx_msg void OnMenuitemReflashlist();
	afx_msg void OnTvnBegindragTreeServer(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedButtonPsDevicelist();
	afx_msg void OnBnClickedButtonPsCameralist();
	afx_msg void OnBnClickedButtonPsRelationlist();
	afx_msg void OnBnClickedButtonRelationShow();
	afx_msg void OnBnClickedButtonPsCycleList();
	afx_msg void OnNMRClickTreeTunecycle(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMenuStartTunecycle();
	afx_msg void OnMenuStopTunecycle();
	afx_msg void OnDestroy();
	afx_msg void OnNMClickListPsRelation(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListPsRelation(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListPsCameraRelation(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkTreeTunecycle(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMenuitemCancelCameraTag();
	afx_msg void OnMenuitemTagCamera();
	afx_msg void OnNMRClickTreePreset(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMenuitemOpenStationCameraVideo();
	afx_msg void OnMenuitemOpenStationCameraVideoAuto();
	afx_msg void OnMenuitemTagClass();
	afx_msg void OnBnClickedButtonSearch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMenuitemViewCameraStatus();
	afx_msg void OnMenuitemSetAnfangCamera();
	afx_msg void OnMenuitemCancelAnfangCamera();
	int UpdateCameraAnFangFlag(char* szCameraCode, int nFlag);
	void SetPageServerItemInfo(HTREEITEM& hInsertItem,int nAnfang_flag,char* pCameraName,int camera_status,int streamless,int diagnosis_flag);
	afx_msg void OnMenuitemOnlyShowAnfangCamera();
	afx_msg void OnMenuitemShowAllCamera();
};

//////////////////////////////////////////////////////////////////////////
extern _T_CAMERA_INFO g_CameraDvrInfo[512];
extern int g_nCameraDvrInfoCount;