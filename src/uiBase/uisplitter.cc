/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:		$Id: uisplitter.cc,v 1.1 2007-05-04 05:49:35 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uisplitter.h"

#include "uigroup.h"
#include "uiobjbody.h"

#include <QSplitter>


class uiSplitterBody : public uiObjBodyImplNoQtNm<uiSplitter,QSplitter>
{
public:

uiSplitterBody( uiSplitter& handle, uiParent* p, const char* nm )
    : uiObjBodyImplNoQtNm<uiSplitter,QSplitter>(handle,p,nm)
{
}

};


uiSplitter::uiSplitter( uiParent* p, const char* txt, bool hor )
    : uiObject(p, txt, mkbody(p,txt) )
{
    body_->setOrientation( hor ? Qt::Horizontal : Qt::Vertical );
}


uiSplitterBody& uiSplitter::mkbody( uiParent* p, const char* nm )
{ 
    body_ = new uiSplitterBody( *this, p, nm );
    return *body_; 
}


void uiSplitter::addObject( uiObject* obj )
{
    if ( obj )
	body_->addWidget( obj->qwidget() );
}


void uiSplitter::addGroup( uiGroup* grp )
{
    if ( grp )
	addObject( grp->attachObj() );
}
