
// rcs_loginsightDlg.h: 头文件
//

#pragma once
#include <thread>

struct AnalysisResult;

#define MSG_LOG_ANALYSIS_DONE	(WM_USER+100) 
void update_progress(long progress, const AnalysisResult&);

// CrcsloginsightDlg 对话框
class CrcsloginsightDlg : public CDialogEx
{
// 构造
public:
	CrcsloginsightDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RCS_LOGINSIGHT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonSelectFile();
	afx_msg void OnEnChangeEditlogFilePath();
	afx_msg LRESULT OnLogAnalysisDone(WPARAM, LPARAM);

private:
	CString log_file_path_;
	std::thread analyzer_thread_;
public:
	afx_msg void OnBnClickedOk();
};
