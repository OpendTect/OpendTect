/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.cc,v 1.3 2001-12-19 11:37:01 arend Exp $
________________________________________________________________________

-*/

#include "uistatusbar.h"
#include "uimainwin.h"
#include "uimain.h" 
#include "uiparentbody.h" 

#include "uibody.h"

#include <qstatusbar.h> 

class uiStatusBarBody : public uiBodyImpl<uiStatusBar,QStatusBar>
{
public:
                        uiStatusBarBody( uiStatusBar& handle, 
					 uiMainWin* parnt, const char* nm,  
					 QStatusBar& sb) 
			    : uiBodyImpl<uiStatusBar,QStatusBar>
				( handle, parnt, sb )
			    {}

    void		message( const char* msg)
			    { qthing()->message(msg); }

    void		repaint()
			    { qthing()->repaint(); }

protected:

    virtual const QWidget*	managewidg_() const	{ return qwidget(); }

};


uiStatusBar::uiStatusBar( uiMainWin* parnt, const char* nm, QStatusBar& sb )
: uiObjHandle( nm, &mkbody(parnt, nm, sb) )
{}

uiStatusBarBody& uiStatusBar::mkbody( uiMainWin* parnt, const char* nm, 
				      QStatusBar& sb)	
{
    body_= new uiStatusBarBody( *this, parnt, nm, sb );

    sb.setBackgroundMode( Qt::PaletteMidlight );
    sb.setSizeGripEnabled( false ); 

    return *body_; 
}

void uiStatusBar::message( const char* msg) 
{
    body_->message( msg );
    body_->repaint();
    uiMain::theMain().flushX();
}
