/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		7/9/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjectstack.cc,v 1.2 2008-11-25 15:35:24 cvsbert Exp $";


#include "uiobjectstack.h"

#include "uigroup.h"
#include "uiobjbody.h"

#include <QStackedWidget>


class uiObjStackBody : public uiObjBodyImplNoQtNm<uiObjectStack,QStackedWidget>
{
public:
uiObjStackBody( uiObjectStack& handle, uiParent* parnt,
		const char* txt )
    : uiObjBodyImplNoQtNm<uiObjectStack,QStackedWidget>(handle,parnt,txt) {}
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
