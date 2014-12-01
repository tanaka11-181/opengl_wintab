//****************************
//CTablet タブレットクラス
//****************************

#ifndef	_TABLET_CLASS_H_
#define	_TABLET_CLASS_H_

#include <windows.h>

#ifndef WIN32
#define WIN32
#endif

#include "wintab.h"


//-----------------
//関数ポインタ

typedef HCTX (API *WTOpen_)(HWND,LOGCONTEXT*,BOOL);
typedef BOOL (API *WTClose_)(HCTX);
typedef UINT (API *WTInfo_)(UINT,UINT,LPVOID);
typedef BOOL (API *WTOverlap_)(HCTX, BOOL);
typedef BOOL (API *WTPacket_)(HCTX, UINT, LPVOID);
typedef BOOL (API *WTQueueSizeSet_)(HCTX,int);

//----------------------
//取得する情報の構造体

#pragma pack (push,1)
typedef struct
{
	UINT	cursor;	//1:ペン 2:消しゴム
	DWORD	button;
	int		x, y;
	UINT	press;
}TABLETPACKET;
#pragma pack (pop)


//------------------
//クラス

class CTablet
{
	HMODULE	m_dll;			//DLLハンドル
	HCTX	m_hTablet;		//タブレットのハンドル
	int		m_nMaxPress;	//最大筆圧
	RECT	m_rcIn,
			m_rcOut,
			m_rcSys;
	SIZE	m_szIn, m_szOut;

	WTOpen_			m_pWTOpen;
	WTClose_		m_pWTClose;
	WTInfo_			m_pWTInfo;
	WTOverlap_		m_pWTOverlap;
	WTPacket_		m_pWTPacket;
	WTQueueSizeSet_	m_pWTQueueSizeSet;

public:
	CTablet();
	~CTablet();

	void Release();
	BOOL Init(HWND hWnd);

	HCTX GetHandle() { return m_hTablet; }
	int GetMaxPress() { return m_nMaxPress; }

	void Active(BOOL bActive);
	BOOL Packet(WPARAM wParam,LPARAM lParam,TABLETPACKET *p)
	{
		return m_pWTPacket((HCTX)lParam, (UINT)wParam, p);
	}

	void GetSysPos(int x,int y,int *pX,int *pY);
};

#endif
