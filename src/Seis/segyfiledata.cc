/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

#include "segyfiledata.h"

#include "ascstream.h"
#include "datainterp.h"
#include "envvars.h"
#include "filepath.h"
#include "iopar.h"
#include "idxable.h"
#include "file.h"
#include "trckeysampling.h"
#include "keystrs.h"
#include "offsetazimuth.h"
#include "oddirs.h"
#include "segyhdr.h"
#include "separstr.h"
#include "seisposindexer.h"
#include "survinfo.h"
#include "segytr.h"
#include "od_iostream.h"


static const char* sKeyNrFiles = "Number of files";

static const char* sKeyTraceSize = "Trace size";
static const char* sKeySampling = "Z sampling";
static const char* sKeyRev1Marked = "File marked as REV. 1";
static const char* sKeyNrStanzas = "Nr REV.1 Text stanzas";
static const char* sKeyStorageType = "Storage type";
static const char* sKeyNrUsable = "Nr Usable";
static const char* sKeyDiscardNull = "Discard Null";


SEGY::FileDataSet::StoredData::StoredData( const char* filename,
	od_stream_Pos offset,
	const DataCharacteristics& dc32 )
    : int32di_( DataInterpreter<int>::create( dc32, false ) )
    , start_( offset )
    , istrm_( new od_istream(filename) )
    , ostrm_( 0 )
{}


SEGY::FileDataSet::StoredData::StoredData( od_ostream& ostrm )
    : int32di_( 0 )
    , start_( ostrm.position() )
    , istrm_( 0 )
    , ostrm_( &ostrm )
{}


SEGY::FileDataSet::StoredData::~StoredData()
{
    delete istrm_;
    delete int32di_;
}


bool SEGY::FileDataSet::StoredData::getKey( od_stream_Pos pos, Seis::PosKey& pk,
					    bool& usable ) const
{
    Threads::Locker lckr( lock_ );

    if ( !istrm_ || pos<0 )
	return false;

    static const int unitsz = sizeof(int)+sizeof(int)+sizeof(int)+sizeof(bool);
    const od_stream_Pos offset = start_+pos*unitsz;
    istrm_->setReadPosition( offset );
    if ( !istrm_->isOK() )
	return false;

    BinID bid;
    int offsetazimuth;
    if ( !DataInterpreter<int>::get( int32di_, *istrm_, bid.inl() ) ||
	 !DataInterpreter<int>::get( int32di_, *istrm_, bid.crl() ) ||
	 !DataInterpreter<int>::get( int32di_, *istrm_, offsetazimuth ) ||
	 istrm_->getBin( usable ).isBad() )
	 return false;

    OffsetAzimuth oa; oa.setFrom( offsetazimuth );

    pk.setBinID( bid );
    pk.setOffset( oa.offset() );

    return true;
}


bool SEGY::FileDataSet::StoredData::add( const Seis::PosKey& pk, bool usable )
{
    if ( !ostrm_ || !ostrm_->isOK() )
	return false;

    const int inl = pk.binID().inl();
    const int crl = pk.binID().crl();

    ostrm_->addBin( inl );
    ostrm_->addBin( crl );

    const OffsetAzimuth oa( pk.offset(), 0 );
    const int oaint = oa.asInt();
    ostrm_->addBin( oaint );
    ostrm_->addBin( usable );

    return ostrm_->isOK();
}


SEGY::FileDataSet::FileDataSet( const IOPar& iop, ascistream& strm )
    : segypars_( iop )
    , storeddata_( 0 )
    , totalsz_( 0 )
    , indexer_( 0 )
    , coords_( 0 )
    , nrusable_( 0 )
    , discardnull_(false)
{
    readVersion1( strm );
}


SEGY::FileDataSet::FileDataSet( const IOPar& iop,
				const char* filename,od_stream_Pos start,
				const DataCharacteristics& int32 )
    : segypars_( iop )
    , storeddata_( new StoredData( filename, start, int32 ) )
    , totalsz_( 0 )
    , indexer_( 0 )
    , coords_( 0 )
    , nrstanzas_( 0 )
    , nrusable_( 0 )
    , discardnull_(false)
{}


SEGY::FileDataSet::FileDataSet( const IOPar& iop )
    : segypars_( iop )
    , storeddata_( 0 )
    , totalsz_( 0 )
    , indexer_( 0 )
    , coords_( 0 )
    , nrstanzas_( 0 )
    , nrusable_( 0 )
    , discardnull_(false)
{}



SEGY::FileDataSet::~FileDataSet()
{
    delete storeddata_;
    delete coords_;
}


bool SEGY::FileDataSet::setOutputStream( od_ostream& strm )
{
    delete storeddata_;
    storeddata_ = new SEGY::FileDataSet::StoredData( strm );

    return true;
}


bool SEGY::FileDataSet::usePar( const IOPar& par )
{
    filenames_.erase();
    cumsizes_.erase();;

    totalsz_ = 0;
    int nrfiles = 0;
    if ( !par.get( sKeyNrFiles, nrfiles ) )
	return false;

    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	BufferString key("File ");
	key += ifile;

	PtrMan<IOPar> filepars = par.subselect( key.buf() );
	if ( !filepars )
	    return false;

	BufferString filenm;
	if ( !filepars->get( sKey::FileName(), filenm ) )
	    return false;

	FilePath filepath( filenm );
	if ( !filepath.isAbsolute() )
	{
	    FilePath absolutepath( GetBaseDataDir() );
	    absolutepath.add( SI().getDirName() );
	    absolutepath.add( filenm );
	    if ( File::exists( absolutepath.fullPath().str() ) )
		filenm = absolutepath.fullPath().str();
	}

	od_int64 filesz;
	if ( !filepars->get( sKey::Size(), filesz ) )
	    return false;

	filenames_.add( filenm );
	cumsizes_ += totalsz_;
	totalsz_ += filesz;
    }

    Seis::getFromPar(par,geom_);

    bool isnotrev0 = !isrev0_;
    par.getYN( sKeyRev1Marked, isnotrev0 );
    isrev0_ = !isnotrev0;
    par.get( sKeySampling, sampling_ );
    par.get( sKeyTraceSize, trcsz_ );
    par.get( sKeyNrStanzas, nrstanzas_ );
    par.get( sKeyNrUsable, nrusable_ );
    par.getYN( sKeyDiscardNull, discardnull_ );

    return true;
}



void SEGY::FileDataSet::fillPar( IOPar& par ) const
{
    const int nrfiles = nrFiles();
    par.set( sKeyNrFiles, nrfiles );
    par.setYN( sKeyRev1Marked, !isrev0_ );
    par.set( sKeySampling, sampling_ );
    par.set( sKeyTraceSize, trcsz_ );
    par.set( sKeyNrStanzas, nrstanzas_ );
    par.set( sKeyNrUsable, nrusable_ );
    par.setYN( sKeyDiscardNull, discardnull_ );
    Seis::putInPar( geom_, par );

    FilePath survdirname( GetBaseDataDir() );
    survdirname.add( SI().getDirName() );
    survdirname.makeCanonical();

    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	IOPar filepars;
	filepars.set( sKey::FileName(), fileName( ifile ) );
	const od_int64 nextsize = ifile<nrfiles-1
	    ? cumsizes_[ifile+1]
	    : totalsz_;

	const od_int64 filesz = nextsize-cumsizes_[ifile];
	filepars.set( sKey::Size(), filesz );
	SEGY::FileSpec::makePathsRelative( filepars );

	BufferString key("File ");
	key += ifile;
	par.mergeComp( filepars, key.buf() );
    }
}


FixedString SEGY::FileDataSet::fileName( int idx ) const
{
    if ( !filenames_.validIdx(idx) )
	return sKey::EmptyString();

    return filenames_[idx]->buf();
}



int SEGY::FileDataSet::nrFiles() const
{ return filenames_.size(); }


void SEGY::FileDataSet::addFile( const char* file )
{
    filenames_.add( file );
    cumsizes_ += totalsz_;
}


bool SEGY::FileDataSet::addTrace( int fileidx, const Seis::PosKey& pk,
				  const Coord& crd, bool usable )
{
    if ( !filenames_.validIdx(fileidx) )
	return false;

    if ( storeddata_ )
    {
	if ( !storeddata_->add( pk, usable ) )
	    return false;
    }
    else
    {
	keys_ += pk;
	usable_ += usable;
    }

    if ( indexer_ ) indexer_->add( pk, totalsz_ );
    if ( Seis::is2D(geom_) && coords_ )
	coords_->set( pk.trcNr(), crd );

    totalsz_ ++;

    if ( usable )
	nrusable_++;

    return true;
}


SEGY::FileDataSet::TrcIdx SEGY::FileDataSet::getFileIndex( od_int64 idx ) const
{
    SEGY::FileDataSet::TrcIdx res;
    if ( idx>=0 )
    {
	IdxAble::findPos( cumsizes_.arr(), cumsizes_.size(), idx, -1,
			  res.filenr_ );
	res.trcidx_ = idx - cumsizes_[res.filenr_];
    }

    return res;
}



bool SEGY::FileDataSet::readVersion1( ascistream& astrm )
{
    totalsz_ = 0;
    cumsizes_.erase();
    keys_.erase();
    usable_.erase();

    int nrfiles = 0;
    segypars_.get( sKeyNrFiles, nrfiles );
    Seis::GeomType gt;
    if ( !Seis::getFromPar(segypars_,gt) )
	return false;

    for ( int idx=0; idx<nrfiles; idx++ )
    {
	if ( !readVersion1File(astrm) )
	    return false;
    }

    return true;
}


void SEGY::FileDataSet::save2DCoords( bool yn )
{
    if ( yn==((bool)coords_) )
	return;

    if ( !coords_ ) coords_ = new SortedTable<int,Coord>;
    else
    {
	delete coords_;
	coords_ = 0;
    }
}


Coord SEGY::FileDataSet::get2DCoord( int nr ) const
{
    Coord res = Coord::udf();
    if ( coords_ )
	coords_->get( nr, res );

    return res;
}


#define mErrRet(s) { ErrMsg(s); return false; }

bool SEGY::FileDataSet::readVersion1File( ascistream& astrm )
{
    BufferString fname;
    int trcsz = -1;
    bool isasc = false;
    bool isrich = false;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::FileName()) )
	    fname = astrm.value();
	if ( astrm.hasKeyword(sKey::Geometry()) )
	{
	    const Seis::GeomType geom = Seis::geomTypeOf( astrm.value() );
	    if ( filenames_.size()==0 )
		geom_ = geom;
	    else if ( geom!=geom_ )
		return false;
	}
	else if ( astrm.hasKeyword(sKeyTraceSize) )
	    trcsz = astrm.getIValue();
	else if ( astrm.hasKeyword(sKeyStorageType) )
	{
	    isasc = *astrm.value() == 'A';
	    FileMultiString fms( astrm.value() );
	    if ( fms.size() > 1 )
		isrich = *fms[1] == 'R';
	}
    }

    if ( fname.isEmpty() )
	mErrRet("No filename found in header")
    else if ( trcsz < 1 )
	mErrRet("Invalid trace size in header")

    addFile( fname.str() );

    const bool is2d = Seis::is2D( geom_ );
    const bool isps = Seis::isPS( geom_ );
    BinID bid;

    const int fileidx = nrFiles()-1;

    if ( isasc )
    {
	FileMultiString keyw; FileMultiString val;

	while ( !atEndOfSection(astrm.next()) )
	{
	    Seis::PosKey pk;
	    Coord crd( Coord::udf() );
	    keyw = astrm.keyWord(); val = astrm.value();
	    if ( is2d )
		pk.setTrcNr( keyw.getIValue(0) );
	    else
		pk.binID().fromString( keyw[0] );
	    if ( isps )
		pk.setOffset( keyw.getFValue(1) );

	    const char ch( *val[0] );
	    const bool usable = ch != 'U';
	    if ( isrich )
		crd.fromString( val[1] );

	    addTrace( fileidx, pk, crd, usable );
	}
    }
    else
    {
	od_istream& strm = astrm.stream();
	unsigned int entrylen = 1 + (isps ? 4 : 0) + sizeof(int);
	if ( !is2d )
	    entrylen += sizeof(int);
	if ( isrich )
	    entrylen += 2*sizeof(double) + sizeof(float);
	char* buf = new char [entrylen];
	Coord crd( Coord::udf() );

	while ( strm.isOK() )
	{
	    const char ch = strm.peek();
	    if ( ch == '\n' )
		{ strm.ignore( 1 ); break; }

	    strm.getBin( buf, entrylen );
	    const bool usable = buf[0] != 'U';

	    char* bufpos = buf + 1;

#define mGtVal(attr,typ)   attr = *((typ*)bufpos); bufpos += sizeof(typ)
#ifdef __little__
# define mGetVal(attr,typ) { mGtVal(attr,typ); }
#else
# define mGetVal(attr,typ) { SwapBytes(bufpos,sizeof(typ)); mGtVal(attr,typ); }
#endif
	    Seis::PosKey pk;

	    if ( is2d )
		mGetVal(pk.trcNr(),int)
	    else
	    {
		mGetVal(pk.inLine(),int)
		mGetVal(pk.xLine(),int)
	    }
	    if ( isps )
		mGetVal(pk.offset(),float)
	    if ( isrich )
	    {
		mGetVal( crd.x, double );
		mGetVal( crd.y, double );
	    }

	    addTrace( fileidx, pk, crd, usable );
	}

	delete [] buf;
	astrm.next();
    }

    return true;
}


void SEGY::FileDataSet::setAuxData( const Seis::GeomType& gt,
				    const SEGYSeisTrcTranslator& tr )
{
    geom_ = gt;

    trcsz_ = tr.inpNrSamples();
    sampling_ = tr.inpSD();
    isrev0_ = tr.isRev0();
    nrstanzas_ = tr.binHeader().entryVal( SEGY::BinHeader::EntryRevCode() + 2 );
}


void SEGY::FileDataSet::getReport( IOPar& iop ) const
{
    iop.add( IOPar::sKeySubHdr(), "General info" );
    if ( totalsz_ < 1 )
    { iop.add( "Number of traces found", "0" ); return; }

    BufferString nrtrcsstr( "", totalsz_, " (" );

    if ( nrusable_ == totalsz_ )
	nrtrcsstr += "all usable)";
    else
    { nrtrcsstr += nrusable_; nrtrcsstr += " usable)"; }
    iop.add( "Number of traces found", nrtrcsstr );
    iop.add( "Number of samples in file", trcsz_ );
    const Interval<float> zrg( sampling_.interval(trcsz_) );
    iop.add( "Z range in file", zrg.start, zrg.stop );
    iop.add( "Z step in file", sampling_.step );
    iop.addYN( "File marked as REV. 1 or higher", !isrev0_ );
    if ( !isrev0_ && nrstanzas_ > 0 )
	iop.add( "Number of REV.1 extra stanzas", nrstanzas_ );

    TrcKeySampling hs( false );
    Interval<int> nrrg;
    Interval<float> offsrg;

    if ( indexer_ )
    {
	hs.set( indexer_->inlRange(), indexer_->crlRange() );
	offsrg = indexer_->offsetRange();
	nrrg = indexer_->trcNrRange();
    }
    else
    {
	int firstok = 0;
	bool usable;
	Seis::PosKey pk;
	for ( ; firstok<totalsz_; firstok++ )
	{
	    if ( getDetails( firstok, pk, usable ) && usable )
		break;
	}

	if ( firstok >= totalsz_ ) return;

	hs.start_ = hs.stop_ = pk.binID();
	nrrg.start = nrrg.stop = pk.trcNr();
	offsrg.start = offsrg.stop = pk.offset();

	for ( int idx=firstok+1; idx<totalsz_; idx++ )
	{
	    if ( !getDetails( idx, pk, usable ) || !usable )
		continue;

	    hs.include( pk.binID() );
	    nrrg.include( pk.trcNr() );
	    offsrg.include( pk.offset() );
	}
    }

    iop.add( IOPar::sKeySubHdr(), "Ranges" );
    if ( Seis::is2D(geom_) )
	iop.add( "Trace number range", nrrg.start, nrrg.stop );
    else
    {
	iop.add( "In-line range", hs.start_.inl(), hs.stop_.inl() );
	iop.add( "Cross-line range", hs.start_.crl(), hs.stop_.crl() );
    }

    if ( Seis::isPS(geom_) )
    {
	BufferString offsetrangestr( "Offset range ", SI().getXYUnitString() );
	iop.add( offsetrangestr, offsrg.start, offsrg.stop );
    }
}


void SEGY::FileDataSet::dump( od_ostream& strm ) const
{
    Seis::PosKey pk; bool usable;
    for ( od_int64 idx=0; idx<totalsz_; idx++ )
    {
	getDetails( idx, pk, usable );
	strm << idx << od_tab << pk.inLine() << od_tab << pk.xLine()
	     << od_tab << pk.offset() << od_tab << (usable?'Y':'N') << od_endl;
    }
}


bool SEGY::FileDataSet::getDetails( od_int64 idx, Seis::PosKey& pk,
				    bool& usable ) const
{
    if ( idx<0 || idx>=totalsz_ )
	return false;

    if ( storeddata_ )
	return storeddata_->getKey( idx, pk, usable );

    pk = keys_[mCast(int,idx)];
    usable = usable_[mCast(int,idx)];

    return true;
}


void SEGY::FileDataSet::setIndexer( Seis::PosIndexer* n )
{
    indexer_ = n;
    if ( indexer_ ) indexer_->setEmpty();
}
