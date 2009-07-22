/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiaction.cc,v 1.5 2009-07-22 16:01:38 cvsbert Exp $";

#include "uiaction.h"
#include "i_qaction.h"

#include "menuhandler.h"
#include "pixmap.h"


uiAction::uiAction( const char* txt )
    : toggled(this)
    , triggered(this)
{
    init( txt );
}


uiAction::uiAction( const char* txt, const CallBack& cb )
    : cb_(cb)
    , toggled(this)
    , triggered(this)
{
    init( txt );
}


uiAction::uiAction( const char* txt, const CallBack& cb, const ioPixmap& pm )
    : cb_(cb)
    , toggled(this)
    , triggered(this)
{
    init( txt );
    setPixmap( pm );
}


uiAction::uiAction( const MenuItem& itm )
    : toggled(this)
    , triggered(this)
{
    init( itm.text );
    setToolTip( itm.tooltip );
    setCheckable( itm.checkable );
    setChecked( itm.checked );
    setEnabled( itm.enabled );
    if ( !itm.iconfnm.isEmpty() )
	setPixmap( ioPixmap(itm.iconfnm) );
}


void uiAction::init( const char* txt )
{
    qaction_ = new QAction( QString(txt), 0 );
}


void uiAction::setText( const char* txt )
{ qaction_->setText( QString(txt) ); }

const char* uiAction::text() const
{
    QString qstr = qaction_->text();
    return qstr.toAscii().data();
}


void uiAction::setIconText( const char* txt )
{ qaction_->setIconText( txt ); }

const char* uiAction::iconText() const
{
    QString qstr = qaction_->iconText();
    return qstr.toAscii().data();
}

void uiAction::setToolTip( const char* txt )
{ qaction_->setToolTip( txt ); }

const char* uiAction::toolTip() const
{
    QString qstr = qaction_->toolTip();
    return qstr.toAscii().data();
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
