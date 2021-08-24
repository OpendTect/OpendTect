/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/



#include "uiinterpollayermodel.h"
#include "interpollayermodel.h"

#include "uigeninput.h"
#include "uilabel.h"


// uiInterpolationLayerModelGrp
mImplFactory1Param( uiInterpolationLayerModelGrp, uiParent*,
		    uiInterpolationLayerModelGrp::factory )

uiInterpolationLayerModelGrp::uiInterpolationLayerModelGrp( uiParent* p )
    : uiGroup(p)
{
    setStretch( 1, 1 );
}


bool uiInterpolationLayerModelGrp::fillPar( IOPar& par ) const
{
    par.set( InterpolationLayerModel::sKeyModelType(), factoryKeyword() );
    return true;
}


bool uiInterpolationLayerModelGrp::usePar( const IOPar& )
{ return true; }


// uiZSliceInterpolationModel
uiZSliceInterpolationModel::uiZSliceInterpolationModel( uiParent* p )
    : uiInterpolationLayerModelGrp(p)
{
    new uiLabel( this, uiString::emptyString() );
}


bool uiZSliceInterpolationModel::fillPar( IOPar& par ) const
{
    if ( !uiInterpolationLayerModelGrp::fillPar(par) )
	return false;

    return true;
}


bool uiZSliceInterpolationModel::usePar( const IOPar& par )
{ return uiInterpolationLayerModelGrp::usePar( par ); }



// uiInterpolationLayerModel
uiInterpolationLayerModel::uiInterpolationLayerModel( uiParent* p )
    : uiGroup(p,"Interpolation Layer Model Selection")
{
    const uiStringSet& usrnms =
	uiInterpolationLayerModelGrp::factory().getUserNames();
    layermodelfld_ = new uiGenInput( this, tr("Interpolate along"),
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
	if ( !grp ) continue;

	grps_ += grp;
	grp->attach( alignedBelow, layermodelfld_ );
    }

    setHAlignObj( layermodelfld_ );
    postFinalise().notify( mCB(this,uiInterpolationLayerModel,selCB) );
}


void uiInterpolationLayerModel::selCB( CallBacker* )
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( layermodelfld_->getIntValue()==idx );
}


void uiInterpolationLayerModel::setModel( const InterpolationLayerModel* mdl )
{
    if ( !mdl ) return;

    const FixedString mdlnm = mdl->factoryKeyword();
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( mdlnm != grps_[idx]->factoryKeyword() )
	{
	    grps_[idx]->display( false );
	    continue;
	}

	layermodelfld_->setValue( idx );
	grps_[idx]->display( true );
	IOPar pars; mdl->fillPar( pars );
	grps_[idx]->usePar( pars );
    }
}


InterpolationLayerModel* uiInterpolationLayerModel::getModel()
{
    IOPar par;
    const int cursel = layermodelfld_->getIntValue();
    const bool res = grps_[cursel]->fillPar( par );
    if ( !res ) return 0;

    BufferString nm;
    par.get( InterpolationLayerModel::sKeyModelType(), nm );
    InterpolationLayerModel* mdl =
			InterpolationLayerModel::factory().create( nm );
    if ( !mdl || !mdl->usePar(par) ) { delete mdl; return 0; }

    return mdl;
}
