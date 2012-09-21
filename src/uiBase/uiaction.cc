/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uiaction.h"
#include "i_qaction.h"

#include "menuhandler.h"
#include "pixmap.h"

#define mInit toggled(this), triggered(this), msgr_(0)

uiAction::uiAction( mQtclass(QAction*) qact )
    : mInit
    , qaction_(qact)
{
    msgr_ = new i_ActionMessenger( qact, this );
}


uiAction::uiAction( const char* txt )
    : mInit
{
    init( txt );
}


uiAction::uiAction( const char* txt, const CallBack& cb )
    : cb_(cb)
    , mInit
{
    init( txt );
}


uiAction::uiAction( const char* txt, const CallBack& cb, const ioPixmap& pm )
    : cb_(cb)
    , mInit
{
    init( txt );
    setPixmap( pm );
}


uiAction::uiAction( const MenuItem& itm )
    : mInit
{
    init( itm.text );
    setToolTip( itm.tooltip );
    setCheckable( itm.checkable );
    setChecked( itm.checked );
    setEnabled( itm.enabled );
    if ( !itm.iconfnm.isEmpty() )
	setPixmap( ioPixmap(itm.iconfnm) );
}

uiAction::~uiAction()
{
    delete msgr_;
}


void uiAction::init( const char* txt )
{
    qaction_ = new mQtclass(QAction)( mQtclass(QString)(txt), 0 );
}


void uiAction::setText( const char* txt )
{ qaction_->setText( mQtclass(QString)(txt) ); }

const char* uiAction::text() const
{
    BufferString str;
    str = qaction_->text().toAscii().data();
    return str;
}


void uiAction::setIconText( const char* txt )
{ qaction_->setIconText( txt ); }

const char* uiAction::iconText() const
{
    mQtclass(QString) qstr = qaction_->iconText();
    return qstr.toAscii().data();
}

void uiAction::setToolTip( const char* txt )
{ qaction_->setToolTip( txt ); }

const char* uiAction::toolTip() const
{
    static BufferString str;
    str = qaction_->toolTip().toAscii().data();
    return str;
}


void uiAction::setPixmap( const ioPixmap& pm )
{ qaction_->setIcon( *pm.qpixmap() ); }


#define mSetGet(setfn,getfn) \
void uiAction::setfn( bool yn ) \
{ qaction_->setfn( yn ); } \
\
bool uiAction::getfn() const \
{ return qaction_->getfn(); }


mSetGet( setCheckable, isCheckable )
mSetGet( setChecked, isChecked )
mSetGet( setEnabled, isEnabled )
mSetGet( setVisible, isVisible )
