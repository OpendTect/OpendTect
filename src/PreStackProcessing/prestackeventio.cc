/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2007
-*/


#include "prestackeventio.h"

#include "ascstream.h"
#include "binidvalset.h"
#include "color.h"
#include "datachar.h"
#include "datainterp.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "ioobj.h"
#include "keystrs.h"
#include "prestackevents.h"
#include "prestackeventtransl.h"
#include "rowcol.h"
#include "safefileio.h"
#include "samplingdata.h"
#include "separstr.h"
#include "survinfo.h"
#include "streamconn.h"
#include "strmoper.h"
#include "offsetazimuth.h"
#include "uistrings.h"

#define mHeaderEventSize (sizeof(int)+sizeof(int)+sizeof(int))


namespace PreStack
{

//Internal classes

class EventPatchFileHeader
{ mODTextTranslationClass(EventPatchFileHeader);
public:
			EventPatchFileHeader();
			~EventPatchFileHeader();

    void		setInterpreters(const DataInterpreter<int>*,
					const DataInterpreter<short>*);

    bool		fromStream(od_istream&);
    bool		toStream(od_ostream&,bool binary);

    int			nrEvents() const;
    void		setNrEvents(int);

    BinID		getBinID(int) const;
    void		setBinID(int,const BinID&);

    od_stream::Pos	getFileOffset(int) const;
    void		setFileOffset(int,od_stream::Pos);

    uiString		errMsg() const	{ return errmsg_; }

protected:

    int					nrevents_;
    char*				buffptr_;

    uiString				errmsg_;

    const DataInterpreter<int>*		int32interpreter_;
    const DataInterpreter<short>*	int16interpreter_;
};


class EventPatchReader : public SequentialTask
{ mODTextTranslationClass(EventPatchReader);
public:
			EventPatchReader(Conn*,EventManager*);
			~EventPatchReader();

    void		getPositions(BinIDValueSet& bidset) const;
    const TrcKeySampling&	getRange() const	{ return filebbox_; }

    void		setSelection(const BinIDValueSet*);
    void		setSelection(const TrcKeySampling*);

    bool		hasDataInRange() const;
    uiString		errMsg() const;

    int			nextStep() override;

protected:

    bool	isWanted(const BinID&) const;
    int	readInt16(od_istream&) const;
    int	readInt32(od_istream&) const;
    int	readUInt8(od_istream&) const;
    float	readFloat(od_istream&) const;

    DataInterpreter<int>*	int32interpreter_;
    DataInterpreter<int>*	int16interpreter_;
    DataInterpreter<float>*	floatinterpreter_;

    EventPatchFileHeader	fileheader_;
    int				headeridx_;

    EventManager*		eventmanager_;
    Conn*			conn_;
    TrcKeySampling			filebbox_;
    const TrcKeySampling*		horsel_;
    const BinIDValueSet*	bidvalsel_;
    uiString			errmsg_;

    bool			readeventtype_;
    bool			readdip_;
};


class EventPatchWriter : public SequentialTask
{ mODTextTranslationClass(EventPatchWriter);
public:
		EventPatchWriter(const char* filename, EventManager&);
		~EventPatchWriter();

    void	setReader(EventPatchReader*);

    void	setSelection(const TrcKeySampling&);
    void	setBinary(bool yn)			{ binary_ = yn; }
    uiString	errMsg() const;

    int		nextStep() override;

protected:

    bool	writeInt16(od_ostream&,int val,const char*) const;
    bool	writeInt32(od_ostream&,int val,const char*) const;
    bool	writeUInt8(od_ostream&,int val,const char*) const;
    bool	writeFloat(od_ostream&,float val,const char*) const;

    EventPatchReader*		reader_;
    EventPatchFileHeader	fileheader_;
    int				headeridx_;

    bool			binary_;
    EventManager&		eventmanager_;
    BufferString		filename_;
    SafeFileIO			fileio_;
    TrcKeySampling			horsel_;
    od_stream::Pos		fileheaderoffset_;
    uiString			errmsg_;
};


//EventReader
EventReader::EventReader( IOObj* ioobj, EventManager* events, bool trigger )
    : Executor( "Reading Prestack events" )
    , eventmanager_( events )
    , ioobj_( ioobj )
    , bidsel_( 0 )
    , horsel_( 0 )
    , trigger_( trigger )
{
    if ( eventmanager_ ) eventmanager_->blockChange( true, true );
}


EventReader::~EventReader()
{
    if ( eventmanager_ ) eventmanager_->blockChange( false, trigger_ );
    delete ioobj_;
    deepErase( patchreaders_ );
}


bool EventReader::getPositions( BinIDValueSet& bidset ) const
{
    for ( int idx=patchreaders_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( const EventPatchReader*, reader, patchreaders_[idx] );
	reader->getPositions( bidset );
    }

    return true;
}


bool EventReader::getBoundingBox( Interval<int>& inlrg,
				  Interval<int>& crlrg ) const
{
    for ( int idx=0; idx<patchreaders_.size(); idx++ )
    {
	mDynamicCastGet( const EventPatchReader*, reader, patchreaders_[idx] );
	const TrcKeySampling& hrg = reader->getRange();
	if ( !idx )
	{
	    inlrg.start = hrg.start_.inl();
	    inlrg.stop = hrg.stop_.inl();
	    crlrg.start = hrg.start_.crl();
	    crlrg.stop = hrg.stop_.crl();
	}
	else
	{
	    inlrg.include( hrg.start_.inl() );
	    inlrg.include( hrg.stop_.inl() );
	    crlrg.include( hrg.start_.crl() );
	    crlrg.include( hrg.stop_.crl() );
	}
    }

    return patchreaders_.size();
}


int EventReader::encodeEventType( VSEvent::Type et )
{
    if ( et==VSEvent::Min )
	return 1;
    if ( et==VSEvent::Max )
	return 2;

    return 0;
}


VSEvent::Type EventReader::decodeEventType( int et )
{
    if ( et==1 )
	return VSEvent::Min;
    if ( et==2 )
	return VSEvent::Max;

    return VSEvent::None;
}


void EventReader::setSelection( const BinIDValueSet* bivs )
{ bidsel_ = bivs; }


void EventReader::setSelection( const TrcKeySampling* hs )
{ horsel_ = hs; }


uiString EventReader::errMsg() const
{ return errmsg_; }


int EventReader::nextStep()
{
    if ( !eventmanager_ ) return Finished();

    if ( !patchreaders_.size() )
    {
	if ( !prepareWork() )
	{
	    //ErrMsg set in prepareWork
	    return ErrorOccurred();
	}

	const BufferString fnm( ioobj_->fullUserExpr(true) );
	if ( !readAuxData( fnm.buf() ) )
	{
	    if ( errmsg_.isEmpty() )
		errmsg_ = tr("Error: Cannot read horizon information");
	    return ErrorOccurred();
	}

	return patchreaders_.size() ? MoreToDo() : Finished();
    }

    const int res = patchreaders_[0]->doStep();
    if ( res<0 )
    {
	errmsg_ = patchreaders_[0]->errMsg();
	return ErrorOccurred();
    }

    if ( !res )
    {
	delete patchreaders_.removeSingle( 0 );
	if ( patchreaders_.size() )
	    return MoreToDo();

	return	Finished();
    }

    return MoreToDo();
}


bool EventReader::readSamplingData( const IOObj& ioobj,
		SamplingData<int>& inlsampling, SamplingData<int>& crlsampling )
{
    const BufferString fnm( ioobj.fullUserExpr(true) );
    if ( !File::isDirectory(fnm.buf()) )
	return false;

    FilePath horidfnm;
    horidfnm.setFileName( EventReader::sAuxDataFileName() );
    horidfnm.setPath( fnm );

    IOPar par;
    if ( par.read( horidfnm.fullPath().buf(),
		   EventReader::sHorizonFileType(), true ) )
    {
	if ( par.get( sKeyISamp(), inlsampling.start, inlsampling.step ) &&
	     par.get( sKeyCSamp(), crlsampling.start, crlsampling.step ) )
	return true;
    }

    IOPar& pars = ioobj.pars();

    return pars.get(sKeyISamp(), inlsampling.start, inlsampling.step ) &&
	   pars.get(sKeyCSamp(), crlsampling.start, crlsampling.step );
}



bool EventReader::prepareWork()
{
    if ( !ioobj_ ) return false;

    const BufferString fnm( ioobj_->fullUserExpr(true) );
    if ( !File::isDirectory(fnm.buf()) )
    {
	errmsg_ = tr("Error: %1 is not a (valid) folder.").arg(fnm);
	return false;
    }

    SamplingData<int> inlsampling;
    SamplingData<int> crlsampling;
    if ( !readSamplingData( *ioobj_, inlsampling, crlsampling ) )
    {
	errmsg_ = tr("Error: Cannot read sampling");
	return false;
    }

    BufferString mask = "*.";
    mask += PSEventTranslatorGroup::sDefExtension();

    const DirList dirlist( fnm.buf(), File::FilesInDir, mask.buf() );
    for ( int idx=0; idx<dirlist.size(); idx++ )
    {
	if ( File::isEmpty( dirlist.fullPath(idx) ) )
	   continue;

       const SeparString sepstr( dirlist[idx]->buf(), '_' );
       TrcKeySampling filehrg;
       if ( !getFromString( filehrg.start_.inl(), sepstr[0], -1 ) ||
	    !getFromString( filehrg.stop_.inl(), sepstr[1], -1 ) ||
	    !getFromString( filehrg.start_.crl(), sepstr[2], -1 ) ||
	    !getFromString( filehrg.stop_.crl(), sepstr[3], -1 ) )
	    continue;

      if ( inlsampling.snap( filehrg.start_.inl() )!=filehrg.start_.inl() ||
	   crlsampling.snap( filehrg.start_.crl() )!=filehrg.start_.crl() )
	    continue;

	bool usefile = true;
	if ( bidsel_ || horsel_ )
	{
	    usefile = false;

	    TrcKeySampling dummy;
	    if ( horsel_ && horsel_->getInterSection(filehrg,dummy))
		usefile = true;

	    if ( !usefile && bidsel_ )
	    {
		BinIDValueSet::SPos pos;
		while ( bidsel_->next(pos,true) )
		{
		    if ( filehrg.includes( bidsel_->getBinID(pos) ))
		    {
			usefile = true;
			break;
		    }
		}
	    }
	}

	if ( !usefile )
	    continue;

	if ( !addReader( dirlist.fullPath(idx) ) )
	    return false;
    }

    return true;
}



bool EventReader::addReader( const char* fnm )
{
    Conn* conn = new StreamConn( fnm, Conn::Read );

    EventPatchReader* reader = new EventPatchReader( conn, eventmanager_ );

    if (reader->errMsg().isSet())
    {
	errmsg_ = tr("Cannot add patch reader. %1").arg(reader->errMsg());
	delete reader;
	return false;
    }

    reader->setSelection( bidsel_ );
    reader->setSelection( horsel_ );

    if ( !reader->hasDataInRange() )
    {
	delete reader;
    }
    else
    {
	patchreaders_ += reader;
    }

    return true;
}


bool EventReader::readAuxData(const char* fnm)
{
    if ( !eventmanager_ )
	return false;

    if ( eventmanager_->getHorizonIDs().size() )
	return true; //The version in mem is newer - keep it

    FilePath horidfnm;
    horidfnm.setPath( fnm );
    horidfnm.add( EventReader::sAuxDataFileName() );

    BufferString auxfilenm = horidfnm.fullPath();

    if ( !File::exists( auxfilenm ) )
	horidfnm.setFileName( sOldHorizonFileName() );

    auxfilenm = horidfnm.fullPath();

    SafeFileIO fileio( auxfilenm );
    if ( !fileio.open( true ) )
    {
	errmsg_ = uiStrings::phrCannotOpen(toUiString(auxfilenm));
	return false;
    }

    IOPar par;
    if ( !par.read( fileio.istrm(), EventReader::sHorizonFileType(), true ) )
    {
	errmsg_ = uiStrings::phrCannotRead(toUiString(auxfilenm));
	fileio.closeFail();
	return false;
    }

    fileio.closeSuccess();

    int nrhors, nexthor;
    if ( !par.get( EventReader::sKeyNrHorizons(), nrhors ) ||
	 !par.get( EventReader::sKeyNextHorizonID(), nexthor ) )
    {
	errmsg_ = uiStrings::phrCannotRead(toUiString(auxfilenm));
	return false;
    }

    NotifyStopper stopper( eventmanager_->change );
    for ( int idx=0; idx<nrhors; idx++ )
    {
	const BufferString key( "", idx );
	int id;
	PtrMan<IOPar> horpar = par.subselect( key.buf() );
	if ( !horpar || !horpar->get( EventReader::sKeyHorizonID(), id ) )
	{
	    errmsg_ = uiStrings::phrCannotRead(tr("horizon %1").arg(key.buf()));
	    return false;
	}

	eventmanager_->addHorizon( id );

	MultiID emref;
	if ( horpar->get( EventReader::sKeyHorizonRef(), emref ) )
	    eventmanager_->setHorizonEMReference( id, emref );
    }

    OD::Color col = OD::Color(255,0,0); //Todo: Make mandatory to have color
    par.get( sKey::Color(), col );
    eventmanager_->setColor( col );

    eventmanager_->setNextHorizonID( nexthor );
    eventmanager_->resetChangedFlag( true );

    EventManager::DipSource primarydipsource;
    EventManager::DipSource secondarydipsource;
    BufferString dipsourcestr;
    if ( par.get( sKeyPrimaryDipSource(), dipsourcestr ) )
    {
	if ( !primarydipsource.use(dipsourcestr.buf()) )
	{
	    errmsg_ = tr("Cannot parse primary dip-source");
	    return false;
	}

	if ( !par.get(sKeySecondaryDipSource(),dipsourcestr) ||
	     !secondarydipsource.use(dipsourcestr.buf()) )
	{
	    errmsg_ = tr("Cannot parse secondary dip-source");
	    return false;
	}
    }

    eventmanager_->setDipSource( primarydipsource, true );
    eventmanager_->setDipSource( secondarydipsource, false );

    return true;
}


//EventWriter
EventWriter::EventWriter( IOObj* ioobj, EventManager& events )
    : Executor( "Writing Prestack events" )
    , eventmanager_( events )
    , ioobj_( ioobj )
{
    eventmanager_.blockChange( true, true );
}


EventWriter::~EventWriter()
{
    eventmanager_.blockChange( false, false );
    delete ioobj_;
    deepErase( patchwriters_ );
}


int EventWriter::nextStep()
{
    if ( !patchwriters_.size() )
    {
	SamplingData<int> inlsampling;
	SamplingData<int> crlsampling;
	if ( !EventReader::readSamplingData(*ioobj_,inlsampling, crlsampling ) )
	{
	    const StepInterval<int> inlrg = SI().inlRange(false);
	    const StepInterval<int> crlrg = SI().crlRange(false);
	    inlsampling.start = inlrg.start; inlsampling.step = inlrg.step*25;
	    crlsampling.start = crlrg.start; crlsampling.step = crlrg.step*25;
	}

	auxinfo_.set( EventReader::sKeyISamp(), inlsampling.start,
		      inlsampling.step );
	auxinfo_.set( EventReader::sKeyCSamp(), crlsampling.start,
		      crlsampling.step );

	const BufferString fnm( ioobj_->fullUserExpr(true) );
	if ( !File::exists( fnm.buf() ) || !File::isDirectory( fnm.buf() ) )
	{
	    if ( !File::isDirectory( fnm.buf() ) )
	    {
		if ( !File::remove( fnm.buf() ) )
		{
		    errmsg_ = tr("Cannot remove %1").arg(fnm);
		    return ErrorOccurred();
		}
	    }

	    if ( !File::createDir(fnm.buf()) )
	    {
		errmsg_ = uiStrings::phrCannotCreateDirectory(toUiString(fnm));
		return ErrorOccurred();
	    }
	}

	eventmanager_.cleanUp( true );

	if ( !writeAuxData( fnm.buf() ) )
	    return ErrorOccurred();

	const MultiDimStorage<EventSet*>& evstor = eventmanager_.getStorage();
	int pos[] = { -1, -1 };
	TypeSet<RowCol> rcols;
	while ( evstor.next( pos, true ) )
	{
	    BinID bid;
	    evstor.getPos( pos, bid );
	    EventSet* ge = evstor.getRef( pos, 0 );

	    if ( !ge->ischanged_ )
		continue;

	    const RowCol rc( (bid.inl()-inlsampling.start)/inlsampling.step,
			     (bid.crl()-crlsampling.start)/crlsampling.step );

	    if ( !rcols.isPresent( rc ) )
		rcols += rc;
	}

	if ( !rcols.size() )
	    return Finished();

	TrcKeySampling hrg( true );

	for ( int idx=0; idx<rcols.size(); idx++ )
	{
	    const RowCol& rc( rcols[idx] );
	    hrg.start_.inl() = inlsampling.atIndex( rc.row() );
	    hrg.stop_.inl() = inlsampling.atIndex(rc.row()+1) - hrg.step_.inl();
	    hrg.start_.crl() = crlsampling.atIndex( rc.col() );
	    hrg.stop_.crl() = crlsampling.atIndex(rc.col()+1) - hrg.step_.crl();

	    SeparString filenamebase( 0, '_' );
	    filenamebase += hrg.start_.inl();
	    filenamebase += hrg.stop_.inl();
	    filenamebase += hrg.start_.crl();
	    filenamebase += hrg.stop_.crl();


	    FilePath filename;
	    filename.setPath( fnm.buf() );
	    filename.add( filenamebase.buf() );
	    filename.setExtension( PSEventTranslatorGroup::sDefExtension() );

	    const BufferString patchfnm = filename.fullPath().buf();

	    EventPatchWriter* writer =
		new EventPatchWriter( patchfnm.buf(), eventmanager_);

	    if ( !File::isEmpty(patchfnm.buf()) )
	    {
		StreamConn* conn = new StreamConn( patchfnm.buf(), Conn::Read );
		if ( conn && conn->forRead() )
		{
		    EventPatchReader* reader =
				new EventPatchReader( conn, &eventmanager_ );

		    if (reader->errMsg().isSet())
		    {
			errmsg_ = reader->errMsg();
			delete reader;
			return ErrorOccurred();
		    }

		    writer->setReader( reader );
		}
		else
		    delete conn;
	    }

	    writer->setSelection( hrg );
	    patchwriters_ += writer;
	}

	return patchwriters_.size() ? MoreToDo() : Finished();
    }

    const int res = patchwriters_[0]->doStep();
    if ( res<0 )
    {
	errmsg_ = patchwriters_[0]->errMsg();
	return ErrorOccurred();
    }

    if ( !res )
    {
	delete patchwriters_.removeSingle( 0 );
	if ( patchwriters_.size() )
	    return MoreToDo();

	eventmanager_.resetChangedFlag( false );
	return	Finished();
    }

    return MoreToDo();
}


uiString EventWriter::errMsg() const
{ return errmsg_; }


bool EventWriter::writeAuxData( const char* fnm )
{
    auxinfo_.set( EventReader::sKeyNrHorizons(),
	     eventmanager_.getHorizonIDs().size());
    auxinfo_.set( EventReader::sKeyNextHorizonID(),
	     eventmanager_.nextHorizonID(false) );

    for ( int idx=eventmanager_.getHorizonIDs().size()-1; idx>=0; idx-- )
    {
	IOPar horpar;
	const int id =  eventmanager_.getHorizonIDs()[idx];
	horpar.set( EventReader::sKeyHorizonID(), id );

	const BufferString key( "", idx );
	auxinfo_.mergeComp( horpar, key.buf() );
    }

    BufferString dipsource;
    eventmanager_.getDipSource( true ).fill( dipsource );
    if ( !dipsource.isEmpty() )
	auxinfo_.set( EventReader::sKeyPrimaryDipSource(), dipsource.buf() );

    eventmanager_.getDipSource( false ).fill( dipsource );
    if ( !dipsource.isEmpty() )
	auxinfo_.set( EventReader::sKeySecondaryDipSource(), dipsource.buf() );

    FilePath horidfnm;
    horidfnm.setPath( fnm );
    horidfnm.add( EventReader::sAuxDataFileName() );

    auxinfo_.set( sKey::Color(), eventmanager_.getColor() );

    SafeFileIO fileio( horidfnm.fullPath().buf(), false );
    if ( !fileio.open( false ) )
    {
	errmsg_ = tr("Cannot open %1 for writing")
		.arg(horidfnm.fullPath().buf());

	return false;
    }

    if ( !auxinfo_.write( fileio.ostrm(),
			  EventReader::sHorizonFileType() ) )
    {
	errmsg_ = tr("Cannot write to %1")
		.arg(horidfnm.fullPath().buf());
	fileio.closeFail();
	return false;
    }

    if ( !fileio.closeSuccess() )
    {
	errmsg_ = tr("Cannot close %1")
		.arg(horidfnm.fullPath().buf());

	return false;
    }

    return true;
}


//Duplicator


EventDuplicator::EventDuplicator( IOObj* from, IOObj* to )
    : Executor( "Copying Prestack events" )
    , from_( from )
    , to_( to )
    , totalnr_( -1 )
{
    const BufferString fromnm( from_->fullUserExpr(true) );
    if ( !File::isDirectory(fromnm.buf()) )
    {
	errmsg_ = tr("Input %1 is not a folder.")
		.arg(fromnm.buf());
	return;
    }

    const BufferString tonm( to_->fullUserExpr(true) );
    if ( File::exists( tonm.buf() ) )
    {
	File::remove( tonm.buf() );
	if ( File::exists( tonm.buf() ) )
	{
	    errmsg_ = tr("Cannot overwrite %1.").arg(toUiString(tonm));
	    return;
	}
    }

    if ( !File::createDir( tonm.buf() ) )
    {
	errmsg_ = uiStrings::phrCannotCreate(toUiString(tonm));
	return;
    }

    const DirList dirlist( fromnm.buf(), File::FilesInDir, "*" );
    for ( int idx=0; idx<dirlist.size(); idx++ )
	filestocopy_.add( dirlist.fullPath( idx ) );

    totalnr_ = filestocopy_.size();

    return;
}



EventDuplicator::~EventDuplicator()
{
    if ( filestocopy_.size() ) //premature exit
	errorCleanup();

    delete from_;
    delete to_;
}


int EventDuplicator::nextStep()
{
    if (errMsg().isSet()) //Catch error in prepareWork
	return ErrorOccurred();

    if ( !filestocopy_.size() )
	return Finished();

    const int idx = filestocopy_.size()-1;

    const BufferString tonm( to_->fullUserExpr(true) );
    if ( !File::isDirectory(tonm.buf()) )
    {
	errmsg_ = tr("%1 is not a (valid) folder")
		.arg(tonm.buf());

	return ErrorOccurred();
    }

    FilePath targetfile( filestocopy_[idx]->buf() );
    targetfile.setPath( tonm.buf() );

    message_ = tr( "Copying %1.").arg( targetfile.fileName() );

    if ( !File::copy( filestocopy_[idx]->buf(), targetfile.fullPath().buf() ) )
    {
	errmsg_ = tr("Cannot copy %1 to %2")
		.arg(filestocopy_[idx]->buf())
		.arg(targetfile.fullPath().buf());
	errorCleanup();
	return ErrorOccurred();
    }

    filestocopy_.removeSingle( idx );
    return MoreToDo();
}


void EventDuplicator::errorCleanup()
{

    const BufferString tonm( to_->fullUserExpr(true) );
    if ( File::exists( tonm.buf() ) )
	File::remove( tonm.buf() );
}


//EventPatchFileHeader


EventPatchFileHeader::EventPatchFileHeader()
    : buffptr_( 0 )
    , int32interpreter_( 0 )
    , int16interpreter_( 0 )
    , nrevents_( 0 )
    , errmsg_( uiString::emptyString() )
{}


EventPatchFileHeader::~EventPatchFileHeader()
{ delete [] buffptr_; }


bool EventPatchFileHeader::fromStream( od_istream& strm )
{
    int nrevents;
    if ( int16interpreter_ )
    {
	const int sz = int32interpreter_->nrBytes();
	mAllocLargeVarLenArr( char, buf, sz );
	if ( !strm.getBin(buf,sz) )
	{
	    errmsg_ = tr("Cannot read #events from stream (bin)");
	    return false;
	}

	nrevents = int16interpreter_->get(buf,0);
    }
    else
    {
	strm >> nrevents;
	if ( !strm.isOK() )
	{
	    errmsg_ = tr("Cannot read #events from stream (asc)");
	    return false;
	}
    }

    setNrEvents( nrevents );

#   define mErrRetNoEvents(s) \
    { \
	nrevents_ = 0; \
	delete [] buffptr_; buffptr_ = 0; \
	errmsg_ = s; \
	return false; \
    }

    if ( int16interpreter_ )
    {
	const int chunksize = nrevents_*mHeaderEventSize;
	if ( !strm.getBin( buffptr_, chunksize ) )
	    mErrRetNoEvents(tr("Cannot read header (bin)"))
	return true;
    }

    for ( int idx=0; idx<nrevents_; idx++ )
    {
	BinID bid; od_stream::Pos offset;
	strm >> bid.inl() >> bid.crl() >> offset;
	if ( !strm.isOK() )
	    mErrRetNoEvents(tr("Cannot read header (asc)"))

	setBinID( idx, bid );
	setFileOffset( idx, offset );
    }

    return true;
}


#define mWriteFixedCharVal( val, post ) \
{ \
    unsigned int uval = abs(val); \
    BufferString valstr( toString(uval) ); \
    const int len = valstr.size(); \
    int totlen = 11; \
    if ( val<0 ) \
	{ strm << '-'; totlen = 10; } \
    for ( int idy=totlen; idy>len; idy-- ) \
	strm << '0'; \
    strm << valstr.buf() << post; \
}


bool EventPatchFileHeader::toStream( od_ostream& strm, bool binary )
{
    if ( binary )
    {
	strm.addBin( &nrevents_, sizeof(nrevents_) );
	strm.addBin( buffptr_, nrevents_*mHeaderEventSize );
    }
    else
    {
	mWriteFixedCharVal( nrevents_, '\n' );
	for ( int idx=0; idx<nrevents_; idx++ )
	{
	    mWriteFixedCharVal( getBinID(idx).inl(), '\t' );
	    mWriteFixedCharVal( getBinID(idx).crl(), '\t' );
	    mWriteFixedCharVal( (int) getFileOffset(idx), '\n' );
	}
    }

    if ( strm.isBad() )
    {
	errmsg_ = tr("Cannot write header"); return false;
    }

    return true;
}


int EventPatchFileHeader::nrEvents() const
{ return nrevents_; }


void EventPatchFileHeader::setNrEvents( int nr )
{
    delete [] buffptr_;
    nrevents_ = nr;
    buffptr_ = 0;

    const int chunksize = nrevents_*mHeaderEventSize;
    buffptr_ = new char[chunksize];
    OD::memZero( buffptr_, chunksize );
}


BinID EventPatchFileHeader::getBinID( int idx ) const
{
    const int offset = idx * mHeaderEventSize;
    const char* baseptr = buffptr_+offset;
    if ( int32interpreter_ )
	return BinID( int32interpreter_->get( baseptr, 0 ),
		      int32interpreter_->get( baseptr, 1 ) );

    return BinID( ((int*) baseptr)[0], ((int*) baseptr)[1] );
}


void EventPatchFileHeader::setBinID( int idx, const BinID& bid )
{
    const int offset = idx * mHeaderEventSize;
    char* baseptr = buffptr_+offset;
    if ( int32interpreter_ )
    {
	int32interpreter_->put( baseptr, 0,  bid.inl() );
	int32interpreter_->put( baseptr, 1,  bid.crl() );
    }
    else
    {
	((int*) baseptr)[0] = bid.inl();
	((int*) baseptr)[1] = bid.crl();
    }
}


od_stream::Pos EventPatchFileHeader::getFileOffset( int idx ) const
{
    const int offset = idx * mHeaderEventSize;
    const char* baseptr = buffptr_+offset+sizeof(int)*2;
    return int32interpreter_ ? int32interpreter_->get( baseptr, 0 )
			     : *((int*) baseptr);
}


void EventPatchFileHeader::setFileOffset( int idx, od_stream::Pos offsetval )
{
    const int offset = idx * mHeaderEventSize;
    char* baseptr = buffptr_+offset+sizeof(int)*2;
    if ( int32interpreter_ )
	int32interpreter_->put( baseptr, 0, (int) offsetval );
    else
	*((int*)baseptr) = (int)offsetval;
}


EventPatchReader::EventPatchReader( Conn* conn, EventManager* events )
    : conn_( conn )
    , eventmanager_( events )
    , horsel_( 0 )
    , bidvalsel_( 0 )
    , headeridx_( 0 )
    , int32interpreter_( 0 )
    , int16interpreter_( 0 )
    , floatinterpreter_( 0 )
    , readeventtype_( false )
    , readdip_( true )
{
    if ( !conn_ || !conn_->forRead() )
    { errmsg_ = tr("Bad input file: cannot read events"); return; }

    StreamConn& sconn = *static_cast<StreamConn*>( conn_ );
    od_istream& strm = sconn.iStream();
    ascistream astream( strm );
    if ( !astream.isOfFileType( EventReader::sFileType() ) )
    {
	errmsg_ = tr("Invalid filetype: %1").arg(strm.fileName());
	return;
    }


    astream.next();
    IOPar par( astream );

    BufferString dcs;
    if ( par.get( EventReader::sKeyInt16DataChar(), dcs ) )
    {
	DataCharacteristics dc; dc.set( dcs.buf() );
	int16interpreter_ = new DataInterpreter<int>( dc );
    }

    if ( par.get( EventReader::sKeyInt32DataChar(), dcs ) )
    {
	DataCharacteristics dc; dc.set( dcs.buf() );
	int32interpreter_ = new DataInterpreter<int>( dc );
    }

    if ( par.get( EventReader::sKeyFloatDataChar(), dcs ) )
    {
	DataCharacteristics dc; dc.set( dcs.buf() );
	floatinterpreter_ = new DataInterpreter<float>( dc );
    }

    filebbox_.usePar( par );

    int version = 1;
    par.get( EventReader::sHorizonFileVersion(), version );
    if ( version==2 )
    {
	readeventtype_ = true;
	readdip_ = false;
    }

    if ( !fileheader_.fromStream(strm) )
    {
	errmsg_ = tr("Could not read file header from %1. %2")
	    .arg(strm.fileName()).arg(fileheader_.errMsg());
    }
}


EventPatchReader::~EventPatchReader()
{ delete conn_; }


void EventPatchReader::setSelection(
	const BinIDValueSet* bidvalset )
{ bidvalsel_ = bidvalset; }



void EventPatchReader::setSelection( const TrcKeySampling* hs )
{ horsel_ = hs; }


bool EventPatchReader::hasDataInRange() const
{
    if ( !horsel_ && !bidvalsel_ )
	return true;

    if ( horsel_ )
    {
	TrcKeySampling dummy;
	if ( horsel_->getInterSection( filebbox_, dummy ) )
	    return true;
    }

    if ( bidvalsel_ )
    {
	for ( int idx=fileheader_.nrEvents()-1; idx>=0; idx-- )
	{
	    if ( bidvalsel_->includes( fileheader_.getBinID(idx) ) )
		return true;
	}
    }

    return false;
}


void EventPatchReader::getPositions( BinIDValueSet& bidset ) const
{
    for ( int idx=fileheader_.nrEvents()-1; idx>=0; idx-- )
	bidset.add( fileheader_.getBinID(idx) );
}


uiString EventPatchReader::errMsg() const
{ return errmsg_; }


int EventPatchReader::nextStep()
{
    if ( !eventmanager_ ) return Finished();

    od_istream& strm = ((StreamConn*)conn_)->iStream();

    BinID curbid;
    while ( headeridx_<fileheader_.nrEvents() )
    {
	curbid = fileheader_.getBinID(headeridx_);
	if ( !isWanted(curbid) )
	    { headeridx_++; continue; }

	RefMan<EventSet> ge =
	    eventmanager_->getEvents( curbid, false, false );
	if ( ge && ge->ischanged_ )
	    { headeridx_++; continue; }

	break;
    }

    if ( headeridx_>=fileheader_.nrEvents() )
	return Finished();

    strm.setReadPosition( fileheader_.getFileOffset(headeridx_) );
    const int nrevents = readInt16( strm );

#   define mErrRetCantReadAt(s,more) \
    { \
	errmsg_ = tr("Could not read %1 from %2 at %3") \
		.arg(s).arg(strm.fileName()) \
		.arg(curbid.toString()); \
	more; \
	return ErrorOccurred(); \
    }

    if ( !strm.isOK() )
	mErrRetCantReadAt(tr("nr events"), )

    EventSet* ge = eventmanager_->getEvents( curbid, false, true );
    ge->ref();
    deepErase( ge->events_ );

    for ( int idx=0; idx<nrevents; idx++ )
    {
	const int nrpicks = readUInt8( strm );
	if ( strm.isBad() )
	{
	    deepErase( ge->events_ );
	    ge->unRef();
	    mErrRetCantReadAt(tr("nr picks.\nEvent nr=%1").arg(idx), )
	}

	Event* pse = new Event( nrpicks, true );

	pse->horid_ = mCast( short, readInt32( strm ) );
	pse->quality_ = mCast( unsigned char, readUInt8( strm ) );
	if ( readeventtype_ )
	{
	    const int eventtype = readUInt8( strm );
	    pse->eventtype_ = EventReader::decodeEventType(eventtype);
	}
	else pse->eventtype_ = VSEvent::None;

	if ( readdip_ )
	{
	    readFloat( strm );
	    readFloat( strm );
	}

	for ( int idy=0; idy<nrpicks; idy++ )
	{
	    pse->pick_[idy] = readFloat( strm );
	    pse->pickquality_[idy] = mCast( unsigned char, readUInt8( strm ) );
	    pse->offsetazimuth_[idy].setFrom( readInt32( strm ) );
	}

	if ( strm.isBad() )
	{
	    deepErase( ge->events_ );
	    ge->unRef();
	    mErrRetCantReadAt(tr("event.\nEvent nr=%1").arg(idx), )
	}

	ge->events_ += pse;
    }

    ge->unRefNoDelete();

    headeridx_++;
    return MoreToDo();
}


bool EventPatchReader::isWanted( const BinID& bid ) const
{
    if ( !filebbox_.includes( bid ) )
	return false;

    if ( !horsel_ && !bidvalsel_ )
	return true;

    if ( horsel_ && horsel_->includes( bid ) )
	return true;

    return bidvalsel_->includes( bid );
}


int EventPatchReader::readInt16( od_istream& strm ) const
{
    if ( int16interpreter_ )
    {
	const int sz = int16interpreter_->nrBytes();
	mAllocLargeVarLenArr( char, buf, sz );
	strm.getBin(buf,sz);
	return int16interpreter_->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}


int EventPatchReader::readUInt8( od_istream& strm ) const
{
    if ( int16interpreter_ )
    {
	char res;
	strm.getBin(&res,1);
	return (unsigned char) res;
    }

    int res;
    strm >> res;
    return res;
}


int EventPatchReader::readInt32( od_istream& strm ) const
{
    if ( int32interpreter_ )
    {
	const int sz = int16interpreter_->nrBytes();
	mAllocLargeVarLenArr( char, buf, sz );
	strm.getBin(buf,sz);
	return int32interpreter_->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}


float EventPatchReader::readFloat( od_istream& strm ) const
{
    if ( floatinterpreter_ )
    {
	const int sz = floatinterpreter_->nrBytes();
	mAllocLargeVarLenArr( char, buf, sz );
	strm.getBin(buf,sz);
	return floatinterpreter_->get(buf,0);
    }

    float res;
    strm >> res;
    return res;
}



EventPatchWriter::EventPatchWriter( const char* filename, EventManager& ev )
    : eventmanager_( ev )
    , headeridx_( 0 )
    , horsel_( true )
    , reader_( 0 )
    , binary_( false )
    , fileio_( filename )
{
    fileio_.usebakwhenmissing_ = false;
    fileio_.removebakonsuccess_ = true;
}


EventPatchWriter::~EventPatchWriter()
{
    delete reader_;
}


void EventPatchWriter::setReader( EventPatchReader* reader )
{
    delete reader_;
    reader_ = reader;
    reader_->setSelection( &horsel_ );
}


void EventPatchWriter::setSelection( const TrcKeySampling& hrg )
{ horsel_ = hrg; }


uiString EventPatchWriter::errMsg() const
{
    return errmsg_;
}

#define mSetDc( type, string ) \
{ \
    type dummy; \
    DataCharacteristics(dummy).toString( dc ); \
}\
    par.set( string, dc.buf() )



int EventPatchWriter::nextStep()
{
    if ( reader_ )
    {
	const int res = reader_->nextStep();
	if ( res == 0 )
	    { delete reader_; reader_ = 0; return MoreToDo(); }
	return res;
    }

    if ( !headeridx_ )
    {
	const MultiDimStorage<EventSet*>& evstor = eventmanager_.getStorage();
	int pos[] = { -1, -1 };

	TypeSet<BinID> bids;
	TrcKeySampling hrg( true );
	while ( evstor.next( pos, true ) )
	{
	    BinID bid;
	    evstor.getPos( pos, bid );
	    if ( !horsel_.includes( bid ) )
		continue;

	    RefMan<EventSet> pses = eventmanager_.getEvents(bid,false,false);

	    bool isempty = true;
	    for ( int idx=pses->events_.size()-1; idx>=0; idx-- )
	    {
		if ( pses->events_[idx]->sz_ )
		{
		    isempty = false;
		    break;
		}
	    }

	    if ( isempty )
		continue;

	    if ( bids.size() ) hrg.include( bid );
	    else { hrg.start_ = hrg.stop_ = bid; }
	    bids += bid;
	}

	if ( !bids.size() )
	{
	    if ( File::exists( fileio_.fileName() ) &&
		 File::isWritable( fileio_.fileName() ) )
	    {
		return File::remove( fileio_.fileName() )
		    ? Finished()
		    : ErrorOccurred();
	    }
	}

	fileheader_.setNrEvents( bids.size() );
	for ( int idx=bids.size()-1; idx>=0; idx-- )
	    fileheader_.setBinID( idx, bids[idx] );

	IOPar par;
	if ( binary_ )
	{
	    BufferString dc;
	    mSetDc( od_int32, EventReader::sKeyInt32DataChar() );
	    mSetDc( short, EventReader::sKeyInt16DataChar() );
	    mSetDc( float, EventReader::sKeyFloatDataChar() );
	}

	par.set( EventReader::sHorizonFileVersion(), 2 );

	hrg.fillPar( par );

	if ( !fileio_.open( false ) )
	{
	    errmsg_ = tr("Cannot open file %1").arg(fileio_.fileName());
	    return ErrorOccurred();
	}

	ascostream astream( fileio_.ostrm() );
	astream.putHeader( EventReader::sFileType() );
	par.putTo( astream );
	od_ostream& ostrm = astream.stream();
	fileheaderoffset_ = ostrm.position();
	if ( !fileheader_.toStream( ostrm, binary_ ) )
	{
	    errmsg_ = tr("Cannot write file header in %1")
		    .arg(fileio_.fileName());
	    fileio_.closeFail();
	    return ErrorOccurred();
	}
    }

    od_ostream& strm = fileio_.ostrm();
    if ( headeridx_>=fileheader_.nrEvents() )
    {
	strm.setWritePosition( fileheaderoffset_ );
	fileheader_.toStream( strm, binary_ );
	if ( !fileio_.closeSuccess() )
	{
	    errmsg_ = tr("Cannot close file %1").arg(fileio_.fileName());
	    return ErrorOccurred();
	}

	return Finished();
    }

    od_stream::Pos curoffset = strm.position();
    const BinID bid = fileheader_.getBinID( headeridx_ );
    fileheader_.setFileOffset( headeridx_, curoffset );

    RefMan<EventSet> pses = eventmanager_.getEvents( bid, false, false);

    TypeSet<int> eventnrtosave;
    for ( int idx=pses->events_.size()-1; idx>=0; idx-- )
    {
	if ( pses->events_[idx]->sz_ )
	    eventnrtosave += idx;
    }

    const int nrevents = eventnrtosave.size();
    writeInt16( strm, nrevents, "\t" );
    for ( int idx=0; idx<eventnrtosave.size(); idx++ )
    {
	const Event* pse = pses->events_[eventnrtosave[idx]];
	const int nrpicks = pse->sz_;
	writeInt16( strm, nrpicks, "\t" );
	writeInt32( strm, pse->horid_, "\t" );
	writeUInt8( strm, pse->quality_, "\t" );
	writeUInt8( strm, EventReader::encodeEventType(pse->eventtype_),
		    "\n\t\t" );

	for ( int idy=0; idy<nrpicks; idy++ )
	{
	    writeFloat( strm, pse->pick_[idy], "\t" );
	    writeUInt8( strm, pse->pickquality_
			      ? pse->pickquality_[idy] : 255, "\t" );
	    writeInt32( strm , pse->offsetazimuth_[idy].asInt(), "\n" );
	    if ( !binary_ )
	    {
		if ( idy!=nrpicks-1 )
		    strm << "\t\t";
		else if ( idx!=nrevents-1 )
		    strm << '\t';
	    }
	}
    }

    if ( !strm.isOK() )
    {
	errmsg_ = tr("Error during write to %1").arg(fileio_.fileName());
	fileio_.closeFail();
	return ErrorOccurred();
    }

    headeridx_++;
    return MoreToDo();
}


bool EventPatchWriter::writeInt16( od_ostream& strm, int val,
						 const char* post ) const
{
    if ( binary_ )
	strm.addBin( &val, sizeof(val) );
    else
	strm << val << post;

    return strm.isOK();
}


bool EventPatchWriter::writeInt32( od_ostream& strm, int val,
						 const char* post ) const
{
    if ( binary_ )
	strm.addBin( &val, sizeof(val) );
    else
	strm << val << post;

    return strm.isOK();
}


bool EventPatchWriter::writeUInt8( od_ostream& strm, int val,
						 const char* post ) const
{
    if ( binary_ )
	strm.addBin( &val, sizeof(val) );
    else
	strm << val << post;

    return strm.isOK();
}


bool EventPatchWriter::writeFloat( od_ostream& strm, float val,
						 const char* post ) const
{
    if ( binary_ )
	strm.addBin( &val, sizeof(val) );
    else
	strm << val << post;

    return strm.isOK();
}


} // namespace PreStack
