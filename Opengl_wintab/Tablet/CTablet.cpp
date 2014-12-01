//****************************
//CTablet タブレットクラス
//****************************

#include "CTablet.h"

//取得するタブレット情報
#define TABLET_PKCUSTOM	(PK_CURSOR | PK_BUTTONS | PK_X | PK_Y | PK_NORMAL_PRESSURE)


CTablet::CTablet()
{
	m_dll = NULL;
	m_hTablet = NULL;
}

CTablet::~CTablet()
{
	Release();
}

//---------------
//解放

void CTablet::Release()
{
	//閉じる

	if(m_hTablet)
	{
		m_pWTClose(m_hTablet);
		m_hTablet = NULL;
	}

	//DLL解放

	if(m_dll)
	{
		::FreeLibrary(m_dll);
		m_dll = NULL;
	}
}

//--------------------
//初期化

BOOL CTablet::Init(HWND hWnd)
{
	AXIS axis;
	LOGCONTEXT logc;

	//DLLロード

	m_dll = ::LoadLibrary("wintab32.dll");
	if(!m_dll) return FALSE;

	//関数ポインタ取得

	m_pWTOpen = (WTOpen_)::GetProcAddress(m_dll, "WTOpenA");
	m_pWTClose = (WTClose_)::GetProcAddress(m_dll, "WTClose");
	m_pWTInfo = (WTInfo_)::GetProcAddress(m_dll, "WTInfoA");
	m_pWTOverlap = (WTOverlap_)::GetProcAddress(m_dll, "WTOverlap");
	m_pWTPacket = (WTPacket_)::GetProcAddress(m_dll, "WTPacket");
	m_pWTQueueSizeSet = (WTQueueSizeSet_)::GetProcAddress(m_dll, "WTQueueSizeSet");

	if(m_pWTOpen == NULL || m_pWTClose == NULL || m_pWTInfo == NULL ||
		m_pWTQueueSizeSet == NULL ||
		m_pWTOverlap == NULL || m_pWTPacket == NULL)
	{
		Release();
		return FALSE;
	}

	//有効か

	if(!m_pWTInfo(0, 0, NULL))
	{
		Release();
		return FALSE;
	}

	//最大筆圧取得

	m_pWTInfo(WTI_DEVICES, DVC_NPRESSURE, &axis);
	m_nMaxPress = axis.axMax;

	//システム設定

	m_pWTInfo(WTI_DEFCONTEXT, 0, &logc);

	lstrcpy(logc.lcName, "WinTab test");
	logc.lcOptions	|= CXO_SYSTEM | CXO_MESSAGES;
	logc.lcPktData	= TABLET_PKCUSTOM;
	logc.lcPktMode	= 0;
	logc.lcMoveMask	= TABLET_PKCUSTOM;
	logc.lcBtnUpMask= logc.lcBtnDnMask;

	//位置変換用データ

	SetRect(&m_rcIn, logc.lcInOrgX, logc.lcInOrgY, logc.lcInExtX, logc.lcInExtY);
	SetRect(&m_rcOut, logc.lcOutOrgX, logc.lcOutOrgY, logc.lcOutExtX, logc.lcOutExtY);
	SetRect(&m_rcSys, logc.lcSysOrgX, logc.lcSysOrgY, logc.lcSysExtX, logc.lcSysExtY);

	m_szIn.cx = abs(logc.lcInExtX);
	m_szIn.cy = abs(logc.lcInExtY);
	m_szOut.cx = abs(logc.lcOutExtX);
	m_szOut.cy = abs(logc.lcOutExtY);

	//受信開始

	m_hTablet = m_pWTOpen(hWnd, &logc, TRUE);
	
	if(!m_hTablet)
	{
		Release();
		return FALSE;
	}
	
	//キューサイズセット

	m_pWTQueueSizeSet(m_hTablet, 32);

	return TRUE;
}

//------------------------
//アクティブ変更時

void CTablet::Active(BOOL bActive)
{
	if(m_hTablet)
	{
		if(bActive) m_pWTOverlap(m_hTablet, TRUE);
	}
}

//--------------------------------------
//タブレット位置からスクリーン座標取得

void CTablet::GetSysPos(int x,int y, int *pX,int *pY)
{
	int xx,yy;
	double dx,dy;

	//X

	xx = x - m_rcIn.left;
	if((m_rcOut.right >= 0) != (m_rcIn.right >= 0)) xx = m_szIn.cx - xx;

	//Y

	yy = y - m_rcIn.top;
	if((m_rcOut.bottom >= 0) == (m_rcIn.bottom >= 0)) yy = m_szIn.cy - yy;

	//スクリーン座標へ

	dx = (double)xx * m_szOut.cx / m_szIn.cx + m_rcOut.left;
	dy = (double)yy * m_szOut.cy / m_szIn.cy + m_rcOut.top;

	*pX = (int)(dx * m_rcSys.right  / m_szOut.cx + m_rcSys.left);
	*pY = (int)(dy * m_rcSys.bottom / m_szOut.cy + m_rcSys.top);
}
