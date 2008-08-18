// userdlg.cpp : implementation file
//

#include "stdafx.h"
#ifdef	SPRSETW
#define	MYAPPIS	CSprsetwApp
#include "sprsetw.h"
#include "Sprsetw.hpp"
#endif
#ifdef	SPRSERV
#define	MYAPPIS	CSprservApp
#include "pages.h"
#include "xtini.h"
#include "monfile.h"
#include "sprserv.h"
#endif
#include "userdlg.h"
#include "clientif.h"
#include "ulist.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserdlg dialog

CUserdlg::CUserdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUserdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUserdlg)
	m_postuser = "";
	m_mail = FALSE;
	m_write = FALSE;
	m_mattn = FALSE;
	m_wattn = FALSE;
	//}}AFX_DATA_INIT
}

void CUserdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserdlg)
	DDX_CBString(pDX, IDC_USER, m_postuser);
	DDV_MaxChars(pDX, m_postuser, 11);
	DDX_Check(pDX, IDC_MAIL, m_mail);
	DDX_Check(pDX, IDC_WRITE, m_write);
	DDX_Check(pDX, IDC_MATTN, m_mattn);
	DDX_Check(pDX, IDC_WATTN, m_wattn);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CUserdlg, CDialog)
	//{{AFX_MSG_MAP(CUserdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserdlg message handlers

BOOL CUserdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_USER);
	if  (((MYAPPIS *)AfxGetApp())->noulist())
		uP->AddString(((MYAPPIS *)AfxGetApp())->m_username);
	else  {
		UUserList	unixusers;
		const  char  FAR *nu;
        while  (nu = unixusers.nextuser())
			uP->AddString(nu);
	}
	return TRUE;
}

const DWORD a111HelpIDs[] = {
	IDC_USER,	IDH_111_165,	// User & Mail 
	IDC_MAIL,	IDH_111_261,	// User & Mail Mail completion
	IDC_WRITE,	IDH_111_262,	// User & Mail Write completion
	IDC_MATTN,	IDH_111_263,	// User & Mail Mail attention
	IDC_WATTN,	IDH_111_264,	// User & Mail Write attention
	0, 0
};

BOOL CUserdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a111HelpIDs[cnt] != 0;  cnt += 2)
		if  (a111HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a111HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
