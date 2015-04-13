/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : November 2007
-*/

static const char* rcsID mUsedVar = "$Id: volprocbodyfiller.cc 36479 2014-09-16 13:06:37Z kristofer.tingdahl@dgbes.com $";

#include "volprocregionfiller.h"

#include "emfault.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emregion.h"
#include "iopar.h"
#include "seisdatapack.h"


static const char* sKeyInsideValue()	{ return "Inside Value"; }

namespace VolProc
{

RegionFiller::RegionFiller()
    : Step()
    , region_(*new EM::Region3D)
    , insideval_(mUdf(float))
    , outsideval_(mUdf(float))
    , startval_(mUdf(float))
    , gradval_(0)
    , startvalkey_(MultiID::udf())
    , gradvalkey_(MultiID::udf())
    , startvalauxidx_(-1)
    , gradvalauxidx_(-1)
{}


RegionFiller::~RegionFiller()
{
    delete &region_;
    releaseData();
}


void RegionFiller::releaseData()
{
    Step::releaseData();
}


bool RegionFiller::prepareComp( int )
{
// TODO: region should know about target TrcKeyZSampling
    return region_.init( 0 );
}


bool RegionFiller::computeBinID( const BinID& bid, int )
{
    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );

    const TrcKey tk( bid );
    const int inlidx = output->sampling().inlIdx( bid.inl() );
    const int crlidx = output->sampling().crlIdx( bid.crl() );
    const int nrz = output->sampling().nrZ();
    for ( int idz=0; idz<nrz; idz++ )
    {
	const float zval = output->sampling().zAtIndex( idz );
	float val =
		region_.isInside(tk,zval,true) ? insideval_ : outsideval_;
	if ( mIsUdf(val) && input )
	    val = input->data().get( inlidx, crlidx, idz );
	output->data(0).set( inlidx, crlidx, idz, val );
    }

    return true;
}


void RegionFiller::setInsideValue( float val )
{ insideval_ = val; }

float RegionFiller::getInsideValue() const
{ return insideval_; }

void RegionFiller::setOutsideValue( float val )
{ outsideval_ = val; }

float RegionFiller::getOutsideValue() const
{ return outsideval_; }

float RegionFiller::getStartValue() const
{ return startval_; }

const MultiID& RegionFiller::getStartValueHorizonID() const
{ return startvalkey_; }

int RegionFiller::getStartValueAuxDataIdx() const
{ return startvalauxidx_; }

float RegionFiller::getGradientValue() const
{ return gradval_; }

const MultiID& RegionFiller::getGradientHorizonID() const
{ return gradvalkey_; }

int RegionFiller::getGradientAuxDataIdx() const
{ return gradvalauxidx_; }

EM::Region3D& RegionFiller::region()
{ return region_; }

const EM::Region3D& RegionFiller::region() const
{ return region_; }


void RegionFiller::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
    region_.fillPar( par );
    par.set( sKeyInsideValue(), insideval_ );
}


bool RegionFiller::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    par.get( sKeyInsideValue(), insideval_ );
    return region_.usePar( par );
}

} // namespace VolProc
