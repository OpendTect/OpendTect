/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tuthortools.h"
#include "emmanager.h"
#include "emsurface.h"
#include "emsurfaceauxdata.h"
#include "ioobj.h"
#include "keystrs.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

Tut::HorTool::HorTool(const char* title)
    : Executor(title)
{
    msg_ = tr("Positions done");
}


Tut::HorTool::~HorTool()
{
    delete iter_;
}


void Tut::HorTool::setHorizons( EM::Horizon3D& hor1, EM::Horizon3D* hor2 )
{
    horizon1_ = &hor1;
    horizon2_ = hor2;
}


od_int64 Tut::HorTool::totalNr() const
{
    return hs_.totalNr();
}


bool Tut::HorTool::doPrepare( od_ostream* strm )
{
    if ( !horizon1_ )
	return false;

    StepInterval<int> inlrg = horizon1_->geometry().rowRange();
    StepInterval<int> crlrg = horizon1_->geometry().colRange();
    hs_.set( inlrg, crlrg );
    if ( !hs_.isDefined() )
	return false;

    delete iter_;
    iter_ = new TrcKeySamplingIterator( hs_ );
    if ( !iter_->next(bid_) || bid_.isUdf() )
	return false;

    return true;
}


bool Tut::HorTool::doFinish( bool success, od_ostream* strm )
{
    deleteAndNullPtr( iter_ );
    return success;
}


Tut::ThicknessCalculator::ThicknessCalculator()
	: HorTool("Calculating Thickness")
	, usrfac_( (float) SI().zDomain().userFactor() )
{}


void Tut::ThicknessCalculator::setAttribName( const char* attribname )
{
    if ( !horizon1_ )
    {
	pErrMsg( "init should be called after the horizons are set" );
	return;
    }

    dataidx_ = horizon1_->auxdata.addAuxData( attribname && *attribname ?
				attribname : (const char*)sKey::Thickness() );
    posid_.setObjectID( horizon1_->id() );
}


bool Tut::ThicknessCalculator::doPrepare( od_ostream* strm )
{
    if ( !horizon2_ )
	return false;

    return HorTool::doPrepare(strm);
}


int Tut::ThicknessCalculator::nextStep()
{
    const EM::SubID subid = bid_.toInt64();
    const float z1 = (float) horizon1_->getPos( subid ).z_;
    const float z2 = (float) horizon2_->getPos( subid ).z_;

    float val = mUdf(float);
    if ( !mIsUdf(z1) && !mIsUdf(z2) )
	val = fabs( z2 - z1 ) * usrfac_;

    posid_.setSubID( subid );
    horizon1_->auxdata.setAuxDataVal( dataidx_, posid_, val );

    nrdone_++;
    return iter_->next( bid_ ) ? MoreToDo() : Finished();
}


Executor* Tut::ThicknessCalculator::dataSaver()
{
    if ( !horizon1_ )
	return nullptr;

    return horizon1_->auxdata.auxDataSaver( dataidx_, true );
}


Tut::HorSmoother::HorSmoother(EM::Horizon3D& hor)
    : HorTool("Smoothing Horizon")
    , horizonoutput_( &hor )
{}


bool Tut::HorSmoother::doPrepare( od_ostream* strm )
{
    if ( !horizonoutput_ )
	return false;

    return HorTool::doPrepare( strm );
}


int Tut::HorSmoother::nextStep()
{
    const int rad = weak_ ? 1 : 2;
    float sum = 0.f;
    int count = 0;
    for ( int inloffs=-rad; inloffs<=rad; inloffs++ )
    {
	for ( int crloffs=-rad; crloffs<=rad; crloffs++ )
	{
	    const BinID binid = BinID( bid_.inl() +inloffs *hs_.step_.inl(),
				       bid_.crl() +crloffs *hs_.step_.crl() );
	    const float z = horizon1_->getZ( binid );
	    if ( mIsUdf(z) )
		continue;

	    sum += z; count++;
	}
    }

    float val = count ? sum/count : mUdf(float);
    horizonoutput_->setZ( bid_, val, false );

    nrdone_++;
    return iter_->next( bid_ ) ? MoreToDo() : Finished();
}


Executor* Tut::HorSmoother::dataSaver( const MultiID& id )
{
    if ( !horizonoutput_ )
	return nullptr;

    return horizonoutput_->geometry().saver( nullptr, &id );
}
