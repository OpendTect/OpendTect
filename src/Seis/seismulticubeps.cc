/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismulticubeps.h"
#include "seispsioprov.h"
#include "seisread.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seispacketinfo.h"
#include "seisbuf.h"
#include "posinfo.h"
#include "od_iostream.h"
#include "ascstream.h"
#include "separstr.h"
#include "ioman.h"
#include "uistrings.h"

static const char* sKeyFileType = "MultiCube Pre-Stack Seismics";


class MultiCubeSeisPSIOProvider : public SeisPSIOProvider
{
public:
			MultiCubeSeisPSIOProvider()
				: SeisPSIOProvider("MultiCube")	{}

    bool		canHandle( bool forread, bool for2d ) const override
			{ return forread && !for2d; }

    SeisPS3DReader*	make3DReader( const char* fnm, int ) const override
			{ return new MultiCubeSeisPSReader(fnm); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const override
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm,
				      const char* lnm ) const override
			{ return 0; }
    SeisPSWriter*	make2DWriter( const char* dirnm,
				      const char* lnm ) const override
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm,
				      Pos::GeomID ) const override
			{ return 0; }
    SeisPSWriter*	make2DWriter( const char* dirnm,
				      Pos::GeomID ) const override
			{ return 0; }

    bool		getLineNames( const char*,
				      BufferStringSet& ) const override
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
    deepErase( rdrs_ );
    delete &posdata_;
}


#define mRetStrmErrMsg(msg,s1) \
    { msg = s1; \
    strm.addErrMsgTo( msg ); return false; }


bool MultiCubeSeisPSReader::getFrom( const char* fnm )
{
    deepErase( rdrs_ ); offs_.erase(); comps_.erase(); errmsg_.setEmpty();
    posdata_ = PosInfo::CubeData();
    const Seis::GeomType gt = Seis::Vol;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mRetStrmErrMsg(errmsg_, uiStrings::phrCannotRead( toUiString(fnm) ) )

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	mRetStrmErrMsg(errmsg_, uiStrings::phrCannotRead( toUiString(fnm) ))

#   define mErrCont(s) { errmsg_ = s; continue; }
    while ( !atEndOfSection(astrm.next()) )
    {
	MultiID mid( astrm.keyWord() );

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    mErrCont(uiStrings::phrCannotFindDBEntry(mid) )

	FileMultiString fms( astrm.value() );
	const int fmssz = fms.size();
	const float offs = fms.getFValue( 0 );
	const int comp = fmssz > 1 ? fms.getIValue( 1 ) : 0;

	auto* rdr = new SeisTrcReader( *ioobj, &gt );
	rdr->setComponent( comp );
	if ( !rdr->ioObj() || !rdr->prepareWork() )
	{
	    if ( !rdr->errMsg().isEmpty() )
		errmsg_ = rdr->errMsg();
	    else
	    {
		errmsg_ = uiStrings::phrCannotRead( ioobj->uiName() );
	    }
	    delete rdr; continue;
	}

	rdrs_ += rdr; offs_ += offs; comps_ += comp;

	PosInfo::CubeData cd; getCubeData( *rdr, cd );
	if ( rdrs_.size() == 1 )
	    posdata_ = cd;
	else
	    posdata_.merge( cd, true );
    }

    bool rv = !rdrs_.isEmpty();
    if ( !rv && errmsg_.isEmpty() )
	errmsg_ = tr("No valid cubes found");

    return true;
}


bool MultiCubeSeisPSReader::putTo( const char* fnm ) const
{
    ObjectSet<MultiID> keys; TypeSet<float> offs; TypeSet<int> comps;
    for ( int irdr=0; irdr<rdrs_.size(); irdr++ )
    {
	const IOObj* ioobj = rdrs_[irdr]->ioObj();
	if ( !ioobj ) continue;
	keys += new MultiID( ioobj->key() );
	offs += offs_[irdr];
	comps += comps_[irdr];
    }

    uiString emsg;
    bool rv = writeData( fnm, keys, offs, comps, emsg );
    if ( !rv )
	errmsg_ = emsg;
    deepErase( keys );
    return rv;
}


bool MultiCubeSeisPSReader::readData( const char* fnm, ObjectSet<MultiID>& keys,
		TypeSet<float>& offs, TypeSet<int>& comps, uiString& emsg )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mRetStrmErrMsg(emsg, uiStrings::phrCannotOpen( toUiString(fnm)) )
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
    {
	emsg = tr( "File:\n%1\nis not of type %2").arg( fnm ).arg(sKeyFileType);
	return false;
    }

    while ( !atEndOfSection(astrm.next()) )
    {
	const MultiID ky( astrm.keyWord() );
	const FileMultiString fms( astrm.value() );
	if ( ky.isUdf() || fms.size() < 1 )
	    continue;
	PtrMan<IOObj> ioobj = IOM().get( ky );
	if ( !ioobj || ioobj->isBad() )
	    continue;

	const float offset = fms.getFValue( 0 );
	const int icomp = fms.getIValue( 1 );
	if ( mIsUdf(offset) || offset < 0 || icomp < 0 )
	    continue;

	keys += new MultiID( ky );
	offs += offset; comps += icomp;
    }

    if ( offs.isEmpty() )
    {
	emsg = tr( "File:\n%1\n contains no valid data" ).arg( fnm );
	return false;
    }
    return true;
}


bool MultiCubeSeisPSReader::writeData( const char* fnm,
	const ObjectSet<MultiID>& keys, const TypeSet<float>& offs,
	const TypeSet<int>& comps, uiString& emsg )
{
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mRetStrmErrMsg(emsg, uiStrings::phrCannotOpen( toUiString(fnm)))

    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	mRetStrmErrMsg(emsg, uiStrings::phrCannotWrite( toUiString(fnm)))

    for ( int idx=0; idx<keys.size(); idx++ )
    {
	FileMultiString fms;
	fms += offs[idx]; fms += comps[idx];
	astrm.put( keys[idx]->toString(), fms );
    }

    if ( !strm.isOK() )
	mRetStrmErrMsg(emsg,uiStrings::phrCannotWrite( toUiString(fnm) ) )
    return true;
}



void MultiCubeSeisPSReader::getCubeData( const SeisTrcReader& rdr,
					 PosInfo::CubeData& cd ) const
{
    const SeisTrcTranslator* trans = rdr.seisTranslator();
    if ( !trans )
	return;
    const SeisPacketInfo& pi(
		const_cast<SeisTrcTranslator*>(trans)->packetInfo());
    if ( pi.cubedata )
	cd = *pi.cubedata;
    else
	cd.generate( BinID(pi.inlrg.start,pi.crlrg.start),
		     BinID(pi.inlrg.stop,pi.crlrg.stop),
		     BinID(pi.inlrg.step,pi.crlrg.step) );
}


SeisTrc* MultiCubeSeisPSReader::getTrace( const BinID& bid, int nr ) const
{
    if ( nr >= rdrs_.size() ) return 0;

    SeisTrcReader& rdr = const_cast<SeisTrcReader&>( *rdrs_[nr] );
    SeisTrc* trc = new SeisTrc;
    if ( !rdr.seisTranslator()->goTo(bid) )
	{ delete trc; trc = 0; }
    else if ( !rdr.get(*trc) )
	{ errmsg_ = rdr.errMsg(); delete trc; trc = 0; }
    else
	trc->info().offset = offs_[nr];

    return trc;
}


bool MultiCubeSeisPSReader::getGather( const BinID& bid, SeisTrcBuf& buf ) const
{
    buf.deepErase(); buf.setIsOwner( true );
    for ( int idx=0; idx<rdrs_.size(); idx++ )
    {
	SeisTrc* newtrc = getTrace( bid, idx );
	if ( newtrc )
	    buf.add( newtrc );
    }

    return buf.isEmpty() ? false : true;
}


void MultiCubeSeisPSReader::usePar( const IOPar& iop )
{
}


MultiCubeSeisPS3DTranslator::~MultiCubeSeisPS3DTranslator()
{}
