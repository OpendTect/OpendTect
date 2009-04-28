/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieextractdata.cc,v 1.3 2009-04-28 14:30:26 cvsbruno Exp $";

#include "welltieextractdata.h"

#include "arrayndimpl.h"
#include "datapointset.h"
#include "survinfo.h"

#include "welldata.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welltrack.h"

WellTieExtractTrack::WellTieExtractTrack(  DataPointSet& dps,
					   const Well::Data& d)
    	: Executor("Extracting Well track positions") 
	, dps_(dps) 
	, wd_(d)	    
	, nrdone_(0)
	, timeintv_(0,0,0)
{
} 


#define mAddRow(bv,pos) \
    dr.pos_.z_ = pos.z; dr.pos_.set( bid, pos ); dps_.addRow( dr )

int WellTieExtractTrack::nextStep()
{
    float time = timeintv_.atIndex( nrdone_ );

    Coord3 pos = wd_.track().getPos( wd_.d2TModel()->getDepth(time) );
    pos.z = time;

    //pos.z = wd_.d2TModel()->getTime( md );
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

//    DataPointSet::ColID dpsdahcolidx = dps_.indexOf( "Time" );
  //  dps_.getValues(nrdone_)[dpsdahcolidx] = time; 
    //wd_.d2TModel()->getDepth(time);
    
    nrdone_ ++;
    return Executor::MoreToDo();
}




#define mErrRet(s) { errmsg = s; return; }
WellTieResampleLog::WellTieResampleLog( ObjectSet< Array1DImpl<float> >& arr, 
					const Well::Log& l,
					const Well::Data& wd )
    	: Executor("Processing log data") 
	, workdata_(arr)
	, wd_(wd)	   
	, nrdone_(0)
	, curlogsample_(0)	    
	, logname_(l.name())

{
    BufferString exnm = "Processing "; exnm += logname_; exnm += " Data"; 
    setName( exnm );
    fillProcLog( l );

    BufferString errmsg;
    errmsg += "no valid "; errmsg += l.name();  errmsg += " log selected";
    if ( !isValidLogData( val_ ) ) mErrRet(errmsg);

    interpolateData( dah_, l.dahStep(true),  true );
    interpolateData( val_, l.dahStep(true), false );
}


bool WellTieResampleLog::isValidLogData( const TypeSet<float>& logdata )
{
    if ( logdata.size() == 0 || getFirstDefIdx(logdata) > logdata.size() )
	return false;
    return true;
}


void WellTieResampleLog::fillProcLog( const Well::Log& log )
{
    for ( int idx=0; idx<log.size(); idx++ )
    {
	val_ += log.valArr()[idx];
	dah_ += log.dah(idx);
    }
}

//TODO put in nextStep()
void WellTieResampleLog::interpolateData( TypeSet<float>& data,
					const float dahstep,	
       					const bool isdah )
{
    int startidx = getFirstDefIdx( data );
    int lastidx = getLastDefIdx( data );

    for ( int idx=startidx; idx<lastidx; idx++)
    {
	//no negative values in dens or vel log assumed
	if  ( !isdah && ( mIsUdf(data[idx]) || data[idx] <0 ) )
	    data[idx] = data[idx-1];
	if ( isdah && (mIsUdf(data[idx]) || data[idx] < data[idx-1]
					 || data[idx] >= data[lastidx])  )
		data[idx] = data[idx-1] + dahstep;
    }
    for ( int idx=0; idx<startidx; idx++ )
	data[idx] = data[startidx];
    for ( int idx=lastidx; idx<data.size(); idx++ )
	data[idx] = data[lastidx];
}


int WellTieResampleLog::getFirstDefIdx( const TypeSet<float>& logdata )
{
    int idx = 0;
    while ( mIsUdf(logdata[idx]) )
	idx++;
    return idx;
}


int WellTieResampleLog::getLastDefIdx( const TypeSet<float>& logdata )
{
    int idx = logdata.size()-1;
    while ( mIsUdf( logdata[idx] ) )
	idx--;
    return idx;
}


int WellTieResampleLog::nextStep()
{
    float curtime = timeintv_.atIndex( nrdone_ );
    float curdah = wd_.d2TModel()->getDepth( curtime );
    float curval = 0;
    int tmpidx = curlogsample_;
    updateLogIdx( curdah, tmpidx  );
    curlogsample_ = tmpidx;
    
    if ( curtime > timeintv_.stop || curlogsample_ >= dah_.size() )
	return Executor::Finished();
   
    if ( tmpidx>6 && tmpidx<dah_.size()-9 )
    {
	for ( int idx = tmpidx-7; idx<tmpidx+8; idx++ )
	    curval += val_[idx] / 15;
    }
    else
	curval = val_[curlogsample_];

    if (  curlogsample_ < dah_.size()  )
    {
	workdata_[colnr_]->setValue( nrdone_, curval );
	workdata_[0]->setValue( nrdone_, curtime );
    }

    nrdone_++;
    return Executor::MoreToDo();
}


void WellTieResampleLog::updateLogIdx( float curdah, int& logidx  )
{
    int tmpidx = logidx;
    while ( curdah >= dah_[tmpidx] && tmpidx < val_.size() )
	tmpidx++;
    
    logidx = tmpidx;
}

