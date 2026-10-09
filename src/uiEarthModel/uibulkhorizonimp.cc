/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibulkhorizonimp.h"

#include "binidvalset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "file.h"
#include "ioman.h"
#include "paralleltask.h"
#include "posinfodetector.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"
#include "task.h"

#include <algorithm>
#include <istream>
#include <streambuf>

#include "uifiledlg.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"


class BulkHorizonAscIO : public Table::AscIO
{
public:
BulkHorizonAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
    , finishedreadingheader_(false)
{}


static Table::FormatDesc* getDesc( const ZDomain::Def& def )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkHorizon" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			    StringInpSpec(sKey::FloatUdf()), Table::Required );
    createDescBody( fd, def );
    return fd;
}


static void createDescBody( Table::FormatDesc* fd, const ZDomain::Def& def )
{
    fd->bodyinfos_ += new Table::TargetInfo( "Horizon name", Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    auto* ti = new Table::TargetInfo( def.key(), FloatInpSpec(),
							Table::Required );
    const Mnemonic::StdType type = def.isTime() ? Mnemonic::Time
						: Mnemonic::Dist;
    ti->setPropertyType( type );
    fd->bodyinfos_ += ti;
}


static void updateDesc( Table::FormatDesc& fd, const ZDomain::Def& def )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, def );
}


bool isXY() const
{ return formOf( false, 1 ) == 0; }


bool readHeader()
{
    if ( !getHdrVals(strm_) )
	return false;

    udfval_ = getFValue( 0 );
    finishedreadingheader_ = true;
    return true;
}


void prepareForBody( float udfval )
{
    const Table::TargetInfo* sepinfo = fd_.headerinfos_.isEmpty()
				? nullptr : fd_.headerinfos_.first();
    if ( sepinfo && sepinfo->name()=="Field separator" )
    {
	const BufferString val = sepinfo->selection_.getVal( 0 );
	iscsv_ = val.startsWith( "Com" );
    }

    udfval_ = udfval;
    finishedreadingheader_ = true;
    hdrread_ = true;
}


float udfValue() const
{ return udfval_; }


bool getData( BufferString& hornm, Coord3& crd )
{
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return false;

	udfval_ = getFValue( 0 );
	finishedreadingheader_ = true;
    }


    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    hornm = getText( 0 );
    crd = getPos3D( 1, 2, 3, udfval_ );
    return true;
}

    od_istream&		strm_;
    float		udfval_;
    bool		finishedreadingheader_;
};


uiBulkHorizonImport::uiBulkHorizonImport( uiParent* p )
    : uiDialog(p,Setup(uiStrings::phrImport(
		       tr("Multiple Horizons")),
		       mODHelpKey(mBulkHorizonImportHelpID)).modal(false))
    , fd_(BulkHorizonAscIO::getDesc(SI().zDomain()))
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::ViewStyle::Table );

    zdomselfld_ = new uiGenInput( this, tr("Horizon is in"),
	BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth()) );
    zdomselfld_->attach( alignedBelow, inpfld_ );
    zdomselfld_->setValue( SI().zIsTime() );
    mAttachCB( zdomselfld_->valueChanged, uiBulkHorizonImport::zDomainCB );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, zdomselfld_ );
}


uiBulkHorizonImport::~uiBulkHorizonImport()
{
    delete fd_;
}


void uiBulkHorizonImport::zDomainCB( CallBacker* cb )
{
    BufferStringSet attrnms;
    const bool istime = zdomselfld_->getBoolValue();
    BulkHorizonAscIO::updateDesc( *fd_, istime ? ZDomain::Time()
					       : ZDomain::Depth() );
}


namespace
{

class CharRangeBuf : public std::streambuf
{
public:
CharRangeBuf( const char* start, od_int64 len )
{
    char* buf = const_cast<char*>( start );
    setg( buf, buf, buf + len );
}

protected:
std::streampos seekoff( std::streamoff off, std::ios_base::seekdir way,
			std::ios_base::openmode which ) override
{
    if ( (which & std::ios_base::in) == 0 )
	return std::streampos(-1);

    char* ptr = nullptr;
    if ( way == std::ios_base::beg )
	ptr = eback() + off;
    else if ( way == std::ios_base::cur )
	ptr = gptr() + off;
    else
	ptr = egptr() + off;

    if ( ptr < eback() || ptr > egptr() )
	return std::streampos(-1);

    setg( eback(), ptr, egptr() );
    return std::streampos( ptr - eback() );
}

std::streampos seekpos( std::streampos pos,
			std::ios_base::openmode which ) override
{
    return seekoff( off_type(pos), std::ios_base::beg, which );
}

};


class CharRangeStream : public std::istream
{
public:
CharRangeStream( const char* start, od_int64 len )
    : std::istream(nullptr)
    , buf_(start, len)
{
    rdbuf( &buf_ );
}

private:
    CharRangeBuf	buf_;
};


class BulkChunk
{
public:
    BufferStringSet	names;
    TypeSet<int>	inls;
    TypeSet<int>	crls;
    TypeSet<float>	zval;
    TypeSet<od_int64>	seq;
    bool		failed	= false;
};


class BulkLineStarts : public ParallelTask
{
public:
BulkLineStarts( const char* text, od_int64 bodystart, od_int64 filesz,
		TypeSet<od_int64>& starts )
    : ParallelTask()
    , text_(text)
    , bodystart_(bodystart)
    , filesz_(filesz)
    , starts_(starts)
{}

od_int64 nrIterations() const override
{ return filesz_ - bodystart_; }

int minThreadSize() const override
{ return 256 * 1024; }

bool doPrepare( int nrthreads ) override
{
    deepErase( perthread_ );
    for ( int idx=0; idx<nrthreads; idx++ )
	perthread_ += new TypeSet<od_int64>;

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    TypeSet<od_int64>& out = *perthread_[threadidx];
    const od_int64 absstart = bodystart_ + start;
    const od_int64 absstop = bodystart_ + stop;
    od_int64 ich = absstart;
    if ( absstart > bodystart_ && text_[absstart-1] != '\n' )
    {
	while ( ich<=absstop && ich<filesz_ && text_[ich] != '\n' )
	    ich++;

	ich++;
    }

    while ( ich<=absstop && ich<filesz_ )
    {
	out += ich;
	while ( ich<=absstop && ich<filesz_ && text_[ich] != '\n' )
	    ich++;

	ich++;
    }

    return true;
}

bool doFinish( bool success ) override
{
    if ( !success )
	return false;

    for ( int idx=0; idx<perthread_.size(); idx++ )
	starts_.append( *perthread_[idx] );

    return true;
}

private:
    const char*				text_;
    od_int64				bodystart_;
    od_int64				filesz_;
    TypeSet<od_int64>&			starts_;
    ObjectSet<TypeSet<od_int64>>	perthread_;
};


class BulkBodyParser : public ParallelTask
{
public:
BulkBodyParser( const char* text, od_int64 filesz,
		const TypeSet<od_int64>& lineoffs,
		const Table::FormatDesc& fd, float udfval, bool isxy )
    : ParallelTask()
    , text_(text)
    , filesz_(filesz)
    , lineoffs_(lineoffs)
    , fd_(fd)
    , udfval_(udfval)
    , isxy_(isxy)
{}

~BulkBodyParser()
{ deepErase( chunks_ ); }

od_int64 nrIterations() const override
{ return lineoffs_.size(); }

int minThreadSize() const override
{ return 1000; }

bool doPrepare( int nrthreads ) override
{
    deepErase( chunks_ );
    for ( int idx=0; idx<nrthreads; idx++ )
	chunks_ += new BulkChunk;

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    BulkChunk& chunk = *chunks_[threadidx];
    const od_int64 from = lineoffs_[(int)start];
    const od_int64 to = stop+1 < lineoffs_.size()
			? lineoffs_[(int)stop+1] : filesz_;
    CharRangeStream crs( text_+from, to-from );
    od_istream istrm( crs );
    BulkHorizonAscIO aio( fd_, istrm );
    aio.prepareForBody( udfval_ );

    BufferString hornm;
    Coord3 crd;
    for ( od_int64 iline=start; iline<=stop; iline++ )
    {
	if ( (iline-start)%4096==0 && !shouldContinue() )
	    return false;

	if ( !aio.getData(hornm, crd) )
	    break;

	if ( hornm.isEmpty() || !crd.isDefined() )
	    continue;

	BinID bid;
	if ( isxy_ )
	    bid = SI().transform( crd.coord() );
	else
	{
	    bid.inl() = mNINT32( crd.x_ );
	    bid.crl() = mNINT32( crd.y_ );
	}

	chunk.names.add( hornm );
	chunk.inls += bid.inl();
	chunk.crls += bid.crl();
	chunk.zval += (float)crd.z_;
	chunk.seq += iline;
    }

    return !chunk.failed;
}

    ObjectSet<BulkChunk>	chunks_;

private:
    const char*			text_;
    od_int64			filesz_;
    const TypeSet<od_int64>&	lineoffs_;
    const Table::FormatDesc&	fd_;
    float			udfval_;
    bool			isxy_;
};

} // namespace


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiBulkHorizonImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("the input file name")) )

    uiRetVal ret;
    const ZDomain::Info& zdominfo = Table::AscIO::zDomain( *fd_, 2, ret );
    if ( ret.isError() )
    {
	uiMSG().error( ret.messages().cat() );
	return false;
    }

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpen(uiStrings::sInputFile().toLower()))

    if ( !dataselfld_->commit() )
	return false;

    const od_int64 filesz = File::getFileSize( fnm );
    if ( filesz < 0 )
	mErrRet(uiStrings::phrCannotRead(uiStrings::sInputFile().toLower()))

    mDeclareAndTryAlloc( char*, text, char[filesz+1] );
    if ( !text )
	mErrRet( uiStrings::phrCannotAllocateMemory() )

    if ( filesz>0 && !strm.getBin(text, filesz) )
    {
	delete [] text;
	mErrRet(uiStrings::phrCannotRead(uiStrings::sInputFile().toLower()))
    }

    text[filesz] = '\0';
    CharRangeStream crs( text, filesz );
    od_istream memstrm( crs );
    BulkHorizonAscIO headerio( *fd_, memstrm );
    if ( !headerio.readHeader() )
    {
	delete [] text;
	mErrRet(uiStrings::phrCannotRead(uiStrings::sInputFile().toLower()))
    }

    od_int64 bodystart = memstrm.position();
    if ( bodystart<0 || bodystart>filesz )
	bodystart = 0;

    TypeSet<od_int64> lineoffs;
    if ( bodystart < filesz )
    {
	BulkLineStarts indexer( text, bodystart, filesz, lineoffs );
	if ( !indexer.execute() )
	{
	    delete [] text;
	    mErrRet(uiStrings::phrCannotRead(uiStrings::sInputFile().toLower()))
	}
    }

    ObjectSet<BinIDValueSet> data;
    BufferStringSet hornms;
    if ( !lineoffs.isEmpty() )
    {
	BulkBodyParser parser( text, filesz, lineoffs, *fd_,
			       headerio.udfValue(), headerio.isXY() );
	if ( !parser.execute() )
	{
	    delete [] text;
	    mErrRet(uiStrings::phrCannotRead(uiStrings::sInputFile().toLower()))
	}

	struct CommitKey
	{
	    bool operator==( const CommitKey& o ) const
	    { return chunk==o.chunk && idx==o.idx; }

	    int		chunk;
	    int		idx;
	};
	TypeSet<CommitKey> keys;
	for ( int ich=0; ich<parser.chunks_.size(); ich++ )
	{
	    const BulkChunk& chunk = *parser.chunks_[ich];
	    for ( int irow=0; irow<chunk.inls.size(); irow++ )
	    {
		CommitKey key;
		key.chunk = ich;
		key.idx = irow;
		keys += key;
	    }
	}

	if ( !keys.isEmpty() )
	{
	    std::sort( keys.arr(), keys.arr()+keys.size(),
		[&parser]( const CommitKey& a, const CommitKey& b )
		{
		    const BulkChunk& ca = *parser.chunks_[a.chunk];
		    const BulkChunk& cb = *parser.chunks_[b.chunk];
		    const BufferString& na = ca.names.get( a.idx );
		    const BufferString& nb = cb.names.get( b.idx );
		    if ( na != nb )
			return na < nb;

		    if ( ca.inls[a.idx] != cb.inls[b.idx] )
			return ca.inls[a.idx] < cb.inls[b.idx];

		    if ( ca.crls[a.idx] != cb.crls[b.idx] )
			return ca.crls[a.idx] < cb.crls[b.idx];

		    return ca.seq[a.idx] < cb.seq[b.idx];
		} );
	}

	BufferString curname;
	BinIDValueSet* cur = nullptr;
	for ( int ikey=0; ikey<keys.size(); ikey++ )
	{
	    const CommitKey& key = keys[ikey];
	    const BulkChunk& chunk = *parser.chunks_[key.chunk];
	    const BufferString& nm = chunk.names.get( key.idx );
	    if ( !cur || nm != curname )
	    {
		cur = new BinIDValueSet( 1, false );
		data += cur;
		hornms.add( nm );
		curname = nm;
	    }

	    const float z = chunk.zval[key.idx];
	    cur->add( BinID(chunk.inls[key.idx], chunk.crls[key.idx]), z );
	}
    }

    delete [] text;

    // TODO: Check if name exists, ask user to overwrite or give new name
    BufferStringSet errors;
    uiTaskRunner taskr( this );
    const BufferString savernm = "Saving Horizons";
    TaskGroup saver;
    saver.setName( savernm );
    TypeSet<MultiID> mids;
    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	RefMan<EM::Horizon3D> hor3d = EM::Horizon3D::create( hornms.get(idx) );
	if ( !hor3d )
	{
	    pErrMsg( "Huh?" );
	    continue;
	}

	BinIDValueSet* bidvs = data[idx];
	BinID bid;
	BinIDValueSet::SPos pos;
	PosInfo::Detector detector( PosInfo::Detector::Setup(false) );
	while ( bidvs->next(pos) )
	{
	    bid = bidvs->getBinID( pos );
	    detector.add( SI().transform(bid), bid );
	}

	detector.finish();
	TrcKeySampling hs;
	detector.getTrcKeySampling( hs );
	ObjectSet<BinIDValueSet> curdata; curdata += bidvs;
	PtrMan<Task> importer = hor3d->importer( curdata, hs );
	if ( !importer || !TaskRunner::execute( &taskr, *importer ) )
	    continue;

	hor3d->setZDomain( zdominfo );
	saver.addTask( hor3d->saver() );
	mids.add( hor3d->multiID() );
    }

    deepErase( data );

    if ( TaskRunner::execute( &taskr, saver ) )
    {
	for ( const auto& mid : mids )
	{
	    PtrMan<IOObj> ioobj = IOM().get( mid );
	    if ( !ioobj )
	    {
		pErrMsg("Should not enter here.");
		continue;
	    }

	    zdominfo.fillPar( ioobj->pars() );
	    IOM().commitChanges( *ioobj );
	}

	const uiString msg = tr("%1 successfully imported."
		      "\n\nDo you want to import more %1?")
		      .arg( uiStrings::sHorizon(mPlural) );
	const bool retval = uiMSG().askGoOn( msg, uiStrings::sYes(),
					     uiStrings::sNoCloseWindow() );
	return !retval;
    }
    else
    {
	saver.uiMessage();
	return false;
    }
}
