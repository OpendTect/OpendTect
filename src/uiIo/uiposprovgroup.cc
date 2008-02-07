/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposprovgroup.cc,v 1.3 2008-02-07 16:51:37 cvsbert Exp $";

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


uiPolyPosProvGroup::uiPolyPosProvGroup( uiParent* p,
					const uiPosProvider::Setup& su )
    : uiPosProvGroup(p,su)
{
    BufferString lbl( su.is2d_ ? "\n2D" : "\n3D" );
    lbl += " Poly group ";
    lbl += su.withz_ ? "+" : "-"; lbl += "Z";
    new uiLabel( this, lbl );
}


void uiPolyPosProvGroup::usePar( const IOPar& )
{
}


bool uiPolyPosProvGroup::fillPar( IOPar& ) const
{
    return true;
}


void uiPolyPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Polygon );
}
