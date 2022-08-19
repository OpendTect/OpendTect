/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiobjectstack.h"

#include "uigroup.h"
#include "uiobjbodyimpl.h"

#include <QStackedWidget>

mUseQtnamespace

class uiObjStackBody : public uiObjBodyImpl<uiObjectStack,QStackedWidget>
{
public:
uiObjStackBody( uiObjectStack& hndle, uiParent* parnt,
		const char* txt )
    : uiObjBodyImpl<uiObjectStack,QStackedWidget>(hndle,parnt,txt) {}
};


uiObjectStack::uiObjectStack( uiParent* p, const char* txt )
    : uiObject(p,txt,mkbody(p,txt))
{}


uiObjStackBody& uiObjectStack::mkbody( uiParent* p, const char* txt )
{
    body_ = new uiObjStackBody( *this, p, txt );
    return *body_;
}


int uiObjectStack::addObject( uiObject* obj )
{
    if ( !obj ) return -1;
    return body_->addWidget( obj->qwidget() );
}


int uiObjectStack::addGroup( uiGroup* grp )
{
    if ( !grp ) return -1;
    return addObject( grp->attachObj() );
}


void uiObjectStack::setCurrentObject( int idx )
{ body_->setCurrentIndex( idx ); }


int uiObjectStack::size() const
{ return body_->count(); }
