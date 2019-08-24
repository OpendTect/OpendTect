/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-1-1998
-*/


#include "seismulticubeps.h"
#include "seispsioprov.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seispacketinfo.h"
#include "seisbuf.h"
#include "cubedata.h"
#include "od_iostream.h"
#include "ascstream.h"
#include "separstr.h"
#include "trckey.h"
#include "uistrings.h"

static const char* sKeyFileType = "MultiCube Pre-Stack Seismics";


class MultiCubeSeisPSIOProvider : public SeisPSIOProvider
{
public:
			MultiCubeSeisPSIOProvider()
				: SeisPSIOProvider("MultiCube")	{}

    virtual bool	canHandle( bool forread, bool for2d ) const
			{ return forread && !for2d; }

    SeisPS3DReader*	make3DReader( const char* fnm, int ) const
			{ return new MultiCubeSeisPSReader(fnm); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm, const char* lnm ) const
			{ return 0; }
    SeisPSWriter*	make2DWriter( const char* dirnm, const char* lnm ) const
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm, Pos::GeomID ) const
			{ return 0; }
    SeisPSWriter*	make2DWriter( const char* dirnm, Pos::GeomID ) const
			{ return 0; }

    bool		getLineNames(const char*,BufferStringSet&) const
			{ return false; }

    static int		factid;
};

// This adds the Multicube type prestack seismics data storage to the factory
int MultiCubeSeisPSIOProvider::factid = SPSIOPF().add(
				new MultiCubeSeisPSIOProvider );


MultiCubeSeisPSReader::MultiCubeSeisPSReader( const char* fnm )
    : posdata_(*new PosInfo::CubeData)
{
    getFrom( fnm );
}


MultiCubeSeisPSReader::~MultiCubeSeisPSReader()
{
    deepErase( provs_ );
    delete &posdata_;
}


#define mRetStrmErrMsg(msg,s1) \
    { msg = s1; \
    strm.addErrMsgTo( msg ); return false; }


bool MultiCubeSeisPSReader::getFrom( const char* fnm )
{
    deepErase( provs_ ); offs_.erase(); comps_.erase(); errmsg_.setEmpty();
    posdata_ = PosInfo::CubeData();

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mRetStrmErrMsg(errmsg_, uiStrings::phrCannotRead( toUiString(fnm) ) )

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	mRetStrmErrMsg(errmsg_, uiStrings::phrCannotRead( toUiString(fnm) ))

#   define mErrCont(s) { errmsg_ = s; continue; }
    while ( !atEndOfSection(astrm.next()) )
    {
	const DBKey dbky( astrm.keyWord() );
	FileMultiString fms( astrm.value() );
	const int fmssz = fms.size();
	const float offs = fms.getFValue( 0 );
	const int comp = fmssz > 1 ? fms.getIValue( 1 ) : 0;

	uiRetVal uirv;
	Seis::Provider* prov = Seis::Provider::create( dbky, &uirv );
	if ( !prov )
	    { errmsg_ = uirv; delete prov; continue; }

	prov->selectComponent( comp );
	provs_ += prov; offs_ += offs; comps_ += comp;

	const auto cd = prov->as3D()->possibleCubeData();
	if ( provs_.size() == 1 )
	    posdata_ = cd;
	else
	    posdata_.merge( cd, true );
    }

    if ( provs_.isEmpty() && errmsg_.isEmpty() )
	errmsg_ = tr("No valid cubes found");

    return true;
}


bool MultiCubeSeisPSReader::putTo( const char* fnm ) const
{
    DBKeySet keys; TypeSet<float> offs; TypeSet<int> comps;
    for ( int iprov=0; iprov<provs_.size(); iprov++ )
    {
	const DBKey dbky = provs_[iprov]->dbKey();
	if ( !dbky.isInvalid() )
	{
	    keys += dbky;
	    offs += offs_[iprov];
	    comps += comps_[iprov];
	}
    }

    uiString emsg;
    bool rv = writeData( fnm, keys, offs, comps, emsg );
    if ( !rv )
	errmsg_ = emsg;
    return rv;
}


bool MultiCubeSeisPSReader::readData( const char* fnm, DBKeySet& keys,
		TypeSet<float>& offs, TypeSet<int>& comps, uiString& emsg )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mRetStrmErrMsg(emsg, uiStrings::phrCannotOpenForRead( fnm) )
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
    {
	emsg = tr("File:\n%1\nis not of type %2").arg( fnm ).arg(sKeyFileType);
	return false;
    }

    while ( !atEndOfSection(astrm.next()) )
    {
	const DBKey ky( astrm.keyWord() );
	const FileMultiString fms( astrm.value() );
	if ( ky.isInvalid() || fms.size() < 1 )
	    continue;
	PtrMan<IOObj> ioobj = ky.getIOObj();
	if ( !ioobj || ioobj->isBad() )
	    continue;

	const float offset = fms.getFValue( 0 );
	const int icomp = fms.getIValue( 1 );
	if ( mIsUdf(offset) || offset < 0 || icomp < 0 )
	    continue;

	keys += ky;
	offs += offset; comps += icomp;
    }

    if ( offs.isEmpty() )
    {
	emsg = tr("File:\n%1\n contains no valid data").arg( fnm );
	return false;
    }
    return true;
}


bool MultiCubeSeisPSReader::writeData( const char* fnm,
	const DBKeySet& keys, const TypeSet<float>& offs,
	const TypeSet<int>& comps, uiString& emsg )
{
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mRetStrmErrMsg(emsg, uiStrings::phrCannotOpenForWrite(fnm) )

    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	mRetStrmErrMsg(emsg, uiStrings::phrCannotWrite( toUiString(fnm)))

    for ( int idx=0; idx<keys.size(); idx++ )
    {
	FileMultiString fms;
	fms += offs[idx]; fms += comps[idx];
	astrm.put( keys[idx].toString(), fms );
    }

    if ( !strm.isOK() )
	mRetStrmErrMsg(emsg,uiStrings::phrCannotWrite( toUiString(fnm) ) )
    return true;
}


SeisTrc* MultiCubeSeisPSReader::getTrace( const TrcKey& tk, int nr ) const
{
    if ( nr >= provs_.size() ) return 0;

    Seis::Provider& prov = const_cast<Seis::Provider&>( *provs_[nr] );
    SeisTrc* trc = new SeisTrc;
    const uiRetVal uirv = prov.getAt( tk, *trc );
    if ( !uirv.isOK() )
    {
	deleteAndZeroPtr( trc );
	if ( !isFinished(uirv) )
	    errmsg_ = uirv;
    }
    else
	trc->info().offset_ = offs_[nr];

    return trc;
}


SeisTrc* MultiCubeSeisPSReader::getTrace( const BinID& bid, int nr ) const
{
    return getTrace( TrcKey(bid), nr );
}


bool MultiCubeSeisPSReader::getGather( const TrcKey& tk, SeisTrcBuf& buf ) const
{
    buf.deepErase(); buf.setIsOwner( true );
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	SeisTrc* newtrc = getTrace( tk.position(), idx );
	if ( newtrc )
	    buf.add( newtrc );
    }

    return !buf.isEmpty();
}


bool MultiCubeSeisPSReader::getGather( const BinID& bid, SeisTrcBuf& buf ) const
{
    return getGather( TrcKey(bid), buf );
}


void MultiCubeSeisPSReader::usePar( const IOPar& iop )
{
}
