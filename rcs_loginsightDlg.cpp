
// rcs_loginsightDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "rcs_loginsight.h"
#include "rcs_loginsightDlg.h"
#include "afxdialogex.h"
#include "Resource.h"
#include <string>

#include "rcs_log_analyzer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CrcsloginsightDlg 对话框



CrcsloginsightDlg::CrcsloginsightDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_RCS_LOGINSIGHT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CrcsloginsightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CrcsloginsightDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCANCEL, &CrcsloginsightDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_FILE, &CrcsloginsightDlg::OnBnClickedButtonSelectFile)
	ON_BN_CLICKED(IDOK, &CrcsloginsightDlg::OnBnClickedOk)
	ON_MESSAGE(MSG_LOG_ANALYSIS_DONE, &CrcsloginsightDlg::OnLogAnalysisDone)
END_MESSAGE_MAP()


// CrcsloginsightDlg 消息处理程序
BOOL CrcsloginsightDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}


void update_progress(long progress, const AnalysisResult& result)
{
	if (progress == -1) {
		CString info("Totally ");
		std::string str_result = std::to_string(result.total_lines);
		str_result += " lines, spent ";
		str_result += result.duration;
		str_result += ", the result file is at:\n";
		info.Append(CString(str_result.c_str()));
		info.Append(result.result_file.GetString());

		SetDlgItemTextW(AfxGetApp()->m_pMainWnd->m_hWnd, IDC_STATIC_INFO,
			info.GetString());
		PostMessage(AfxGetApp()->m_pMainWnd->m_hWnd, MSG_LOG_ANALYSIS_DONE,
			0, 0);
		
	} else {
		auto info = "Processing " + std::to_string(progress);
		SetDlgItemTextA(AfxGetApp()->m_pMainWnd->m_hWnd, IDC_STATIC_INFO,
			LPCSTR(info.c_str()));
	}

}

void CrcsloginsightDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CrcsloginsightDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CrcsloginsightDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CrcsloginsightDlg::OnBnClickedCancel()
{
	if (analyzer_thread_.joinable()) {
		auto choice = MessageBox(
			L"Some work is in progress, abort it?",
			L"warning",
			MB_OKCANCEL);
		if (choice == IDCANCEL) {
			return;
		}
		CRcsLogAnalyzer::stop_work();
		analyzer_thread_.detach();
	}
	CDialogEx::OnCancel();
}


void CrcsloginsightDlg::OnBnClickedButtonSelectFile()
{
	CFileDialog dlg_select_log(true, _T("RCS Log Files(*.log)"));
	CString log_file_path;
	if (dlg_select_log.DoModal() == IDOK) {
		log_file_path_ = dlg_select_log.GetPathName();
		SetDlgItemText(IDC_STATIC_LOG_FILE_PATH, log_file_path_);
	}
}


void CrcsloginsightDlg::OnEnChangeEditlogFilePath()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}


void CrcsloginsightDlg::OnBnClickedOk()
{
	if (analyzer_thread_.joinable()) {
		AfxMessageBox(CString("Some job is in progress, please try it later!"));
		return;
	}
	std::thread t(CRcsLogAnalyzer::do_work, log_file_path_, update_progress);
	analyzer_thread_ = std::move(t);
}


LRESULT CrcsloginsightDlg::OnLogAnalysisDone(WPARAM, LPARAM)
{
	if (analyzer_thread_.joinable()) {
		analyzer_thread_.join();
	}
	return 0;
}
