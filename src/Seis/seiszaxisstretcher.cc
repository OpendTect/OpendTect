/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: seiszaxisstretcher.cc,v 1.4 2009-07-22 16:01:35 cvsbert Exp $";

#include "seiszaxisstretcher.h"

#include "genericnumer.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
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
    , totalnr_( outcs.hrg.totalNr() )
    , sampler_( 0 )
    , outtrc_( 0 )
    , outputptr_( 0 )
    , voiid_( -1 )
    , forward_( forward )
{
    SeisIOObjInfo info( in );
    is2d_ = info.is2D();

    ztransform_.ref();

    CubeSampling cs( true );
    cs.hrg = outcs_.hrg;

    seisreader_ = new SeisTrcReader( &in );
    seisreader_->setSelData( new  Seis::RangeSelData(cs) );
    if ( !seisreader_->prepareWork() )
    {
	delete seisreader_;
	seisreader_ = 0;
	return;
    }

    seiswriter_ = new SeisTrcWriter( &out );

    SamplingData<double> sd( outcs_.zrg );
    sampler_ =  new ZAxisTransformSampler(ztransform_,forward_,BinID(0,0),sd);

    outtrc_ = new SeisTrc( outcs_.nrZ() );
    outtrc_->setStartPos( outcs_.zrg.start );
    outputptr_ = new float[outcs_.nrZ()];
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

    sampler_->setBinID( curbid );

    if ( curhrg_.isEmpty() || !curhrg_.includes(curbid) )
	nextChunk();
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

void SeisZAxisStretcher::nextChunk()
{
    int chunksize = mMaxNrTrc/outcs_.hrg.nrCrl();
    if ( chunksize<1 ) chunksize = 1;

    if ( curhrg_.isEmpty() )
    {
	curhrg_ = outcs_.hrg;
	curhrg_.stop.inl = curhrg_.start.inl + curhrg_.step.inl * chunksize;
    }
    else
    {
	curhrg_.start.inl = curhrg_.stop.inl + curhrg_.step.inl;
	curhrg_.stop.inl = curhrg_.start.inl + curhrg_.step.inl * chunksize;

	if ( curhrg_.stop.inl> outcs_.hrg.stop.inl )
	    curhrg_.stop.inl = outcs_.hrg.stop.inl;
    }

    CubeSampling cs( outcs_ );
    cs.hrg = curhrg_;

    if ( voiid_<0 )
	voiid_ = ztransform_.addVolumeOfInterest( cs, forward_ );
    else
	ztransform_.setVolumeOfInterest( voiid_, cs, forward_ );

    ztransform_.loadDataIfMissing( voiid_ );
}
