

/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : May 2007
-*/

static const char* rcsID = "$ $";

#include "tuthortools.h"
#include "ioobj.h"


Tut::HorTools::~HorTools()
{
    delete iter_;
    hortop_->unRef();
    horbot_->unRef();
}


int Tut::HorTools::totalNr() const
{
    return hs_.totalNr();
}


void Tut::HorTools::setOutHor( bool top )
{
    horout_ = top ? hortop_ : horbot_;
}


void Tut::HorTools::setHorSamp( StepInterval<int> inlrg, 
				StepInterval<int> crlrg )
{
    hs_.set( inlrg, crlrg );
    iter_ = new HorSamplingIterator( hs_ );
}


int Tut::HorTools::nextStep()
{
    if ( !iter_->next(bid_) )
	return Executor::Finished;

    const EM::SubID subid = bid_.getSerialized();
    const float z1 = hortop_->getPos( hortop_->sectionID(0), subid ).z;
    const float z2 = horbot_->getPos( horbot_->sectionID(0), subid ).z;
		        
    float val = mUdf(float);
    if ( !mIsUdf(z1) && !mIsUdf(z2) )
    {
        val = z2 - z1;

	if ( SI().zIsTime() ) val *= 1000;
									            }

    posid_.setSubID( subid );
    horout_->auxdata.setAuxDataVal( dataidx_, posid_, val );

    nrdone_++;
    return Executor::MoreToDo;
}

///////////////////////////////////////////////////////////////////////////////

Tut::HorFilter::~HorFilter()
{
    delete iter_;
    hor_->unRef();
}


int Tut::HorFilter::totalNr() const
{
    return hs_.totalNr();
}


void Tut::HorFilter::setHorSamp( StepInterval<int> inlrg, 
				StepInterval<int> crlrg )
{
    hs_.set( inlrg, crlrg );
    iter_ = new HorSamplingIterator( hs_ );
}


int Tut::HorFilter::nextStep()
{
    if ( !iter_->next(bid_) )
	return Executor::Finished;

    int inl = bid_.r(); int crl = bid_.c();
    float sum = 0;
    int count = 0;
    for( int idx=-1; idx<2; idx++ )
    {
	for( int cdx=-1; cdx<2; cdx++ )
	{
	    if( idx && cdx ) continue;
	    BinID binid = BinID( inl + idx * hs_.step.r(),
		    		 crl + cdx * hs_.step.c() );
	    if( hs_.includes( binid ) )
	    {
	        const EM::SubID subid = binid.getSerialized();
	        const float z = hor_->getPos( hor_->sectionID(0), subid ).z;
	        if ( mIsUdf(z) ) continue;
	        sum += z; count++;
	    }
	}
    }
    
    float val = count ? sum / count : mUdf(float);

    subid_ = bid_.getSerialized();
    Coord3 pos = hor_->getPos( hor_->sectionID(0), subid_ );
    pos.z = val;
    hor_->setPos( hor_->sectionID(0), subid_, pos, false );

    nrdone_++;
    return Executor::MoreToDo;
}


