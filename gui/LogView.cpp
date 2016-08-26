// LogView.cpp : implementation file
//

#include "LogView.h"
#include "..\core\log.h"
#include "stdafx.h"
#include <stdio.h>
#include <direct.h>



/////////////////////////////////////////////////////////////////////////////
// CLogView


IMPLEMENT_DYNCREATE( CLogView, CRichEditView )

BEGIN_MESSAGE_MAP( CLogView, CRichEditView )
	//{{AFX_MSG_MAP(CLogView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()




CLogView::CLogView()
{
}




CLogView::~CLogView()
{
}




#ifdef _DEBUG

void CLogView::AssertValid() const
{
	CRichEditView::AssertValid();
}


void CLogView::Dump(CDumpContext& dc) const
{
	CRichEditView::Dump(dc);
}

#endif 




BOOL CLogView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// want view to be read-only
	BOOL rc = CRichEditView::PreCreateWindow(cs);

	cs.style = AFX_WS_DEFAULT_VIEW | WS_VSCROLL
			    | ES_AUTOHSCROLL | ES_AUTOVSCROLL 
			    | ES_MULTILINE | ES_READONLY;

	return rc;
}




void CLogView::AddMessage( CString msg, COLORREF color )
{
	// add message to list
	msg += "\r\n";
		
	CRichEditCtrl& ctrl = GetRichEditCtrl();

	// will append message at end position
	int len =  ctrl.GetTextLength();
	ctrl.SetSel( len, len );

	CHARFORMAT2A cf;

	// determine any text attributes depending on indicator flag
	if ( color != 0xffffffff )
	{
		cf.cbSize = sizeof CHARFORMAT2A;
		cf.dwMask = CFM_COLOR;
		cf.crTextColor = color;	
		cf.dwEffects = ! CFE_AUTOCOLOR;
	}
	else
	{
		cf.cbSize = sizeof CHARFORMAT2A;
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = CFE_AUTOCOLOR;
	}

	SetCharFormat( cf );

	// append message
	ctrl.ReplaceSel( msg );

	// scroll one line down and refresh window
	ctrl.LineScroll( 1 );
	UpdateWindow();
}




void CLogView::clear()
{
	CRichEditCtrl& ctrl = GetRichEditCtrl();

	// select all
	ctrl.SetSel( 0, -1 );
	ctrl.ReplaceSel( "" );
}




void CLogView::saveToFile()
{
	CRichEditCtrl& ctrl = GetRichEditCtrl();


	char filename[100];
	static CTime now = CTime::GetCurrentTime();
	sprintf( filename, "trace_%s.txt",
			 now.Format( "%Y%m%d_%H%M%S" ) );


	// CFileDialog has a stupid side-effect when saving a file
	// that causes the current directory to be changed. So we
	// have to save the current directory and restore it back.
	char cur_dir[ _MAX_PATH ];
	_getcwd( cur_dir, _MAX_PATH );
	

	CFileDialog dlg( FALSE, 
					 ".txt", 
					 filename, 
					 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					 NULL,
					 this );
	if ( dlg.DoModal() == IDCANCEL )
		return;


	BeginWaitCursor();
	{
		CString path_name = dlg.GetPathName();

		FILE* f = fopen( path_name, "w" );

		int lines = ctrl.GetLineCount();

		// save contents of log window, line by line...
		const int MAX_LINE_LENGTH = 200;
		char buffer[ MAX_LINE_LENGTH + 1 ];
		for ( int k = 0; k < lines; ++k )
		{
			int copied = ctrl.GetLine( k, buffer, MAX_LINE_LENGTH );
			buffer[ copied ] = 0;

			fprintf( f, "%s", buffer );
		}

		fclose( f );

		LOG( Msg::TRACE_SAVED << path_name.operator LPCTSTR() );


		// CFileDialog has changed the local directory. So we
		// restore it back.
		_chdir( cur_dir );
	}
	EndWaitCursor();
}



