/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          August 2001
 RCS:           $Id: od_ProgressViewer.cc,v 1.1 2002-01-29 07:54:05 bert Exp $
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
#include <iostream.h>


class uiProgressViewer : public uiMainWin
{
public:

    		uiProgressViewer(uiParent*,istream&,bool);
    		~uiProgressViewer();

protected:

    void	quitFn(CallBacker*);
    void	helpFn(CallBacker*);

    istream&	strm;
    uiTextEdit*	txtfld;
    uiMenuItem*	quitmi;
    Timer*	tim;
    bool	delayed;
    bool	first_time;
    BufferString fullline;

    void	startWork();
    void	doWork(CallBacker*);
    void	update(char*,int&);
};


uiProgressViewer::uiProgressViewer( uiParent* p, istream& s, bool b )
	: uiMainWin(p,"Progress",1)
	, tim(0)
	, strm(s)
	, delayed(b)
	, first_time(false)
{
    topGroup()->setBorder(0);
    topGroup()->setSpacing(0);

    uiPopupMenu* popmnu = new uiPopupMenu( this, "&File" );
    menuBar()->insertItem( popmnu );
    uiMenuItem* mi = new uiMenuItem( "Help", mCB(this,uiProgressViewer,helpFn));
    popmnu->insertItem( mi );
    popmnu->insertSeparator();
    quitmi = new uiMenuItem( "&Quit / Stop process",
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


void uiProgressViewer::startWork()
{
}


void uiProgressViewer::doWork( CallBacker* )
{
    if ( first_time )
    {
	first_time = false;
	tim->start( 1 , true );
	return;
    }

    if ( strm.eof() || strm.fail() )
    {
	quitmi->setText( "&Quit" );
	return;
    }

    static char buf[80];
    int endnr = 0;
    char c;
    while ( (c = strm.peek()) != EOF )
    {
	buf[endnr] = c; endnr++;

	if ( !delayed || isspace(c) || endnr == 79 )
	    update( buf, endnr );

	strm.ignore( 1 );
    }
    update( buf, endnr );

    tim->start( 1 , true );
}


void uiProgressViewer::update( char* buf, int& nr )
{
    if ( !nr ) return;

    buf[nr] = '\0';
    if ( buf[nr-1] != '\n' )
	fullline += buf;
    else
    {
	buf[nr-1] = '\0';
	txtfld->append( fullline );
        fullline = buf;
    }

    statusBar()->message( fullline );
    uiMain::theMain().flushX();
    nr = 0;
}


void uiProgressViewer::quitFn( CallBacker* )
{
    uiMain::theMain().exit(0);
}


void uiProgressViewer::helpFn( CallBacker* )
{
    HelpViewer::use( 0 );
}


int main( int argc, char** argv )
{
    uiMain app( argc, argv );
    bool delayed = argc > 1 && !strcmp(argv[1],"+D");
    uiProgressViewer* pv = new uiProgressViewer( 0, cin, delayed );

    app.setTopLevel( pv );
    pv->show();
    return app.exec();
}


