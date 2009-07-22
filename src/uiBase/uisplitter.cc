/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisplitter.cc,v 1.5 2009-07-22 16:01:38 cvsbert Exp $";


#include "uisplitter.h"

#include "uigroup.h"
#include "uiobjbody.h"

#include <QSplitter>


class uiSplitterBody : public uiObjBodyImpl<uiSplitter,QSplitter>
{
public:

uiSplitterBody( uiSplitter& handle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSplitter,QSplitter>(handle,p,nm)
{
}

};


uiSplitter::uiSplitter( uiParent* p, const char* txt, bool hor )
    : uiObject(p, txt, mkbody(p,txt) )
{
    body_->setOrientation( hor ? Qt::Horizontal : Qt::Vertical );
    setStretch( 2, 2 );
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
