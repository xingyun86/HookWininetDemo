
// HookWininetDemo.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "HookWininetDemo.h"
#include "MainFrm.h"

#include "HookWininetDemoDoc.h"
#include "HookWininetDemoView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CHookWininetDemoApp

BEGIN_MESSAGE_MAP(CHookWininetDemoApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CHookWininetDemoApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
END_MESSAGE_MAP()


// CHookWininetDemoApp construction

CHookWininetDemoApp::CHookWininetDemoApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CHookWininetDemoApp object

CHookWininetDemoApp theApp;

#include <winsvc.h>
#include <wtsapi32.h>
#pragma comment(lib, "wtsapi32.lib")
#include <userenv.h>
#pragma comment(lib,"userenv")
#include <tlhelp32.h>

#include <string>

class SvcMgr {
public:
    SvcMgr() {
        m_strProgramName.resize(MAX_PATH, '\0');
        GetModuleFileNameA(NULL, (LPSTR)m_strProgramName.data(), m_strProgramName.size());
        m_strProgramName = m_strProgramName.c_str();
        m_strRootPath = m_strProgramName.substr(0, m_strProgramName.rfind('\\'));
        m_strServiceName = m_strProgramName.substr(m_strProgramName.rfind('\\') + 1);
        m_strSingleInstanceMutexName = ("__" + m_strServiceName + "__SINGLE_INSTANCE_MUTEX__");
    }
    virtual ~SvcMgr() {}
public:
    std::string m_strRootPath = ("");
    std::string m_strProgramName = ("");
    std::string m_strServiceName = ("");
    std::string m_strSingleInstanceMutexName = ("");
public:
    SERVICE_STATUS m_ServiceStatus = { 0 };
    SERVICE_STATUS_HANDLE m_hServiceStatusHandle = NULL;
public:
    //通用版将wstring转化为string
    std::string W_To_A(const std::wstring& ws, unsigned int cp = CP_ACP)
    {
        if (!ws.empty())
        {
            std::string s(WideCharToMultiByte(cp, 0, ws.data(), -1, NULL, 0, NULL, NULL), ('\0'));
            return s.substr(0, WideCharToMultiByte(cp, 0, ws.c_str(), -1, (LPSTR)s.data(), (int)s.size(), NULL, NULL) - 1);
        }
        return ("");
    }
    //通用版将string转化为wstring
    std::wstring A_To_W(const std::string& s, unsigned int cp = CP_ACP)
    {
        if (!s.empty())
        {
            std::wstring ws(MultiByteToWideChar(cp, 0, s.data(), -1, NULL, 0), (L'\0'));
            return ws.substr(0, MultiByteToWideChar(cp, 0, s.data(), -1, (LPWSTR)ws.data(), (int)ws.size()) - 1);
        }
        return (L"");
    }
    //  ANSI to Unicode
    std::wstring AToW(const std::string& s)
    {
        return A_To_W(s);
    }
    //Unicode to ANSI
    std::string WToA(const std::wstring& ws)
    {
        return W_To_A(ws);
    }
#if !defined(UNICODE) && !defined(_UNICODE)
#define AToT(X) X
#define WToT(X) WToA(X)
#define TToA(X) X
#define TToW(X) AToW(X)
#else
#define AToT(X) AToW(X)
#define WToT(X) X
#define TToA(X) WToA(X)
#define TToW(X) X
#endif
public:
    BOOL CreateProcessWithUI(LPCTSTR lpProcessName, LPCTSTR lpCommandLine, LPPROCESS_INFORMATION process)
    {
        BOOL bResult = FALSE;
        HANDLE hToken = NULL;
        STARTUPINFO si = { 0 };
        DWORD dwSessionId = 0L;
        HANDLE hTokenDuplicate = NULL;

        if (!lpProcessName || !(*lpProcessName))
        {
            goto __LEAVE_CLEAN__;
        }

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
        {
            goto __LEAVE_CLEAN__;
        }

        if (!DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityAnonymous, TokenPrimary, &hTokenDuplicate))
        {
            goto __LEAVE_CLEAN__;
        }

        dwSessionId = WTSGetActiveConsoleSessionId();
        if (!SetTokenInformation(hTokenDuplicate, TokenSessionId, &dwSessionId, sizeof(dwSessionId)))
        {
            CloseHandle(hToken);
            CloseHandle(hTokenDuplicate);
            return FALSE;
        }

        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.wShowWindow = SW_SHOW;
        si.dwFlags = STARTF_USESHOWWINDOW;
        if (!CreateProcessAsUser(hTokenDuplicate, lpProcessName, (LPTSTR)lpCommandLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, process))
        {
            goto __LEAVE_CLEAN__;
        }

        bResult = TRUE;

    __LEAVE_CLEAN__:

        if (hToken != NULL)
        {
            CloseHandle(hToken);
            hToken = NULL;
        }
        if (hTokenDuplicate != NULL)
        {
            CloseHandle(hTokenDuplicate);
            hTokenDuplicate = NULL;
        }

        return bResult;
    }
    void ExecuteProgram()
    {
        PROCESS_INFORMATION pi = { 0 };
        if (CreateProcessWithUI(AToT(m_strProgramName).c_str(), TEXT(" --program --execute"), &pi) && (pi.hProcess != NULL))
        {
            CloseHandle(pi.hProcess);
        }
    }

    static void WINAPI ServiceHandler(DWORD fdwControl)
    {
        switch (fdwControl)
        {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
        {
            SvcMgr::Inst()->m_ServiceStatus.dwWin32ExitCode = 0;
            SvcMgr::Inst()->m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SvcMgr::Inst()->m_ServiceStatus.dwCheckPoint = 0;
            SvcMgr::Inst()->m_ServiceStatus.dwWaitHint = 0;
            //add you quit code here
        }
        break;
        default:
        {
            return;
        }
        break;
        };
        if (!SetServiceStatus(SvcMgr::Inst()->m_hServiceStatusHandle, &SvcMgr::Inst()->m_ServiceStatus))
        {
            return;
        }
    }

    static void WINAPI ServiceMain(int argc, char** argv)
    {
        SvcMgr::Inst()->m_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;// SERVICE_WIN32;
        SvcMgr::Inst()->m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
        SvcMgr::Inst()->m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
        SvcMgr::Inst()->m_ServiceStatus.dwWin32ExitCode = 0;
        SvcMgr::Inst()->m_ServiceStatus.dwServiceSpecificExitCode = 0;
        SvcMgr::Inst()->m_ServiceStatus.dwCheckPoint = 0;
        SvcMgr::Inst()->m_ServiceStatus.dwWaitHint = 0;
        SvcMgr::Inst()->m_hServiceStatusHandle = RegisterServiceCtrlHandler(SvcMgr::Inst()->AToT(SvcMgr::Inst()->m_strServiceName).c_str(), ServiceHandler);
        if (SvcMgr::Inst()->m_hServiceStatusHandle == NULL)
        {
            return;
        }

        //add your service thread here
        SvcMgr::Inst()->ExecuteProgram();

        // Initialization complete - report running status 
        SvcMgr::Inst()->m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        SvcMgr::Inst()->m_ServiceStatus.dwCheckPoint = 0;
        SvcMgr::Inst()->m_ServiceStatus.dwWaitHint = 9000;
        if (!SetServiceStatus(SvcMgr::Inst()->m_hServiceStatusHandle, &SvcMgr::Inst()->m_ServiceStatus))
        {
            return;
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    VOID MySvcInstall()
    {
        SC_HANDLE schSCManager;
        SC_HANDLE schService;
        std::string m_strBinPath = m_strProgramName + " --service";

        // Get a handle to the SCM database. 
        schSCManager = OpenSCManager(
            NULL,                    // local computer
            NULL,                    // ServicesActive database 
            SC_MANAGER_ALL_ACCESS);  // full access rights 
        if (NULL == schSCManager)
        {
            printf("OpenSCManager failed (%d)\n", GetLastError());
            return;
        }
        // Create the service
        schService = CreateService(
            schSCManager,              // SCM database 
            AToT(m_strServiceName).c_str(),                   // name of service 
            AToT(m_strServiceName).c_str(),                   // service name to display 
            SERVICE_ALL_ACCESS,        // desired access 
            SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, // service type 
            SERVICE_DEMAND_START,      // start type 
            SERVICE_ERROR_NORMAL,      // error control type 
            AToT(m_strBinPath).c_str(),                    // path to service's binary 
            NULL,                      // no load ordering group 
            NULL,                      // no tag identifier 
            NULL,                      // no dependencies 
            NULL,                      // LocalSystem account 
            NULL);                     // no password 
        if (schService == NULL)
        {
            printf("CreateService failed (%d)\n", GetLastError());
            CloseServiceHandle(schSCManager);
            return;
        }
        StartService(schService, 0, NULL);
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    }
    void MySvcRemove()
    {
        SC_HANDLE schSCManager;
        SC_HANDLE schService;
        // Get a handle to the SCM database. 
        schSCManager = OpenSCManager(
            NULL,                    // local computer
            NULL,                    // ServicesActive database 
            SC_MANAGER_ALL_ACCESS);  // full access rights 
        if (NULL == schSCManager)
        {
            printf("OpenSCManager failed (%d)\n", GetLastError());
            return;
        }
        // Create the service
        schService = OpenService(
            schSCManager,              // SCM database 
            AToT(m_strServiceName).c_str(),                   // name of service
            SERVICE_ALL_ACCESS        // desired access
        );
        if (schService == NULL)
        {
            printf("CreateService failed (%d)\n", GetLastError());
            CloseServiceHandle(schSCManager);
            return;
        }
        SERVICE_STATUS ss = { 0 };
        ControlService(schService, SERVICE_CONTROL_STOP, &ss);
        if (DeleteService(schService) == FALSE)
        {
            printf("Service remove failed (%d)\n", GetLastError());
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    }
public:
    bool Run()
    {
        // TODO: Place code here.
        INT nArgvNum = 0;
        LPWSTR lpwCmdLine = GetCommandLineW();
        CommandLineToArgvW(lpwCmdLine, &nArgvNum);
        if (nArgvNum == 1)
        {
            HANDLE hMutex = CreateMutex(NULL, FALSE, AToT(m_strSingleInstanceMutexName).c_str());
            if (hMutex == NULL)
            {
                return false;
            }

            //如果程序已经存在并且正在运行
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                SetCurrentDirectory(AToT(m_strRootPath).c_str());
                //卸载服务
                MySvcRemove();
                //安装服务并启动服务
                MySvcInstall();
            }
            CloseHandle(hMutex);
        }
        else if (nArgvNum == 2)
        {
            SERVICE_TABLE_ENTRY ServiceTable[2] = { 0 };

            ServiceTable[0].lpServiceName = (LPTSTR)(AToT(m_strServiceName).c_str());
            ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

            ServiceTable[1].lpServiceName = NULL;
            ServiceTable[1].lpServiceProc = NULL;
            // 启动服务的控制分派机线程
            StartServiceCtrlDispatcher(ServiceTable);
        }
        else if (nArgvNum == 3)
        {
            SvcMgr::Inst()->MySvcRemove();
            return true;
        }
        else
        {
            SetCurrentDirectory(AToT(m_strRootPath).c_str());
            //卸载服务
            MySvcRemove();
        }
        return false;
    }
public:
    static SvcMgr* Inst()
    {
        static SvcMgr SvcMrgInstance;
        return &SvcMrgInstance;
    }
};
// CHookWininetDemoApp initialization

BOOL CHookWininetDemoApp::InitInstance()
{
    /*if (SvcMgr::Inst()->Run() == false)
    {
        return (-1);
    }*/
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CHookWininetDemoDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CHookWininetDemoView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);


	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CHookWininetDemoApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CHookWininetDemoApp message handlers



