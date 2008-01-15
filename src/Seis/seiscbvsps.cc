/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seiscbvsps.cc,v 1.22 2008-01-15 16:19:43 cvsbert Exp $";

#include "seiscbvsps.h"
#include "seispsioprov.h"
#include "seiscbvs.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "cbvsreadmgr.h"
#include "filepath.h"
#include "filegen.h"
#include "strmprov.h"
#include "survinfo.h"
#include "segposinfo.h"
#include "dirlist.h"
#include "strmoper.h"
#include "iopar.h"
#include "errh.h"

class CBVSSeisPSIOProvider : public SeisPSIOProvider
{
public:
			CBVSSeisPSIOProvider() : SeisPSIOProvider("CBVS") {}
    SeisPS3DReader*	makeReader( const char* dirnm, int inl ) const
			{ return new SeisCBVSPS3DReader(dirnm,inl); }
    SeisPSWriter*	makeWriter( const char* dirnm ) const
			{ return new SeisCBVSPS3DWriter(dirnm); }
    static int		factid;
};

// This adds the CBVS type pre-stack seismics data storage to the factory
int CBVSSeisPSIOProvider::factid = SPSIOPF().add( new CBVSSeisPSIOProvider );


SeisCBVSPSIO::SeisCBVSPSIO( const char* dirnm )
    	: dirnm_(dirnm)
{
    BufferString& sm = const_cast<BufferString&>( selmask_ );
    sm = "*."; sm += CBVSSeisTrcTranslator::sKeyDefExtension;
}


SeisCBVSPSIO::~SeisCBVSPSIO()
{
}


SeisCBVSPS3DReader::SeisCBVSPS3DReader( const char* dirnm, int inl )
    	: SeisCBVSPSIO(dirnm)
    	, posdata_(*new PosInfo::CubeData)
    	, curtr_(0)
    	, curinl_(mUdf(int))
{
    if ( !File_isDirectory(dirnm_) )
    {
	errmsg_ = "Directory '"; errmsg_ += dirnm_;
	errmsg_ += "' does not exist";
	return;
    }

    if ( mIsUdf(inl) )
    {
	DirList dl( dirnm_, DirList::FilesOnly, selmask_.buf() );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    BufferString fnm( dl.get(idx) );
	    char* ptr = fnm.buf();
	    while ( *ptr && !isdigit(*ptr) ) ptr++;
	    while ( *ptr && isdigit(*ptr) ) ptr++;
	    *ptr = '\0';
	    if ( fnm.isEmpty() ) continue;

	    addInl( atoi(fnm.buf()) );
	}

	if ( posdata_.size() < 1 )
	{
	    errmsg_ = "Directory '"; errmsg_ += dirnm_;
	    errmsg_ += "' contains no usable pre-stack data files";
	    return;
	}
	posdata_.sort();
    }
    else if ( inl >= 0 )
	addInl( inl );
}


SeisCBVSPS3DReader::~SeisCBVSPS3DReader()
{
    delete &posdata_;
    delete curtr_;
}


void SeisCBVSPS3DReader::addInl( int inl )
{
    if ( !mkTr(inl) ) return;

    PosInfo::LineData* newid = new PosInfo::LineData( inl );
    const CBVSInfo::SurvGeom& sg = curtr_->readMgr()->info().geom;

    if ( sg.fullyrectandreg )

	newid->segments_ += PosInfo::LineData::Segment( sg.start.inl,
					sg.stop.inl, sg.step.inl );
    else
    {
	const PosInfo::CubeData& cd = sg.cubedata;
	if ( cd.size() < 1 )
	    { pErrMsg("Huh? should get error earlier"); delete newid; return; }

	PosInfo::LineData::Segment seg( cd[0]->linenr_, cd[0]->linenr_, 1 );
	if ( cd.size() > 1 )
	{
	    seg.stop = cd[1]->linenr_;
	    seg.step = seg.stop - seg.start;
	    for ( int idx=2; idx<cd.size(); idx++ )
	    {
		const PosInfo::LineData& id = *cd[idx];
		if ( seg.stop == seg.start )
		    { seg.stop = id.linenr_; seg.step = seg.stop - seg.start; }
		else if ( id.linenr_ != cd[idx-1]->linenr_ + seg.step )
		{
		    newid->segments_ += seg;
		    seg.start = seg.stop = id.linenr_;
		}
		else
		    seg.stop = id.linenr_;
	    }
	    newid->segments_ += seg;
	}
    }

    posdata_ += newid;
}


bool SeisCBVSPS3DReader::mkTr( int inl ) const
{
    if ( curtr_ && curinl_ == inl )
	return true;
    else if ( mIsUdf(inl) )
	return false;

    delete curtr_; curtr_ = 0;
    curinl_ = inl;

    FilePath fp( dirnm_ );
    BufferString fnm = inl; fnm += ext();
    fp.add( fnm );

    BufferString filep = fp.fullPath();

    if( !File_exists( (const char*)filep ) )
    {
	errmsg_ = "No Pre Stack data available for that inline.\n";
	errmsg_ += "Please choose another inline.";
	return false;
    }

    errmsg_ = "";
    curtr_ = CBVSSeisTrcTranslator::make( fp.fullPath(), false, false,
					  &errmsg_ );
    return curtr_;
}


bool SeisCBVSPS3DReader::getGather( int crl, SeisTrcBuf& gath ) const
{
    gath.deepErase();
    gath.setIsOwner( true );
    if ( !curtr_->goTo( BinID(crl,1) ) )
	{ errmsg_ = "Crossline not present"; return false; }

    const BinID bid( curinl_, crl );
    const Coord coord = SI().transform( bid );
    while ( true )
    {
	SeisTrc* trc = new SeisTrc;
	if ( !curtr_->read(*trc) )
	{
	    delete trc;
	    errmsg_ = curtr_->errMsg();
	    return errmsg_.isEmpty();
	}
	else if ( trc->info().binid.inl != bid.crl )
	    { delete trc; return true; }

	trc->info().nr = trc->info().binid.crl;
	trc->info().binid = bid; trc->info().coord = coord;
	gath.add( trc );
    }

    // Not reached
    return true;
}


bool SeisCBVSPS3DReader::getGather( const BinID& bid, SeisTrcBuf& gath ) const
{
    return mkTr( bid.inl ) && getGather( bid.crl, gath );
}


bool SeisCBVSPS3DReader::getSampleNames( BufferStringSet& nms ) const
{
    FilePath fp( dirnm_ ); fp.add( "samplenames.txt" );
    const BufferString fnm( fp.fullPath() );

    StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
    if ( !sd.usable() ) return false;

    nms.deepErase();
    BufferString nm;
    while ( StrmOper::readLine(*sd.istrm,&nm) )
	nms.add( nm.buf() );
    sd.close();

    return true;
}


SeisCBVSPS3DWriter::SeisCBVSPS3DWriter( const char* dirnm )
    	: SeisCBVSPSIO(dirnm)
    	, reqdtype_(DataCharacteristics::Auto)
    	, tr_(0)
    	, prevbid_(*new BinID(mUdf(int),mUdf(int)))
	, nringather_(1)
{
    if ( !File_isDirectory(dirnm_) )
    {
	if ( File_exists(dirnm_) )
	{
	    errmsg_ = "Existing file '";
	    errmsg_ += dirnm_; errmsg_ += "'. Remove or rename.";
	    return;
	}
	File_createDir(dirnm_,0);
    }
}


SeisCBVSPS3DWriter::~SeisCBVSPS3DWriter()
{
    close();
    delete &prevbid_;
}


void SeisCBVSPS3DWriter::close()
{
    delete tr_; tr_ = 0;
    prevbid_ = BinID( mUdf(int), mUdf(int) );
    nringather_ = 1;
}


void SeisCBVSPS3DWriter::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( CBVSSeisTrcTranslator::sKeyDataStorage );
    if ( res && *res )
	reqdtype_ = (DataCharacteristics::UserType)(*res-'0');
}


bool SeisCBVSPS3DWriter::newInl( const SeisTrc& trc )
{
    if ( mIsUdf(prevbid_.inl) )
    {
	if ( reqdtype_ == DataCharacteristics::Auto )
	    dc_ = trc.data().getInterpreter()->dataChar();
	else
	    dc_ = DataCharacteristics( reqdtype_ );
    }

    const BinID& trcbid = trc.info().binid;
    BufferString fnm = trcbid.inl; fnm += ext();
    FilePath fp( dirnm_ ); fp.add( fnm );
    fnm = fp.fullPath();

    if ( tr_ ) delete tr_;
    tr_ = CBVSSeisTrcTranslator::getInstance();
    if ( !tr_->initWrite(new StreamConn(fnm,Conn::Write),trc) )
    {
	errmsg_ = "Trying to start writing to '";
	errmsg_ += fnm; errmsg_ += "':\n";
	errmsg_ += tr_->errMsg();
	return false;
    }
    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci= tr_->componentInfo();
    for ( int idx=0; idx<ci.size(); idx++ )
	ci[idx]->datachar = dc_;

    return true;
}


bool SeisCBVSPS3DWriter::put( const SeisTrc& trc )
{
    SeisTrcInfo& ti = const_cast<SeisTrcInfo&>( trc.info() );
    const BinID trcbid = ti.binid;
    if ( trcbid.inl != prevbid_.inl )
    {
	if ( !newInl(trc) )
	    return false;
    }
    if ( trcbid.crl != prevbid_.crl )
	nringather_ = 1;
    prevbid_ = trcbid;

    ti.binid = BinID( trcbid.crl, nringather_ );
    bool res = tr_->write( trc );
    ti.binid = trcbid;
    if ( !res )
	errmsg_ = tr_->errMsg();
    else
	nringather_++;

    return res;
}


bool SeisCBVSPS3DWriter::setSampleNames( const BufferStringSet& nms ) const
{
    FilePath fp( dirnm_ ); fp.add( "samplenames.txt" );
    const BufferString fnm( fp.fullPath() );
    if ( nms.isEmpty() )
    {
	if ( File_exists(fnm) )
	    File_remove( fnm, NO );
	return true;
    }

    StreamData sd( StreamProvider(fp.fullPath()).makeOStream() );
    if ( !sd.usable() ) return false;

    *sd.ostrm << nms.get(0);
    for ( int idx=1; idx<nms.size(); idx++ )
	*sd.ostrm << '\n' << nms.get(idx);
    sd.close();

    return true;
}
