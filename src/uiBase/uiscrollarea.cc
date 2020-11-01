/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiscrollarea.h"

#include "uiobjbodyimpl.h"
#include "q_uiimpl.h"

#include <QScrollArea>

mUseQtnamespace

class uiScrollAreaBody : public uiObjBodyImpl<uiScrollArea,QScrollArea>
{
public:
			uiScrollAreaBody(uiScrollArea&,uiParent*,const char*);
			~uiScrollAreaBody();

protected:

    void		resizeEvent(QResizeEvent*);
};


uiScrollAreaBody::uiScrollAreaBody( uiScrollArea& hndle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiScrollArea,QScrollArea>(hndle,p,nm)
{}

uiScrollAreaBody::~uiScrollAreaBody()
{}


void uiScrollAreaBody::resizeEvent( QResizeEvent* ev )
{
    // TODO: Resize subwindows
    QScrollArea::resizeEvent( ev );
}


uiScrollArea::uiScrollArea( uiParent* p, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
{
    setStretch( 2, 2 );
}


uiScrollAreaBody& uiScrollArea::mkbody( uiParent* p, const char* nm )
{
    body_ = new uiScrollAreaBody( *this, p, nm );
    return *body_;
}


uiScrollArea::~uiScrollArea()
{ delete body_; }


void uiScrollArea::setObject( uiObject* obj )
{
    object_ = obj;
    if ( obj )
	body_->setWidget( obj->qwidget() );
}


uiObject* uiScrollArea::getObject()
{
    if ( !object_ )
	return nullptr;

    body_->takeWidget();
    return object_;
}

