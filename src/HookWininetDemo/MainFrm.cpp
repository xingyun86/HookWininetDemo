
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "HookWininetDemo.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

CDlgRequest CMainFrame::s_dlgRequest;
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}
BOOL EnableDebugPrivilege(BOOL bEnableDebugPrivilege)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        return FALSE;
    }

    if (!::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
    {
        ::CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnableDebugPrivilege)
    {
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    }
    else
    {
        tp.Privileges[0].Attributes = 0;
    }

    if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
    {
        ::CloseHandle(hToken);
        return FALSE;
    }

    ::CloseHandle(hToken);

    if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        return FALSE;
    }
    return TRUE;
}
LSTATUS SetCurrentIECoreVersion(DWORD dwIEVersion=11000)
{
	HKEY hKey = NULL;
	DWORD dwDisposition = 0;
	WCHAR* wRegPath = L"SOFTWARE\\Microsoft\\Internet Explorer\\MAIN\\FeatureControl\\FEATURE_BROWSER_EMULATION";
	WCHAR wAppName[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, wAppName, sizeof(wAppName) / sizeof(*wAppName));
	WCHAR* wValueName = wcsrchr(wAppName, L'\\') + 1;
	LSTATUS lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wRegPath, 0, REG_LEGAL_OPTION, &hKey);
	if (lStatus != ERROR_SUCCESS)
	{
		lStatus = RegCreateKeyExW(HKEY_LOCAL_MACHINE, wRegPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition);
	}
	if (lStatus == ERROR_SUCCESS)
	{
		lStatus = RegSetValueExW(hKey, wValueName, NULL, REG_DWORD, (BYTE*)&dwIEVersion, sizeof(dwIEVersion));
		lStatus = RegCloseKey(hKey);
	}
	return lStatus;
}
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

    EnableDebugPrivilege(TRUE);
	SetCurrentIECoreVersion();
	s_dlgRequest.Create( CDlgRequest::IDD, this);
	s_dlgRequest.CenterWindow(this);
	s_dlgRequest.ShowWindow(SW_SHOW);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers
