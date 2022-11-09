/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tuthortools.h"
#include "emhorizon3d.h"
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
    delete iter_;
    iter_ = new TrcKeySamplingIterator( hs_ );
}


Tut::ThicknessCalculator::ThicknessCalculator()
	: HorTool("Calculating Thickness")
	, usrfac_( (float) SI().zDomain().userFactor() )
{
}


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


int Tut::ThicknessCalculator::nextStep()
{
    BinID bid;
    if ( !iter_->next(bid) )
	return Finished();

    const EM::SubID subid = bid.toInt64();
    const float z1 = (float) horizon1_->getPos( subid ).z;
    const float z2 = (float) horizon2_->getPos( subid ).z;

    float val = mUdf(float);
    if ( !mIsUdf(z1) && !mIsUdf(z2) )
	val = fabs( z2 - z1 ) * usrfac_;

    posid_.setSubID( subid );
    horizon1_->auxdata.setAuxDataVal( dataidx_, posid_, val );

    nrdone_++;
    return MoreToDo();
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
    BinID bid;
    if ( !iter_->next(bid) )
	return Finished();

    const int rad = weak_ ? 1 : 2;
    float sum = 0.f;
    int count = 0;
    for ( int inloffs=-rad; inloffs<=rad; inloffs++ )
    {
	for ( int crloffs=-rad; crloffs<=rad; crloffs++ )
	{
	    const BinID binid = BinID( bid.inl() +inloffs *hs_.step_.inl(),
				       bid.crl() +crloffs *hs_.step_.crl() );
	    const float z = horizon1_->getZ( binid );
	    if ( mIsUdf(z) )
		continue;

	    sum += z; count++;
	}
    }

    float val = count ? sum/count : mUdf(float);
    horizon1_->setZ( bid, val, false );

    nrdone_++;
    return MoreToDo();
}


Executor* Tut::HorSmoother::dataSaver( const MultiID& id )
{
    return horizon1_->geometry().saver( nullptr, &id );
}
