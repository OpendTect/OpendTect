/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2016
-*/

#include "seiskeytracker.h"
#include "od_iostream.h"
#include "ascbinstream.h"


Seis::TrackRecord::Entry* Seis::TrackRecord::Entry::getFrom(
				    ascbinistream& strm, bool is2d )
{
    char key; pos_type nr1, nr2; strm.get( key ).get( nr1 ).get( nr2 );
    if ( strm.isBad() )
	return 0;

    const bool isstop = key == 'E';
    if ( isstop && is2d )
	return new StopEntry2D( nr1, nr2 );
    pos_type nr3; strm.get( nr3 );
    if ( strm.isBad() )
	return 0;
    if ( isstop )
	return new StopEntry3D( nr1, nr2, nr3 );

    if ( key == 'O' )
    {
	int nroffs = nr3;
	if ( !is2d )
	{
	    strm.get( nroffs );
	    if ( strm.isBad() )
		return 0;
	}
	OffsEntry* oentry;
	if ( is2d )
	    oentry = new OffsEntry2D( nr1 );
	else
	    oentry = new OffsEntry3D( nr1, nr2 );

	oentry->offsets_.setSize( nroffs, 0.f );
	strm.getArr( oentry->offsets_.arr(), nroffs );
	if ( strm.isBad() )
	    { delete oentry; return 0; }

	return oentry;
    }

    if ( is2d )
	return new StartEntry2D( nr1, nr2, nr3 );

    pos_type step; strm.get( step );
    if ( strm.isBad() )
	return 0;

    const bool inldir = key == 'L';
    return new StartEntry3D( nr1, nr2, nr3, step, !inldir );
}


bool Seis::TrackRecord::Entry::dump( ascbinostream& strm ) const
{
    strm.add( *fileKey() );

    if ( isOffs() )
    {
	if ( !is2D() )
	    strm.add( inl() );
	strm.add( trcNr() );
	mDynamicCastGet(const OffsEntry*,oentry,this)
	strm.addArr( oentry->offsets_.arr(), oentry->offsets_.size() );
    }
    else
    {
	const bool isstart = isStart();
	strm.add( seqNr() );
	if ( !is2D() )
	    strm.add( inl() );
	strm.add( trcnr_, isstart ? od_tab : od_newline );
	if ( isstart )
	{
	    mDynamicCastGet(const StartEntry*,sentry,this)
	    strm.add( sentry->step_, od_newline );
	}
    }

    return strm.isOK();
}


Seis::TrackRecord::TrackRecord( Seis::GeomType gt )
    : is2d_(Seis::is2D(gt))
    , isps_(Seis::isPS(gt))
{
}


Seis::TrackRecord& Seis::TrackRecord::addStartEntry( SeqNrType seqnr,
				const BinID& bid, pos_type step, bool diriscrl )
{
    Entry* entry;

    if ( is2d_ )
	entry = new StartEntry2D( seqnr, bid.crl(), step );
    else
	entry = new StartEntry3D( seqnr, bid.inl(), bid.crl(), step, diriscrl);

    entries_ += entry;
    return *this;
}


Seis::TrackRecord& Seis::TrackRecord::addEndEntry( SeqNrType seqnr,
						   const BinID& bid )
{
    Entry* entry;

    if ( is2d_ )
	entry = new StopEntry2D( seqnr, bid.crl() );
    else
	entry = new StopEntry3D( seqnr, bid.inl(), bid.crl() );

    entries_ += entry;
    return *this;
}


Seis::TrackRecord& Seis::TrackRecord::addOffsetEntry( const BinID& bid,
						    const TypeSet<float>& offs )
{
    OffsEntry* newentry;
    if ( is2d_ )
	newentry = new OffsEntry2D( bid.crl() );
    else
	newentry = new OffsEntry3D( bid.inl(), bid.crl() );

    newentry->offsets_ = offs;
    entries_ += newentry;
    return *this;
}


bool Seis::TrackRecord::getFrom( od_istream& instrm, bool bin )
{
    setEmpty();
    ascbinistream strm( instrm, bin );

    idx_type nrentries;
    strm.get( nrentries );
    if ( nrentries < 1 )
	return true;

    for ( int idx=0; idx<nrentries; idx++ )
    {
	Entry* entry = Entry::getFrom( strm, is2d_ );
	if ( !entry )
	    return false;

	entries_ += entry;
    }

    return true;
}


bool Seis::TrackRecord::dump( od_ostream& instrm, bool bin ) const
{
    ascbinostream strm( instrm, bin );
    const idx_type nrentries = entries_.size();
    strm.add( nrentries, od_newline );

    for ( int ientry=0; ientry<nrentries; ientry++ )
	if ( !entries_[ientry]->dump(strm) )
	    return false;

    return true;
}


Seis::KeyTracker::KeyTracker( TrackRecord& tr )
    : trackrec_(tr)
{
    reset();
}


void Seis::KeyTracker::finish()
{
    if ( !finished_ )
    {
	addOffsetEntry();
	addEndEntry( seqnr_, prevbid_ );
	finished_ = true;
    }
}


void Seis::KeyTracker::reset()
{
    trackrec_.setEmpty();
    seqnr_ = 0;
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
    if ( isPS() )
    {
	offsets_ += offs;
	offsetschanged_ = true;
    }
}


void Seis::KeyTracker::addFirstFollowUp( const BinID& bid, float offs )
{
    if ( isPS() )
    {
	if ( isSamePos(bid,prevbid_) )
	    { offsets_ += offs; return; }
    }

    diriscrl_ = bid.crl() != prevbid_.crl();
    getNewIncs( bid );
    addStartEntry( 0, prevbid_ );
    if ( isPS() )
    {
	addOffsetEntry();
	offsidx_ = 0;
	checkCurOffset( offs );
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
    (is2D() || diriscrl_ ? bid.crl() : bid.inl()) += step_;
}


bool Seis::KeyTracker::isSamePos( const BinID& bid1, const BinID& bid2 ) const
{
    return bid1.crl() == bid2.crl() && (is2D() || bid1.inl() == bid2.inl());
}


void Seis::KeyTracker::addNext( const BinID& bid, float offs )
{
    BinID predbid( prevbid_ );
    getNextPredBinID( predbid );
    const bool atnextpred = isSamePos( bid, predbid );
    const bool atprevbid = bid == prevbid_;
    const bool atexpectedbid = isPS() ? atnextpred || atprevbid : atnextpred;

    if ( !atexpectedbid )
    {
	addEndEntry( seqnr_-1, prevbid_ );
	getNewIncs( bid );
	addStartEntry( seqnr_, bid );
    }

    if ( isPS() )
	addNextPS( bid, offs );
}


void Seis::KeyTracker::checkCurOffset( float offs )
{
    if ( !equalOffset(offs,offsets_[offsidx_]) )
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
	    addOffsetEntry();
	    offsidx_++;
	    offsets_[offsidx_] = offs;
	}
    }
    else
    {
	// we're past last offset, now we should be at a new position
	if ( !atprevbid )
	{
	    addOffsetEntry();
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
    if ( !finished_ )
	add( BinID(0,trcnr), offs );
}


void Seis::KeyTracker::add( const BinID& bid, float offs )
{
    if ( finished_ )
	return;

    if ( seqnr_ == 0 )
	addFirst( bid, offs );
    else if ( seqnr_ == 1 || (isPS() && step_ == 0) )
	addFirstFollowUp( bid, offs );
    else
	addNext( bid, offs );

    seqnr_++;
    prevbid_ = bid;
}


void Seis::KeyTracker::addStartEntry( SeqNrType seqnr, const BinID& bid )
{
    trackrec_.addStartEntry( seqnr, bid, step_, diriscrl_ );
}


void Seis::KeyTracker::addEndEntry( SeqNrType seqnr, const BinID& bid )
{
    trackrec_.addEndEntry( seqnr, bid );
}


void Seis::KeyTracker::addOffsetEntry()
{
    if ( !offsetschanged_ )
	return;
    if ( offsets_.isEmpty() )
	{ pErrMsg("Huh"); return; }

    trackrec_.addOffsetEntry( prevbid_, offsets_ );

    offsidx_ = -1;
    offsetschanged_ = false;
}
