/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposprovgroup.cc,v 1.2 2008-02-07 16:17:05 cvsbert Exp $";

#include "uiposprovgroupstd.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "iopar.h"
#include "keystrs.h"

mImplFactory2Param(uiPosProvGroup,uiParent*,const uiPosProvider::Setup&,
		   uiPosProvGroup::factory);


uiPosProvGroup::uiPosProvGroup( uiParent* p, const uiPosProvider::Setup& su )
    : uiGroup(p,su.seltxt_)
{
}

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
