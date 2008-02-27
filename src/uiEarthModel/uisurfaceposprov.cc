/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uisurfaceposprov.cc,v 1.1 2008-02-27 13:42:08 cvsbert Exp $";

#include "uisurfaceposprov.h"
#include "emsurfaceposprov.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "emsurfacetr.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "keystrs.h"
#include "ioobj.h"
#include "iopar.h"


uiSurfacePosProvGroup::uiSurfacePosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , ctio1_(*mMkCtxtIOObj(EMHorizon3D))
    , ctio2_(*mMkCtxtIOObj(EMHorizon3D))
    , zfac_(SI().zFactor())
    , surf1fld_(0)
{
    if ( su.is2d_ )
    {
	new uiLabel( this, "Not implemented for 2D" );
	return;
    }
    ctio1_.fillIfOnlyOne( IOObjContext::Surf );
    surf1fld_ = new uiIOObjSel( this, ctio1_, "Surface" );

    selfld_ = new uiGenInput( this, "Select",
	    		BoolInpSpec(true,"On surface","To a 2nd surface") );
    selfld_->attach( alignedBelow, surf1fld_ );

    surf2fld_ = new uiIOObjSel( this, ctio2_, "Bottom surface" );
    surf2fld_->attach( alignedBelow, selfld_ );

    zstepfld_ = new uiSpinBox( this, 0, "Z step" );
    zstepfld_->attach( alignedBelow, surf2fld_ );
    float zstep = SI().zRange(true).step;
    int v = (int)((zstep * zfac_) + .5);
    zstepfld_->setValue( v );
    BufferString txt( "Z step " ); txt += SI().getZUnit();
    new uiLabel( this, txt, zstepfld_ );

    setHAlignObj( surf1fld_ );
}


uiSurfacePosProvGroup::~uiSurfacePosProvGroup()
{
    delete ctio1_.ioobj; delete &ctio1_;
    delete ctio2_.ioobj; delete &ctio2_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mGetSurfKey(k) \
    IOPar::compKey(sKey::Surface,Pos::EMSurfaceProvider3D::k##Key())


void uiSurfacePosProvGroup::usePar( const IOPar& iop )
{
    if ( !surf1fld_ ) return;

    surf1fld_->usePar( iop, mGetSurfKey(id1) );
    surf2fld_->usePar( iop, mGetSurfKey(id2) );
    selfld_->setValue( ((bool)ctio2_.ioobj) );

    float zstep = zstepfld_->getValue() / zfac_;
    iop.get( mGetSurfKey(zstep), zstep );
    int v = (int)((zstep * zfac_) + .5);
    zstepfld_->setValue( v );
}


bool uiSurfacePosProvGroup::fillPar( IOPar& iop ) const
{
    if ( !surf1fld_ ) return false;

    if ( !surf1fld_->fillPar(iop,mGetSurfKey(id1)) )
	mErrRet("Please select the surface")

    const bool isbtwn = selfld_->getBoolValue();
    if ( !selfld_->getBoolValue() )
	iop.removeWithKey( mGetSurfKey(id2) );
    else
    {
	if ( !surf2fld_->fillPar(iop,mGetSurfKey(id2)) )
	    mErrRet("Please select the bottom surface")

	const float zstep = zstepfld_->getValue() / zfac_;
	iop.set( mGetSurfKey(zstep), zstep );
    }

    iop.set( sKey::Type, sKey::Surface );
    return true;
}


void uiSurfacePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Surface );
}
