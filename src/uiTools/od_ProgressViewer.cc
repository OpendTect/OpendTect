/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          August 2001
 RCS:           $Id: od_ProgressViewer.cc,v 1.2 2002-01-29 11:12:28 bert Exp $
________________________________________________________________________

-*/

#include "helpview.h"
#include "timer.h"
#include "uimenu.h"
#include "uitextedit.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uigroup.h"
#include "uistatusbar.h"
#include "prog.h"
#include "sighndl.h"
#include <iostream.h>
#include <ctype.h>


class uiProgressViewer : public uiMainWin
{
public:

    		uiProgressViewer(uiParent*,istream&,int);
    		~uiProgressViewer();

protected:

    void	quitFn(CallBacker*);
    void	helpFn(CallBacker*);

    istream&	strm;
    uiTextEdit*	txtfld;
    uiMenuItem*	quitmi;
    Timer*	tim;
    int		ppid;
    int		sz;
    char	fullline[81];

    void	doWork(CallBacker*);
    void	update(char*,int&);
};


uiProgressViewer::uiProgressViewer( uiParent* p, istream& s, int i )
	: uiMainWin(p,"Progress",1)
	, tim(0)
	, strm(s)
	, ppid(i)
	, sz(0)
{
    topGroup()->setBorder(0);
    topGroup()->setSpacing(0);

    uiPopupMenu* popmnu = new uiPopupMenu( this, "&File" );
    menuBar()->insertItem( popmnu );
    uiMenuItem* mi = new uiMenuItem( "Help", mCB(this,uiProgressViewer,helpFn));
    popmnu->insertItem( mi );
    popmnu->insertSeparator();
    quitmi = new uiMenuItem( ppid ? "Stop process and &Quit" : "&Quit",
	    		 mCB(this,uiProgressViewer,quitFn) );
    popmnu->insertItem( quitmi );

    txtfld = new uiTextEdit( this, "", true );
    txtfld->setPrefHeightInChar(6);

    tim = new Timer( "Progress" );
    tim->tick.notify( mCB(this,uiProgressViewer,doWork) );
    tim->start( 50, true );
}


uiProgressViewer::~uiProgressViewer()
{
    delete tim;
}


void uiProgressViewer::doWork( CallBacker* )
{
    if ( strm.eof() || strm.fail() )
    {
	if ( sz )
	    txtfld->append( fullline );
	fullline[0] = '\0';
	statusBar()->message( fullline );
	uiMain::theMain().flushX();
	quitmi->setText( "&Quit" );
	ppid = 0;
	return;
    }

    char c = strm.peek();
    strm.ignore( 1 );
    if (  c == '\n' || isprint(c) )
    {
	if ( c == '\n' || sz == 80 )
	{
	    txtfld->append( fullline );
	    sz = 0;
	}

	if ( c != '\n' && isprint(c) )
	    fullline[sz++] = c;
	fullline[sz] = '\0';
	statusBar()->message( fullline );
	uiMain::theMain().flushX();
    }

    tim->start( 1, true );
}


void uiProgressViewer::quitFn( CallBacker* )
{
    if ( ppid )
	SignalHandling::stopProcess( ppid );
    uiMain::theMain().exit(0);
}


void uiProgressViewer::helpFn( CallBacker* )
{
    HelpViewer::use( 0 );
}


int main( int argc, char** argv )
{
    uiMain app( argc, argv );
    int ppid = argc > 1 ? atoi(argv[1]) : 0;
    uiProgressViewer* pv = new uiProgressViewer( 0, cin, ppid );

    app.setTopLevel( pv );
    pv->show();
    return app.exec();
}


