/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposprovgroup.cc,v 1.1 2008-02-07 16:10:40 cvsbert Exp $";

#include "uiposprovgroupstd.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "iopar.h"
#include "keystrs.h"

mImplFactory2Param(uiPosProvGroup,uiParent*,const uiPosProvider::Setup&,
		   uiPosProvGroup::factory);


uiRangePosProvGroup::uiRangePosProvGroup( uiParent* p,
					  const uiPosProvider::Setup& su )
    : uiPosProvGroup(p,su)
{
    BufferString lbl( su.is2d_ ? "2D" : "3D" );
    lbl += " Range group ";
    lbl += su.withz_ ? "+" : "-"; lbl += "Z";
    new uiLabel( this, lbl );
}


void uiRangePosProvGroup::usePar( const IOPar& )
{
}


bool uiRangePosProvGroup::fillPar( IOPar& ) const
{
    return true;
}


void uiRangePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Range );
}
