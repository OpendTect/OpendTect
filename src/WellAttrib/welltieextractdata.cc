/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieextractdata.cc,v 1.11 2009-09-03 14:04:30 cvsbruno Exp $";

#include "welltieextractdata.h"
#include "welltiegeocalculator.h"

#include "arrayndimpl.h"
#include "interpol1d.h"
#include "datapointset.h"
#include "survinfo.h"

#include "welldata.h"
#include "welltiedata.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welltrack.h"


namespace WellTie
{


#define mAddRow(bv,pos) \
    dr.pos_.z_ = pos.z; dr.pos_.set( bid, pos ); dps_.addRow( dr )
int TrackExtractor::nextStep()
{
    double time = timeintv_.atIndex( nrdone_ );

    Coord3 pos = wd_.track().getPos( wd_.d2TModel()->getDepth(time) );
    pos.z = time;

    const BinID bid = SI().transform( pos );
    if ( !bid.inl && !bid.crl )
    {
	nrdone_++;
       	return Executor::MoreToDo();
    }
    const int d2tsz = wd_.d2TModel()->size();
    if ( time>timeintv_.stop )
	return Executor::Finished();
    DataPointSet::DataRow dr;
    mAddRow( bid, pos );
    
    dps_.dataChanged();

    nrdone_ ++;
    return Executor::MoreToDo();
}


#define mErrRet(s) { errmsg = s; return; }
LogResampler::LogResampler( WellTie::DataSet& arr, 
			    const Well::Log& l, const Well::Data* d,
			    WellTie::GeoCalculator& geocalc )
    	: Executor("Processing log data") 
	, workdata_(arr)
	, wd_(*d)	   
	, nrdone_(0)
	, curlogsample_(0)	    
	, logname_(l.name())
{
    BufferString exnm = "Processing "; exnm += logname_; exnm += " Data"; 
    setName( exnm );
    fillProcLog( l );

    BufferString errmsg;
    errmsg += "no valid "; errmsg += l.name();  errmsg += " log selected";
    if ( !geocalc.isValidLogData( val_ ) ) mErrRet(errmsg);

    geocalc.interpolateLogData( dah_, l.dahStep(true), true );
    geocalc.interpolateLogData( val_, l.dahStep(true), false );
}


void LogResampler::fillProcLog( const Well::Log& log )
{
    for ( int idx=0; idx<log.size(); idx++ )
    {
	val_ += log.valArr()[idx];
	dah_ += log.dah(idx);
    }
}


int LogResampler::nextStep()
{
    float curtime = timeintv_.atIndex( nrdone_ );
    float curdah = wd_.d2TModel()->getDepth( curtime );
    float curval = 0;
    int tmpidx = curlogsample_;
    updateLogIdx( curdah, tmpidx  );
    curlogsample_ = tmpidx;
    
    if ( curtime > timeintv_.stop ||  nrdone_ >= workdata_.getLength() )
	return Executor::Finished();
   
    if ( curdah < dah_[0] )
	curval = val_[0];
    else if ( curdah > dah_[dah_.size()-1] )
	curval = mUdf(float);
    else
    {
	if ( tmpidx>1 && tmpidx<dah_.size()-2 )
	{
    //	Interpolate::PolyReg1DWithUdf<float> pr;
    //	pr.set( val_[tmpidx], val_[tmpidx], val_[tmpidx+1], val_[tmpidx+1]);
    //	curval += pr.apply( ( curdah - dah_[tmpidx] )
    //			  / ( dah_[tmpidx+1] - dah_[tmpidx] ) );

    //3 points avg seems to work better than the polynomial interpolation
	    curval += ( (val_[tmpidx+1]+val_[tmpidx-1]+val_[tmpidx])*1/3 );
	}
	else
	    curval = val_[curlogsample_];
    }

    workdata_.get(logname_)->setValue( nrdone_, curval );
    workdata_.get(dptnm_)->setValue( nrdone_, curdah );
    workdata_.get(timenm_)->setValue( nrdone_, curtime );

    nrdone_++;
    return Executor::MoreToDo();
}


void LogResampler::updateLogIdx( float curdah, int& logidx  )
{
    int tmpidx = logidx;
    while ( tmpidx < val_.size() && curdah >= dah_[tmpidx] )
	tmpidx++;
    
    logidx = tmpidx;
}

}; //namespace WellTie
