/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2000
________________________________________________________________________

-*/

#include "uistatusbar.h"

#include "uibody.h"
#include "uimainwin.h"
#include "uimain.h"

#include "uistrings.h"
#include "envvars.h"

#include "q_uiimpl.h"

#include <QStatusBar>
#include <QLabel>
#include <QToolTip>

static Threads::Atomic<int> nodispatall_( -1 );

mUseQtnamespace

class uiStatusBarBody : public uiBodyImpl<uiStatusBar,QStatusBar>
{

public:
uiStatusBarBody( uiStatusBar& hndl, uiMainWin* parnt, const char* nm,
		 QStatusBar& sb)
: uiBodyImpl<uiStatusBar,QStatusBar>( hndl, parnt, sb )
{
    if ( nodispatall_ == -1 )
	nodispatall_ = GetEnvVarYN( "OD_NO_STATUSBAR_MSGS" ) ? 1 : 0;
}


int size() const
{
    return qthing()->children().size();
}

void setText( const uiString& msg, int idx, int msecs )
{
    if ( nodispatall_ )
	return;

    const QString qstr( toQString(msg) );
    if ( msgs_.validIdx(idx) && msgs_[idx] )
    {
	if ( msgs_[idx]->text() != qstr )
	    msgs_[idx]->setText( qstr );
    }
    else if ( msg.isEmpty() )
	qthing()->clearMessage();
    else
    {
	if ( msecs > 0 )
	    qthing()->showMessage( qstr, msecs );
	else if ( qthing()->currentMessage() != qstr )
	    qthing()->showMessage( qstr, 0 );
    }
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
    QLabel* lbl = new QLabel( toQString(lbltxt) );
    int idx = msgs_.size();
    msgs_ += lbl;

    if ( !lbltxt.isEmpty() )
    {
	QLabel* txtlbl = new QLabel( toQString(lbltxt) );
	lbl->setBuddy( txtlbl );

	qthing()->addWidget( txtlbl );
	txtlbl->setFrameStyle(QFrame::NoFrame);
    }

    qthing()->addWidget( lbl, stretch );
    return idx;
}


void addWidget( QWidget* qw )
{
    qthing()->addWidget( qw );
}

void repaint()
{
    if ( !isMainThreadCurrent() )
	return;

    qthing()->repaint();
    for( int idx=0; idx<msgs_.size(); idx++)
	if (msgs_[idx]) msgs_[idx]->repaint();
}

virtual const QWidget*	managewidg_() const
{
    return qwidget();
}

    friend class	uiStatusBar;
    ObjectSet<QLabel>	msgs_;

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


void uiStatusBar::setPartiallyEmpty( int startat )
{
    const int nrflds = nrFields();
    for ( int idx=startat; idx<nrflds; idx++ )
	body_->setText( uiString::empty(), idx, -1 );
}


void uiStatusBar::message( const uiString& msg, int fldidx, int msecs )
{
    while( messages_.size() <= fldidx )
	messages_.add( uiString::empty() );

    messages_[fldidx] = msg;
    body_->setText( msg, fldidx, msecs );
    body_->repaint();
    uiMain::theMain().repaint();
}


void uiStatusBar::message( const uiStringSet& msgs, int msecs )
{
    messages_ = msgs;
    for ( int idx=0; idx<msgs.size(); idx++ )
	body_->setText( msgs.get(idx), idx, msecs );
    body_->repaint();
    uiMain::theMain().repaint();
}


void uiStatusBar::setBGColor( int fldidx, const Color& col )
{
    body_->setBGColor( fldidx, col );
}


QWidget* uiStatusBar::getWidget(int)
{ return body_->qthing(); }


Color uiStatusBar::getBGColor( int fldidx ) const
{
    return body_->getBGColor( fldidx );
}


int uiStatusBar::addMsgFld( const uiString& lbltxt, const uiString& tooltip,
			    OD::Alignment al, int stretch )
{
    int idx = body_->addMsgFld( lbltxt, stretch );

    setLabelTxt( idx, lbltxt );
    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}

int uiStatusBar::addMsgFld( const uiString& tooltip, OD::Alignment al,
			    int stretch )
{
    int idx = body_->addMsgFld( uiString::empty(), stretch );

    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}


bool uiStatusBar::addObject( uiObject* obj )
{
    if ( !obj )
	return false;
    QWidget* qw = obj->getWidget(0);
    if ( !qw )
	return false;

    body_->addWidget( qw );
    return true;
}


void uiStatusBar::setToolTip( int idx, const uiString& tooltip )
{
    if ( !body_->msgs_.validIdx(idx) ) return;

    if ( !tooltip.isEmpty() && body_->msgs_[idx] )
	body_->msgs_[idx]->setToolTip( toQString(tooltip) );
}


void uiStatusBar::setTxtAlign( int idx, OD::Alignment al )
{
    if ( !body_->msgs_.validIdx(idx) ) return;
    body_->msgs_[idx]->setAlignment( (Qt::Alignment)al.uiValue() );
}


void uiStatusBar::setLabelTxt( int idx, const uiString& lbltxt )
{
    if ( nodispatall_ )
	return;
    if ( !body_->msgs_.validIdx(idx) )
	return;

    QLabel* lbl = dynamic_cast<QLabel*>(body_->msgs_[idx]->buddy());

    if ( lbl )
	lbl->setText( toQString(lbltxt) );
}
