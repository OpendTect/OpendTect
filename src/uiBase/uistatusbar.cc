/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.cc,v 1.4 2002-01-18 14:27:39 arend Exp $
________________________________________________________________________

-*/

#include "uistatusbar.h"
#include "uimainwin.h"
#include "uimain.h" 
#include "uiparentbody.h" 

#include "uibody.h"

#include <qstatusbar.h> 
#include <qlabel.h> 
#include <qtooltip.h>

class uiStatusBarBody : public uiBodyImpl<uiStatusBar,QStatusBar>
{
public:
                        uiStatusBarBody( uiStatusBar& handle, 
					 uiMainWin* parnt, const char* nm,  
					 QStatusBar& sb) 
			    : uiBodyImpl<uiStatusBar,QStatusBar>
				( handle, parnt, sb )
			{}

    void		message( const char* msg, int idx )
			{ 
			    if( msgs.size() > 0 && msgs[0] )
			    {
				if( idx > 0 && idx < msgs.size() && msgs[idx] )
				    msgs[idx]->setText(msg); 
				else msgs[0]->setText(msg);
			    }
			    else
				qthing()->message(msg);
			}

    void		addMsgFld( const char* tooltip, 
				   uiStatusBar::txtAlign algn,  int stretch )
			{
			    QLabel* msg_ = new QLabel( qthing(), tooltip );

			    if( tooltip && *tooltip) 
				QToolTip::add( msg_, tooltip );

			    msgs += msg_;
			    qthing()->addWidget( msg_, stretch );

			    int qalgn = 0;
			    switch( algn )
			    {
				case uiStatusBar::centre:
				    qalgn = Qt::AlignCenter; break;
				case uiStatusBar::right:
				    qalgn = Qt::AlignRight; break;
				case uiStatusBar::left:
				    qalgn = Qt::AlignLeft; break;
			    }
			    if( qalgn ) msg_->setAlignment( qalgn );
			}

    void		repaint()
			    { qthing()->repaint(); }

protected:

    virtual const QWidget*	managewidg_() const	{ return qwidget(); }

private:

    ObjectSet<QLabel>		msgs;

};


uiStatusBar::uiStatusBar( uiMainWin* parnt, const char* nm, QStatusBar& sb )
: uiObjHandle( nm, &mkbody(parnt, nm, sb) )
{}

uiStatusBarBody& uiStatusBar::mkbody( uiMainWin* parnt, const char* nm, 
				      QStatusBar& sb)	
{
    body_= new uiStatusBarBody( *this, parnt, nm, sb );

//    sb.setBackgroundMode( Qt::PaletteMidlight );
    sb.setSizeGripEnabled( false ); 


    return *body_; 
}

void uiStatusBar::message( const char* msg, int fldidx ) 
{
    body_->message( msg, fldidx );
    body_->repaint();
    uiMain::theMain().flushX();
}


void uiStatusBar::addMsgFld( const char* tooltip, txtAlign al, int stretch )
    { body_->addMsgFld( tooltip, al, stretch ); }


