//****************************
//CTablet �^�u���b�g�N���X
//****************************

#include "CTablet.h"

//�擾����^�u���b�g���
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
//���

void CTablet::Release()
{
	//����

	if(m_hTablet)
	{
		m_pWTClose(m_hTablet);
		m_hTablet = NULL;
	}

	//DLL���

	if(m_dll)
	{
		::FreeLibrary(m_dll);
		m_dll = NULL;
	}
}

//--------------------
//������

BOOL CTablet::Init(HWND hWnd)
{
	AXIS axis;
	LOGCONTEXT logc;

	//DLL���[�h

	m_dll = ::LoadLibrary("wintab32.dll");
	if(!m_dll) return FALSE;

	//�֐��|�C���^�擾

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

	//�L����

	if(!m_pWTInfo(0, 0, NULL))
	{
		Release();
		return FALSE;
	}

	//�ő�M���擾

	m_pWTInfo(WTI_DEVICES, DVC_NPRESSURE, &axis);
	m_nMaxPress = axis.axMax;

	//�V�X�e���ݒ�

	m_pWTInfo(WTI_DEFCONTEXT, 0, &logc);

	lstrcpy(logc.lcName, "WinTab test");
	logc.lcOptions	|= CXO_SYSTEM | CXO_MESSAGES;
	logc.lcPktData	= TABLET_PKCUSTOM;
	logc.lcPktMode	= 0;
	logc.lcMoveMask	= TABLET_PKCUSTOM;
	logc.lcBtnUpMask= logc.lcBtnDnMask;

	//�ʒu�ϊ��p�f�[�^

	SetRect(&m_rcIn, logc.lcInOrgX, logc.lcInOrgY, logc.lcInExtX, logc.lcInExtY);
	SetRect(&m_rcOut, logc.lcOutOrgX, logc.lcOutOrgY, logc.lcOutExtX, logc.lcOutExtY);
	SetRect(&m_rcSys, logc.lcSysOrgX, logc.lcSysOrgY, logc.lcSysExtX, logc.lcSysExtY);

	m_szIn.cx = abs(logc.lcInExtX);
	m_szIn.cy = abs(logc.lcInExtY);
	m_szOut.cx = abs(logc.lcOutExtX);
	m_szOut.cy = abs(logc.lcOutExtY);

	//��M�J�n

	m_hTablet = m_pWTOpen(hWnd, &logc, TRUE);
	
	if(!m_hTablet)
	{
		Release();
		return FALSE;
	}
	
	//�L���[�T�C�Y�Z�b�g

	m_pWTQueueSizeSet(m_hTablet, 32);

	return TRUE;
}

//------------------------
//�A�N�e�B�u�ύX��

void CTablet::Active(BOOL bActive)
{
	if(m_hTablet)
	{
		if(bActive) m_pWTOverlap(m_hTablet, TRUE);
	}
}

//--------------------------------------
//�^�u���b�g�ʒu����X�N���[�����W�擾

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

	//�X�N���[�����W��

	dx = (double)xx * m_szOut.cx / m_szIn.cx + m_rcOut.left;
	dy = (double)yy * m_szOut.cy / m_szIn.cy + m_rcOut.top;

	*pX = (int)(dx * m_rcSys.right  / m_szOut.cx + m_rcSys.left);
	*pY = (int)(dy * m_rcSys.bottom / m_szOut.cy + m_rcSys.top);
}
