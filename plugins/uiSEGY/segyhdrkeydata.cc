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


void SEGY::HdrEntryDataSet::add( int hdidx, int val )
{
    if ( rejectedidxs_.isPresent(hdidx) )
	return;

    HdrEntryRecord& rec = (*this)[ size()-1 ];
    const int listpos = idxs_.indexOf( hdidx );
    if ( listpos >= 0 )
	rec[listpos] = val;
    else if ( size() > 1 )
	reject( hdidx ); // shld already have been in rejected list?
    else
	{ idxs_ += hdidx; rec += val; }
}


void SEGY::HdrEntryDataSet::reject( int hdidx )
{
    if ( rejectedidxs_.isPresent(hdidx) )
	return;

    rejectedidxs_.add( hdidx );
    const int sz = size();
    if ( sz < 1 )
	return;

    const int idx = idxs_.indexOf( hdidx );
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


void SEGY::HdrEntryDataSet::rejectConstants( int start, int stop )
{
    if ( idxs_.size() < 1 || stop-start < 1 )
	return;

    TypeSet<int> toreject;
    for ( int idx=0; idx<idxs_.size(); idx++ )
    {
	const int firstval = ((*this)[start])[idx];
	bool havevariation = false;
	for ( int irec=start+1; irec<=stop; irec++ )
	{
	    const HdrEntryRecord& rec = (*this)[irec];
	    if ( rec[idx] != firstval )
		{ havevariation = true; break; }
	}
	if ( !havevariation )
	    toreject += idxs_[idx];
    }

    for ( int idx=0; idx<toreject.size(); idx++ )
	reject( toreject[idx] );
}


void SEGY::HdrEntryDataSet::rejectNoProgress( int start, int stop )
{
    if ( idxs_.size() < 1 || stop-start < 1 )
	return;

    TypeSet<int> toreject;
    for ( int idx=0; idx<idxs_.size(); idx++ )
    {
	int prevval = ((*this)[start])[idx];
	const bool isupwd = ((*this)[start+1])[idx] > prevval;
	bool isbad = false;
	for ( int irec=start+1; irec<=stop; irec++ )
	{
	    const HdrEntryRecord& rec = (*this)[irec];
	    int curval = rec[idx];

	    if ( curval == prevval || (curval>prevval) != isupwd )
		{ isbad = true; break; }
	}
	if ( isbad )
	    toreject += idxs_[idx];
    }

    for ( int idx=0; idx<toreject.size(); idx++ )
	reject( toreject[idx] );
}


void SEGY::HdrEntryDataSet::merge( const HdrEntryDataSet& oth )
{
    if ( size() == 0 )
    {
	TypeSet<HdrEntryRecord>::operator =( oth );
	idxs_ = oth.idxs_;
	rejectedidxs_ = oth.rejectedidxs_;
	return;
    }

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
    mDoAllDSs( reject(mHdrEntry(Scalel)) );
    mDoAllDSs( reject(mHdrEntry(Scalco)) );
    mDoAllDSs( reject(mHdrEntry(CoUnit)) );
    mDoAllDSs( reject(mHdrEntry(DelRt)) );
    mDoAllDSs( reject(mHdrEntry(Trwf)) );
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
    newfileat_.setEmpty();
    init();
}


void SEGY::HdrEntryKeyData::add( const SEGY::TrcHeader& thdr, bool isswpd,
				 bool isnewline )
{
    if ( isnewline )
	newfileat_ += size();

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


void SEGY::HdrEntryKeyData::finish( bool isps )
{
    const int sz = size();
    if ( sz < 2 )
	return;

    TypeSet<int> lineidxs;
    if ( newfileat_.isEmpty() || newfileat_[0] != 0 )
	lineidxs += 0;
    lineidxs.append( newfileat_ );
    lineidxs += sz;

    for ( int iline=1; iline<lineidxs.size(); iline++ )
    {
	const int start = lineidxs[iline-1];
	const int stop = lineidxs[iline] - 1;

	offs_.rejectConstants( start, stop );
	refnr_.rejectConstants( start, stop );
	if ( !isps )
	    trcnr_.rejectNoProgress( start, stop );
    }
}


void SEGY::HdrEntryKeyData::merge( const HdrEntryKeyData& oth )
{
    const int addsz = oth.size();
    if ( addsz < 1 )
	return;
    const int orgsz = size();

    inl_.merge( oth.inl_ ); crl_.merge( oth.crl_ );
    trcnr_.merge( oth.trcnr_ ); refnr_.merge( oth.refnr_ );
    offs_.merge( oth.offs_ );
    x_.merge( oth.x_ ); y_.merge( oth.y_ );

    const int othnrlns = oth.newfileat_.size();
    if ( othnrlns < 1 )
	newfileat_ += orgsz;
    for ( int idx=0; idx<othnrlns; idx++ )
	newfileat_ += orgsz + oth.newfileat_[idx];
}


void SEGY::HdrEntryKeyData::setCurOrPref( HdrEntry& he,
			   const HdrEntryDataSet& ds, int prefhdidx,
			   int defidx ) const
{
    TypeSet<int> prefhdidxs; prefhdidxs += prefhdidx;
    setCurOrPref( he, ds, prefhdidxs, defidx );
}


void SEGY::HdrEntryKeyData::setCurOrPref(
		HdrEntry& he, const HdrEntryDataSet& ds,
		const TypeSet<int>& prefhdidxs, int defidx ) const
{
    if ( ds.isEmpty() )
	return;

    const HdrDef& hdrdef = TrcHeader::hdrDef();
    HdrEntry::BytePos bytepos = he.bytepos_;
    if ( bytepos%2 ) bytepos--;

    // see if already a valid one is selected
    for ( int idx=0; idx<ds.idxs_.size(); idx++ )
    {
	const HdrEntry& defhe = *hdrdef[ ds.idxs_[idx] ];
	if ( defhe.bytepos_ == bytepos )
	{
	    // we're cool, current is a valid one.
	    he.bytepos_ = bytepos; // just ensure it's internal bytenr
	    return;
	}
    }

    // current is invalid, try use a preferred one
    for ( int idx=0; idx<ds.idxs_.size(); idx++ )
    {
	if ( prefhdidxs.isPresent(ds.idxs_[idx]) )
	    { he = *hdrdef[ ds.idxs_[idx] ]; return; }
    }

    // none of the preferred ones there, use default
    if ( defidx >= ds.idxs_.size() )
	defidx = ds.idxs_.size() - 1;
    he = *hdrdef[ ds.idxs_[defidx] ];
}


void SEGY::HdrEntryKeyData::setBest( TrcHeaderDef& th ) const
{
#define mSetCurOrPref(thmemb,mymemb,thstd,deflt) \
    setCurOrPref( th.thmemb, mymemb, TrcHeader::Entry##thstd(), deflt )

    mSetCurOrPref( inl_, inl_, Inline, 0 );
    mSetCurOrPref( crl_, crl_, Crossline, 1 );
    mSetCurOrPref( trnr_, trcnr_, Cdp, 0 );
    mSetCurOrPref( refnr_, refnr_, SP, 1 );
    mSetCurOrPref( offs_, offs_, Offset, 2 );

    TypeSet<int> hdidxs;
    hdidxs += TrcHeader::EntrySx(); hdidxs += TrcHeader::EntryGx();
    hdidxs += TrcHeader::EntryXcdp();
    setCurOrPref( th.xcoord_, x_, hdidxs, 0 );

    hdidxs.setEmpty();
    hdidxs += TrcHeader::EntrySy(); hdidxs += TrcHeader::EntryGy();
    hdidxs += TrcHeader::EntryYcdp();
    setCurOrPref( th.ycoord_, y_, hdidxs, 0 );
}
