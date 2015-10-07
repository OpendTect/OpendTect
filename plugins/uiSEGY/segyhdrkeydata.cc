/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "segyhdrkeydata.h"
#include "segyhdr.h"
#include "segyhdrdef.h"
#include "od_istream.h"
#include "survinfo.h"

#define mMaxReasOffset 150000
	// 150k feet is about 50 km


void SEGY::HdrEntryDataSet::erase()
{
    idxs_.setEmpty();
    rejectedidxs_.setEmpty();
    TypeSet<HdrEntryRecord>::erase();
}


void SEGY::HdrEntryDataSet::addRecord()
{
    *this += HdrEntryRecord( idxs_.size(), 0 );
}


void SEGY::HdrEntryDataSet::add( int heidx, int val )
{
    if ( rejectedidxs_.isPresent(heidx) )
	return;

    HdrEntryRecord& rec = (*this)[ size()-1 ];
    const int listpos = idxs_.indexOf( heidx );
    if ( listpos >= 0 )
	rec[listpos] = val;
    else if ( size() > 1 )
	reject( heidx ); // shld already have been in rejected list?
    else
	{ idxs_ += heidx; rec += val; }
}


void SEGY::HdrEntryDataSet::reject( int heidx )
{
    if ( rejectedidxs_.isPresent(heidx) )
	return;

    rejectedidxs_.add( heidx );
    const int sz = size();
    if ( sz < 1 )
	return;

    const int idx = idxs_.indexOf( heidx );
    if ( idx < 0 )
	return;

    // a previously OK field now turns bad - remove from all recs
    idxs_.removeSingle( idx );
    for ( int irec=0; irec<sz; irec++ )
    {
	HdrEntryRecord& rec = (*this)[irec];
	rec.removeSingle( idx );
	if ( rec.size() != idxs_.size() )
	    { pErrMsg("Logic error"); rec.setSize( idxs_.size(), 0 ); }
    }
}


void SEGY::HdrEntryDataSet::rejectConstants()
{
    if ( idxs_.size() < 1 || size() < 2 )
	return;

    TypeSet<int> toreject;
    for ( int idx=0; idx<idxs_.size(); idx++ )
    {
	const int firstval = ((*this)[0])[idx];
	bool havevariation = false;
	for ( int irec=1; irec<size(); irec++ )
	{
	    const HdrEntryRecord& rec = (*this)[irec];
	    if ( rec[idx] != firstval )
		{ havevariation = true; break; }
	}
	if ( !havevariation )
	    toreject += idx;
    }

    for ( int idx=0; idx<toreject.size(); idx++ )
	reject( toreject[idx] );
}


void SEGY::HdrEntryDataSet::rejectNoProgress()
{
    if ( idxs_.size() < 1 || size() < 2 )
	return;

    TypeSet<int> toreject;
    for ( int idx=0; idx<idxs_.size(); idx++ )
    {
	int prevval = ((*this)[0])[idx];
	const bool isupwd = ((*this)[1])[idx] > prevval;
	bool isbad = false;
	for ( int irec=1; irec<size(); irec++ )
	{
	    const HdrEntryRecord& rec = (*this)[irec];
	    int curval = rec[idx];

	    if ( curval == prevval || (curval>prevval) != isupwd )
		{ isbad = true; break; }
	}
	if ( isbad )
	    toreject += idx;
    }

    for ( int idx=0; idx<toreject.size(); idx++ )
	reject( toreject[idx] );
}


void SEGY::HdrEntryDataSet::merge( const HdrEntryDataSet& oth )
{
    for ( int idx=0; idx<oth.rejectedidxs_.size(); idx++ )
	if ( !rejectedidxs_.isPresent(oth.rejectedidxs_[idx]) )
	    reject( oth.rejectedidxs_[idx] );

    TypeSet<int> transtbl;
    for ( int idx=0; idx<idxs_.size(); idx++ )
    {
	int tridx = oth.idxs_.indexOf( idxs_[idx] );
	if ( tridx < 0 )
	    { pErrMsg("Huh"); tridx = 0; }
	transtbl += tridx;
    }

    for ( int irec=0; irec<oth.size(); irec++ )
    {
	const HdrEntryRecord& othrec = oth[irec];
	addRecord();
	HdrEntryRecord& rec = (*this)[ size()-1 ];
	for ( int ival=0; ival<transtbl.size(); ival++ )
	    rec[ival] = othrec[ transtbl[ival] ];
    }
}


#define mDoAllDSs( oper ) \
    inl_.oper; crl_.oper; trcnr_.oper; refnr_.oper; offs_.oper; x_.oper; y_.oper

#define mHdrEntry(nm) TrcHeader::Entry##nm()
#define mRejectEntry(set,nm) set.reject( mHdrEntry(nm) )


SEGY::HdrEntryKeyData::HdrEntryKeyData()
{
    init();
}


void SEGY::HdrEntryKeyData::init()
{
    mDoAllDSs( reject(mHdrEntry(DUse)) );
    mDoAllDSs( reject(mHdrEntry(Scalco)) );
    mDoAllDSs( reject(mHdrEntry(CoUnit)) );
    mDoAllDSs( reject(mHdrEntry(Ns)) );
    mDoAllDSs( reject(mHdrEntry(Dt)) );

    // don't want X and Y swapped by user - gotta draw the line somewhere
    mRejectEntry( x_, Sy );
    mRejectEntry( x_, Gy );
    mRejectEntry( x_, Ycdp );
    mRejectEntry( y_, Sx );
    mRejectEntry( y_, Gx );
    mRejectEntry( y_, Xcdp );
}


void SEGY::HdrEntryKeyData::setEmpty()
{
    mDoAllDSs( setEmpty() );
    init();
}


void SEGY::HdrEntryKeyData::add( const SEGY::TrcHeader& thdr, bool isswpd )
{
    const HdrDef& hdrdef = TrcHeader::hdrDef();
    const void* buf = thdr.buf_;
    const int nrhe = hdrdef.size();

    mDoAllDSs( addRecord() );

    for ( int ihe=0; ihe<nrhe; ihe++ )
    {
	const HdrEntry& he = *hdrdef[ihe];

	const int val = he.getValue( buf, isswpd );
	if ( val <= 0 )
	{
	    inl_.reject( ihe ); crl_.reject( ihe ); trcnr_.reject( ihe );
	    x_.reject( ihe ); y_.reject( ihe );
	    if ( val == 0 )
		refnr_.reject( ihe );
	}
	if ( val > mMaxReasOffset || val < -mMaxReasOffset )
	    offs_.reject( ihe );

	inl_.add( ihe, val ); crl_.add( ihe, val );
	trcnr_.add( ihe, val ); refnr_.add( ihe, val );
	x_.add( ihe, val ); y_.add( ihe, val );
	offs_.add( ihe, val < 0 ? -val : val );
    }
}


void SEGY::HdrEntryKeyData::finish()
{
    offs_.rejectConstants();

    // SEG-Y files can have a single inline, crossline or gather

    // For 2D PS the following will make valid single-gather files unloadable:
    trcnr_.rejectNoProgress();
    refnr_.rejectConstants();
    // So be it. The benefit for all other 2D files is too big ...
}


void SEGY::HdrEntryKeyData::merge( const HdrEntryKeyData& oth )
{
    inl_.merge( oth.inl_ ); crl_.merge( oth.crl_ );
    trcnr_.merge( oth.trcnr_ ); refnr_.merge( oth.refnr_ );
    offs_.merge( oth.offs_ );
    x_.merge( oth.x_ ); y_.merge( oth.y_ );
}


void SEGY::HdrEntryKeyData::setCurOrFirst( HdrEntry& he,
					   const HdrEntryDataSet& ds ) const
{
    if ( ds.isEmpty() )
	return;

    const HdrDef& hdrdef = TrcHeader::hdrDef();
    const int bytepos = he.bytepos_ - 1; // he has a 'user' byte number

    // see if already a valid one is selected
    for ( int idx=0; idx<ds.idxs_.size(); idx++ )
    {
	const HdrEntry& defhe = *hdrdef[ ds.idxs_[idx] ];
	if ( defhe.bytepos_ == bytepos )
	    return; // we're cool, current is a valid one
    }

    // current is invalid, use first valid one
    he = *hdrdef[ ds.idxs_[0] ];
    he.bytepos_++; // convert to 'user' byte number
}


void SEGY::HdrEntryKeyData::setBest( TrcHeaderDef& th ) const
{

    setCurOrFirst( th.inl_, inl_ );
    setCurOrFirst( th.crl_, crl_ );
    setCurOrFirst( th.trnr_, trcnr_ );
    setCurOrFirst( th.refnr_, refnr_ );
    setCurOrFirst( th.offs_, offs_ );
    setCurOrFirst( th.xcoord_, x_ );
    setCurOrFirst( th.ycoord_, y_ );

    // ... and for X and Y it looks too stupid if we select a non-specific one
    const HdrDef& hdrdef = TrcHeader::hdrDef();

    for ( int ihe=0; ihe<x_.size(); ihe++ )
    {
	if ( ihe == mHdrEntry(Sx) || ihe == mHdrEntry(Gx)
	  || ihe == mHdrEntry(Xcdp) )
	{
	    th.xcoord_ = *hdrdef[ x_.idxs_[ihe] ];
	    th.xcoord_.bytepos_++;
	}
    }

    for ( int ihe=0; ihe<y_.size(); ihe++ )
    {
	if ( ihe == mHdrEntry(Sy) || ihe == mHdrEntry(Gy)
	  || ihe == mHdrEntry(Ycdp) )
	{
	    th.ycoord_ = *hdrdef[ y_.idxs_[ihe] ];
	    th.ycoord_.bytepos_++;
	}
    }
}
