/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uiinterpollayermodel.h"
#include "interpollayermodel.h"

#include "uigeninput.h"


uiInterpolationLayerModel::uiInterpolationLayerModel( uiParent* p )
    : uiGroup(p,"Interpolation Layer Model Selection")
{
    const TypeSet<uiString>& usrnms =
	uiInterpolationLayerModelGrp::factory().getUserNames();
    layermodelfld_ = new uiGenInput( this, "Interpolate along",
				     StringListInpSpec(usrnms) );
    layermodelfld_->valuechanged.notify(
		mCB(this,uiInterpolationLayerModel,selCB) );

    const BufferStringSet& names =
	uiInterpolationLayerModelGrp::factory().getNames();
    for ( int idx=0; idx<names.size(); idx++ )
    {
	uiInterpolationLayerModelGrp* grp =
		uiInterpolationLayerModelGrp::factory().create(
		names.get(idx), this, true );
	grp->attach( alignedBelow, layermodelfld_ );
	grps_ += grp;
    }

    setHAlignObj( layermodelfld_ );
    selCB( 0 );
}


void uiInterpolationLayerModel::selCB( CallBacker* )
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( layermodelfld_->getIntValue()==idx );
}


bool uiInterpolationLayerModel::fillPar( IOPar& par ) const
{
    const int cursel = layermodelfld_->getIntValue();
    return grps_[cursel]->fillPar( par );
}



// uiInterpolationLayerModelGrp
mImplFactory1Param( uiInterpolationLayerModelGrp, uiParent*,
		    uiInterpolationLayerModelGrp::factory )

uiInterpolationLayerModelGrp::uiInterpolationLayerModelGrp( uiParent* p )
    : uiGroup(p)
{
}


bool uiInterpolationLayerModelGrp::fillPar( IOPar& par ) const
{
    par.set( InterpolationLayerModel::sKeyModelType(), factoryKeyword() );
    return true;
}


// uiZSliceInterpolationModel
uiZSliceInterpolationModel::uiZSliceInterpolationModel( uiParent* p )
    : uiInterpolationLayerModelGrp(p)
{
}


bool uiZSliceInterpolationModel::fillPar( IOPar& par ) const
{
    if ( !uiInterpolationLayerModelGrp::fillPar(par) )
	return false;

    return true;
}
