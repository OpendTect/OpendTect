/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uisurfaceposprov.h"
#include "emsurfaceposprov.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uiselsurvranges.h"
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
    , zfac_(mCast(float,SI().zDomain().userFactor()))
    , zstepfld_(0)
    , extrazfld_(0)
{
    if ( su.is2d_ )
    {
	new uiLabel( this, "Not implemented for 2D" );
	return;
    }
    surf1fld_ = new uiIOObjSel( this, ctio1_, "Horizon" );

    const CallBack selcb( mCB(this,uiSurfacePosProvGroup,selChg) );
    issingfld_ = new uiGenInput( this, "Select",
	    		BoolInpSpec(true,"On Horizon","To a 2nd Horizon") );
    issingfld_->attach( alignedBelow, surf1fld_ );
    issingfld_->valuechanged.notify( selcb );

    surf2fld_ = new uiIOObjSel( this, ctio2_, "Bottom Horizon" );
    surf2fld_->attach( alignedBelow, issingfld_ );

    BufferString txt;
    if ( su.withstep_ )
    {
	zstepfld_ = new uiSpinBox( this, 0, "Z step" );
	zstepfld_->attach( alignedBelow, surf2fld_ );
	float zstep = SI().zRange(true).step * 10;
	int v = (int)((zstep * zfac_) + .5);
	zstepfld_->setValue( v );
	zstepfld_->setInterval( StepInterval<int>(1,999999,1) );
	txt = "Z step "; txt += SI().getZUnitString();
	zsteplbl_ = new uiLabel( this, txt, zstepfld_ );
    }

    if ( su.withz_ )
    {
	txt = "Extra Z";
	extrazfld_ = new uiSelZRange( this, false, true, txt );
	if ( zstepfld_ )
	    extrazfld_->attach( alignedBelow, zstepfld_ );
	else
	    extrazfld_->attach( alignedBelow, surf2fld_ );
    }

    setHAlignObj( surf1fld_ );
    postFinalise().notify( selcb );
}


uiSurfacePosProvGroup::~uiSurfacePosProvGroup()
{
    delete ctio1_.ioobj; delete &ctio1_;
    delete ctio2_.ioobj; delete &ctio2_;
}


void uiSurfacePosProvGroup::selChg( CallBacker* )
{
    const bool isbtwn = !issingfld_->getBoolValue();
    surf2fld_->display( isbtwn );
    if ( zstepfld_ )
    {
	zstepfld_->display( isbtwn );
	zsteplbl_->display( isbtwn );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mGetSurfKey(k) \
    IOPar::compKey(sKey::Surface(),Pos::EMSurfaceProvider3D::k##Key())


void uiSurfacePosProvGroup::usePar( const IOPar& iop )
{
    if ( !surf1fld_ ) return;

    surf1fld_->setInput( MultiID(iop.find(mGetSurfKey(id1))) );
    const char* res = iop.find( mGetSurfKey(id2) );
    const bool issing = !res || !*res;
    if ( !issing )
	surf2fld_->setInput( MultiID(res) );
    issingfld_->setValue( issing );

    if ( zstepfld_ )
    {
	float zstep = zstepfld_->getValue() / zfac_;
	iop.get( mGetSurfKey(zstep), zstep );
	int v = (int)((zstep * zfac_) + .5);
	zstepfld_->setValue( v );
    }

    if ( extrazfld_ )
    {
	StepInterval<float> ez = extrazfld_->getRange();
	iop.get( mGetSurfKey(extraZ), ez );
	extrazfld_->setRange( ez );
    }

    selChg( 0 );
}


bool uiSurfacePosProvGroup::fillPar( IOPar& iop ) const
{
    if ( !surf1fld_ ) return false;

    if ( !surf1fld_->commitInput() )
	mErrRet("Please select the surface")
    iop.set( mGetSurfKey(id1), ctio1_.ioobj->key() );

    Interval<float> ez( 0, 0 );
    if ( issingfld_->getBoolValue() )
	iop.removeWithKey( mGetSurfKey(id2) );
    else
    {
	if ( !surf2fld_->commitInput() )
	    mErrRet("Please select the bottom horizon")
	 if (  ctio2_.ioobj->key() ==  ctio1_.ioobj->key() )
	     mErrRet("Please select two different horizons")
	iop.set( mGetSurfKey(id2), ctio2_.ioobj->key() );
    }

    const float zstep = zstepfld_ ? zstepfld_->getValue() / zfac_ 
				  : SI().zStep();
    iop.set( mGetSurfKey(zstep), zstep );

    if ( mIsUdf(ez.start) ) ez.start = 0;
    if ( mIsUdf(ez.stop) ) ez.stop = 0;

    if ( extrazfld_ ) assign( ez, extrazfld_->getRange() );
    iop.set( mGetSurfKey(extraZ), ez );
    iop.set( sKey::Type(), sKey::Surface() );
    return true;
}


void uiSurfacePosProvGroup::getSummary( BufferString& txt ) const
{
    if ( !surf1fld_ ) return;
    txt += issingfld_->getBoolValue() ? "On Horizon" : "Between Horizons";
}


void uiSurfacePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Surface() );
}
