/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.34 2009-10-02 13:43:20 cvsbruno Exp $";

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
	, dataholder_(dh)  
	, ads_(ads)
	, wd_(*dh->wd()) 
	, params_(*dh->dpms())		 
	, logsset_(*dh->logsset())	   
	, tr_(tr)		  
      	, d2tmgr_(dh->d2TMGR())
	, dps_(new DataPointSet(false, false))	   
	, wvltset_(dataholder_->wvltset())
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
    logsset_.resetData( params_ );
  
    if ( !resampleLogs() ) 	   return false;
    if ( !computeReflectivity() )  return false;
    if ( !extractWellTrack() )     return false;
    if ( !extractSeismics() ) 	   return false;
    if ( !computeWvltPack() ) 	   return false;

    return true;	
}


bool DataPlayer::computeWvltPack()
{
    if ( !logsset_.size() ) 	   return false;
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

    resLogExecutor( lognms, true,  params_.timeintvs_[0] );

    MouseCursorManager::restoreOverride();

    return true;
}


bool DataPlayer::resLogExecutor( const BufferStringSet& lognms, bool fromwd, 
				 const StepInterval<float>& intv )
{
    bool wasdone = false;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	mDynamicCastGet( WellTie::Log*, tlog, logsset_.getLog(lognms.get(idx)));
	const Well::Log* l = fromwd ? wd_.logs().getLog(lognms.get(idx)): tlog;
	if ( !tlog || !l  ) continue;
	
	const Well::Log* log = new Well::Log( *l );
	WellTie::LogResampler logres( tlog, *log, &wd_, dataholder_ );
	logres.timeintv_ = intv; logres.isavg_ = fromwd;
	wasdone = tr_->execute( logres );
	delete log;
    }
    return wasdone;
}


#define mSetData(newnm,dahnm,val)\
{\
    logsset_.setVal( newnm, logsset_.getVal(dahnm,true), true );\
    logsset_.setVal( newnm, &val );\
}
bool DataPlayer::computeReflectivity()
{ 
    Array1DImpl<float> tmpai( params_.timeintvs_[0].nrSteps() ), 
		       tmpref( params_.timeintvs_[1].nrSteps() );

    geocalc_->computeAI( *logsset_.getVal(params_.currvellognm_),
	      		 *logsset_.getVal(params_.denlognm_), tmpai );
    geocalc_->lowPassFilter( tmpai,  1/( 4*SI().zStep() ) );
    geocalc_->computeReflectivity( tmpai, tmpref, params_.step_ );

    mSetData( params_.ainm_, params_.denlognm_, tmpai );
    
    BufferStringSet lognms; lognms.add( params_.currvellognm_ );
    lognms.add( params_.denlognm_ ); lognms.add( params_.ainm_ );
    resLogExecutor( lognms, false, params_.timeintvs_[1] );

    mSetData( params_.refnm_, params_.ainm_, tmpref );

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
    Wavelet* wvlt = isinitwvltactive ? wvltset_[0] : wvltset_[1]; 
    const int wvltsz = wvlt->size();
    if ( !wvlt || wvltsz <= 0 || wvltsz > params_.timeintvs_[1].nrSteps() ) 
	return false;
    Array1DImpl<float> wvltvals( wvlt->size() );
    memcpy( wvltvals.getData(), wvlt->samples(), wvltsz*sizeof(float) );

    int wvltidx = wvlt->centerSample();

    Array1DImpl<float> tmpsynth ( params_.timeintvs_[1].nrSteps() );
    geocalc_->convolveWavelet( wvltvals, *logsset_.getVal(params_.refnm_), 
			       tmpsynth, wvltidx );
    mSetData( params_.synthnm_, params_.refnm_, tmpsynth );

    return true;
}


bool DataPlayer::estimateWavelet()
{
    const StepInterval<float> si = params_.timeintvs_[2];
    const int datasz = si.nrSteps(); 
    const Interval<float> dhrg = params_.d2T( params_.timeintvs_[2], false );

    delete wvltset_.remove(1); 
    Wavelet* wvlt = new Wavelet( *wvltset_[0] );
    wvltset_ += wvlt;
    if ( !wvlt ) return false;


    wvlt->setName( "Estimated Wavelet" );
    const int wvltsz = params_.estwvltlength_;
    if ( datasz < wvltsz +1 )
       return false;

    const bool iswvltodd = wvltsz%2;
    if ( iswvltodd ) wvlt->reSize( wvltsz+1 );
   
    Array1DImpl<float> wvltarr( datasz ), wvltvals( wvltsz );
    //performs deconvolution
    geocalc_->deconvolve( *logsset_.getVal( params_.attrnm_, false, &dhrg ), 
	    		  *logsset_.getVal( params_.refnm_, false, &dhrg ), 
			   wvltarr, wvltsz );

    //retrieve wvlt samples from the deconvolved vector
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
    const Interval<float> itv = params_.d2T( params_.timeintvs_[2], false );
    const int sz = si.nrSteps();

    Array1DImpl<float> tmpcrosscorr( sz );
    const Array1DImpl<float> syn = *logsset_.getVal( 
						params_.synthnm_, false, &itv );
    const Array1DImpl<float> attr =  *logsset_.getVal( 
						params_.attrnm_, false, &itv );

    geocalc_->crosscorr( syn, attr, tmpcrosscorr );

    mSetData( params_.crosscorrnm_, params_.synthnm_, tmpcrosscorr );
    //computes cross-correl coeff
    LinStats2D ls2d;
    ls2d.use( syn.getData(), attr.getData(), sz );

    dataholder_->corrcoeff() = ls2d.corrcoeff;

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
