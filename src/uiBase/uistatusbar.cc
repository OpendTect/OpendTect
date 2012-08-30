/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistatusbar.cc,v 1.24 2012-08-30 07:52:52 cvsnageswara Exp $";

#include "uistatusbar.h"
#include "uimainwin.h"
#include "uimain.h" 
#include "uiparentbody.h" 

#include "uibody.h"

#include <qstatusbar.h> 
#include <qlabel.h> 
#include <qtooltip.h>

class uiStatusBarBody : public uiBodyImpl<uiStatusBar,mQtclass(QStatusBar)>
{
friend class		uiStatusBar;
public:
                        uiStatusBarBody( uiStatusBar& hndl, 
					 uiMainWin* parnt, const char* nm,  
					 mQtclass(QStatusBar&) sb) 
			    : uiBodyImpl<uiStatusBar,mQtclass(QStatusBar)>
				( hndl, parnt, sb )
			{}

    void		message( const char* msg, int idx, int msecs )
			{ 
			    if ( !msgs.isEmpty() )
			    {
#ifdef __debug__
				if ( msecs >= 0 )
				    pErrMsg("No auto-erase for SB with fields");
#endif
				if ( idx > 0 && idx < msgs.size() && msgs[idx] )
				    msgs[idx]->setText(msg); 
				else msgs[0]->setText(msg);
			    }
			    else if ( msg && *msg )
				qthing()->showMessage( msg, msecs<0?0:msecs );
			    else
				qthing()->clearMessage();
			}

    void		setBGColor( int idx, const Color& col )
			{ 
			    mQtclass(QWidget*) widget = 0;
			    if ( msgs.size()>0 && msgs[0] )
			    {
				if ( idx>0 && idx<msgs.size() && msgs[idx] )
				    widget = msgs[idx];
				else widget = msgs[0];
			    }
			    else 
				widget = qthing();

			    const mQtclass(QColor) qcol(col.r(),col.g(),
				    			col.b());
			    mQtclass(QPalette) palette;
			    palette.setColor( widget->backgroundRole(), qcol );
			    widget->setPalette(palette);
			}

    Color		getBGColor( int idx )
			{
			    const mQtclass(QWidget*) widget = 0;
			    if ( msgs.size()>0 && msgs[0] )
			    {
				if ( idx>0 && idx<msgs.size() && msgs[idx] )
				    widget = msgs[idx];
				else widget = msgs[0];
			    }
			    else 
				widget = qthing();

			    const mQtclass(QBrush&) qbr =
				widget->palette().brush(
						     widget->backgroundRole() );
			    const mQtclass(QColor&) qc = qbr.color();
			    return Color( qc.red(), qc.green(), qc.blue() );
			}

    int			addMsgFld( const char* lbltxt, int stretch )
			{
			    mQtclass(QLabel*) msg_ =
						new mQtclass(QLabel)( lbltxt );
			    int idx = msgs.size();
			    msgs += msg_;

			    if ( lbltxt )
			    {
				mQtclass(QLabel*) txtlbl =
				    		new mQtclass(QLabel)( lbltxt );
				msg_->setBuddy( txtlbl );

				qthing()->addWidget( txtlbl );
				txtlbl->setFrameStyle(mQtclass(QFrame)::NoFrame);
			    }

			    qthing()->addWidget( msg_, stretch );
			    return idx;
			}

    void		repaint()
			    { 
				qthing()->repaint();
				for( int idx=0; idx<msgs.size(); idx++)
				    if (msgs[idx]) msgs[idx]->repaint();
			     }

protected:

    virtual const mQtclass(QWidget*)	managewidg_() const
    							{ return qwidget(); }

    ObjectSet<mQtclass(QLabel)>		msgs;

};


uiStatusBar::uiStatusBar( uiMainWin* parnt, const char* nm,
			  mQtclass(QStatusBar&) sb )
    : uiBaseObject(nm,&mkbody(parnt,nm,sb))
{
}


uiStatusBar::~uiStatusBar()
{
    delete body_;
}


uiStatusBarBody& uiStatusBar::mkbody( uiMainWin* parnt, const char* nm, 
				      mQtclass(QStatusBar&) sb)	
{
    body_= new uiStatusBarBody( *this, parnt, nm, sb );

#ifndef __mac__ //TODO: Bugfix for gripper on Mac
    sb.setSizeGripEnabled( false ); 
#endif

    return *body_; 
}


void uiStatusBar::message( const char* msg, int fldidx, int msecs ) 
{
    body_->message( msg, fldidx, msecs );
    body_->repaint();
    uiMain::theMain().flushX();
}


void uiStatusBar::setBGColor( int fldidx, const Color& col )
{
    body_->setBGColor( fldidx, col );
}


Color uiStatusBar::getBGColor( int fldidx ) const
{
    return body_->getBGColor( fldidx );
}


int uiStatusBar::addMsgFld( const char* lbltxt, const char* tooltip,
			    Alignment::HPos al, int stretch )
{
    int idx = body_->addMsgFld( lbltxt, stretch );

    setLabelTxt( idx, lbltxt );
    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}

int uiStatusBar::addMsgFld( const char* tooltip,
			    Alignment::HPos al, int stretch )
{
    int idx = body_->addMsgFld( 0, stretch );

    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}


void uiStatusBar::setToolTip( int idx, const char* tooltip )
{
    if ( ! body_->msgs.validIdx(idx) ) return;

    if ( tooltip && *tooltip && body_->msgs[idx] )
	body_->msgs[idx]->setToolTip( tooltip );
}


void uiStatusBar::setTxtAlign( int idx, Alignment::HPos hal )
{
    if ( ! body_->msgs.validIdx(idx) ) return;

    Alignment al( hal );
    body_->msgs[idx]->setAlignment( (mQtclass(Qt)::Alignment)al.hPos() );
}


void uiStatusBar::setLabelTxt( int idx, const char* lbltxt )
{
    if ( idx<0 || idx >= body_->msgs.size() ) return;

    mQtclass(QLabel*) lbl =
		     dynamic_cast<mQtclass(QLabel*)>(body_->msgs[idx]->buddy());

    if ( lbl ) lbl->setText( lbltxt );
}
