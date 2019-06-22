/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : May 2007
-*/


#include "tuthortools.h"
#include "emhorizon3d.h"
#include "emsurface.h"
#include "trckeysampling.h"
#include "survinfo.h"
#include "emsurfaceauxdata.h"
#include "statruncalc.h"
#include "keystrs.h"



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
    iter_ = new TrcKeySamplingIterator( hs_ );
}


Tut::ThicknessCalculator::ThicknessCalculator()
	: HorTool("Calculating Thickness")
	, dataidx_(0)
	, usrfac_( (float) SI().zDomain().userFactor() )
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
				attribname : (const char*)sKey::Thickness() );
}


int Tut::ThicknessCalculator::nextStep()
{
    posid_ = EM::PosID::getFromRowCol( iter_->curBinID() );
    const float z1 = (float) horizon1_->getPos( posid_ ).z_;
    const float z2 = (float) horizon2_->getPos( posid_ ).z_;

    float val = mUdf(float);
    if ( !mIsUdf(z1) && !mIsUdf(z2) )
	val = fabs( z2 - z1 ) * usrfac_;

    horizon1_->auxdata.setAuxDataVal( dataidx_, posid_, val );

    nrdone_++;
    return iter_->next() ? Executor::MoreToDo() : Executor::Finished();
}


Executor* Tut::ThicknessCalculator::dataSaver()
{
    return horizon1_->auxdata.auxDataSaver( dataidx_, true );
}


Tut::HorSmoother::HorSmoother()
    : HorTool("Smoothing Horizon")
{
}


int Tut::HorSmoother::nextStep()
{
    if ( !iter_->next() )
	return Executor::Finished();

    const int rad = weak_ ? 1 : 2;
    const BinID bid( iter_->curBinID() );
    float sum = 0; int count = 0;
    for ( int inloffs=-rad; inloffs<=rad; inloffs++ )
    {
	for ( int crloffs=-rad; crloffs<=rad; crloffs++ )
	{
	    const BinID binid = BinID( bid.inl() +inloffs *hs_.step_.inl(),
				       bid.crl() +crloffs *hs_.step_.crl());
	    const EM::PosID posid = EM::PosID::getFromRowCol( binid );
	    const float z = (float) horizon1_->getPos( posid ).z_;
	    if ( mIsUdf(z) ) continue;
	    sum += z; count++;
	}
    }
    float val = count ? sum / count : mUdf(float);

    posid_ = EM::PosID::getFromRowCol( bid );
    Coord3 pos = horizon1_->getPos( posid_ );
    pos.z_ = val;
    horizon1_->setPos( posid_, pos, false );

    nrdone_++;
    return Executor::MoreToDo();
}


Executor* Tut::HorSmoother::dataSaver( const DBKey& id )
{
    return horizon1_->saver();
}
