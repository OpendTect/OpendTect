/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "tuthortools.h"
#include "emhorizon3d.h"
#include "emsurface.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "emsurfaceauxdata.h"
#include "statruncalc.h"



#include "ioobj.h"

Tut::HorTool::HorTool(const char* title)
    : Executor(title)
    , horizon1_(0)
    , horizon2_(0)
    , iter_(0)
    , nrdone_(0)
{
}


Tut::HorTool::~HorTool()
{
    delete iter_;
    if ( horizon1_ ) horizon1_->unRef();
    if ( horizon2_ ) horizon2_->unRef();
}


void Tut::HorTool::setHorizons( EM::Horizon3D* hor1, EM::Horizon3D* hor2 )
{
    horizon1_ = hor1;
    horizon2_ = hor2;
    if( !horizon1_ )
	return;

    StepInterval<int> inlrg = horizon1_->geometry().rowRange();
    StepInterval<int> crlrg = horizon1_->geometry().colRange();
    setHorSamp( inlrg, crlrg );
}


od_int64 Tut::HorTool::totalNr() const
{
    return hs_.totalNr();
}


void Tut::HorTool::setHorSamp( const StepInterval<int>& inlrg, 
				const StepInterval<int>& crlrg )
{
    hs_.set( inlrg, crlrg );
    iter_ = new HorSamplingIterator( hs_ );
}


Tut::ThicknessCalculator::ThicknessCalculator()
	: HorTool("Calculating Thickness")
	, dataidx_(0)
	, usrfac_(SI().zIsTime() ? 1000 : 1)
{
}


void Tut::ThicknessCalculator::init( const char* attribname )
{
    if ( !horizon1_ )
    {
	pErrMsg( "init should be called after the horizons are set" );
	return;
    }

    dataidx_ = horizon1_->auxdata.addAuxData( attribname && *attribname ?
	    				attribname : "Thickness" );
    posid_.setObjectID( horizon1_->id() );
}


int Tut::ThicknessCalculator::nextStep()
{
    if ( !iter_->next(bid_) )
	return Executor::Finished();

    int nrsect = horizon1_->nrSections();
    if ( horizon2_->nrSections() < nrsect ) nrsect = horizon2_->nrSections();

    for ( EM::SectionID isect=0; isect<nrsect; isect++ )
    {
	const EM::SubID subid = bid_.toInt64();
	const float z1 = horizon1_->getPos( isect, subid ).z;
	const float z2 = horizon2_->getPos( isect, subid ).z;
		        
	float val = mUdf(float);
	if ( !mIsUdf(z1) && !mIsUdf(z2) )
	    val = fabs( z2 - z1 ) * usrfac_;

	posid_.setSubID( subid );
	posid_.setSectionID( isect );
	horizon1_->auxdata.setAuxDataVal( dataidx_, posid_, val );
    }

    nrdone_++;
    return Executor::MoreToDo();
}


Executor* Tut::ThicknessCalculator::dataSaver()
{
    return horizon1_->auxdata.auxDataSaver( dataidx_, true );
}


Tut::HorSmoother::HorSmoother()
    : HorTool("Smoothing Horizon")
    , subid_(0)
{
}


int Tut::HorSmoother::nextStep()
{
    if ( !iter_->next(bid_) )
	return Executor::Finished();

    const int nrsect = horizon1_->nrSections();
    const int rad = weak_ ? 1 : 2;
    for ( EM::SectionID isect=0; isect<nrsect; isect++ )
    {
	float sum = 0; int count = 0;
	for ( int inloffs=-rad; inloffs<=rad; inloffs++ )
	{
	    for ( int crloffs=-rad; crloffs<=rad; crloffs++ )
	    {
		const BinID binid = BinID( bid_.inl + inloffs * hs_.step.inl,
					   bid_.crl + crloffs * hs_.step.crl );
		const EM::SubID subid = binid.toInt64();
		const float z = horizon1_->getPos( isect, subid ).z;
		if ( mIsUdf(z) ) continue;
		sum += z; count++;
	    }
	}
	float val = count ? sum / count : mUdf(float);

	subid_ = bid_.toInt64();
	Coord3 pos = horizon1_->getPos( isect, subid_ );
	pos.z = val;
	horizon1_->setPos( isect, subid_, pos, false );
    }

    nrdone_++;
    return Executor::MoreToDo();
}


Executor* Tut::HorSmoother::dataSaver( const MultiID& id )
{
    return horizon1_->geometry().saver( 0, &id );
}
