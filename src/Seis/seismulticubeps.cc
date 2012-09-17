/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seismulticubeps.cc,v 1.11 2011/03/25 15:02:34 cvsbert Exp $";

#include "seismulticubeps.h"
#include "seispsioprov.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "seisread.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "posinfo.h"
#include "strmprov.h"
#include "ascstream.h"
#include "separstr.h"
#include "ioman.h"

static const char* sKeyFileType = "MultiCube Pre-Stack Seismics";


class MultiCubeSeisPSIOProvider : public SeisPSIOProvider
{
public:
			MultiCubeSeisPSIOProvider()
			    	: SeisPSIOProvider("MultiCube")
    			{}
    SeisPS3DReader*	make3DReader( const char* fnm, int ) const
			{ return new MultiCubeSeisPSReader(fnm); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm, const char* lnm ) const
			{ return 0; }
    SeisPSWriter*	make2DWriter( const char* dirnm, const char* lnm ) const
			{ return 0; }
    bool		getLineNames(const char*,BufferStringSet&) const
			{ return false; }
    static int		factid;
};

// This adds the Multicube type pre-stack seismics data storage to the factory
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


bool MultiCubeSeisPSReader::getFrom( const char* fnm )
{
    deepErase( rdrs_ ); offs_.erase(); comps_.erase(); errmsg_.setEmpty();
    posdata_ = PosInfo::CubeData();

    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
    {
	errmsg_ = "Data store definition file '"; errmsg_ += fnm;
	errmsg_ += "' not readable";
	return false;
    }

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
    {
	errmsg_ = "The file '"; errmsg_ += fnm;
	errmsg_ += "' holds no "; errmsg_ += sKeyFileType;
	sd.close(); return false;
    }

#   define mErrCont(s) { errmsg_ = s; continue; }
    while ( !atEndOfSection(astrm.next()) )
    {
	MultiID mid( astrm.keyWord() );
	if ( !IOObj::isKey(mid.buf()) )
	    mErrCont(BufferString("Invalid object ID: '",mid.buf(),"'"))

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    mErrCont(BufferString("Cannot find object: '",mid.buf(),"'"))

	FileMultiString fms( astrm.value() );
	const int fmssz = fms.size();
	const float offs = toFloat( fms[0] );
	const int comp = fmssz > 1 ? toInt( fms[1] ) : 0;

	SeisTrcReader* rdr = new SeisTrcReader( ioobj );
	rdr->setComponent( comp );
	if ( !rdr->ioObj() || !rdr->prepareWork() )
	{
	    if ( rdr->errMsg() )
		errmsg_ = rdr->errMsg();
	    else
	    {
		errmsg_ = "Error creating reader for '";
		errmsg_ += ioobj->name(); errmsg_ += "'";
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
	errmsg_ = "No valid cubes found";

    sd.close();
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

    BufferString emsg;
    bool rv = writeData( fnm, keys, offs, comps, emsg );
    if ( !rv )
	errmsg_ = emsg;
    deepErase( keys );
    return rv;
}


bool MultiCubeSeisPSReader::writeData( const char* fnm,
	const ObjectSet<MultiID>& keys, const TypeSet<float>& offs,
	const TypeSet<int>& comps, BufferString& emsg )
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
    {
	emsg = "Cannot open new file '"; emsg += fnm;
	emsg += "'"; return false;
    }

    ascostream astrm( *sd.ostrm );
    if ( !astrm.putHeader(sKeyFileType) )
    {
	emsg = "Cannot write to new file '"; emsg += fnm;
	emsg += "'"; sd.close(); return false;
    }

    for ( int idx=0; idx<keys.size(); idx++ )
    {
	FileMultiString fms;
	fms += offs[idx]; fms += comps[idx];
	astrm.put( *keys[idx], fms );
    }

    bool rv = sd.ostrm->good();
    sd.close();
    if ( !rv )
	{ emsg = "Error during write to file '"; emsg += fnm; emsg += "'"; }
    return rv;
}



void MultiCubeSeisPSReader::getCubeData( const SeisTrcReader& rdr,
					 PosInfo::CubeData& cd ) const
{
    const SeisTrcTranslator* tr = rdr.seisTranslator();
    mDynamicCastGet(const CBVSSeisTrcTranslator*,cbvstr,tr)
    if ( !cbvstr )
	return;

    const CBVSInfo::SurvGeom& sg = cbvstr->readMgr()->info().geom;
    if ( !sg.fullyrectandreg )
	cd = sg.cubedata;
    else
    {
	for ( int iinl=sg.start.inl; iinl<=sg.stop.inl; iinl+=sg.step.inl )
	{
	    PosInfo::LineData* ld = new PosInfo::LineData( iinl );
	    ld->segments_ += PosInfo::LineData::Segment( sg.start.crl,
		    				sg.stop.crl, sg.step.crl );
	    cd += ld;
	}
    }
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
