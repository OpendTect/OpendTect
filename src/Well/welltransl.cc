/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltransl.h"

#include "file.h"
#include "filepath.h"
#include "hdf5arraynd.h"
#include "ioman.h"
#include "ioobj.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odjson.h"
#include "strmprov.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welldisp.h"
#include "wellioprov.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellodreader.h"
#include "wellodwriter.h"
#include "wellreadaccess.h"
#include "wellwriteaccess.h"
#include "welltrack.h"


// odWellDataIOProvider

class odWellDataIOProvider : public WellDataIOProvider
{
public:
			odWellDataIOProvider();
			~odWellDataIOProvider();

private:

    Well::ReadAccess*	makeReadAccess(const IOObj&,Well::Data&,
				       uiString& errmsg) const override;
    Well::WriteAccess*	makeWriteAccess(const IOObj&,const Well::Data&,
					uiString& errmsg) const override;

public:
    static void		initClass();

};


odWellDataIOProvider::odWellDataIOProvider()
    : WellDataIOProvider(odWellTranslator::translKey())
{}


odWellDataIOProvider::~odWellDataIOProvider()
{}


Well::ReadAccess* odWellDataIOProvider::makeReadAccess( const IOObj& ioobj,
				Well::Data& wd, uiString& emsg ) const
{
    return new Well::odReader( ioobj, wd, emsg );
}


Well::WriteAccess* odWellDataIOProvider::makeWriteAccess( const IOObj& ioobj,
				const Well::Data& wd, uiString& emsg ) const
{
    return new Well::odWriter( ioobj, wd, emsg );
}


void odWellDataIOProvider::initClass()
{
    WDIOPF().add( new odWellDataIOProvider );
}


// WellTranslatorGroup

defineTranslatorGroup(Well,"Well");

uiString WellTranslatorGroup::sTypeName( int num )
{ return uiStrings::sWell( num ); }


// WellTranslator

mDefSimpleTranslatorSelector(Well);
mDefSimpleTranslatorioContext(Well,WllInf)


bool WellTranslator::implRename_( const IOObj* ioobj,
				  const char* /* newnm */ ) const
{
    if ( !ioobj )
	return false;

    if ( Well::MGR().isLoaded(ioobj->key()) )
    {
	RefMan<Well::Data> wd = Well::MGR().get( ioobj->key() );
	if ( wd )
	    wd->setName( ioobj->name() );
    }

    return true;
}


// odWellTranslator

const WellDataIOProvider& odWellTranslator::getProv() const
{
    return *WDIOPF().provider( userName() );
}

#define mImplStart(fn) \
    if ( !ioobj || ioobj->translator()!=translKey() ) \
	return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
\
    BufferString pathnm = iostrm->fileSpec().fullDirName(); \
    BufferString filenm = iostrm->fileSpec().fileName(); \
    StreamProvider prov( filenm ); \
    prov.addPathIfNecessary( pathnm ); \
    if ( !prov.fn ) return false;

#define mRemove(ext,nr,extra) \
{ \
    StreamProvider sp( Well::odIO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    const bool exists = sp.exists( true ); \
    if ( exists && !sp.remove(false) ) \
	return false; \
    extra; \
}

bool odWellTranslator::implRemove( const IOObj* ioobj, bool ) const
{
    mImplStart(remove(false));

    FilePath fp( filenm );
    fp.setExtension( nullptr );
    const BufferString bnm = fp.fullPath();
    mRemove(Well::odIO::sExtMarkers(),0,)
    mRemove(Well::odIO::sExtD2T(),0,)
    mRemove(Well::odIO::sExtCSMdl(),0,)
    mRemove(Well::odIO::sExtDispProps(),0,)
    mRemove(Well::odIO::sExtDefaults(),0,)
    mRemove(Well::odIO::sExtWellTieSetup(),0,)
    for ( int idx=1; ; idx++ )
	mRemove(Well::odIO::sExtLog(),idx,if ( !exists ) break)

    return true;
}


#define mRename(ext,nr,extra) \
{ \
    StreamProvider sp( Well::odIO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    StreamProvider spnew( Well::odIO::mkFileName(newbnm,ext,nr) ); \
    spnew.addPathIfNecessary( pathnm ); \
    const bool exists = sp.exists( true ); \
    if ( exists && !sp.rename(spnew.fileName()) ) \
	return false; \
    extra; \
}

bool odWellTranslator::implRename( const IOObj* ioobj, const char* newnm ) const
{
    mImplStart(rename(newnm));

    FilePath fp( filenm );
    fp.setExtension( nullptr );
    const BufferString bnm = fp.fullPath();
    fp.set( newnm );
    fp.setExtension( nullptr );
    const BufferString newbnm = fp.fullPath();
    mRename(Well::odIO::sExtMarkers(),0,)
    mRename(Well::odIO::sExtD2T(),0,)
    mRename(Well::odIO::sExtCSMdl(),0,)
    mRename(Well::odIO::sExtDispProps(),0,)
    mRename(Well::odIO::sExtDefaults(),0,)
    mRename(Well::odIO::sExtWellTieSetup(),0,)
    for ( int idx=1; ; idx++ )
	mRename(Well::odIO::sExtLog(),idx,if ( !exists ) break)

    return WellTranslator::implRename_( ioobj, newnm );
}


bool odWellTranslator::implSetReadOnly( const IOObj* ioobj, bool ro ) const
{
    mImplStart(setReadOnly(ro));
    return true;
}


odWellTranslator* odWellTranslator::getInstance()
{
    return new odWellTranslator( "od", translKey() );
}


const char* odWellTranslator::translKey()
{
    return "dGB";
}


void odWellTranslator::initClass()
{
    auto* tr = new odWellTranslator( "od", translKey() );
    WellTranslatorGroup::theInst().add( tr );
    odWellDataIOProvider::initClass();
}



namespace Well
{

class HDF5Writer;

/*!\brief stuff common to HDF5 well reader and writer  */

mExpClass(Well) HDF5Access
{
public:

    static const char*		sTrackGrpName();
    static const char*		sLogsGrpName();
    static const char*		sMarkersGrpName();
    static const char*		sTDsGrpName();
    static const char*		sCSsGrpName();
    static const char*		sDispParsGrpName();

    static const char*		sCoordsDSName();
    static const char*		sMDsDSName();
    static const char*		sTWTsDSName();
    static const char*		sValuesDSName();
    static const char*		sNamesDSName();
    static const char*		sColorsDSName();
    static const char*		sLvlIDsDSName();

    static const char*		sKeyLogDel();
};


/*!\brief Reads Well::Data from HDF5 file  */

class HDF5Reader : public ReadAccess
		 , public HDF5Access
{ mODTextTranslationClass(Well::HDF5Reader)
public:

			HDF5Reader(const IOObj&,Data&,uiString& errmsg);
			HDF5Reader(const char* fnm,Data&,uiString& errmsg);
			HDF5Reader(const HDF5Writer&,Data&,uiString& errmsg);
			~HDF5Reader();

private:

    bool		getInfo() const override;
    bool		getTrack() const override;
    bool		getLogs(bool needjustinfo) const override;
    bool		getMarkers() const override;

    bool		getD2T() const override;
    bool		getCSMdl() const override;

    bool		getLog(const char* lognm) const override;
    void		getLogInfo(BufferStringSet&) const override;
    bool		getDefLogs() const override;
    bool		getDispProps() const override;

    const		uiString& errMsg() const override    { return errmsg_; }

    void		init(const char*);
    bool		ensureFileOpen() const;
    bool		doGetD2T(bool) const;
    bool		getLogPars(const HDF5::DataSetKey&,IOPar&) const;
    Log*		getWL(const HDF5::DataSetKey&) const;

    bool		get() const override		{ return true; }

    uiString&		errmsg_;
    PtrMan<HDF5::Reader> rdr_;
    mutable IOPar	infoiop_;

};


/*!\brief Writes Well::Data to HDF5 file */

class HDF5Writer : public WriteAccess
		 , public HDF5Access
{ mODTextTranslationClass(Well::HDF5Writer)
public:

			HDF5Writer(const IOObj&,const Data&,uiString& errmsg);
			HDF5Writer(const char* fnm,const Data&,uiString&);
			~HDF5Writer();

    HDF5::Reader*	createCoupledHDFReader() const;
    static bool		useHDF5(const IOObj&,uiString&);

private:

    bool		put() const override;
    bool		putInfoAndTrack() const override;
    bool		putInfo() const;
    bool		putTrack() const;
    bool		putLogs() const override;
    bool		putDefLogs() const override;
    bool		putMarkers() const override;
    bool		putD2T() const override;
    bool		putCSMdl() const override;
    bool		putDispProps() const override;

    const uiString&	errMsg() const override		{ return errmsg_; }

    PtrMan<HDF5::Writer> wrr_;
    BufferString	filename_;
    uiString&		errmsg_;

    void		init(const char*,bool* nmchg=nullptr);
    bool		initGroups();
    void		putDepthUnit(IOPar&) const;
    bool		doPutD2T(bool) const;
    bool		ensureFileOpen() const;
    void		ensureCorrectDSSize(const HDF5::DataSetKey&,int,int,
					    uiRetVal&) const;
    int			getLogIndex(const char* lognm ) const;
    bool		putLog(const Log&) const override;
    bool		setLogAttribs(const HDF5::DataSetKey&,const Log*) const;

    bool		isFunctional() const override;

};

} // namespace Well


/*!\brief WellTranslator for 'HDF5' stored wells, OD's newest well format. */

class hdfWellTranslator : public WellTranslator
{			  isTranslator(hdf,Well)
public:
			mDefEmptyTranslatorConstructor(hdf,Well)

private:

    const WellDataIOProvider&	getProv() const override;

    const char*		iconName() const override;
    const char*		defExtension() const override;
    bool		isUserSelectable(bool forread) const override;

    bool		implRename(const IOObj*,const char*) const override;

};


// hdfWellDataIOProvider

class hdfWellDataIOProvider : public WellDataIOProvider
{
public:
			hdfWellDataIOProvider();
			~hdfWellDataIOProvider();

private:

    Well::ReadAccess*	makeReadAccess(const IOObj&,Well::Data&,
				       uiString& errmsg) const override;
    Well::WriteAccess*	makeWriteAccess(const IOObj&,const Well::Data&,
					uiString& errmsg) const override;

public:
    static void		initClass();

};


hdfWellDataIOProvider::hdfWellDataIOProvider()
    : WellDataIOProvider(hdfWellTranslator::translKey())
{}


hdfWellDataIOProvider::~hdfWellDataIOProvider()
{}


Well::ReadAccess* hdfWellDataIOProvider::makeReadAccess( const IOObj& ioobj,
				Well::Data& wd, uiString& emsg ) const
{
    return new Well::HDF5Reader( ioobj, wd, emsg );
}


Well::WriteAccess* hdfWellDataIOProvider::makeWriteAccess( const IOObj& ioobj,
				const Well::Data& wd, uiString& emsg ) const
{
    return nullptr;
//    return new Well::HDF5Writer( ioobj, wd, emsg );
}


void hdfWellDataIOProvider::initClass()
{
    WDIOPF().add( new hdfWellDataIOProvider );
}


const WellDataIOProvider& hdfWellTranslator::getProv() const
{
    return *WDIOPF().provider( userName() );
}


const char* hdfWellTranslator::iconName() const
{
    return HDF5::Access::sIconName();
}


const char* hdfWellTranslator::defExtension() const
{
    return HDF5::Access::sFileExtension();
}


bool hdfWellTranslator::isUserSelectable( bool forread ) const
{
    return forread ? HDF5::isEnabled( nullptr ) : false;
}


bool hdfWellTranslator::implRename( const IOObj* ioobj,
				    const char* newnm ) const
{
    if ( !ioobj || ioobj->translator()!=translKey() )
	return false;

    if ( !WellTranslator::implRename_(ioobj,newnm) )
	return false;

    PtrMan<HDF5::Writer> wrr = HDF5::mkWriter();
    if ( !wrr )
    {
	WellTranslator::implRename_( ioobj, newnm );
	return false;
    }

    const uiRetVal uirv = wrr->open4Edit( newnm );
    if ( uirv.isOK() )
	wrr->setAttribute( "Well name", ioobj->name().str() );

    return WellTranslator::implRename_( ioobj, newnm );
}


hdfWellTranslator* hdfWellTranslator::getInstance()
{
    return new hdfWellTranslator( "hdf", translKey() );
}


const char* hdfWellTranslator::translKey()
{
    return "HDF";
}


void hdfWellTranslator::initClass()
{
    auto* tr = new hdfWellTranslator( "hdf", translKey() );
    WellTranslatorGroup::theInst().add( tr );
    hdfWellDataIOProvider::initClass();
}


void inithdfWellTranslator()
{
    hdfWellTranslator::initClass();
}


// Groups
const char* Well::HDF5Access::sTrackGrpName()	{ return "Track"; }
const char* Well::HDF5Access::sLogsGrpName()	{ return "Logs"; }
const char* Well::HDF5Access::sMarkersGrpName() { return "Markers"; }
const char* Well::HDF5Access::sTDsGrpName()	{ return "Time-Depth Models"; }
const char* Well::HDF5Access::sCSsGrpName()	{ return "Checkshots"; }
const char* Well::HDF5Access::sDispParsGrpName(){ return "Display Parameters"; }

// Datasets
const char* Well::HDF5Access::sCoordsDSName()	{ return "Coordinates"; }
const char* Well::HDF5Access::sMDsDSName()	{ return "MDs"; }
const char* Well::HDF5Access::sTWTsDSName()	{ return "TWTs"; }
const char* Well::HDF5Access::sValuesDSName()	{ return "Values"; }
const char* Well::HDF5Access::sNamesDSName()	{ return "Names"; }
const char* Well::HDF5Access::sColorsDSName()	{ return "Colors"; }
const char* Well::HDF5Access::sLvlIDsDSName()	{ return "LevelIDs"; }
const char* Well::HDF5Access::sKeyLogDel()	{ return "Deleted"; }

#define mErrRetIfUiRvNotOK(dsky) \
    if ( !uirv.isOK() ) \
	{ errmsg_.set( uirv ); return false; }


Well::HDF5Reader::HDF5Reader( const char* fnm, Well::Data& wd,
			      uiString& errmsg )
    : Well::ReadAccess(wd)
    , errmsg_(errmsg)
{
    FilePath fp( fnm );
    fp.setExtension( nullptr );
    wd_.info().setName( fp.fileName() );
    init( fnm );
}


Well::HDF5Reader::HDF5Reader( const IOObj& ioobj, Well::Data& wd,
			      uiString& errmsg )
    : Well::ReadAccess(wd)
    , errmsg_(errmsg)
{
    wd_.info().setName( ioobj.name() );
    wd_.setMultiID( ioobj.key() );
    init( ioobj.mainFileName() );
}


Well::HDF5Reader::HDF5Reader( const HDF5Writer& wrr, Well::Data& wd,
			      uiString& errmsg )
    : Well::ReadAccess(wd)
    , errmsg_(errmsg)
    , rdr_(wrr.createCoupledHDFReader())
{
    if ( !rdr_ )
    {
	pErrMsg("No coupled reader created");
	return;
    }
}


Well::HDF5Reader::~HDF5Reader()
{
}


#define mErrRet( s )	{ errmsg_ = s; return; }

void Well::HDF5Reader::init( const char* fnm )
{
    if ( !HDF5::isAvailable() )
	mErrRet( HDF5::Access::sHDF5NotAvailable(fnm) )
    if ( !File::exists(fnm) )
	mErrRet( uiStrings::phrFileDoesNotExist(fnm) )
    if ( !HDF5::isHDF5File(fnm) )
	mErrRet( HDF5::Access::sNotHDF5File(fnm) )

    rdr_ = HDF5::mkReader();
    if ( !rdr_ )
    {
	pErrMsg("Available but no reader?");
	return;
    }

    uiRetVal uirv = rdr_->open( fnm );
    if ( !uirv.isOK() )
    {
	rdr_ = nullptr;
	mErrRet( uirv )
    }
}


bool Well::HDF5Reader::ensureFileOpen() const
{
    if ( rdr_ && rdr_->isOpen() )
	return true;

    if ( errmsg_.isEmpty() )
	errmsg_.set( HDF5::Access::sHDF5FileNoLongerAccessible() );

    return false;
}


bool Well::HDF5Reader::getInfo() const
{
    if ( !ensureFileOpen() )
	return false;

    infoiop_.setEmpty();
    uiRetVal uirv = rdr_->get( infoiop_ );
    mErrRetIfUiRvNotOK( uirv );

    wd_.info().usePar( infoiop_ );
    return true;
}


bool Well::HDF5Reader::getTrack() const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sTrackGrpName(), "" );
    dsky.setDataSetName( sCoordsDSName() );

    uiRetVal uirv;
    const int sz = rdr_->dimSize( dsky, 0, uirv );
    if ( sz < 1 )
	return false;

    Array2DImpl<double> arr( sz, 3 );
    HDF5::ArrayNDTool<double> arrtool( arr );
    auto& rdr = cCast(HDF5::Reader&,*rdr_);
    uirv = arrtool.getAll( dsky, rdr );
    mErrRetIfUiRvNotOK( uirv );

    dsky.setDataSetName( sMDsDSName() );
    TypeSet<double> mds;
    uirv = rdr_->get( dsky, mds );
    mErrRetIfUiRvNotOK( uirv );
    IOPar mdiop;
    uirv = rdr_->get( mdiop, &dsky );

    Well::Track& trck = wd_.track();
    trck.setEmpty();
    for ( int idx=0; idx<sz; idx++ )
    {
	const double& dah = mds[idx];
	const Coord3 crd( arr.get(idx,0), arr.get(idx,1), arr.get(idx,2) );
	trck.addPoint( crd, dah );
    }

    return true;
}


bool Well::HDF5Reader::doGetD2T( bool csmdl ) const
{
    if ( !ensureFileOpen() )
	return false;

    D2TModel* d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    if ( !d2t )
	d2t = new D2TModel;

    const int modelid = 0; // TODO: suppport multiple models
    const HDF5::DataSetKey grpky =
	HDF5::DataSetKey::groupKey( csmdl ? sCSsGrpName() : sTDsGrpName(),
				    toString(modelid) );
    HDF5::DataSetKey dsky( nullptr, sMDsDSName() );
    dsky.setGroupName( grpky.fullDataSetName() );
    if ( !rdr_->hasDataSet(dsky) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(dsky) );
	return false;
    }

    IOPar hdriop;
    uiRetVal uirv = rdr_->get( hdriop, &dsky );
    mErrRetIfUiRvNotOK( uirv );
    d2t->useHeaderPar( hdriop );

    const int sz = rdr_->dimSize( dsky, 0, uirv );
    TypeSet<double> mds;
    uirv = rdr_->get( dsky, mds );
    mErrRetIfUiRvNotOK( uirv );

    dsky.setDataSetName( sTWTsDSName() );
    TypeSet<double> times;
    uirv = rdr_->get( dsky, times );
    mErrRetIfUiRvNotOK( uirv );

    d2t->setEmpty();
    for ( int idx=0; idx<sz; idx++ )
    {
	const double& dah = mds.get( idx );
	const double& val = times.get( idx );
	d2t->insertAtDah( dah, val );
    }

    if ( !updateDTModel(d2t,csmdl,errmsg_) )
	return false;

    return true;
}


bool Well::HDF5Reader::getD2T() const
{
    return doGetD2T( false );
}


bool Well::HDF5Reader::getCSMdl() const
{
    return doGetD2T( true );
}


bool Well::HDF5Reader::getLogs( bool needjustinfo ) const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sLogsGrpName() );
    errmsg_.setEmpty();
    for ( int ilog=1; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	Log* wl = getWL( grpkey );
	addToLogSet( wl, needjustinfo );
	if ( !wl )
	    break;
    }

    return errmsg_.isEmpty() && getDefLogs();
}


bool Well::HDF5Reader::getLogPars( const HDF5::DataSetKey& dsky,
				   IOPar& iop ) const
{
    if ( !rdr_->hasGroup(dsky.fullDataSetName()) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(dsky) );
	return false;
    }

    uiRetVal uirv = rdr_->get( iop, &dsky );
    mErrRetIfUiRvNotOK( uirv )
    return !iop.isTrue( sKeyLogDel() ) && getDefLogs();
}


#define mErrRetNullIfUiRvNotOK() \
    if ( !uirv.isOK() ) \
	{ errmsg_.set( uirv ); return nullptr; }

Well::Log* Well::HDF5Reader::getWL( const HDF5::DataSetKey& dsky ) const
{
    IOPar iop;
    if ( !getLogPars(dsky,iop) )
	return nullptr;

    uiRetVal uirv;
    HDF5::DataSetKey logkey( nullptr, sMDsDSName() );
    logkey.setGroupName( dsky.fullDataSetName() );
    if ( !rdr_->hasDataSet(logkey) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(logkey) );
	return nullptr;
    }

    const int sz = rdr_->dimSize( logkey, 0, uirv );
    TypeSet<double> mds;
    uirv = rdr_->get( logkey, mds );
    mErrRetNullIfUiRvNotOK();

    logkey.setDataSetName( sValuesDSName() );
    TypeSet<double> vals;
    uirv = rdr_->get( logkey, vals );
    mErrRetNullIfUiRvNotOK();

    BufferString lognm;
    iop.get( sKey::Name(), lognm );
    auto* wl = new Log( lognm );

    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = mds.get( idx );
	const float val = vals.get( idx );
	wl->addValue( dah, val );
    }

    BufferString uomlbl;
    if ( iop.get(Log::sKeyUnitLbl(),uomlbl) )
	wl->setUnitMeasLabel( uomlbl );

    BufferString mnemlbl;
    if ( iop.get(Log::sKeyMnemLbl(),mnemlbl) )
	wl->setMnemonicLabel( mnemlbl );

    Interval<float> dahrange;
    if ( iop.get(Log::sKeyDahRange(),dahrange) )
    {
	wl->addValue( dahrange.start, mUdf(float) );
	wl->addValue( dahrange.stop, mUdf(float) );
	wl->dahRange().set( dahrange.start, dahrange.stop );
    }

    Interval<float> logrange;
    if ( iop.get(Log::sKeyLogRange(),logrange) )
	wl->setValueRange( logrange );

    iop.removeWithKey( sKey::Name() );
    iop.removeWithKey( sKey::DepthUnit() );
    iop.removeWithKey( Log::sKeyUnitLbl() );
    iop.removeWithKey( Log::sKeyMnemLbl() );
    iop.removeWithKey( Log::sKeyDahRange() );
    iop.removeWithKey( Log::sKeyLogRange() );
    bool havehdrinfo = false;
    if ( iop.getYN(Log::sKeyHdrInfo(),havehdrinfo) && havehdrinfo )
	wl->pars().merge( iop );
    // TODO_HDF5Reader
    // I think this is done, please confirm and I will remove the comment.
    // wl->setPars( iop );

    iop.removeWithKey( Log::sKeyHdrInfo() );
    return wl;
}


bool Well::HDF5Reader::getLog( const char* reqlognm ) const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sLogsGrpName() );
    BufferStringSet loggrps;
    rdr_->getSubGroups( sLogsGrpName(), loggrps );
    for ( int ilog=1; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	IOPar iop;
	if ( !getLogPars(grpkey,iop) )
	    return false;

	BufferString lognm;
	iop.get( sKey::Name(), lognm );
	if ( lognm == reqlognm )
	{
	    Log* wl = getWL( grpkey );
	    return addToLogSet( wl );
	}
    }
}


void Well::HDF5Reader::getLogInfo( BufferStringSet& nms ) const
{
    if ( !ensureFileOpen() )
	return;

    HDF5::DataSetKey dsky( sLogsGrpName() );
    for ( int ilog=1; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	IOPar iop;
	if ( !getLogPars(grpkey,iop) )
	    break;

	BufferString lognm;
	iop.get( sKey::Name(), lognm );
	nms.add( lognm );
    }

    getDefLogs();
}


bool Well::HDF5Reader::getDefLogs() const
{
    if ( !ensureFileOpen() )
	return false;

    const HDF5::DataSetKey dsky( sLogsGrpName(), nullptr );
    OD::JSON::Array jsonarr( true );
    const uiRetVal uirv = rdr_->readJSonAttribute( Well::LogSet::sKeyDefMnem(),
						   jsonarr, &dsky );
    if ( !uirv.isOK() || jsonarr.size() < 1 )
	return true; //do not return false if missing, this can happen

    IOPar defmniop;
    for ( int idx=0; idx<jsonarr.size(); idx++ )
    {
	const OD::JSON::Object& subobj = jsonarr.object( idx );
	const BufferString key = subobj.getStringValue( "mnemonic" );
	const BufferString val = subobj.getStringValue( "log" );
	defmniop.set( key.buf(), val.buf() );
    }

    IOPar defiop;
    defiop.mergeComp( defmniop, Well::LogSet::sKeyDefMnem() );
    wd_.logs().defaultLogUsePar( defiop );

    return true;
}


bool Well::HDF5Reader::getMarkers() const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sMarkersGrpName(), "" );
    MarkerSet& ms = wd_.markers();

    dsky.setDataSetName( sMDsDSName() );
    TypeSet<double> mds;
    uiRetVal uirv = rdr_->get( dsky, mds );
    mErrRetIfUiRvNotOK( uirv );
    IOPar mdiop;
    uirv = rdr_->get( mdiop, &dsky );

    dsky.setDataSetName( sNamesDSName() );
    BufferStringSet nms;
    uirv = rdr_->get( dsky, nms );
    mErrRetIfUiRvNotOK( uirv )

    dsky.setDataSetName( sColorsDSName() );
    BufferStringSet colors;
    uirv = rdr_->get( dsky, colors );
    mErrRetIfUiRvNotOK( uirv )

    dsky.setDataSetName( sLvlIDsDSName() );
    TypeSet<int> lvlids;
    uirv = rdr_->get( dsky, lvlids );
    mErrRetIfUiRvNotOK( uirv )

    ms.setEmpty();
    const int sz = mds.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = mds[idx];
	const BufferString nm( nms.validIdx(idx) ? nms.get(idx).buf()
						 : "" );
	OD::Color col( OD::Color::NoColor() );
	if ( colors.validIdx(idx) )
	    col.setStdStr( colors.get(idx) );
	const int lvlid = lvlids.validIdx(idx) ? lvlids[idx] : -1;

	auto* mrkr = new Marker( nm, dah, col );
	mrkr->setLevelID( Strat::LevelID(lvlid) );
	ms.insertNew( mrkr );
    }

    return true;
}


bool Well::HDF5Reader::getDispProps() const
{
    if ( !ensureFileOpen() )
	return false;

    const char* usernm = GetInterpreterName();
    const HDF5::DataSetKey dsky( sDispParsGrpName(), usernm );
    if ( !rdr_->hasDataSet(dsky) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(dsky) );
	return false;
    }

    IOPar iop;
    uiRetVal uirv = rdr_->get( iop, &dsky );
    mErrRetIfUiRvNotOK( uirv )
    wd_.displayProperties(false).usePar( iop );
    wd_.displayProperties(true).usePar( iop );
    return true;
}


namespace Well
{
    static const int nrrowsperblock = 20;
};


Well::HDF5Writer::HDF5Writer( const char* fnm, const Data& wd,
			      uiString& errmsg )
    : WriteAccess(wd)
    , errmsg_(errmsg)
{
    init( fnm );
}


Well::HDF5Writer::HDF5Writer( const IOObj& ioobj, const Data& wd,
			      uiString& errmsg )
    : WriteAccess(wd)
    , errmsg_(errmsg)
{
    if ( !useHDF5(ioobj,errmsg_) || !errmsg_.isEmpty() )
	return;

    bool fnmchgd = false;
    init( ioobj.mainFileName(), &fnmchgd );
    if ( errmsg_.isEmpty() && fnmchgd )
    {
	PtrMan<IOObj> ioobjclone = ioobj.clone();
	mDynamicCastGet(IOStream*,iostrm,ioobjclone.ptr())
	if ( iostrm )
	{
	    iostrm->fileSpec().setFileName( filename_ );
	    IOM().commitChanges( *iostrm );
	}
    }
}


Well::HDF5Writer::~HDF5Writer()
{
}


bool Well::HDF5Writer::useHDF5( const IOObj& ioobj, uiString& emsg )
{
    const BufferString fnm( ioobj.mainFileName() );
    bool usehdf = HDF5::isEnabled( HDF5::sWellType() );
    emsg.setEmpty();
    if ( !File::isEmpty(fnm) )
    {
	if ( File::exists(fnm) )
	{
	    usehdf = HDF5::isHDF5File( fnm );
	    if ( usehdf && !HDF5::isAvailable() )
		emsg = HDF5::Access::sHDF5NotAvailable( fnm );
	}
	else
	{
	    const FilePath fp( fnm );
	    const Translator* hdfwelltr = hdfWellTranslator::getInstance();
	    usehdf = hdfwelltr &&
		     StringView(fp.extension()) == hdfwelltr->defExtension();
	}
    }

    return usehdf;
}


HDF5::Reader* Well::HDF5Writer::createCoupledHDFReader() const
{
    return wrr_ ? wrr_->createCoupledReader() : nullptr;
}


void Well::HDF5Writer::init( const char* inpfnm, bool* fnmchgd )
{
    const BufferString orgfnm( inpfnm );
    if ( orgfnm.isEmpty() || !HDF5::isAvailable() )
    {
	errmsg_ = HDF5::Access::sHDF5NotAvailable( orgfnm );
	return;
    }

    wrr_ = HDF5::mkWriter();
    if ( !wrr_ )
    {
	pErrMsg("Available but no writer?");
	return;
    }

    filename_.set( orgfnm );
    if ( fnmchgd )
    {
	FilePath fp( orgfnm );
	fp.setExtension( HDF5::Access::sFileExtension() );
	filename_.set( fp.fullPath() );
	*fnmchgd = filename_ != orgfnm;
    }

    if ( !File::exists(filename_.buf()) )
	initGroups();
}


bool Well::HDF5Writer::initGroups()
{
    if ( !ensureFileOpen() )
	return false;

    uiRetVal uirv;
    if ( !wrr_->ensureGroup(sTrackGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sLogsGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sMarkersGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sTDsGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sCSsGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sDispParsGrpName(),uirv) )
	return false;

    return true;
}


bool Well::HDF5Writer::isFunctional() const
{
    return wrr_ && !filename_.isEmpty();
}


bool Well::HDF5Writer::ensureFileOpen() const
{
    if ( !wrr_ )
	return false;
    if ( wrr_->isOpen() )
	return true;
    if ( filename_.isEmpty() )
	return false;

    auto& wrr = cCast(HDF5::Writer&,*wrr_);
    const bool neededit = HDF5::isHDF5File( filename_ );
    uiRetVal uirv = neededit
		  ? wrr.open4Edit( filename_ )
		  : wrr.open( filename_ );

    if ( !uirv.isOK() )
    {
	const_cast<HDF5Writer*>(this)->wrr_ = nullptr;
	errmsg_.set( uirv );
	return false;
    }

    return true;
}


bool Well::HDF5Writer::put() const
{
    return putInfo()
	&& putTrack()
	&& putD2T()
	&& putLogs()
	&& putMarkers()
	&& putCSMdl()
	&& putDispProps();
}


void Well::HDF5Writer::putDepthUnit( IOPar& iop ) const
{
    iop.set( sKey::DepthUnit(),
	     UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
}


void Well::HDF5Writer::ensureCorrectDSSize( const HDF5::DataSetKey& dsky,
				int dim0, int dim1, uiRetVal& uirv ) const
{
    const int nrdims = dim1 > 0 ? 2 : 1;
    PtrMan<ArrayNDInfo> arrinf;
    if ( nrdims == 1 )
	arrinf = new Array1DInfoImpl( dim0 );
    else
	arrinf = new Array2DInfoImpl( dim0, dim1 );

    auto& wrr = cCast(HDF5::Writer&,*wrr_);
    uirv = wrr.resizeDataSet( dsky, *arrinf );
}


bool Well::HDF5Writer::putInfoAndTrack() const
{
    return putInfo() && putTrack();
}


bool Well::HDF5Writer::putInfo() const
{
    if ( !ensureFileOpen() )
	return false;

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    IOPar iop;
    wd_.info().fillPar( iop );
    putDepthUnit( iop );
    uiRetVal uirv = wrr.set( iop );
    mErrRetIfUiRvNotOK( HDF5::DataSetKey() );

    return true;
}


bool Well::HDF5Writer::putTrack() const
{
    if ( !ensureFileOpen() )
	return false;

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    const Well::Track& track = wd_.track();
    const int sz = track.size();
    Array2DImpl<double> crdarr( sz, 3 );
    Array1DImpl<double> mdarr( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord3& c = track.pos( idx );
	crdarr.set( idx, 0, c.x );
	crdarr.set( idx, 1, c.y );
	crdarr.set( idx, 2, c.z );
	mdarr.set( idx, track.dah( idx ) );
    }

    HDF5::DataSetKey dsky( sTrackGrpName(), "" );
    dsky.setMaximumSize( 0, nrrowsperblock );
    dsky.setDataSetName( sCoordsDSName() );
    const Array1DInfoImpl changedir( 1 );

    uiRetVal uirv;
    wrr.createDataSetIfMissing( dsky, OD::F64, crdarr.info(), changedir );
    ensureCorrectDSSize( dsky, sz, 3, uirv );
    mErrRetIfUiRvNotOK( trackdsky );
    HDF5::ArrayNDTool<double> arrtool( crdarr );
    uirv = arrtool.put( wrr, dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sMDsDSName() );
    wrr.createDataSetIfMissing( dsky, OD::F64, mdarr.info(), changedir );
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr.put( dsky, mdarr.arr(), sz );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::doPutD2T( bool csmdl ) const
{
    if ( !ensureFileOpen() )
	return false;

    const D2TModel* d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    if ( !d2t )
	return true;

    const int sz = d2t->size();
    Array1DImpl<double> mdarr( sz );
    Array1DImpl<double> timearr( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	mdarr.set( idx, d2t->dah( idx ) );
	timearr.set( idx, d2t->t( idx ) );
    }

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    uiRetVal uirv;
    const int modelid = 0; // TODO: suppport multiple models
    const HDF5::DataSetKey grpky =
	HDF5::DataSetKey::groupKey( csmdl ? sCSsGrpName() : sTDsGrpName(),
				    toString(modelid) );
    if ( !wrr.ensureGroup(grpky.groupName(),uirv) )
	return false;

    HDF5::DataSetKey dsky( nullptr, sMDsDSName() );
    dsky.setGroupName( grpky.fullDataSetName() );
    dsky.setMaximumSize( 0, nrrowsperblock );
    const Array1DInfoImpl changedir( 1 );
    wrr.createDataSetIfMissing( dsky, OD::F64, mdarr.info(), changedir );

    ensureCorrectDSSize( dsky, sz, -1, uirv );
    HDF5::ArrayNDTool<double> arrtool( mdarr );
    uirv = arrtool.put( wrr, dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sTWTsDSName() );
    wrr.createDataSetIfMissing( dsky, OD::F64, timearr.info(), changedir );
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr.put( dsky, timearr.arr(), sz );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( "" );
    IOPar iop;
    d2t->fillHeaderPar( iop );
    putDepthUnit( iop );
    uirv = wrr.set( iop, &dsky );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::putD2T() const
{
    return doPutD2T( false );
}


bool Well::HDF5Writer::putCSMdl() const
{
    return doPutD2T( true );
}


bool Well::HDF5Writer::putLogs() const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey logdsky( sLogsGrpName() );
    logdsky.setMaximumSize( 0, nrrowsperblock );
    const LogSet& logs = wd_.logs();
    const int nrlogs = logs.size();
    uiRetVal uirv;
    for ( int idx=0; idx<nrlogs; idx++ )
    {
	const Log& wl = logs.getLog( idx );
	HDF5::DataSetKey dsky = logdsky;
	dsky.setDataSetName( toString(idx) );
	const bool success = putLog( wl );
	if ( !success || !uirv.isOK() )
	    mErrRetIfUiRvNotOK( dsky );
    }

    PtrMan<HDF5::Reader> rdr = createCoupledHDFReader();
    if ( !rdr )
    {
	errmsg_.set( mINTERNAL("Write logs: cannot create coupled reader") );
	return false;
    }

    // remove possible extra data sets (can be there if logs were removed)
    for ( int idx=nrlogs+1; ; idx++ )
    {
	HDF5::DataSetKey dsky = logdsky;
	dsky.setDataSetName( toString(idx) );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	if ( rdr->hasGroup(grpkey.fullDataSetName()) )
	    setLogAttribs( grpkey, nullptr );
	else
	    break;
    }

    return putDefLogs();
}


bool Well::HDF5Writer::putDefLogs() const
{
    IOPar defiop;
    wd_.logs().defaultLogFillPar( defiop );
    PtrMan<IOPar> defmniop = defiop.subselect( Well::LogSet::sKeyDefMnem() );
    if ( !defmniop )
	return true;

    const HDF5::DataSetKey dsky( sLogsGrpName(), nullptr );
    if ( !ensureFileOpen() )
	return false;

    auto& wrr = cCast(HDF5::Writer&,*wrr_);
    OD::JSON::Array jsonarr( true );
    IOParIterator iter( *defmniop.ptr() );
    BufferString key, val;
    while( iter.next(key,val) )
    {
	OD::JSON::Object* subobj = jsonarr.add( new OD::JSON::Object );
	subobj->set( "mnemonic", key.buf() );
	subobj->set( "log", val.buf() );
    }

    const uiRetVal uirv = wrr.writeJSonAttribute( Well::LogSet::sKeyDefMnem(),
						  jsonarr, &dsky );
    return uirv.isOK();
}


bool Well::HDF5Writer::setLogAttribs( const HDF5::DataSetKey& dsky,
				      const Log* wl ) const
{
    IOPar iop;
    if ( wl )
    {
	iop = wl->pars();
	const bool havepars = !iop.isEmpty();
	iop.set( sKey::Name(), wl->name() );
	const BufferString uomlbl = wl->unitMeasLabel();
	if ( uomlbl.isEmpty() )
	    iop.removeWithKey( Log::sKeyUnitLbl() );
	else
	    iop.set( Log::sKeyUnitLbl(), uomlbl );

	const bool havemnemonics = !StringView(wl->mnemonicLabel()).isEmpty();
	if ( havemnemonics )
	    iop.set( Log::sKeyMnemLbl(), wl->mnemonicLabel());
	else
	    iop.removeWithKey( Log::sKeyMnemLbl() );

	iop.set( Log::sKeyHdrInfo(), havepars );
	const Interval<float>& dahrange = wl->dahRange();
	if ( dahrange.isUdf() )
	    iop.removeWithKey( Log::sKeyDahRange() );
	else
	    iop.set( Log::sKeyDahRange(), dahrange.start, dahrange.stop );

	const Interval<float> logrange = wl->valueRange();
	if ( logrange.isUdf() )
	    iop.removeWithKey( Log::sKeyLogRange() );
	else
	    iop.set( Log::sKeyLogRange(), logrange.start, logrange.stop );
    }

    iop.setYN( sKeyLogDel(), !wl );

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    putDepthUnit( iop );
    const uiRetVal uirv = wrr.set( iop, &dsky );
    return uirv.isOK();
}


int Well::HDF5Writer::getLogIndex( const char* lognm ) const
{
    const int nrlogs = wd_.logs().size();
    int logidx = wd_.logs().indexOf( lognm );
    //TODO: to be replaced by a proper well log identifier:
    if ( logidx < 0 )
    {
	//Unsafe !!!
	logidx = nrlogs < 0 ? 0 : nrlogs;
    }

    logidx++;
    return logidx;
}


bool Well::HDF5Writer::putLog( const Log& wl ) const
{
    if ( !ensureFileOpen() )
	return false;

    const int sz = wl.size();
    Array1DImpl<double> mdarr( sz );
    Array1DImpl<double> valarr( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	mdarr.set( idx, wl.dah( idx ) );
	valarr.set( idx, wl.value( idx ) );
    }

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    uiRetVal uirv;

    const int logidx = getLogIndex( wl.name() );
    HDF5::DataSetKey grpky =
	HDF5::DataSetKey::groupKey( sLogsGrpName(), toString(logidx) );
    if ( !wrr.ensureGroup(grpky.groupName(),uirv) )
	return false;

    HDF5::DataSetKey dsky( nullptr, sMDsDSName() );
    dsky.setGroupName( grpky.fullDataSetName() );

    dsky.setMaximumSize( 0, nrrowsperblock );
    const Array1DInfoImpl changedir( 1 );
    wrr.createDataSetIfMissing( dsky, OD::F64, mdarr.info(), changedir );

    ensureCorrectDSSize( dsky, sz, -1, uirv );
    HDF5::ArrayNDTool<double> arrtool( mdarr );
    uirv = arrtool.put( wrr, dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sValuesDSName() );
    wrr.createDataSetIfMissing( dsky, OD::F64, valarr.info(), changedir );
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr.put( dsky, valarr.arr(), sz );
    mErrRetIfUiRvNotOK( dsky );

    return setLogAttribs( grpky, &wl );
}


bool Well::HDF5Writer::putMarkers() const
{
    if ( !ensureFileOpen() )
	return false;

    const MarkerSet& ms = wd_.markers();
    if ( ms.isEmpty() )
	return true;

    HDF5::DataSetKey dsky( sMarkersGrpName(), "" );
    dsky.setMaximumSize( 0, nrrowsperblock );

    BufferStringSet nms, colors;
    TypeSet<double> mds;
    TypeSet<int> lvlids;
    for ( const auto* mrkr : ms )
    {
	nms.add( mrkr->name() );
	colors.add( mrkr->color().getStdStr() );
	mds.add( mrkr->dah() );
	lvlids.add( mrkr->levelID().asInt() );
    }

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    const int nrmarkers = ms.size();
    const Array1DInfoImpl arrinfo( nrmarkers );
    const Array1DInfoImpl changedir( 1 );
    dsky.setDataSetName( sMDsDSName() );
    wrr.createDataSetIfMissing( dsky, OD::F64, arrinfo, changedir );
    uiRetVal uirv;
    ensureCorrectDSSize( dsky, nrmarkers, -1, uirv );
    uirv = wrr.put( dsky, mds );
    mErrRetIfUiRvNotOK( dsky );
    IOPar iop;
    putDepthUnit( iop );
    uirv = wrr.set( iop, &dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sNamesDSName() );
    uirv = wrr.put( dsky, nms );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sColorsDSName() );
    uirv = wrr.put( dsky, colors );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sLvlIDsDSName() );
    wrr.createDataSetIfMissing( dsky, OD::SI32, arrinfo, changedir );
    ensureCorrectDSSize( dsky, nrmarkers, -1, uirv );
    uirv = wrr.put( dsky, lvlids );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::putDispProps() const
{
    if ( !ensureFileOpen() )
	return false;

    IOPar iop;
    wd_.displayProperties(true).fillPar( iop );
    wd_.displayProperties(false).fillPar( iop );
    putDepthUnit( iop );

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    const char* usernm = GetInterpreterName();
    const HDF5::DataSetKey grpdsky( sDispParsGrpName(), usernm );
    wrr.createTextDataSet( grpdsky );
    uiRetVal uirv = wrr.set( iop, &grpdsky );
    mErrRetIfUiRvNotOK( grpdsky );

    return true;
}
