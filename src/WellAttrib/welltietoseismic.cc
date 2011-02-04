/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.52 2011-02-04 14:00:54 cvsbruno Exp $";

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

#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltieextractdata.h"


namespace WellTie
{
#define mGetWD() Well::Data* wd = data_.wd_; if ( !wd ) return;
DataPlayer::DataPlayer( Data& data, const MultiID& seisid, const LineKey* lk ) 
    : data_(data)		    
    , aimodel_(0)
    , seisid_(seisid)
    , linekey_(lk)
{
    refsz_ = data_.timeintv_.nrSteps();
}


DataPlayer::~DataPlayer()
{
    delete aimodel_; aimodel_ = 0;
}


void DataPlayer::computeAll()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );

    resetAIModel();
    generateSynthetics();
    extractSeismics(); 
    copyDataToLogSet();

    MouseCursorManager::restoreOverride();
}


void DataPlayer::extractSeismics()
{
    mGetWD()

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
}


static const int cDefaultZResamplingFactor = 20;
void DataPlayer::resetAIModel()
{
    delete aimodel_; aimodel_ = 0;
    mGetWD()
    if ( !wd->d2TModel() || wd->d2TModel()->size() <= 2 ) 
	return; 
    TypeSet<AILayer> pts;
    StepInterval<float> workintv = data_.timeintv_; 
    workintv.step /= cDefaultZResamplingFactor;
    const int worksz = workintv.nrSteps();

    const Well::Log* sonlog = wd->logs().getLog( data_.sonic() );
    const Well::Log* denlog = wd->logs().getLog( data_.density() );
    if ( !denlog || !sonlog ) return;
    Well::Log vellog( *sonlog );
    CheckShotCorr cscorr( vellog, *wd->d2TModel(), !data_.isSonic() );
    float lastdah = 0; 
    for ( int idx=0; idx<worksz; idx++ )
    {
	float dah = wd->d2TModel()->getDah( workintv.atIndex(idx) );
	float vel = vellog.getValue( dah, true ); 
	float den = denlog->getValue( dah, true ); 
	pts += AILayer( dah, vel, den );
	lastdah = dah;
    }
    aimodel_ = new AIModel( pts, lastdah );
    aimodel_->antiAlias();
}


void DataPlayer::generateSynthetics()
{
    if ( !aimodel_ ) return;
    const Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_ 
						  : data_.estimatedwvlt_;
    AIModel* newaimodel = new AIModel(*aimodel_); 
    Wavelet* newwvlt = new Wavelet(wvlt);
    int sz;
    SamplingData<float> sd = data_.timeintv_;
	//Seis::SynthGenerator::getDefOutSampling( *aimodel_, *wvlt, sz );
    Seis::SynthGenerator gen( *newaimodel, *newwvlt );
    gen.setOutSampling( sd, refsz_ );
    gen.generate();

    const SeisTrc& res = gen.result(); //.getExtendedTo( timeintv_ );
    data_.synthtrc_.copyDataFrom( res );
    //delete res;
}


void DataPlayer::copyDataToLogSet()
{
    mGetWD()
    if ( !aimodel_ ) return;
    TypeSet<float> dah, son, den, refs, outprefs, ai, synth;
    const Well::Log* sonlog = wd->logs().getLog( data_.currvellog() );
    for ( int idx=0; idx<refsz_; idx++ )
    {
	const float time = data_.timeintv_.atIndex(idx);
	dah += wd->d2TModel()->getDah( time );
	AIModel::Domain dom = AIModel::TWT;
	son += aimodel_->velocityAt( dom, time );
	den += aimodel_->densityAt( dom, time );
	ai += aimodel_->aiAt( dom, time );
    }

    SamplingData<float> sd( data_.timeintv_ );
    aimodel_->getReflectivity( sd, refs );
    for ( int idx=0; idx<dah.size(); idx++ )
	outprefs += idx < refs.size() ? refs[idx] : 0;

    createLog( data_.sonic(), dah, son ); 
    createLog( data_.density(), dah, den ); 
    createLog( data_.ai(), dah, ai );
    createLog( data_.reflectivity(), dah, outprefs  );

    return;
    if ( data_.isSonic() )
    {
	GeoCalculator gc;
	Well::Log* vellog = data_.logset_.getLog( data_.sonic() );
	if ( vellog )
	    gc.velLogConv( *vellog, GeoCalculator::Vel2Son );
    }
}


void DataPlayer::createLog( const char* nm, const TypeSet<float>& dah, 
				const TypeSet<float>& vals )
{
    Well::Log* log = 0;
    if ( data_.logset_.indexOf( nm ) < 0 ) 
    {
	log = new Well::Log( nm );
	data_.logset_.add( log );
    }
    else
	log = data_.logset_.getLog( nm );

    log->erase();

    for( int idx=0; idx<vals.size(); idx ++)\
	log->addValue( dah[idx], vals[idx] );
}


}; //namespace WellTie
