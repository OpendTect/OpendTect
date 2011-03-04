/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.54 2011-03-04 15:46:52 cvsbruno Exp $";

#include "welltietoseismic.h"

#include "arrayndimpl.h"
#include "ioman.h"
#include "mousecursor.h"
#include "raytrace1d.h"
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
#include "welltiesetup.h"


namespace WellTie
{
static const int cDefTimeResampFac = 2;

#define mGetWD() { wd_ = data_.wd_; if ( !wd_ ) return false; }
DataPlayer::DataPlayer( Data& data, const MultiID& seisid, const LineKey* lk ) 
    : data_(data)		    
    , seisid_(seisid)
    , linekey_(lk)
    , raytracer_(new RayTracer1D(RayTracer1D::Setup()))
{
    disprg_ = data_.timeintv_;
    dispsz_ = disprg_.nrSteps();

    workrg_ = disprg_; workrg_.step /= cDefTimeResampFac;
    worksz_ = workrg_.nrSteps();
}


DataPlayer::~DataPlayer()
{
    delete raytracer_; 
}


bool DataPlayer::computeAll()
{
    mGetWD()

    if ( !setAIModel() || !runRayTracer() 
	    || !generateSynthetics() || !extractSeismics() )
	return false;

    copyDataToLogSet();

    return true;
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool DataPlayer::processLog( const Well::Log* log, 
			     Well::Log& outplog, const char* nm ) 
{
    BufferString msg;
    if ( !log ) 
	{ msg += "Can not find "; msg += nm; mErrRet( msg ); }

    const int sz = log->size();
    if ( sz <= 2 )
    {
	msg += nm;
	msg +="log size too small, please check your input log";
	mErrRet(msg)
    }

    outplog.setUnitMeasLabel( log->unitMeasLabel() );

    for ( int idx=1; idx<sz-1; idx++ )
    {
	const float prvval = log->value( idx-1 );
	const float curval = log->value( idx );
	const float nxtval = log->value( idx+1 );
	outplog.addValue( log->dah(idx), (prvval + curval + nxtval )/3 );
    }

    GeoCalculator gc; 
    gc.removeSpikes( outplog.valArr(), sz, 10, 3 );

    return true;
}


bool DataPlayer::setAIModel()
{
    aimodel_.erase();

    const Well::Log* sonlog = wd_->logs().getLog( data_.sonic() );
    const Well::Log* denlog = wd_->logs().getLog( data_.density() );

    Well::Log pslog, pdlog;
    if ( !processLog( sonlog, pslog, data_.sonic() ) 
	    || !processLog( denlog, pdlog, data_.density() ) )
	return false;

    if ( data_.isSonic() )
	{ GeoCalculator gc; gc.velLogConv( pslog, GeoCalculator::Son2Vel ); }

    if ( !wd_->d2TModel() )
	mErrRet( "No depth/time model computed" );

    for ( int idx=0; idx<worksz_; idx++ )
    {
	const float dah = wd_->d2TModel()->getDah( workrg_.atIndex( idx ) );
	const float vel = pslog.getValue( dah, true ); 
	const float den = pdlog.getValue( dah, true ); 
	aimodel_ += AILayer( dah, vel, den );
    }
    return true;
}


bool DataPlayer::runRayTracer()
{
    raytracer_->setModel( true, aimodel_ );
    TypeSet<float> offsets; offsets += 0;
    raytracer_->setOffsets( offsets );
    TaskRunner taskrunner;
    if ( !taskrunner.execute( *raytracer_ ) )
	mErrRet( raytracer_->errMsg() )

    raytracer_->getReflectivity( 0, refmodel_ );
    for ( int idx=0; idx<worksz_-1; idx++ )
	refmodel_[idx].time_ = workrg_.atIndex(idx+1);

    return true;
}


bool DataPlayer::generateSynthetics()
{
    Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_ 
					    : data_.estimatedwvlt_;
    Seis::ODSynthGenerator gen;
    gen.setModel( refmodel_ );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.setOutSampling( disprg_ );
    TaskRunner taskrunner;
    if ( !taskrunner.execute( gen ) )
	return false;

    const SeisTrc& res = gen.result(); 
    data_.synthtrc_.copyDataFrom( res );
    gen.getSampledReflectivities( reflvals_ );

    return true;
}


bool DataPlayer::extractSeismics()
{
    TaskRunner taskrunner;
    TrackExtractor wtextr( wd_->track(), wd_->d2TModel() );
    taskrunner.execute( wtextr ); 

    const IOObj& ioobj = *IOM().get( seisid_ );
    IOObj* seisobj = ioobj.clone();

    SeismicExtractor seisextr( *seisobj );
    if ( linekey_ )
	seisextr.setLineKey( linekey_ );
    seisextr.setBIDValues( wtextr.getBIDs() );
    seisextr.setInterval( disprg_ );
    taskrunner.execute( seisextr );
    data_.seistrc_ = seisextr.result(); 
    return true;
}


bool DataPlayer::copyDataToLogSet()
{
    if ( aimodel_.isEmpty() ) 
	mErrRet( "No data found" )

    TypeSet<float> dah, son, den, ai, synth, refs;
    StepInterval<float> workintv = data_.timeintv_; 
    const int worksz = workintv.nrSteps();
    for ( int idx=0; idx<dispsz_; idx++ )
    {
	const float time = disprg_.atIndex(idx);
	const int workidx = workrg_.getIndex( time );
	const AILayer& layer = aimodel_[workidx];
	dah += layer.depth_;
	son += layer.vel_;
	den += layer.den_;
	ai += layer.vel_*layer.den_;
    }

    for ( int idx=0; idx<dah.size()-1; idx++ )
	refs += reflvals_[idx];

    createLog( data_.sonic(), dah, son ); 
    createLog( data_.density(), dah, den ); 
    createLog( data_.ai(), dah, ai );
    createLog( data_.reflectivity(), dah, refs  );

    /*
    if ( data_.isSonic() )
    {
	GeoCalculator gc;
	Well::Log* vellog = data_.logset_.getLog( data_.sonic() );
	if ( vellog )
	    gc.velLogConv( *vellog, GeoCalculator::Vel2Son );
    }
    */
    return true;
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
