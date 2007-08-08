/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id: uiaction.cc,v 1.1 2007-08-08 05:22:41 cvsnanne Exp $
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


void uiAction::setCheckable( bool yn )
{ qaction_->setCheckable( yn ); }

bool uiAction::isCheckable() const
{ return qaction_->isCheckable(); }

void uiAction::setChecked( bool yn )
{ qaction_->setCheckable( yn ); }

bool uiAction::isChecked() const
{ return qaction_->isChecked(); }

void uiAction::setEnabled( bool yn )
{ qaction_->setCheckable( yn ); }

bool uiAction::isEnabled() const
{ return qaction_->isEnabled(); }
