/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/
static const char* rcsID = "$Id: segyfiledata.cc,v 1.18 2009-07-22 16:01:34 cvsbert Exp $";

#include "segyfiledata.h"
#include "iopar.h"
#include "survinfo.h"
#include "horsampling.h"
#include "ascstream.h"
#include "separstr.h"
#include "keystrs.h"
#include "envvars.h"
#include <iostream>

static const char* sKeyTraceSize = "Trace size";
static const char* sKeyFormat = "SEG-Y sample format";
static const char* sKeySampling = "Z sampling";
static const char* sKeyNrStanzas = "Nr REV.1 Text stanzas";
static const char* sKeyStorageType = "Storage type";
static bool writeascii = !__islittle__ || GetEnvVarYN("OD_WRITE_SEGYDEF_ASCII");


SEGY::FileData::FileData( const char* fnm, Seis::GeomType gt )
    : ManagedObjectSet<TraceInfo>(false)
    , fname_(fnm)
    , geom_(gt)
    , trcsz_(-1)
    , sampling_(SI().zRange(false).start,SI().zRange(false).step)
    , segyfmt_(0)
    , isrev1_(true)
    , nrstanzas_(0)
{
}


SEGY::FileData& SEGY::FileData::operator =( const SEGY::FileData& fd )
{
    if ( this == &fd ) return *this;

    fname_ = fd.fname_;
    geom_ = fd.geom_;
    trcsz_ = fd.trcsz_;
    sampling_ = fd.sampling_;
    segyfmt_ = fd.segyfmt_;
    isrev1_ = fd.isrev1_;
    nrstanzas_ = fd.nrstanzas_;

    deepErase( *this );
    for ( int idx=0; idx<fd.size(); idx++ )
	*this += fd[idx]->clone();

    return *this;
}


bool SEGY::FileData::isRich() const
{
    return isEmpty() ? false : (*this)[0]->isRich();
}


int SEGY::FileData::nrNullTraces() const
{
    if ( !isRich() ) return 0;

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


void SEGY::FileData::getReport( IOPar& iop ) const
{
    iop.add( IOPar::sKeyHdr(), BufferString("Info for '",fname_.buf(),"'") );
    iop.add( IOPar::sKeySubHdr(), "General info" );
    const int nrtrcs = size();
    if ( nrtrcs < 1 )
	{ iop.add( "Number of traces found", "0" ); return; }

    int nr = nrNullTraces();
    BufferString nrtrcsstr( "", nrtrcs );
    if ( nr < 1 )
	nrtrcsstr += " (none null; ";
    else
	{ nrtrcsstr += " ("; nrtrcsstr += nr; nrtrcsstr += " null; "; }
    nr = nrUsableTraces();
    if ( nr == nrtrcs )
	nrtrcsstr += "all usable)";
    else
	{ nrtrcsstr += nr; nrtrcsstr += " usable)"; }
    iop.add( "Number of traces found", nrtrcsstr );
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
    Interval<float> offsrg( offset(firstok), offset(firstok) );
    Interval<float> azimrg( azimuth(firstok), azimuth(firstok) );
    for ( int idx=firstok+1; idx<size(); idx++ )
    {
	if ( !isUsable(idx) ) continue;
	hs.include( binID(idx) );
	nrrg.include( trcNr(idx) );
	const Coord c( coord(idx) );
	xrg.include( c.x ); yrg.include( c.y );
	offsrg.include( offset(idx) );
	azimrg.include( azimuth(idx) );
    }

    iop.add( IOPar::sKeySubHdr(), "Ranges" );
    if ( Seis::is2D(geom_) )
	iop.add( "Trace number range", nrrg.start, nrrg.stop );
    else
    {
	iop.add( "Inline range", hs.start.inl, hs.stop.inl );
	iop.add( "Crossline range", hs.start.crl, hs.stop.crl );
    }
    if ( isRich() )
    {
	iop.add( "X range", xrg.start, xrg.stop );
	iop.add( "Y range", yrg.start, yrg.stop );
    }
    if ( Seis::isPS(geom_) )
    {
	iop.add( "Offset range", offsrg.start, offsrg.stop );
	if ( isRich() )
	    iop.add( "Azimuth range", azimrg.start, azimrg.stop );
    }
}


#define mErrRet(s) { ErrMsg(s); return false; }

bool SEGY::FileData::getFrom( ascistream& astrm )
{
    erase(); fname_.setEmpty(); trcsz_ = -1;
    bool isasc = false; bool isrich = false;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::FileName) )
	    fname_ = astrm.value();
	if ( astrm.hasKeyword(sKey::Geometry) )
	    geom_ = Seis::geomTypeOf( astrm.value() );
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
	else if ( astrm.hasKeyword(sKeyStorageType) )
	{
	    isasc = *astrm.value() == 'A';
	    FileMultiString fms( astrm.value() );
	    if ( fms.size() > 1 )
		isrich = *fms[1] == 'R';
	}
    }
    if ( fname_.isEmpty() )
	mErrRet("No filename found in header")
    else if ( trcsz_ < 1 )
	mErrRet("Invalid trace size in header")

    const bool is2d = Seis::is2D( geom_ );
    const bool isps = Seis::isPS( geom_ );
    BinID bid;

    if ( isasc )
    {
	FileMultiString keyw; FileMultiString val;

	while ( !atEndOfSection(astrm.next()) )
	{
	    keyw = astrm.keyWord(); val = astrm.value();
	    RichTraceInfo* rti = isrich ? new RichTraceInfo( geom_ ) : 0;
	    TraceInfo* ti = rti ? rti : new TraceInfo( geom_ );
	    if ( is2d )
		ti->pos_.setTrcNr( keyw.getIValue(0) );
	    else
		ti->pos_.binID().use( keyw[0] );
	    if ( isps )
		ti->pos_.setOffset( keyw.getFValue(1) );

	    const char ch( *val[0] );
	    ti->usable_ = ch != 'U';
	    if ( isrich )
	    {
		rti->coord_.use( val[1] );
		rti->azimuth_ = val.getFValue( 2 );
		rti->null_ = ch == 'N';
	    }

	    *this += ti;
	}
    }
    else
    {
	std::istream& strm = astrm.stream();
	int entrylen = 1 + (isps ? 4 : 0) + sizeof(int);
	if ( !is2d )
	    entrylen += sizeof(int);
	if ( isrich )
	    entrylen += 2*sizeof(double) + sizeof(float);
	char* buf = new char [entrylen];

	while ( strm.good() )
	{
	    const char ch = strm.peek();
	    if ( ch == '\n' )
		{ strm.ignore( 1 ); break; }

	    strm.read( buf, entrylen );
	    RichTraceInfo* rti = is2d ? new RichTraceInfo( geom_ ) : 0;
	    TraceInfo* ti = rti ? rti : new TraceInfo( geom_ );
	    ti->usable_ = buf[0] != 'U';

	    char* bufpos = buf + 1;

#define mGtVal(attr,typ)   attr = *((typ*)bufpos); bufpos += sizeof(typ)
#ifdef __little__
# define mGetVal(attr,typ) { mGtVal(attr,typ); }
#else
# define mGetVal(attr,typ) { SwapBytes(bufpos,sizeof(typ)); mGtVal(attr,typ); }
#endif

	    if ( is2d )
		mGetVal(ti->pos_.trcNr(),int)
	    else
	    {
		mGetVal(ti->pos_.inLine(),int)
		mGetVal(ti->pos_.xLine(),int)
	    }
	    if ( isps )
		mGetVal(ti->pos_.offset(),float)

	    if ( isrich )
	    {
		mGetVal(rti->coord_.x,double);
		mGetVal(rti->coord_.y,double);
		mGetVal(rti->azimuth_,double);
		rti->null_ = buf[0] == 'N';
	    }

	    *this += ti;
	}

	delete [] buf;
	astrm.next();
    }

    return true;
}


bool SEGY::FileData::putTo( ascostream& astrm ) const
{
    const bool isrich = !isEmpty() && (*this)[0]->isRich();

    astrm.put( sKey::FileName, fname_ );
    astrm.put( sKey::Geometry, Seis::nameOf(geom_) );
    astrm.put( sKeyTraceSize, trcsz_ );
    astrm.put( sKeySampling, sampling_.start, sampling_.step );
    astrm.put( sKeyFormat, segyfmt_ );
    if ( isrev1_ && nrstanzas_ > 0 )
	astrm.put( sKeyNrStanzas, nrstanzas_ );
    FileMultiString fms( writeascii ? "Ascii" : "Binary" );
    if ( isrich ) fms += "Rich";
    astrm.put( sKeyStorageType, fms );
    astrm.newParagraph();

    const bool is2d = Seis::is2D( geom_ );
    const bool isps = Seis::isPS( geom_ );

    if ( writeascii )
    {
	FileMultiString keyw; FileMultiString val;
	for ( int itrc=0; itrc<size(); itrc++ )
	{
	    const TraceInfo& ti = *(*this)[itrc];
	    keyw.setEmpty();
	    if ( is2d )
		keyw += ti.trcNr();
	    else
		ti.binID().fill( keyw.buf() );
	    if ( isps )
		keyw += ti.offset();

	    val = ti.usable_ ? (ti.isNull() ? "Null" : "OK") : "Unusable";
	    if ( isrich )
	    {
		BufferString s( 128, true );
		ti.coord().fill( s.buf() ); val += s;
		val += ti.azimuth();
	    }
	    astrm.put( keyw.buf(), val.buf() );
	}
    }
    else
    {
	std::ostream& strm = astrm.stream();
	for ( int itrc=0; itrc<size(); itrc++ )
	{
	    const TraceInfo& ti = *(*this)[itrc];
	    const char ch = ti.usable_ ? (ti.isNull() ? 'N' : 'O') : 'U';
	    strm.write( &ch, 1 );

#define mPutVal(attr,typ) \
	    { \
		typ tmp; tmp = ti.attr; \
		strm.write( (const char*)(&tmp), sizeof(typ) ); \
	    }
	    if ( is2d )
		mPutVal(trcNr(),int)
	    else
		{ mPutVal(pos_.inLine(),int); mPutVal(pos_.xLine(),int) }
	    if ( isps )
		mPutVal(offset(),float)
	    if ( isrich )
	    {
		mPutVal(coord().x,double);
		mPutVal(coord().y,double);
		mPutVal(azimuth(),float);
	    }
	}

	strm << "\n";
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
	{ ti.trcidx_ = -1; ti.filenr_= 0; }

    if ( isEmpty() || ti.filenr_ >= size() )
	{ ti.filenr_ = -1; ti.trcidx_ = 0; return false; }

    ti.trcidx_++;
    if ( ti.trcidx_ >= (*this)[ti.filenr_]->size() )
	{ ti.filenr_++; ti.trcidx_ = -1; return toNext( ti, nll, unu ); }

    if ( nll && unu )
	return true;

    const FileData& fd = *(*this)[ti.filenr_];
    if ( (!nll && fd.isNull(ti.trcidx_))
      || (!unu && !fd.isUsable(ti.trcidx_)) )
	return toNext( ti, nll, unu );

    return true;
}
