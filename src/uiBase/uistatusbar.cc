/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistatusbar.h"
#include "uimainwin.h"
#include "uimain.h"
#include "uiparentbody.h"
#include "uistrings.h"

#include "uibody.h"

#include <qstatusbar.h>
#include <qlabel.h>
#include <qtooltip.h>

#include "envvars.h"
static Threads::Atomic<int> nodispatall_( -1 );

mUseQtnamespace

class uiStatusBarBody : public uiBodyImpl<uiStatusBar,QStatusBar>
{
friend class		uiStatusBar;

public:
uiStatusBarBody( uiStatusBar& hndl, uiMainWin* parnt, const char* nm,
		 QStatusBar& sb)
: uiBodyImpl<uiStatusBar,QStatusBar>( hndl, parnt, sb )
{
    if ( nodispatall_ == -1 )
	nodispatall_ = GetEnvVarYN( "OD_NO_STATUSBAR_MSGS" ) ? 1 : 0;
}


int size() const
{ return qthing()->children().size(); }


void message( const uiString& msg, int idx, int msecs )
{
    if ( nodispatall_ )
	return;

    if ( msgs_.validIdx(idx) && msgs_[idx] )
	msgs_[idx]->setText(msg.getQString());
    else if ( !msg.isEmpty() )
	qthing()->showMessage( msg.getQString(), msecs<0?0:msecs );
    else
	qthing()->clearMessage();
}


void setBGColor( int idx, const Color& col )
{
    QWidget* widget = 0;
    if ( msgs_.validIdx(idx) && msgs_[idx] )
	widget = msgs_[idx];
    else
	widget = qthing();

    const QColor qcol( col.r(),col.g(), col.b() );
    QPalette palette;
    palette.setColor( widget->backgroundRole(), qcol );
    widget->setPalette( palette );
}


Color getBGColor( int idx )
{
    const QWidget* widget = 0;
    if ( msgs_.validIdx(idx) && msgs_[idx] )
	widget = msgs_[idx];
    else
	widget = qthing();

    const QBrush& qbr = widget->palette().brush( widget->backgroundRole() );
    const QColor& qc = qbr.color();
    return Color( qc.red(), qc.green(), qc.blue() );
}


int addMsgFld( const uiString& lbltxt, int stretch )
{
    QLabel* lbl = new QLabel( lbltxt.getQString() );
    int idx = msgs_.size();
    msgs_ += lbl;

    if ( !lbltxt.isEmpty() )
    {
	QLabel* txtlbl = new QLabel( lbltxt.getQString() );
	lbl->setBuddy( txtlbl );

	qthing()->addWidget( txtlbl );
	txtlbl->setFrameStyle(QFrame::NoFrame);
    }

    qthing()->addWidget( lbl, stretch );
    return idx;
}


void addWidget( QWidget* qw )
{ qthing()->addWidget( qw ); }


void repaint()
{
    if ( !isMainThreadCurrent() )
	return;

    qthing()->repaint();
    for( int idx=0; idx<msgs_.size(); idx++)
	if (msgs_[idx]) msgs_[idx]->repaint();
}


protected:

virtual const QWidget*	managewidg_() const
{ return qwidget(); }

ObjectSet<QLabel>		msgs_;

};



uiStatusBar::uiStatusBar( uiMainWin* parnt, const char* nm, QStatusBar& sb )
    : uiBaseObject(nm,&mkbody(parnt,nm,sb))
{
}


uiStatusBar::~uiStatusBar()
{
    delete body_;
}


uiStatusBarBody& uiStatusBar::mkbody( uiMainWin* parnt, const char* nm,
				      QStatusBar& sb)
{
    body_= new uiStatusBarBody( *this, parnt, nm, sb );

#ifndef __mac__ //TODO: Bugfix for gripper on Mac
    sb.setSizeGripEnabled( false );
#endif

    return *body_;
}


int uiStatusBar::nrFields() const
{
    return body_->size();
}


void uiStatusBar::setEmpty( int startat )
{
    const int nrflds = nrFields();
    for ( int idx=startat; idx<nrflds; idx++ )
	body_->message( uiStrings::sEmptyString(), idx, -1 );
}


void uiStatusBar::message( const uiString& msg, int fldidx, int msecs )
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


int uiStatusBar::addMsgFld( const uiString& lbltxt, const uiString& tooltip,
			    Alignment::HPos al, int stretch )
{
    int idx = body_->addMsgFld( lbltxt, stretch );

    setLabelTxt( idx, lbltxt );
    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}

int uiStatusBar::addMsgFld( const uiString& tooltip,
			    Alignment::HPos al, int stretch )
{
    int idx = body_->addMsgFld( uiStrings::sEmptyString(), stretch );

    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}


bool uiStatusBar::addObject( uiObject* obj )
{
    if ( !obj )
	return false;
    QWidget* qw = obj->qwidget();
    if ( !qw )
	return false;

    body_->addWidget( qw );
    return true;
}


void uiStatusBar::setToolTip( int idx, const uiString& tooltip )
{
    if ( !body_->msgs_.validIdx(idx) ) return;

    if ( !tooltip.isEmpty() && body_->msgs_[idx] )
	body_->msgs_[idx]->setToolTip( tooltip.getQString() );
}


void uiStatusBar::setTxtAlign( int idx, Alignment::HPos hal )
{
    if ( !body_->msgs_.validIdx(idx) ) return;

    Alignment al( hal );
    body_->msgs_[idx]->setAlignment( (Qt::Alignment)al.hPos() );
}


void uiStatusBar::setLabelTxt( int idx, const uiString& lbltxt )
{
    if ( nodispatall_ )
	return;

    if ( !body_->msgs_.validIdx(idx) ) return;

    QLabel* lbl = dynamic_cast<QLabel*>(body_->msgs_[idx]->buddy());

    if ( lbl ) lbl->setText( lbltxt.getQString() );
}
