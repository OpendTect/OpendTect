/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.cc,v 1.7 2002-04-23 13:31:01 arend Exp $
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
friend class		uiStatusBar;
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


    int			addMsgFld( const char* lbltxt, int stretch )
			{
			    QLabel* msg_ = new QLabel( qthing(), lbltxt );
			    int idx = msgs.size();
			    msgs += msg_;

			    if( lbltxt )
			    {
				QLabel* txtlbl = new QLabel( qthing(), lbltxt );
				msg_->setBuddy( txtlbl );

				qthing()->addWidget( txtlbl );
				txtlbl->setFrameStyle(QFrame::NoFrame);
			    }

			    qthing()->addWidget( msg_, stretch );

			    return idx;
			}

    void		repaint()
			    { 
				qthing()->repaint();
				for( int idx=0; idx<msgs.size(); idx++)
				    if(msgs[idx]) msgs[idx]->repaint();
			     }

protected:

    virtual const QWidget*	managewidg_() const	{ return qwidget(); }

    ObjectSet<QLabel>		msgs;

};


uiStatusBar::uiStatusBar( uiMainWin* parnt, const char* nm, QStatusBar& sb )
	: uiObjHandle( nm, &mkbody(parnt, nm, sb) )
{
}

uiStatusBarBody& uiStatusBar::mkbody( uiMainWin* parnt, const char* nm, 
				      QStatusBar& sb)	
{
    body_= new uiStatusBarBody( *this, parnt, nm, sb );

    sb.setSizeGripEnabled( false ); 

    return *body_; 
}

void uiStatusBar::message( const char* msg, int fldidx ) 
{
    body_->message( msg, fldidx );
    body_->repaint();
    uiMain::theMain().flushX();
}


int uiStatusBar::addMsgFld( const char* lbltxt, const char* tooltip,
			    TxtAlign al, int stretch )
{
    int idx = body_->addMsgFld( lbltxt, stretch );

    setLabelTxt( idx, lbltxt );
    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}

int uiStatusBar::addMsgFld( const char* tooltip,
			    TxtAlign al, int stretch )
{
    int idx = body_->addMsgFld( 0, stretch );

    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}


void uiStatusBar::setToolTip(int idx ,const char* tooltip)
{
    if( idx<0 || idx >= body_->msgs.size() ) return;

    if( tooltip && *tooltip) 
	QToolTip::add( body_->msgs[idx], tooltip );
}


void uiStatusBar::setTxtAlign(int idx ,TxtAlign algn )
{
    if( idx<0 || idx >= body_->msgs.size() ) return;

    int qalgn = 0;
    switch( algn )
    {
	case uiStatusBar::Centre:
	    qalgn = Qt::AlignCenter; break;
	case uiStatusBar::Right:
	    qalgn = Qt::AlignRight; break;
	case uiStatusBar::Left:
	    qalgn = Qt::AlignLeft; break;
    }
    if( qalgn ) body_->msgs[idx]->setAlignment( qalgn );
}

void uiStatusBar::setLabelTxt( int idx,const char* lbltxt )
{
    if( idx<0 || idx >= body_->msgs.size() ) return;

    QLabel* lbl = dynamic_cast<QLabel*>(body_->msgs[idx]->buddy());

    if ( lbl ) lbl->setText( lbltxt );
}
