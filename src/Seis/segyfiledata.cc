/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segyfiledata.cc,v 1.7 2008-11-19 20:24:49 cvsbert Exp $";

#include "segyfiledata.h"
#include "iopar.h"
#include "survinfo.h"
#include "horsampling.h"
#include "ascstream.h"
#include "separstr.h"
#include "keystrs.h"
#include <iostream>

static const char* sKeyTraceSize = "Trace size";
static const char* sKeyFormat = "SEG-Y sample format";
static const char* sKeySampling = "Z sampling";
static const char* sKeyNrStanzas = "Nr REV.1 Text stanzas";


SEGY::FileData::FileData( const char* fnm )
    : fname_(fnm)
    , trcsz_(-1)
    , sampling_(SI().zRange(false).start,SI().zRange(false).step)
    , segyfmt_(0)
    , isrev1_(true)
    , nrstanzas_(0)
{
}


int SEGY::FileData::nrNullTraces() const
{
    int nr = 0;
    for ( int idx=0; idx<size(); idx++ )
	if ( isNull(idx) ) nr++;
    return nr;
}


int SEGY::FileData::nrUsableTraces() const
{
    int nr = 0;
    for ( int idx=0; idx<size(); idx++ )
	if ( isUsable(idx) ) nr++;
    return nr;
}


void SEGY::FileData::getReport( IOPar& iop, Seis::GeomType gt ) const
{
    BufferString str( "Info for '" ); str += fname_; str += "'";
    iop.add( "->", str );
    const int nrtrcs = size();
    if ( nrtrcs < 1 )
	{ iop.add( "Number of traces found", "0" ); return; }

    int nr = nrNullTraces();
    str = nrtrcs;
    if ( nr < 1 )
	str += " (none null; ";
    else
	{ str += " ("; str += nr; str += " null; "; }
    nr = nrUsableTraces();
    if ( nr == nrtrcs )
	str += "all usable)";
    else
	{ str += nr; str += " usable)"; }
    iop.add( "Number of traces found", str );
    iop.add( "Number of samples in file", trcsz_ );
    const Interval<float> zrg( sampling_.start,
	    		       sampling_.start + (trcsz_-1)*sampling_.step );
    iop.add( "Z range in file", zrg.start, zrg.stop );
    iop.add( "Z step in file", sampling_.step );
    iop.addYN( "File marked as REV. 1", isrev1_ );
    if ( isrev1_ && nrstanzas_ > 0 )
	iop.add( "Number of REV.1 extra stanzas", nrstanzas_ );

    int firstok = 0;
    for ( ; firstok<nrtrcs; firstok++ )
	if ( isUsable(firstok) ) break;
    if ( firstok >= nrtrcs ) return;

    HorSampling hs( false ); hs.start = hs.stop = binID(firstok);
    Interval<int> nrrg( trcNr(firstok), trcNr(firstok) );
    const Coord c0( coord(firstok) );
    Interval<double> xrg( c0.x, c0.x ), yrg( c0.y, c0.y );
    Interval<double> offsrg( offset(firstok), offset(firstok) );
    for ( int idx=1; idx<size(); idx++ )
    {
	if ( !isUsable(idx) ) continue;
	hs.include( binID(idx) );
	nrrg.include( trcNr(idx) );
	const Coord c( coord(idx) );
	xrg.include( c.x ); yrg.include( c.y );
	offsrg.include( offset(idx) );
    }

    if ( Seis::is2D(gt) )
	iop.add( "Trace number range", nrrg.start, nrrg.stop );
    else
    {
	iop.add( "Inline range", hs.start.inl, hs.stop.inl );
	iop.add( "Crossline range", hs.start.crl, hs.stop.crl );
    }
    iop.add( "X range", xrg.start, xrg.stop );
    iop.add( "Y range", yrg.start, yrg.stop );
    if ( Seis::isPS(gt) )
	iop.add( "Offset range", offsrg.start, offsrg.stop );
}


bool SEGY::FileData::getFrom( ascistream& astrm, Seis::GeomType gt )
{
    erase(); fname_.setEmpty(); trcsz_ = -1;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::FileName) )
	    fname_ = astrm.value();
	else if ( astrm.hasKeyword(sKeyTraceSize) )
	    trcsz_ = astrm.getIValue();
	else if ( astrm.hasKeyword(sKeyFormat) )
	    segyfmt_ = astrm.getIValue();
	else if ( astrm.hasKeyword(sKeyNrStanzas) )
	    nrstanzas_ = astrm.getIValue();
	else if ( astrm.hasKeyword(sKeySampling) )
	{
	    sampling_.start = astrm.getFValue(0);
	    sampling_.step = astrm.getFValue(1);
	}
    }
    if ( fname_.isEmpty() || trcsz_ < 1 )
	return false;

    FileMultiString keyw; FileMultiString val;
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    while ( !atEndOfSection(astrm.next()) )
    {
	keyw = astrm.keyWord(); val = astrm.value();
	TraceInfo ti;
	if ( is2d )
	    ti.nr_ = keyw.getIValue(0);
	else
	    ti.binid_.use( keyw[0] );
	if ( isps )
	    ti.offset_ = keyw.getFValue(1);

	const char ch( *val[0] );
	ti.usable_ = ch != 'X';
	ti.null_ = ch == 'N';
	if ( is2d )
	    { ti.coord_.x = val.getDValue(1); ti.coord_.y = val.getDValue(2); }

	*this += ti;
    }

    return true;
}


bool SEGY::FileData::putTo( ascostream& astrm, Seis::GeomType gt ) const
{
    astrm.put( sKey::FileName, fname_ );
    astrm.put( sKeyTraceSize, trcsz_ );
    astrm.put( sKeySampling, sampling_.start, sampling_.step );
    astrm.put( sKeyFormat, segyfmt_ );
    if ( isrev1_ && nrstanzas_ > 0 )
	astrm.put( sKeyNrStanzas, nrstanzas_ );
    astrm.newParagraph();

    FileMultiString keyw; FileMultiString val;
    const bool is2d = Seis::is2D( gt );
    const bool isps = Seis::isPS( gt );
    for ( int itrc=0; itrc<size(); itrc++ )
    {
	const TraceInfo& ti = (*this)[itrc];
	keyw.setEmpty();
	if ( is2d )
	    keyw += ti.nr_;
	else
	    ti.binid_.fill( keyw.buf() );
	if ( isps )
	    keyw += ti.offset_;

	val = ti.usable_ ? (ti.null_ ? "N" : "O") : "X";
	if ( is2d )
	    { val += ti.coord_.x; val += ti.coord_.y; }

	astrm.put( keyw.buf(), val.buf() );
    }

    astrm.newParagraph();
    return astrm.stream().good();
}


SEGY::FileDataSet& SEGY::FileDataSet::operator =( const SEGY::FileDataSet& fds )
{
    if ( this != &fds )
    {
	deepErase( *this );
	deepCopy( *this, fds );
	pars_ = fds.pars_;
    }
    return *this;
}


bool SEGY::FileDataSet::toNext( SEGY::FileDataSet::TrcIdx& ti, bool nll,
				bool unu ) const
{
    if ( ti.filenr_ < 0 )
	{ ti.trcnr_ = -1; ti.filenr_= 0; }

    if ( isEmpty() || ti.filenr_ >= size() )
	{ ti.filenr_ = -1; ti.trcnr_ = 0; return false; }

    ti.trcnr_++;
    if ( ti.trcnr_ >= (*this)[ti.filenr_]->size() )
	{ ti.filenr_++; ti.trcnr_ = -1; return toNext( ti, nll, unu ); }

    if ( nll && unu )
	return true;

    const FileData& fd = *(*this)[ti.filenr_];
    if ( (!nll && fd.isNull(ti.trcnr_))
      || (!unu && !fd.isUsable(ti.trcnr_)) )
	return toNext( ti, nll, unu );

    return true;
}
