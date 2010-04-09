/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: seiszaxisstretcher.cc,v 1.7 2010-04-09 09:16:53 cvsranojay Exp $";

#include "seiszaxisstretcher.h"

#include "genericnumer.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seispacketinfo.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "zaxistransform.h"


SeisZAxisStretcher::SeisZAxisStretcher( const IOObj& in, const IOObj& out,
					const CubeSampling& outcs,
					ZAxisTransform& ztf,
       					bool forward )
    : Executor( "Z Axis Stretcher" )
    , seisreader_( 0 )
    , seiswriter_( 0 )
    , curhrg_( false )
    , outcs_( outcs )
    , ztransform_( ztf )
    , nrdone_( 0 )
    , sampler_( 0 )
    , outtrc_( 0 )
    , outputptr_( 0 )
    , voiid_( -1 )
    , forward_( forward )
{
    SeisIOObjInfo info( in );
    is2d_ = info.is2D();

    ztransform_.ref();

    seisreader_ = new SeisTrcReader( &in );
    if ( !seisreader_->prepareWork(Seis::Scan) ||!seisreader_->seisTranslator())
    {
	delete seisreader_;
	seisreader_ = 0;
	return;
    }

    if ( !is2d_ )
    {
	const SeisPacketInfo& spi = seisreader_->seisTranslator()->packetInfo();
	HorSampling storhrg; storhrg.set( spi.inlrg, spi.crlrg );
	outcs_.hrg.limitTo( storhrg );
    }
    
    CubeSampling cs( true );
    cs.hrg = outcs_.hrg;
     seisreader_->setSelData( new Seis::RangeSelData(cs) );

    totalnr_ = cs.hrg.totalNr();

    seiswriter_ = new SeisTrcWriter( &out );

    StepInterval<float> trcrg = ztransform_.getZInterval( !forward_ );
    trcrg.step = ztransform_.getGoodZStep();

    SamplingData<double> sd( trcrg );
    sampler_ =  new ZAxisTransformSampler(ztransform_,forward_,BinID(0,0),sd);

    outtrc_ = new SeisTrc( trcrg.nrSteps()+1 );
    outtrc_->info().sampling = sd;
    outputptr_ = new float[trcrg.nrSteps()+1];
}


SeisZAxisStretcher::~SeisZAxisStretcher()
{
    delete seisreader_;
    delete seiswriter_;
    delete sampler_;
    delete outtrc_;
    delete [] outputptr_;


    if ( voiid_>=0 )
	ztransform_.removeVolumeOfInterest( voiid_ );

    ztransform_.unRef();
}


bool SeisZAxisStretcher::isOK() const
{
    return seisreader_ && seiswriter_ && ztransform_.isOK();
}


void SeisZAxisStretcher::setLineKey( const char* lk )
{
    Seis::RangeSelData sd; sd.copyFrom( *seisreader_->selData() );
    sd.lineKey() = lk;
    seisreader_->setSelData( sd.clone() );
    if ( !seisreader_->prepareWork() )
    {
	delete seisreader_;
	seisreader_ = 0;
	return;
    }

    sd.copyFrom( seiswriter_->selData() ? *seiswriter_->selData()
	    				: *new Seis::RangeSelData(true) );
    sd.lineKey() = lk;
    seiswriter_->setSelData( sd.clone() );
}


int SeisZAxisStretcher::nextStep()
{
    SeisTrc intrc;
    if ( !seisreader_->get(intrc) )
	return Finished();

    BinID curbid = intrc.info().binid;
    if ( is2d_ )
    {
	curbid.inl = ztransform_.lineIndex(
				seisreader_->selData()->lineKey().lineName() );
	curbid.crl = intrc.info().nr;
    }

    if ( !outcs_.hrg.includes( curbid ) )
	return MoreToDo();

    sampler_->setBinID( curbid );

    if ( curhrg_.isEmpty() || !curhrg_.includes(curbid) )
    {
	if ( !newChunk( curbid.inl ) )
	    return MoreToDo();
    }

    sampler_->computeCache( Interval<int>( 0, outtrc_->size()-1) );

    const SeisTrcFunction trcfunc( intrc, 0 );

    reSample( trcfunc, *sampler_, outputptr_, outtrc_->size() );

    for ( int idx=outtrc_->size()-1; idx>=0; idx-- )
	outtrc_->set( idx, outputptr_[idx], 0 );

    outtrc_->info().nr = intrc.info().nr;
    outtrc_->info().binid = intrc.info().binid;
    outtrc_->info().coord = intrc.info().coord;
    if ( !seiswriter_->put( *outtrc_ ) )
	return ErrorOccurred();

    nrdone_++;
    return MoreToDo();
}


#define mMaxNrTrc	5000

bool SeisZAxisStretcher::newChunk( int inl )
{
    int chunksize = is2d_ ? 1 : mMaxNrTrc/outcs_.hrg.nrCrl();
    if ( chunksize<1 ) chunksize = 1;

    curhrg_ = outcs_.hrg;
    curhrg_.start.inl = inl;
    curhrg_.stop.inl = curhrg_.start.inl + curhrg_.step.inl * (chunksize-1);
    if ( curhrg_.stop.inl>outcs_.hrg.stop.inl )
	curhrg_.stop.inl = outcs_.hrg.stop.inl;

    CubeSampling cs( outcs_ );
    cs.hrg = curhrg_;

    if ( voiid_<0 )
	voiid_ = ztransform_.addVolumeOfInterest( cs, !forward_ );
    else
	ztransform_.setVolumeOfInterest( voiid_, cs, !forward_ );

    return ztransform_.loadDataIfMissing( voiid_ );
}
