/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "matchdeltaattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "valseriesevent.h"
#include "survinfo.h"


namespace Attrib
{
    
mAttrDefCreateInstance(MatchDelta)
    
void MatchDelta::initClass()
{
    mAttrStartInitClass
	
    FloatParam* maxshift = new FloatParam( maxshiftStr() );
    desc->addParam( maxshift );
    
    desc->addInput( InputSpec("Ref cube",true) );
    desc->addInput( InputSpec("Match cube",true) );
    
    desc->setNrOutputs( Seis::UnknowData, 2 );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


MatchDelta::MatchDelta( Desc& desc )
    : Provider( desc )
    , dessamps_(-20,20)
{
    if ( !isOK() ) return;

    mGetFloat( maxshift_, maxshiftStr() );
    maxshift_ /= zFactor();
}


bool MatchDelta::getInputData( const BinID& relpos, int zintv )
{
    refcubedata_ = inputs_[0]->getData( relpos, zintv );
    mtchcubedata_ = inputs_[1]->getData( relpos, zintv );
    if ( !refcubedata_ || !mtchcubedata_ )
	return false;

    refintv_.start = refcubedata_->z0_;
    refintv_.stop = refcubedata_->z0_ + refcubedata_->nrsamples_ - 1;
    mtchintv_.start = mtchcubedata_->z0_;
    mtchintv_.stop = mtchcubedata_->z0_ + mtchcubedata_->nrsamples_ - 1;
    maxsamps_ = maxshift_ / refstep_;

    refseries_ = refcubedata_->series( getDataIndex(0) );
    mtchseries_ = mtchcubedata_->series( getDataIndex(1) );
    return refseries_ && mtchseries_;
}


const Interval<int>* MatchDelta::desZSampMargin(int,int) const
{
    return &dessamps_;
}


void MatchDelta::findEvents( int z0, int nrsamples ) const
{
    Interval<int> worksamps( z0 + dessamps_.start,
	    		     z0 + dessamps_.stop + nrsamples - 1 );
    worksamps.limitTo( refintv_ ); worksamps.limitTo( mtchintv_ );

    SamplingData<float> refsd( refintv_.start, 1 );
    SamplingData<float> mtchsd( mtchintv_.start, 1 );
    ValueSeriesEvFinder<float,float> refevf( *refseries_,
	    refcubedata_->nrsamples_-1, refsd );
    ValueSeriesEvFinder<float,float> mtchevf( *mtchseries_,
	    mtchcubedata_->nrsamples_-1, mtchsd );

    ValueSeriesEvent<float,float> refev( 0, worksamps.start - 2 );
    Interval<float> sampsleft( 0, worksamps.stop );
    while ( true )
    {
	sampsleft.start = refev.pos + 2;
	if ( sampsleft.start > sampsleft.stop )
	    break;

	refev = refevf.find( VSEvent::Max, sampsleft );
	if ( mIsUdf(refev.pos) )
	    break;

	Interval<float> mtchintv( refev.pos+1, refev.pos-maxsamps_ );
	ValueSeriesEvent<float,float> mtchevup =
	    mtchevf.find( VSEvent::Max, mtchintv );
	mtchintv = Interval<float>( refev.pos-1, refev.pos+maxsamps_ );
	ValueSeriesEvent<float,float> mtchevdn =
	    mtchevf.find( VSEvent::Max, mtchintv );

	const bool haveevup = !mIsUdf(mtchevup.pos);
	const bool haveevdn = !mIsUdf(mtchevdn.pos);
	ValueSeriesEvent<float,float> mtchev( mtchevup );
	if ( !haveevup && !haveevdn )
	    continue;
	else if ( haveevup && haveevdn )
	{
	    if ( fabs(mtchevup.pos-refev.pos) > fabs(mtchevdn.pos-refev.pos) )
		mtchev = mtchevdn;
	}
	else if ( haveevdn )
	    mtchev = mtchevdn;

	const float posdelta = mtchev.pos - refev.pos;
	if ( fabs(posdelta) > maxsamps_ )
	    continue; // Happens rather often; gate is widened a bit by finder

	deltas_ += posdelta;
	poss_ += refev.pos;
    }

}


void MatchDelta::fillOutput( const DataHolder& output,
			     int z0, int nrsamples ) const
{
    const float outfac = refstep_ * SI().zDomain().userFactor();
    if ( poss_.size() < 2 )
    {
	const float deltaval = (deltas_.isEmpty() ? 0 : deltas_[0]) * outfac;
	const float posval = (poss_.isEmpty() ? z0*outfac : poss_[0]) * outfac;
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    setOutputValue( output, 0, idx, z0, deltaval );
	    setOutputValue( output, 1, idx, z0, posval );
	}
	return;
    }

    // Fill from start to first event
    const int startsampnr = (int)poss_[0];
    int curposidx = 0;
    float prevpos = poss_[curposidx];
    float prevdelta = deltas_[curposidx];
    for ( int idx=z0; idx<=startsampnr; idx++ )
    {
	setOutputValue( output, 0, idx-z0, z0, prevdelta * outfac );
	setOutputValue( output, 1, idx-z0, z0, prevpos * outfac );
    }

    // Fill from first event to last event
    curposidx++;
    float curpos = poss_[curposidx];
    float curdelta = deltas_[curposidx];
    float posdist = curpos - prevpos;
    const int stopsampnr = (int)poss_[ poss_.size() - 1 ];
    for ( int sampnr=startsampnr+1; sampnr<stopsampnr; sampnr++ )
    {
	if ( sampnr < z0 ) continue;
	if ( sampnr >= z0+nrsamples ) break;
	if ( sampnr > curpos )
	{
	    prevpos = curpos;
	    prevdelta = curdelta;
	    curposidx++;
	    curpos = poss_[curposidx];
	    curdelta = deltas_[curposidx];
	    posdist = curpos - prevpos;
	}
	if ( mIsZero( posdist, 1e-3 ) )
	    posdist = 0.001;
	const float relpos = (sampnr - prevpos) / posdist;
	const float val = relpos * curdelta + (1-relpos) * prevdelta;
	const float pos = relpos < 0.5 ? prevpos : curpos;
	setOutputValue( output, 0, sampnr-z0, z0, val * outfac );
	setOutputValue( output, 1, sampnr-z0, z0, pos * outfac );
    }

    // Fill from last event to end of requested Z gate
    for ( int idx=stopsampnr; idx<z0+nrsamples; idx++ )
    {
	setOutputValue( output, 0, idx-z0, z0, curdelta * outfac );
	setOutputValue( output, 1, idx-z0, z0, curpos * outfac );
    }

}


bool MatchDelta::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples, int threadid ) const
{
    poss_.erase(); deltas_.erase();
    findEvents( z0, nrsamples );
    fillOutput( output, z0, nrsamples );
    return true;
}

}; //namespace
