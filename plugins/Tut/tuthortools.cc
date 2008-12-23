

/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: tuthortools.cc,v 1.7 2008-12-23 13:53:37 cvsbert Exp $";

#include "tuthortools.h"
#include "ioobj.h"


Tut::HorTools::~HorTools()
{
    delete iter_;
    if( horizon1_ ) horizon1_->unRef();
    if( horizon2_ ) horizon2_->unRef();
}


void Tut::HorTools::setHorizons( EM::Horizon3D* hor1, EM::Horizon3D* hor2 )
{
    horizon1_ = hor1;
    horizon2_ = hor2;
    if( !horizon1_ )
	return;

    StepInterval<int> inlrg = horizon1_->geometry().rowRange();
    StepInterval<int> crlrg = horizon1_->geometry().colRange();
    setHorSamp( inlrg, crlrg );
}


od_int64 Tut::HorTools::totalNr() const
{
    return hs_.totalNr();
}


void Tut::HorTools::setHorSamp( StepInterval<int> inlrg, 
				StepInterval<int> crlrg )
{
    hs_.set( inlrg, crlrg );
    iter_ = new HorSamplingIterator( hs_ );
}


//////////////////////////////////////////////////////////////////////////////


void Tut::ThicknessFinder::init( const char* attribname )
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


int Tut::ThicknessFinder::nextStep()
{
    if ( !iter_->next(bid_) )
	return Executor::Finished();

    const EM::SubID subid = bid_.getSerialized();
    const float z1 = horizon1_->getPos( horizon1_->sectionID(0), subid ).z;
    const float z2 = horizon2_->getPos( horizon2_->sectionID(0), subid ).z;
		        
    float val = mUdf(float);
    if ( !mIsUdf(z1) && !mIsUdf(z2) )
    {
        val = fabs( z2 - z1 );
	if ( SI().zIsTime() ) val *= 1000;
    }

    posid_.setSubID( subid );
    horizon1_->auxdata.setAuxDataVal( dataidx_, posid_, val );

    nrdone_++;
    return Executor::MoreToDo();
}


Executor* Tut::ThicknessFinder::dataSaver()
{
    return horizon1_->auxdata.auxDataSaver();
}


///////////////////////////////////////////////////////////////////////////////


int Tut::HorSmoother::nextStep()
{
    if ( !iter_->next(bid_) )
	return Executor::Finished();

    int inl = bid_.r(); int crl = bid_.c();
    float sum = 0;
    int count = 0;
    const int rad = weak_ ? 1 : 2;
    for ( int idx=-rad; idx<=rad; idx++ )
    {
	for ( int cdx=-rad; cdx<=rad; cdx++ )
	{
	    BinID binid = BinID( inl + idx * hs_.step.r(),
		    		 crl + cdx * hs_.step.c() );
	    if ( hs_.includes( binid ) )
	    {
	        const EM::SubID subid = binid.getSerialized();
	        const float z = horizon1_->getPos( horizon1_->sectionID(0),
							subid ).z;
	        if ( mIsUdf(z) ) continue;
	        sum += z; count++;
	    }
	}
    }
    
    float val = count ? sum / count : mUdf(float);

    subid_ = bid_.getSerialized();
    Coord3 pos = horizon1_->getPos( horizon1_->sectionID(0), subid_ );
    pos.z = val;
    horizon1_->setPos( horizon1_->sectionID(0), subid_, pos, false );

    nrdone_++;
    return Executor::MoreToDo();
}


Executor* Tut::HorSmoother::dataSaver( const MultiID& id )
{
    return horizon1_->geometry().saver( 0, &id );
}
