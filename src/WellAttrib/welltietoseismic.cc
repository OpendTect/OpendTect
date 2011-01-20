/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.50 2011-01-20 11:14:51 cvsbruno Exp $";

#include "welltietoseismic.h"

#include "aimodel.h"
#include "arrayndimpl.h"
#include "ioman.h"
#include "mousecursor.h"
#include "synthseis.h"
#include "seistrc.h"
#include "task.h"
#include "wavelet.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"

#include "welltiedata.h"
#include "welltieextractdata.h"


namespace WellTie
{
#define mGetWD() Well::Data* wd = data_.wd_; if ( !wd ) return;
DataPlayer::DataPlayer( Data& data, const MultiID& seisid, const LineKey* lk ) 
    : data_(data)		    
    , aimodel_(0)
    , linekey_(lk)
    , seisid_(seisid)
{
    refsz_ = data_.timeintv_.nrSteps();
}


DataPlayer::~DataPlayer()
{
    delete aimodel_;
}


void DataPlayer::computeAll()
{
    resetAIModel();
    generateSynthetics();
    extractSeismics(); 
    copyDataToLogSet();
}


void DataPlayer::extractSeismics()
{
    mGetWD()
    MouseCursorManager::setOverride( MouseCursor::Wait );

    TaskRunner taskrunner;
    TrackExtractor wtextr( wd->track(), wd->d2TModel() );
    taskrunner.execute( wtextr ); 

    const IOObj& ioobj = *IOM().get( seisid_ );
    IOObj* seisobj = ioobj.clone();

    SeismicExtractor seisextr( *seisobj );
    if ( linekey_ )
	seisextr.setLineKey( linekey_ );
    seisextr.setBIDValues( wtextr.getBIDs() );
    seisextr.setInterval( data_.timeintv_ );
    taskrunner.execute( seisextr );
    data_.seistrc_ = seisextr.result(); 

    MouseCursorManager::restoreOverride();
}


static const int cDefaultZResamplingFactor = 20;
void DataPlayer::resetAIModel()
{
    mGetWD()
    delete aimodel_; aimodel_ = 0;
    if ( !wd->d2TModel() || wd->d2TModel()->size() <= 2 ) 
	return; 
    TypeSet<AIModel::DataPoint> pts;
    StepInterval<float> workintv = data_.timeintv_; 
    workintv.step /= cDefaultZResamplingFactor;
    const int worksz = workintv.nrSteps();

    const Well::Log* sonlog = wd->logs().getLog( data_.currvellog() );
    const Well::Log* denlog = wd->logs().getLog( data_.density() );
    if ( !sonlog || !denlog ) return;
    Well::Log vellog( *sonlog );
    if ( data_.isSonic() )
	{ GeoCalculator gc; gc.velLogConv( vellog, GeoCalculator::Son2Vel ); }
    float lastdah = 0;
    for ( int idx=0; idx<worksz; idx++ )
    {
	float dah = wd->d2TModel()->getDah( workintv.atIndex(idx) );
	float vel = vellog.getValue( dah, true ); 
	float den = denlog->getValue( dah, true ); 
	pts += AIModel::DataPoint( dah, vel, den );
	lastdah = dah;
    }
    aimodel_ = new AIModel( pts, lastdah );
    aimodel_->antiAlias();
}


void DataPlayer::generateSynthetics()
{
    if ( !aimodel_ ) return;
    Seis::SynthGenerator gen( *aimodel_, data_.initwvlt_ ); 
    SamplingData<float> sd( data_.timeintv_ ); 
    sd.start -= aimodel_->startTime() + sd.step;
    gen.setOutSampling( sd, refsz_ );
    gen.generate();
    data_.synthtrc_ = gen.result();
}


void DataPlayer::copyDataToLogSet()
{
    mGetWD()
    if ( !aimodel_ ) return;
    TypeSet<float> dah, son, den, refs, ai, synth;
    for ( int idx=0; idx<refsz_; idx++ )
    {
	dah += wd->d2TModel()->getDah( data_.timeintv_.atIndex(idx) );
	AIModel::Domain dom = AIModel::TWT;
	float time = aimodel_->convertTo( dah[idx], AIModel::TWT );
	son += aimodel_->velocityAt( dom, time );
	den += aimodel_->densityAt( dom, time );
	ai += aimodel_->aiAt( dom, time );
    }
    SamplingData<float> sd( data_.timeintv_ ); 
    sd.start -= aimodel_->startTime() + sd.step;
    aimodel_->getReflectivity( sd, refs );

    data_.logset_.empty();
    createLog( data_.sonic(), dah, son ); 
    createLog( data_.density(), dah, den ); 
    createLog( data_.ai(), dah, ai );
    createLog( data_.reflectivity(), dah, refs  );
}


void DataPlayer::createLog( const char* nm, const TypeSet<float>& dah, 
				const TypeSet<float>& vals )
{
    Well::Log* log = new Well::Log( nm );
    data_.logset_.add( log );
    for( int idx=0; idx<vals.size(); idx ++)\
	log->addValue( dah[idx], vals[idx] );
}


}; //namespace WellTie
