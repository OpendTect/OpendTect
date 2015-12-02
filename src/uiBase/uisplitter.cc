/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uisplitter.h"

#include "uigroup.h"
#include "uiobjbody.h"

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
