/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.37 2009-10-22 12:01:32 cvsbruno Exp $";

#include "welltietoseismic.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "ioman.h"
#include "linear.h"
#include "mousecursor.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "task.h"
#include "wavelet.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welld2tmodel.h"
#include "welltrack.h"

#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltiegeocalculator.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"


namespace WellTie
{

DataPlayer::DataPlayer( WellTie::DataHolder* dh, 
			const Attrib::DescSet& ads,
			TaskRunner* tr ) 
    	: wtsetup_(dh->setup())
	, dholder_(dh)  
	, ads_(ads)
	, wd_(*dh->wd()) 
	, params_(*dh->dpms())		 
	, logset_(*dh->logset())	   
	, tr_(tr)		  
      	, d2tmgr_(dh->d2TMGR())
	, dps_(new DataPointSet(false, false))	   
{
    dps_->dataSet().add( new DataColDef( params_.attrnm_ ) );
    geocalc_ = new WellTie::GeoCalculator(*dh);
} 


DataPlayer::~DataPlayer()
{
    delete geocalc_;
    delete tr_;
    delete dps_;
}


bool DataPlayer::computeAll()
{
    dholder_->resetLogData();
  
    if ( !resampleLogs() ) 	   return false;
    if ( !computeReflectivity() )  return false;
    if ( !extractWellTrack() )     return false;
    if ( !extractSeismics() ) 	   return false;
    if ( !computeWvltPack() ) 	   return false;

    return true;	
}


bool DataPlayer::computeWvltPack()
{
    if ( !logset_.size() ) 	   return false;
    if ( !convolveWavelet() ) 	   return false;
    if ( !estimateWavelet() )	   return false;
    if ( !computeCrossCorrel() )   return false;

    return true;
}


bool DataPlayer::extractWellTrack()
{
    dps_->bivSet().empty();
    dps_->dataChanged();

    MouseCursorManager::setOverride( MouseCursor::Wait );
    
    WellTie::TrackExtractor wtextr( dps_, &wd_ );
    wtextr.timeintv_ = params_.timeintvs_[1];
    if ( !tr_->execute( wtextr ) ) return false;

    MouseCursorManager::restoreOverride();
    dps_->dataChanged();

    return true;
}


bool DataPlayer::resampleLogs()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );

    BufferStringSet lognms; 
    lognms.add( wtsetup_.corrvellognm_ );
    lognms.add( wtsetup_.vellognm_ );
    lognms.add( wtsetup_.denlognm_ );

    resLogExecutor( lognms, true, params_.timeintvs_[0] );

    MouseCursorManager::restoreOverride();

    return true;
}


bool DataPlayer::resLogExecutor( const BufferStringSet& lognms, bool fromwd, 
				 const StepInterval<float>& intv )
{
    bool wasdone = false;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	Well::Log* newlog = logset_.getLog( lognms.get(idx) );
	const Well::Log* log = fromwd ? wd_.logs().getLog(lognms.get(idx)) 
				      : newlog;
	if ( !newlog || !log )
	    continue;
	
	const Well::Log* orglog = new Well::Log( *log );
	WellTie::LogResampler logres( newlog, *orglog, &wd_, dholder_ );
	logres.setTimeIntv( intv ); logres.isavg_ = fromwd;
	wasdone = tr_->execute( logres );
	delete orglog;
    }
    return wasdone;
}


#define mSetData(lognm,dahlognm,arr)\
    dholder_->setLogVal( lognm, &arr, dholder_->getLogVal( dahlognm, true ) );
bool DataPlayer::computeReflectivity()
{ 
    BufferStringSet lognms; lognms.add( params_.currvellognm_ );
    lognms.add( params_.denlognm_ ); lognms.add( params_.ainm_ );

    Array1DImpl<float> ai( params_.timeintvs_[0].nrSteps() );
    Array1DImpl<float> ref( params_.timeintvs_[1].nrSteps() );
    geocalc_->computeAI( *dholder_->arr( lognms.get(0) ), 
			 *dholder_->arr( lognms.get(1) ), ai ); 

    geocalc_->lowPassFilter( ai, 1/( 4*SI().zStep() ) );
    geocalc_->computeReflectivity( ai, ref, params_.step_ );

    mSetData( params_.ainm_, lognms.get(1), ai );
    resLogExecutor( lognms, false, params_.timeintvs_[1] );
    mSetData( params_.refnm_, lognms.get(1), ref );

    return true;
}


bool DataPlayer::extractSeismics()
{
    Attrib::EngineMan aem; BufferString errmsg;
    PtrMan<Executor> tabextr = aem.getTableExtractor( *dps_, ads_, errmsg,
						       dps_->nrCols()-1 );
    if ( !tabextr ) return false;
    if (!tr_->execute( *tabextr )) return false;
    dps_->dataChanged();

    Array1DImpl<float> tmpseis ( params_.timeintvs_[1].nrSteps() );
    getDPSZData( *dps_, tmpseis );

    mSetData( params_.attrnm_, params_.refnm_, tmpseis );

    return true;
}


bool DataPlayer::convolveWavelet()
{
    bool isinitwvltactive = params_.isinitwvltactive_;
    Wavelet* wvlt = isinitwvltactive ? dholder_->wvltset()[0] 
				     : dholder_->wvltset()[1]; 
    const int wvltsz = wvlt->size();
    if ( !wvlt || wvltsz <= 0 || wvltsz > params_.timeintvs_[1].nrSteps() ) 
	return false;
    Array1DImpl<float> wvltvals( wvlt->size() );
    memcpy( wvltvals.getData(), wvlt->samples(), wvltsz*sizeof(float) );

    int wvltidx = wvlt->centerSample();

    Array1DImpl<float> tmpsynth ( params_.timeintvs_[1].nrSteps() );

    geocalc_->convolveWavelet( wvltvals, *dholder_->arr(params_.refnm_), 
				tmpsynth, wvltidx );
    mSetData( params_.synthnm_, params_.refnm_, tmpsynth );

    return true;
}


bool DataPlayer::estimateWavelet()
{
    if ( !params_.isinitwvltactive_ ) return true;
    const StepInterval<float> si = params_.timeintvs_[2];
    const int datasz = si.nrSteps(); 

    WellTie::LogResampler refres( 0, *logset_.getLog(params_.refnm_), &wd_ );
    refres.setTimeIntv( si ); refres.isavg_ = false;
    refres.execute(); 
    
    WellTie::LogResampler attrres( 0, *logset_.getLog(params_.attrnm_), &wd_ );
    attrres.setTimeIntv( si ); attrres.isavg_ = false;
    attrres.execute(); 

    Wavelet* wvlt = dholder_->wvltset()[1]; 
    if ( !wvlt ) return false;

    wvlt->setName( "Estimated Wavelet" );
    int wvltsz = params_.estwvltlength_;
    if ( datasz < wvltsz +1 )
       return false;

    wvltsz += wvltsz%2 ? 0 : 1;
    wvlt->reSize( wvltsz );
   
    Array1DImpl<float> wvltarr( datasz ), wvltvals( wvltsz );
    geocalc_->deconvolve( *attrres.vals_, *refres.vals_, wvltarr, wvltsz );

    for ( int idx=0; idx<wvltsz; idx++ )
	wvlt->samples()[idx] = wvltarr.get( datasz/2 + idx - wvltsz/2 + 1 );
    
    memcpy( wvltvals.getData(),wvlt->samples(), wvltsz*sizeof(float) );
    ArrayNDWindow window( Array1DInfoImpl(wvltsz), false, "CosTaper", .05 );
    window.apply( &wvltvals );
    memcpy( wvlt->samples(), wvltvals.getData(), wvltsz*sizeof(float) );
  
    return true;
}


bool DataPlayer::computeCrossCorrel()
{
    const StepInterval<float> si = params_.timeintvs_[2];
    const int sz = si.nrSteps();
    
    WellTie::LogResampler synres( logset_.getLog(params_.crosscorrnm_), 
	    			  *logset_.getLog(params_.synthnm_), &wd_ );
    synres.setTimeIntv( si ); synres.isavg_ = false;
    synres.execute(); 
    
    WellTie::LogResampler attrres( 0, *logset_.getLog(params_.attrnm_), &wd_ );
    attrres.setTimeIntv( si ); attrres.isavg_ = false;
    attrres.execute();

    Array1DImpl<float> tmpcrosscorr( sz );

    geocalc_->crosscorr( *synres.vals_, *attrres.vals_, tmpcrosscorr );

    dholder_->setLogVal( params_.crosscorrnm_, &tmpcrosscorr, synres.dahs_ );
    //computes cross-correl coeff
    LinStats2D ls2d;
    ls2d.use( synres.vals_->getData(), attrres.vals_->getData(), sz );

    dholder_->corrcoeff() = ls2d.corrcoeff;

    return true;
}


void DataPlayer::getDPSZData( const DataPointSet& dps, Array1DImpl<float>& vals)
{
    TypeSet<float> zvals, tmpvals;
    for ( int idx=0; idx<dps.size(); idx++ )
	zvals += dps.z(idx);

    const int sz = zvals.size();
    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), mVarLenArr(zidxs), sz );

    for ( int colidx=0; colidx<dps.nrCols(); colidx++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    float val = dps.getValues(zidxs[idx])[colidx];
	    tmpvals += mIsUdf(val) ? 0 : val;
	}
    }
    memcpy(vals.getData(), tmpvals.arr(), vals.info().getSize(0)*sizeof(float));
}    


}; //namespace WellTie
