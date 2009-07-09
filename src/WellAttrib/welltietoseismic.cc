/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.18 2009-07-09 14:36:52 cvsbruno Exp $";

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
#include "task.h"
#include "unitofmeasure.h"
#include "wavelet.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltrack.h"

#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltieextractdata.h"
#include "welltiegeocalculator.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"

WellTieToSeismic::WellTieToSeismic( WellTieDataHolder* dh, 
				    const Attrib::DescSet& ads,
       				    TaskRunner* tr ) 
    	: wtsetup_(dh->setup())
	, ads_(ads)
	, wd_(*dh->wd()) 
	, params_(*dh->dpms())		 
	, datamgr_(*dh->datamgr())	   
	, dispdata_(*dh->dispData())		   
	, workdata_(*dh->extrData())		   
	, wtdata_(dh->data())					   
	, corrdata_(*dh->corrData())		   
	, tr_(tr)		  
      	, d2tmgr_(dh->d2TMGR())
	, dps_(new DataPointSet(false, false))	   
{
    dps_->dataSet().add( new DataColDef( params_.attrnm_ ) );
    geocalc_ = new WellTieGeoCalculator( dh->params(), &wd_ );
} 


WellTieToSeismic::~WellTieToSeismic()
{
    if ( geocalc_ ) delete geocalc_;
    if ( tr_ ) 	    delete tr_;
    if ( dps_ )	    delete dps_;
}


bool WellTieToSeismic::computeAll()
{
    //setUpData 
    datamgr_.resetData();
  
    //compute Synth  
    if ( !resampleLogs() ) 	   return false;
    if ( !computeSynthetics() )    return false;

    //WorkData resampled and put in DispData at seismic sample rate 
    datamgr_.rescaleData( workdata_, dispdata_, 6, params_.step_ );

    if ( !extractWellTrack() )     return false;
    if ( !extractSeismics() ) 	   return false;

    createDispLogs();

    //DispData rescaled between user-specified times
    datamgr_.rescaleData( dispdata_, corrdata_, params_.nrdatacols_, 
	    params_.corrtimeintv_.start, params_.corrtimeintv_.stop );
    if ( !estimateWavelet() )	   return false;
    if ( !computeCrossCorrel() )   return false;
    
    return true;	
}


bool WellTieToSeismic::extractWellTrack()
{
    dps_->bivSet().empty();
    dps_->dataChanged();

    MouseCursorManager::setOverride( MouseCursor::Wait );
    
    WellTieExtractTrack wtextr( *dps_, &wd_ );
    wtextr.timeintv_ = params_.getTimeScale();
    wtextr.timeintv_.step = params_.timeintv_.step*params_.step_;
    if ( !tr_->execute( wtextr ) ) return false;

    MouseCursorManager::restoreOverride();
    dps_->dataChanged();

    return true;
}


bool WellTieToSeismic::resampleLogs()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );

    resLogExecutor( wtsetup_.corrvellognm_ );
    resLogExecutor( wtsetup_.vellognm_ );
    resLogExecutor( wtsetup_.denlognm_ );

    MouseCursorManager::restoreOverride();

    return true;
}


bool WellTieToSeismic::resLogExecutor( const char* logname )
{
    const Well::Log* log =  wd_.logs().getLog( logname );
    if ( !log  ) return false;

    WellTieResampleLog reslog( workdata_, *log, &wd_, *geocalc_ );
    reslog.timenm_ = params_.timenm_; reslog.dptnm_ = params_.dptnm_;
    reslog.timeintv_ = params_.getTimeScale();
    return tr_->execute( reslog );
}


bool WellTieToSeismic::computeSynthetics()
{ 
    geocalc_->computeAI( *workdata_.get(params_.currvellognm_),
	      		 *workdata_.get(wtsetup_.denlognm_),
	      	 	 *workdata_.get(params_.ainm_) );
    
    //geocalc_->lowPassFilter( *workdata_.get(params_.ainm_), 
    //				1/SI().zStep()/params_.step_ );

    geocalc_->computeReflectivity( *workdata_.get(params_.ainm_),
       				   *dispdata_.get(params_.refnm_), 
				   params_.step_ );
    convolveWavelet();
    
    return true;
}


void WellTieToSeismic::createDispLogs()
{
    for ( int logidx=0; logidx<params_.colnms_.size(); logidx++ )
    {
	Well::Log* log = new Well::Log();
	wtdata_.logset_ += log;

	if ( wtsetup_.vellognm_ == params_.colnms_.get(logidx) )
	    log->setName( params_.vellognm_ );
	else if ( wtsetup_.corrvellognm_ == params_.colnms_.get(logidx) ) 
	    log->setName( params_.corrvellognm_ );
	else if ( wtsetup_.denlognm_ == params_.colnms_.get(logidx) )
	    log->setName( params_.denlognm_ );
	else
	    log->setName( params_.colnms_.get(logidx) );

	wd_.logs().add( log );

	const char* colnm = params_.colnms_.get(logidx);
	for ( int idx=0; idx<dispdata_.getLength(); idx++ )
	{
	    log->addValue( dispdata_.get(params_.dptnm_,idx), 
		    	   dispdata_.get(colnm,idx) );
	}
    }
}


bool WellTieToSeismic::extractSeismics()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    Attrib::EngineMan aem; BufferString errmsg;
    PtrMan<Executor> tabextr = aem.getTableExtractor( *dps_, ads_, errmsg,
						       dps_->nrCols()-1 );
    MouseCursorManager::restoreOverride();
    if (!tr_->execute( *tabextr )) return false;
    dps_->dataChanged();

    //retrieve data from DPS    
    datamgr_.getSortedDPSDataAlongZ( *dps_, *dispdata_.get( params_.attrnm_ ));

    return true;
}


void WellTieToSeismic::convolveWavelet()
{
    IOObj* ioobj = IOM().get( wtsetup_.wvltid_ );
    Wavelet* wvlt = new Wavelet( *Wavelet::get( ioobj ) );
    if ( !wvlt ) return;
    Array1DImpl<float> wvltvals( wvlt->size() );
    memcpy( wvltvals.getData(), wvlt->samples(), wvlt->size()*sizeof(float) );

    int wvltidx = wvlt->centerSample();
    geocalc_->convolveWavelet( wvltvals, *dispdata_.get(params_.refnm_),
			       *dispdata_.get(params_.synthnm_), wvltidx );
    delete wvlt;
}


bool WellTieToSeismic::estimateWavelet()
{
    const int datasz = params_.corrsize_; 
    //copy initial wavelet
    Wavelet* wvlt = new Wavelet( *Wavelet::get(IOM().get(wtsetup_.wvltid_)) );
    const int wvltsz = wvlt->size();
    if ( datasz < wvltsz )
       return false;

    const bool iswvltodd = wvltsz%2;
    if ( iswvltodd ) wvlt->reSize( wvltsz+1 );
   
    Array1DImpl<float> wvltarr( datasz ), wvltvals( wvltsz );
    //performs deconvolution
    geocalc_->deconvolve( *corrdata_.get(params_.attrnm_), 
	    		  *corrdata_.get(params_.refnm_), 
			  wvltarr, wvltsz );

    //retrieve wvlt samples from the deconvolved vector
    for ( int idx=0; idx<wvltsz; idx++ )
	wvlt->samples()[idx] = wvltarr.get( datasz/2 + idx - wvltsz/2 );
    
    memcpy( wvltvals.getData(),wvlt->samples(), wvltsz*sizeof(float) );
    ArrayNDWindow window( Array1DInfoImpl(wvltsz), false, "CosTaper", 0.15 );
    window.apply( &wvltvals );
    memcpy( wvlt->samples(), wvltvals.getData(), wvltsz*sizeof(float) );

    geocalc_->reverseWavelet( *wvlt );
    wtdata_.wvltest_ = *wvlt;
    return true;
}


bool WellTieToSeismic::computeCrossCorrel()
{
    geocalc_->crosscorr( *corrdata_.get(params_.synthnm_), 
	    		 *corrdata_.get(params_.attrnm_), 
	    		 *corrdata_.get(params_.crosscorrnm_));

    //computes cross-correl coeff
    LinStats2D ls2d;
    ls2d.use( corrdata_.get(params_.synthnm_)->getData(),
	      corrdata_.get(params_.attrnm_)->getData(),
	      params_.corrsize_ );
    wtdata_.corrcoeff_ = ls2d.corrcoeff;

    return true;
}

