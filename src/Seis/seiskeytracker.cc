/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2016
-*/

#include "seiskeytracker.h"
#include "od_ostream.h"

#define mOffsEps 1e-6


Seis::KeyTracker::KeyTracker( od_ostream& strm, Seis::GeomType gt, bool bin )
    : strm_(strm)
    , is2d_(Seis::is2D(gt))
    , isps_(Seis::isPS(gt))
    , binary_(bin)
{
    reset();
}


void Seis::KeyTracker::finish()
{
    if ( !finished_ )
    {
	recordOffsets();
	recordPos( prevbid_, false );
	finished_ = true;
    }
}


void Seis::KeyTracker::reset()
{
    nrhandled_ = 0;
    prevbid_ = BinID( mUdf(int), mUdf(int) );
    diriscrl_ = true;
    step_ = 0;
    offsidx_ = -1;
    offsets_.setEmpty();
    offsetschanged_ = false;
    finished_ = false;
}


void Seis::KeyTracker::addFirst( const BinID& bid, float offs )
{
    if ( isps_ )
    {
	offsets_ += offs;
	offsetschanged_ = true;
    }
}


void Seis::KeyTracker::addFirstFollowUp( const BinID& bid, float offs )
{
    if ( isps_ )
    {
	if ( isSamePos(bid,prevbid_) )
	    { offsets_ += offs; return; }
    }

    diriscrl_ = bid.crl() != prevbid_.crl();
    getNewIncs( bid );
    recordPos( prevbid_, true );
    if ( isps_ )
    {
	recordOffsets();
	offsidx_ = 0;
	checkCurOffset( offs );
    }
}


void Seis::KeyTracker::recordOffsets()
{
    if ( !offsetschanged_ )
	return;
    if ( offsets_.isEmpty() )
	{ pErrMsg("Huh"); return; }

    Index_Type pos[2]; int nrvals = 0;
    if ( !is2d_ )
	{ pos[nrvals] = prevbid_.inl(); nrvals++; }
    pos[nrvals] = prevbid_.crl(); nrvals++;
    if ( binary_ )
    {
	strm_.addBin( "O", 1 );
	strm_.addBin( pos, nrvals*sizeof(Index_Type) );
	const int nroffs = offsets_.size();
	strm_.addBin( &nroffs, sizeof(int) );
	strm_.addBin( offsets_.arr(), nroffs );
    }
    else
    {
	strm_ << "O";
	for ( int idx=0; idx<nrvals; idx++ )
	    strm_ << "\t" << pos[idx];
	strm_ << "\t" << offsets_.size();
	for ( int idx=0; idx<offsets_.size(); idx++ )
	    strm_ << "\t" << offsets_[idx];
	strm_ << od_endl;
    }

    offsidx_ = -1;
    offsetschanged_ = false;
}


void Seis::KeyTracker::recordPos( const BinID& bid, bool isstart )
{
    const char* key = !isstart ? "E" : (diriscrl_ ? "T" : "L");
    Index_Type pos[3]; int nrvals = 0;
    if ( !is2d_ )
	{ pos[nrvals] = bid.inl(); nrvals++; }
    pos[nrvals] = bid.crl(); nrvals++;
    if ( isstart )
	{ pos[nrvals] = step_; nrvals++; }
    if ( binary_ )
    {
	strm_.addBin( key, 1 );
	strm_.addBin( pos, nrvals * sizeof(Index_Type) );
    }
    else
    {
	strm_ << key;
	for ( int idx=0; idx<nrvals; idx++ )
	    strm_ << "\t" << pos[idx];
	strm_ << od_endl;
    }
}


void Seis::KeyTracker::getNewIncs( const BinID& bid )
{
    if ( diriscrl_ && bid.crl() == prevbid_.crl() )
	diriscrl_ = false;
    else if ( !diriscrl_ && bid.inl() == prevbid_.inl() )
	diriscrl_ = true;

    step_ = diriscrl_ ? bid.crl() - prevbid_.crl()
			    : bid.inl() - prevbid_.inl();
}


void Seis::KeyTracker::getNextPredBinID( BinID& bid ) const
{
    (is2d_ || diriscrl_ ? bid.crl() : bid.inl()) += step_;
}


bool Seis::KeyTracker::isSamePos( const BinID& bid1, const BinID& bid2 ) const
{
    return bid1.crl() == bid2.crl() && (is2d_ || bid1.inl() == bid2.inl());
}


void Seis::KeyTracker::addNext( const BinID& bid, float offs )
{
    BinID predbid( prevbid_ );
    getNextPredBinID( predbid );
    const bool atnextpred = isSamePos( bid, predbid );
    const bool atprevbid = bid == prevbid_;
    const bool atexpectedbid = isps_ ? atnextpred || atprevbid : atnextpred;

    if ( !atexpectedbid )
    {
	recordPos( prevbid_, false );
	getNewIncs( bid );
	recordPos( bid, true );
    }

    if ( isps_ )
	addNextPS( bid, offs );
}


void Seis::KeyTracker::checkCurOffset( float offs )
{
    if ( !mIsEqual(offs,offsets_[offsidx_],mOffsEps) )
    {
	offsets_[offsidx_] = offs;
	offsetschanged_ = true;
    }
}


void Seis::KeyTracker::addNextPS( const BinID& bid, float offs )
{
    offsidx_++;
    const bool havepredictedoffset = offsets_.validIdx( offsidx_ );
    const bool atprevbid = bid == prevbid_;

    if ( havepredictedoffset )
    {
	if ( atprevbid )
	    checkCurOffset( offs );
	else
	{
	   // we've lost one or more offsets
	    offsets_.removeRange( offsidx_, offsets_.size()-1 );
	    offsetschanged_ = true;
	    recordOffsets();
	    offsidx_++;
	    offsets_[offsidx_] = offs;
	}
    }
    else
    {
	// we're past last offset, now we should be at a new position
	if ( !atprevbid )
	{
	    recordOffsets();
	    offsidx_ = 0;
	    checkCurOffset( offs );
	}
	else
	{
	    offsets_ += offs;
	    offsetschanged_ = true;
	}
    }
}


void Seis::KeyTracker::add( int trcnr, float offs )
{
    add( BinID(0,trcnr), offs );
}


void Seis::KeyTracker::add( const BinID& bid, float offs )
{
    if ( nrhandled_ == 0 )
	addFirst( bid, offs );
    else if ( nrhandled_ == 1 || (isps_ && step_ == 0) )
	addFirstFollowUp( bid, offs );
    else
	addNext( bid, offs );

    nrhandled_++;
    prevbid_ = bid;
}
