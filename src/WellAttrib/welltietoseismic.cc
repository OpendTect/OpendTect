/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.8 2009-05-20 16:48:25 cvsbruno Exp $";

#include "welltietoseismic.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "ioman.h"
#include "mousecursor.h"
#include "posvecdataset.h"
#include "task.h"
#include "unitofmeasure.h"
#include "wavelet.h"

#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welltiecshot.h"
#include "welltrack.h"
#include "welltiegeocalculator.h"
#include "welltieunitfactors.h"
#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltiesetup.h"
#include "welltieextractdata.h"

WellTieToSeismic::WellTieToSeismic( Well::Data* wd, 
				    WellTieParams* p,
				    const Attrib::DescSet& ads,
			       	    WellTieDataMGR& mgr,
       				    TaskRunner* tr )
    	: wtsetup_(p->getSetup())
	, params_(*p)  
	, ads_(ads)	       
	, wd_(*wd)  
	, datamgr_(mgr)	   
	, dispdata_(*mgr.getDispData())		   
	, workdata_(*mgr.getWorkData())		   
	, tr_(tr)		  
      	, d2tmgr_(0)
	, dps_(new DataPointSet(false, false))	    
{
    dps_->dataSet().add( new DataColDef( params_.attrnm_ ) );
    if ( wd_.checkShotModel() )
    {
	params_.currvellognm_ = wtsetup_.corrvellognm_;
	WellTieCSCorr cscorr( wd_, params_ );
    }
    geocalc_ = new WellTieGeoCalculator( p, wd );
    d2tmgr_  = new WellTieD2TModelMGR( wd_, wtsetup_, *geocalc_ );
} 


WellTieToSeismic::~WellTieToSeismic()
{
    //if ( d2tmgr_ )  delete d2tmgr_;
    if ( geocalc_ ) delete geocalc_;
    if ( tr_ ) 	    delete tr_;
    if ( dps_ )	    delete dps_;
}


bool WellTieToSeismic::computeAll()
{
    params_.resetTimeParams(0);
    datamgr_.resetData();

    if ( !resampleLogs() ) 	    return false;
    if ( !computeSynthetics() )     return false;

    //resamples WorkData and put in DispData
    datamgr_.setWork2DispData();

    if ( !extractWellTrack() )      return false;
    if ( !extractSeismics() ) 	    return false;
    
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
    
    //geocalc_->lowPassFilter( *workdata_.get(params_.ainm_), 125/params_.step_ );
    
    geocalc_->computeReflectivity( *workdata_.get(params_.ainm_),
       				   *dispdata_.get(params_.refnm_), 
				   params_.step_ );
    convolveWavelet();
    
    return true;
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
    datamgr_.getSortedDPSDataAlongZ( *dps_,
	    *dispdata_.get( params_.attrnm_ ));

    return true;
}


void WellTieToSeismic::convolveWavelet()
{
    IOObj* ioobj = IOM().get( wtsetup_.wvltid_ );
    Wavelet* wvlt = new Wavelet( *Wavelet::get( ioobj ) );
    Array1DImpl<float> wvltvals( wvlt->size() );
    memcpy( wvltvals.getData(), wvlt->samples(), wvlt->size()*sizeof(float) );

    int wvltidx = wvlt->centerSample();
    geocalc_->convolveWavelet( wvltvals, *dispdata_.get(params_.refnm_),
	    			*dispdata_.get(params_.synthnm_), wvltidx );
    
    delete wvlt;
}


Wavelet* WellTieToSeismic::estimateWavelet()
{
    //copy initial wavelet
    Wavelet* wvlt = new Wavelet( *Wavelet::get(IOM().get(wtsetup_.wvltid_)) );
    const int datasz = params_.dispsize_;
    const int wvltsz = wvlt->size();
    const bool iswvltodd = wvltsz%2;
    if ( iswvltodd ) wvlt->reSize( wvltsz+1 );
    Array1DImpl<float> wvltdata( datasz ), wvltvals( wvltsz );

    geocalc_->deconvolve( *dispdata_.get(params_.attrnm_),
	   		  *dispdata_.get(params_.refnm_), wvltdata, wvltsz );
    
    for ( int idx=0; idx<wvltsz; idx++ )
	wvlt->samples()[idx] = wvltdata.get( datasz/2 + idx - wvltsz/2 );
    
    memcpy( wvltvals.getData(),wvlt->samples(), wvltsz*sizeof(float) );
    ArrayNDWindow window( Array1DInfoImpl(wvltsz),
			  false, "CosTaper", 0.15 );
    window.apply( &wvltvals );
    memcpy( wvlt->samples(), wvltvals.getData(), wvltsz*sizeof(float) );
    geocalc_->reverseWavelet( *wvlt );

    return wvlt;
}


/*
bool WellTieToSeismic::computeCorrelCoeff()
{
    genericCrossCorrelation(
    
}
*/

