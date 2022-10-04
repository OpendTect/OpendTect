/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisplitter.h"

#include "uigroup.h"
#include "uiobjbodyimpl.h"

#include <QSplitter>

mUseQtnamespace

class uiSplitterBody : public uiObjBodyImpl<uiSplitter,QSplitter>
{
public:

uiSplitterBody( uiSplitter& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSplitter,QSplitter>(hndl,p,nm)
{
}

};


uiSplitter::uiSplitter( uiParent* p, const char* txt, bool hor )
    : uiObject(p, txt, mkbody(p,txt) )
{
    body_->setOrientation( hor ? Qt::Horizontal : Qt::Vertical );
    setStretch( 2, 2 );
}


uiSplitter::uiSplitter( uiParent* p, const char* txt, OD::Orientation orient )
    : uiObject(p, txt, mkbody(p,txt) )
{
    body_->setOrientation(
		orient==OD::Horizontal ? Qt::Horizontal : Qt::Vertical );
    setStretch( 2, 2 );
}


uiSplitter::~uiSplitter()
{}


uiSplitterBody& uiSplitter::mkbody( uiParent* p, const char* nm )
{
    body_ = new uiSplitterBody( *this, p, nm );
    return *body_;
}


void uiSplitter::addGroup( uiGroup* grp )
{
    if ( grp && grp->attachObj() )
	body_->addWidget( grp->attachObj()->qwidget() );
}
