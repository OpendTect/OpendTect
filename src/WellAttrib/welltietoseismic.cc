/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.1 2009-04-21 13:56:00 cvsbruno Exp $";

#include "welltietoseismic.h"

#include "arrayndimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "mousecursor.h"
#include "posvecdataset.h"
#include "sorting.h"
#include "survinfo.h"
#include "seisioobjinfo.h"
#include "task.h"
#include "unitofmeasure.h"
#include "wavelet.h"

#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welltrack.h"
#include "welltiecshot.h"
#include "welltiegeocalculator.h"
#include "welltied2tmodelmanager.h"
#include "welltiesetup.h"
#include "welltieextractdata.h"
#include "welltiesynthetics.h"

WellTieToSeismic::WellTieToSeismic(const WellTieSetup& wts,
       				const Attrib::DescSet& ads,
				DataPointSet& dps,
			       	ObjectSet< Array1DImpl<float> >& data,
       				Well::Data& d,TaskRunner* tr)
    	: wtsetup_(wts)
	, ads_(ads)
	, dps_(dps)	   
	, dispdata_(data)		   
	, wd_(d)	   
      	, d2tmgr_(0)
	, tr_(tr)		  
{
    if ( !createDPSCols() ) return;
    if ( !setLogsParams() ) return;
    checkShotCorr();
    geocalc_ = new WellTieGeoCalculator( wts );
    d2tmgr_  = new WellTieD2TModelManager( wd_, *geocalc_ );
} 


WellTieToSeismic::~WellTieToSeismic()
{
    if ( d2tmgr_ ) delete d2tmgr_;
    if ( cscorr_ ) delete cscorr_;
    if ( geocalc_ ) delete geocalc_;
    if ( tr_ ) delete tr_;
    for (int idx=0; idx< workdata_.size(); idx++)
	delete ( workdata_.remove(idx) );
}


#define mStep 20
#define mComputeStepFactor SI().zStep()/mStep
bool WellTieToSeismic::setLogsParams()
{
    const Well::Log& vellog = *wd_.logs().getLog( wtsetup_.denlognm_ );
    const Well::Track& track = wd_.track();

    timeintv_.start = wd_.d2TModel()->getTime( vellog.dah(0) );
    timeintv_.stop  = wd_.d2TModel()->getTime( vellog.dah(vellog.size()-1) );
    timeintv_.step  =  mComputeStepFactor;

    float samplenr = ( timeintv_.stop - timeintv_.start) /  timeintv_.step;
    for ( int idx=0; idx<8; idx++)
    {
	workdata_ += new Array1DImpl<float>( (int)samplenr+1 );
	dispdata_ += new Array1DImpl<float>( (int)(samplenr/mStep)+1 );	
    }
    return true;
}


bool WellTieToSeismic::computeAll()
{
    if ( !extractWellTrack() )  return false;
    if ( !resampleLogs() ) 	return false;
    if ( !computeSynthetics() ) return false;
    if ( !extractSeismics() ) 	return false;
    fillDispData();
    
    return true;	
}


void WellTieToSeismic::checkShotCorr()
{
    if ( wd_.checkShotModel() )
	cscorr_ = new WellTieCSCorr( wd_, wtsetup_ );
}


bool WellTieToSeismic::extractWellTrack()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    
    WellTieExtractTrack wtextr( dps_, wd_ );
    wtextr.timeintv_ = timeintv_;
    wtextr.timeintv_.step = timeintv_.step*mStep;
    if ( !tr_->execute( wtextr ) ) return false;

    MouseCursorManager::restoreOverride();
    dps_.dataChanged();

    return true;
}


bool WellTieToSeismic::resampleLogs()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );

    resLogExecutor( wtsetup_.corrvellognm_, 1 );
    resLogExecutor( wtsetup_.vellognm_, 2  );
    resLogExecutor( wtsetup_.denlognm_, 3  );

    MouseCursorManager::restoreOverride();
    dps_.dataChanged();

    return true;
}


bool WellTieToSeismic::resLogExecutor( const char* logname, int colnr )
{
    const Well::Log* log =  wd_.logs().getLog( logname );
    if ( !log  ) return false;

    WellTieResampleLog reslog( workdata_, *log, wd_ );
    reslog.colnr_ = colnr;
    reslog.timeintv_ = timeintv_;
    return tr_->execute( reslog );
}


bool WellTieToSeismic::computeSynthetics()
{
    wtsynth_ = new WellTieSynthetics( wtsetup_, workdata_, dispdata_ );
    if ( !wtsynth_->computeSyntheticsData( *geocalc_) )  
	return false;

    return true;    
}


bool WellTieToSeismic::extractSeismics()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    Attrib::EngineMan aem; BufferString errmsg;
    PtrMan<Executor> tabextr = aem.getTableExtractor( dps_, ads_, errmsg,
						     dps_.nrCols()-1);
    MouseCursorManager::restoreOverride();
    if (!tr_->execute( *tabextr )) return false;
    dps_.dataChanged();

    return true;
}


void WellTieToSeismic::fillDispData()
{
    for ( int colidx=0; colidx<5; colidx++)
    {
	for ( int idx=0; idx<dps_.size(); idx++)
	    dispdata_[colidx]->setValue( idx,workdata_[colidx]->get(mStep*idx)); 
    }

    sortDPSDataAlongZ();
    for ( int colidx=0; colidx<dps_.nrCols(); colidx++ )
    {
	for ( int idx=0; idx<dps_.size(); idx++ )
	    dispdata_[dispdata_.size()-1]->setValue( idx, 
					     dps_.getValues(idx)[colidx] );
    }
}


void WellTieToSeismic::sortDPSDataAlongZ()
{
    TypeSet<float> zvals;
    for ( int idx=0; idx<dps_.size(); idx++ )
	zvals += dps_.z(idx);

    int sz = zvals.size();
    if ( !sz )  return;

    int zidxs[sz];
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), zidxs, sz );

    TypeSet<float> data;
    for ( int colidx=1; colidx<dps_.nrCols(); colidx++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	    data += dps_.getValues(idx)[colidx];
	for ( int idx=0; idx<sz; idx++ )
	    dps_.getValues(idx)[colidx] = data[zidxs[idx]];
	data.erase();
    }
}


/*
for ( int idx=0; idx<dps_.size(); idx++  )
{
if  ( dps_.z(idx ) == workdata_[0]->get( mStep*idx ) )
{
for ( int colidx=0; colidx<dps_.nrCols()-2; colidx++  )
	       dps_.getValues(idx)[colidx] = workdata_[colidx]->get(mStep*idx);

	    for ( int dps_.nrCols()-2; colidx<dps_.nrCols(); colidx++  )
		dps_.getValues(idx)[colidx] = workdata_[colidx]->get(idx);
	}
    }*/


#define mAddCol(nm)  \
     dps_.dataSet().add( new DataColDef( nm ) );
bool WellTieToSeismic::createDPSCols()
{
   // mAddCol( wtsetup_.denlognm_ );
   // mAddCol( wtsetup_.vellognm_ );
   // mAddCol( wtsetup_.corrvellognm_ );
   // mAddCol( "Computed AI" );
   // mAddCol( "Computed Reflectivity" );
   // mAddCol( "Synthetics" );
    
    const Attrib::Desc* ad = ads_.getDesc( wtsetup_.attrid_ );
    if ( !ad ) return false;
    Attrib::SelInfo attrinf( &ads_, 0, ads_.is2D() );
    BufferStringSet bss;
    SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(0) ) );
    sii.getDefKeys( bss, true );
    const char* defkey = bss.get(0).buf();
    BufferString attrnm = ad->userRef();
    BufferString attr2cube = SeisIOObjInfo::defKey2DispName(defkey,attrnm);
    mAddCol( attr2cube );

    return true;
}


Wavelet* WellTieToSeismic::estimateWavelet()
{ 
    return  wtsynth_->estimateWvlt( *geocalc_);
}



