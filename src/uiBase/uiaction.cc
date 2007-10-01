/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id: uiaction.cc,v 1.2 2007-10-01 12:11:48 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiaction.h"
#include "i_qaction.h"

#include "pixmap.h"

uiAction::uiAction( const char* txt )
    : checked_(false)
    , toggled(this)
    , triggered(this)
{
    init( txt );
}


uiAction::uiAction( const char* txt, const CallBack& cb )
    : checked_(false)
    , cb_(cb)
    , toggled(this)
    , triggered(this)
{
    init( txt );
}


uiAction::uiAction( const char* txt, const CallBack& cb, const ioPixmap& pm )
    : checked_(false)
    , cb_(cb)
    , toggled(this)
    , triggered(this)
{
    init( txt );
    setPixmap( pm );
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
