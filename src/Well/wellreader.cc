/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "wellreader.h"
#include "wellodreader.h"
#include "wellioprov.h"
#include "welltransl.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "od_istream.h"
#include "uistrings.h"


const char* Well::odIO::sKeyWell()	{ return "Well"; }
const char* Well::odIO::sKeyTrack()	{ return "Track"; }
const char* Well::odIO::sKeyLog()	{ return "Well Log"; }
const char* Well::odIO::sKeyMarkers()	{ return "Well Markers"; }
const char* Well::odIO::sKeyD2T()	{ return "Depth2Time Model"; }
const char* Well::odIO::sKeyDispProps()	{ return "Display Properties"; }
const char* Well::odIO::sExtWell()	{ return ".well"; }
const char* Well::odIO::sExtLog()	{ return ".wll"; }
const char* Well::odIO::sExtMarkers()	{ return ".wlm"; }
const char* Well::odIO::sExtD2T()	{ return ".wlt"; }
const char* Well::odIO::sExtCSMdl()	{ return ".csmdl"; }
const char* Well::odIO::sExtDispProps()	{ return ".disp"; }
const char* Well::odIO::sExtWellTieSetup() { return ".tie"; }



bool Well::ReadAccess::addToLogSet( Well::Log* newlog ) const
{
    if ( !newlog )
	return false;

    newlog->removeTopBottomUdfs();
    if ( newlog->isEmpty() )
	return false;

    wd_.logs().add( newlog );
    return true;
}


bool Well::ReadAccess::updateDTModel( D2TModel& dtmodel, bool ischeckshot,
				      uiString& errmsg ) const

{
    uiString msg;
    if ( !dtmodel.ensureValid(wd_,msg) )
    {
	errmsg = msg;
	return false;
    }

    if ( ischeckshot )
	wd_.checkShotModel() = dtmodel;
    else
	wd_.d2TModel() = dtmodel;

    return true;
}


Well::Reader::Reader( const IOObj& ioobj, Well::Data& wd )
    : ra_(0)
{
    init( ioobj, wd );
}


Well::Reader::Reader( const DBKey& ky, Well::Data& wd )
    : ra_(0)
{
    IOObj* ioobj = IOM().get( ky );
    if ( !ioobj )
	errmsg_ = tr( "Cannot find well ID %1 in data store" ).arg( ky );
    else
    {
	init( *ioobj, wd );
	delete ioobj;
    }
}


void Well::Reader::init( const IOObj& ioobj, Well::Data& wd )
{
    if ( ioobj.group() != mTranslGroupName(Well) )
	errmsg_ = tr( "%1 is not a Well, but a %2" )
	    .arg( ioobj.name() ).arg( ioobj.group() );
    else
    {
	ra_ = WDIOPF().getReadAccess( ioobj, wd, errmsg_ );
	if ( !ra_ )
	    errmsg_ = tr( "Cannot create reader of type %1" )
		   .arg( ioobj.translator() );
    }
}


Well::Reader::~Reader()
{
    delete ra_;
}


#define mImplWRFn(rettyp,fnnm,typ,arg,udf) \
rettyp Well::Reader::fnnm( typ arg ) const \
{ return ra_ ? ra_->fnnm(arg) : udf; }
#define mImplSimpleWRFn(fnnm) \
bool Well::Reader::fnnm() const { return ra_ ? ra_->fnnm() : false; }

mImplSimpleWRFn(getInfo)
mImplSimpleWRFn(getTrack)
mImplSimpleWRFn(getLogs)
mImplSimpleWRFn(getMarkers)
mImplSimpleWRFn(getDispProps)


bool Well::Reader::get() const
{
    if ( !ra_ )
	return false;

    Well::Data* wd = const_cast<Well::Data*>( data() );
    if ( !wd )
	return false;

    wd->d2TModel().setEmpty();
    wd->checkShotModel().setEmpty();
    if ( SI().zIsTime() )
    {
	if ( !getD2T() )
	    return false;
	getCSMdl();
    }
    else
    {
	if ( !getTrack() || !getInfo() )
	    return false;
    }

    getLogs();
    getMarkers();
    getDispProps();

    return true;
}


bool Well::Reader::getD2T() const
{
    if ( !getTrack() || !getInfo() )
	return false;

    return ra_ ? ra_->getD2T() : false;
}


bool Well::Reader::getCSMdl() const
{
    if ( !getTrack() || !getInfo() )
	return false;

    return ra_ ? ra_->getCSMdl() : false;
}


mImplWRFn(bool,getLog,const char*,lognm,false)
void Well::Reader::getLogInfo( BufferStringSet& lognms ) const
{ if ( ra_ ) ra_->getLogInfo( lognms ); }
Well::Data* Well::Reader::data()
{ return ra_ ? &ra_->data() : 0; }


bool Well::Reader::getMapLocation( Coord& coord ) const
{
    if ( !data() || !getInfo() )
	return false;

    const Well::Data& wd = *data();
    coord = wd.info().surfaceCoord();
    return true;
}



Well::odIO::odIO( const char* f, uiString& e )
    : basenm_(f)
    , errmsg_(e)
{
    FilePath fp( basenm_ );
    fp.setExtension( 0, true );
    const_cast<BufferString&>(basenm_) = fp.fullPath();
}


const char* Well::odIO::getFileName( const char* ext, int nr ) const
{
    return mkFileName( basenm_, ext, nr );
}


const char* Well::odIO::mkFileName( const char* bnm, const char* ext, int nr )
{
    mDeclStaticString( fnm );
    fnm = bnm;
    if ( nr )
	{ fnm += "^"; fnm += nr; }
    fnm += ext;
    return fnm;
}


const char* Well::odIO::getMainFileName( const IOObj& ioobj )
{
    return ioobj.fullUserExpr( true );
}


const char* Well::odIO::getMainFileName( const DBKey& mid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) return 0;
    return getMainFileName( *ioobj );
}


bool Well::odIO::removeAll( const char* ext ) const
{
    for ( int idx=1; ; idx++ )
    {
	BufferString fnm( getFileName(ext,idx) );
	if ( !File::exists(fnm) )
	    break;
	else if ( !File::remove(fnm) )
	    { errmsg_ = uiString( toUiString("%1 %2") )
		.arg( uiStrings::sCannotRemove() ).arg( fnm ); return false; }
    }
    return true;
}


static const char* rdHdr( od_istream& strm, const char* fileky,
			  double& ver )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(fileky) )
    {
	BufferString msg( strm.fileName(), " has type '" );
	msg += astrm.fileType(); msg += "' whereas it should be '";
	msg += fileky; msg += "'";
	ErrMsg( msg );
	return 0;
    }

    ver = (double)astrm.majorVersion() +
	  ((double)astrm.minorVersion() / (double)10);
    mDeclStaticString( hdrln ); hdrln = astrm.headerStartLine();
    return hdrln.buf();
}


#define mGetInpStream(ext,nr,seterr,todo) \
    errmsg_.setEmpty(); \
    od_istream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) \
    { \
	if ( seterr ) \
	    setInpStrmOpenErrMsg( strm ); \
	todo; \
    }



Well::odReader::odReader( const char* f, Well::Data& w, uiString& e )
    : Well::odIO(f,e)
    , Well::ReadAccess(w)
{
    FilePath fp( f );
    fp.setExtension( 0 );
    wd_.info().setName( fp.fileName() );
}


Well::odReader::odReader( const IOObj& ioobj, Well::Data& w, uiString& e )
    : Well::odIO(ioobj.fullUserExpr(true),e)
    , Well::ReadAccess(w)
{
    wd_.info().setName( ioobj.name() );
    wd_.setDBKey( ioobj.key() );
}


bool Well::odReader::getInfo() const
{
    mGetInpStream( sExtWell(), 0, true, return false );

    wd_.info().setDataSource( getFileName(sExtWell()) );
    return getInfo( strm );
}


void Well::odReader::setInpStrmOpenErrMsg( od_istream& strm ) const
{
    errmsg_ = uiStrings::phrCannotOpen( toUiString(strm.fileName()) );
    strm.addErrMsgTo( errmsg_ );
}


void Well::odReader::setStrmOperErrMsg( od_istream& strm,
					const uiString& oper ) const
{
    errmsg_ = tr( "Cannot %1 for %2." ).arg( oper ).arg( strm.fileName() );
    strm.addErrMsgTo( errmsg_ );
}


#define mErrStrmOper(oper,todo) { setStrmOperErrMsg( strm, oper ); todo; }
#define mErrRetStrmOper(oper) mErrStrmOper(oper,return false)

static const char* sKeyOldreplvel()	{ return "Replacement velocity"; }
static const char* sKeyOldgroundelev()	{ return "Ground Level elevation"; }

bool Well::odReader::getInfo( od_istream& strm ) const
{
    double version = 0.0;
    const char* hdrln = rdHdr( strm, sKeyWell(), version );
    if ( !hdrln )
	mErrRetStrmOper( tr("read header") )
    bool badhdr = *hdrln != 'd';
    if ( !badhdr )
    {
	if ( *(hdrln+1) == 'G' )
	{
	    errmsg_.set( "Cannot read old time wells" );
	    strm.addErrMsgTo( errmsg_ );
	    return false;
	}
	else if ( *(hdrln+1) != 'T' )
	    badhdr = true;
    }
    if ( badhdr )
	mErrRetStrmOper( tr("find proper file header in main well file") )

    ascistream astrm( strm, false );
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(Well::Info::sKeyUwid()) )
	    wd_.info().setUWI( astrm.value() );
	else if ( astrm.hasKeyword(Well::Info::sKeyOper()) )
	    wd_.info().setWellOperator( astrm.value() );
	else if ( astrm.hasKeyword(Well::Info::sKeyState()) )
	    wd_.info().setState( astrm.value() );
	else if ( astrm.hasKeyword(Well::Info::sKeyCounty()) )
	    wd_.info().setCounty( astrm.value() );
	else if ( astrm.hasKeyword(sKeyOldreplvel()) ||
		  astrm.hasKeyword(Well::Info::sKeyReplVel()) )
	    wd_.info().setReplacementVelocity( astrm.getFValue() );
	else if ( astrm.hasKeyword(sKeyOldgroundelev()) ||
		  astrm.hasKeyword(Well::Info::sKeyGroundElev()) )
	    wd_.info().setGroundElevation( astrm.getFValue() );
	else if ( astrm.hasKeyword(Well::Info::sKeyCoord()) )
	{
	    Coord coord; coord.fromString( astrm.value() );
	    wd_.info().setSurfaceCoord( coord );
	}
	else if ( astrm.hasKeyword(Well::Info::sKeyWellType()) )
	{
	    int welltype = astrm.getIValue();
	    wd_.info().setWellType( (Info::WellType)welltype );
	}
    }

    const Coord surfcoord = wd_.info().surfaceCoord();
    if ( (mIsZero(surfcoord.x_,0.001) && mIsZero(surfcoord.x_,0.001))
	    || (mIsUdf(surfcoord.x_) && mIsUdf(surfcoord.x_)) )
    {
	if ( wd_.track().isEmpty() && !getTrack(strm) )
	    return false;

	wd_.info().setSurfaceCoord( wd_.track().firstPos().getXY() );
    }

    return true;
}


bool Well::odReader::getTrack( od_istream& strm ) const
{
    Coord3 c, c0; float dah;
    wd_.track().setEmpty();
    while ( strm.isOK() )
    {
	strm >> c.x_ >> c.y_ >> c.z_ >> dah;
	if ( !strm.isOK() || c.distTo<float>(c0) < 1 ) break;
	wd_.track().addPoint( c.getXY(), (float) c.z_, dah );
    }
    if ( wd_.track().isEmpty() )
	mErrRetStrmOper( tr("find track data") )

    return true;
}


bool Well::odReader::getTrack() const
{
    od_istream strm( getFileName(sExtWell(),0) );
    if ( !strm.isOK() )
	mErrRetStrmOper( tr("find valid main well file") )

    ascistream astrm( strm );
    const double version = (double)astrm.majorVersion() +
			   ((double)astrm.minorVersion()/(double)10);
    IOPar dum; dum.getFrom( astrm );

    const bool isok = getTrack( strm );
    if ( SI().zInFeet() && version < 4.195 )
    {
	Well::Track& track = wd_.track();
	for ( int idx=0; idx<track.size(); idx++ )
	{
	    Coord3 pos = track.posByIdx( idx );
	    pos.z_ *= mToFeetFactorF;
	    track.setPoint( track.pointIDFor(idx), pos.getXY(), (float)pos.z_ );
	}
    }

    return isok;
}


void Well::odReader::getLogInfo( BufferStringSet& nms ) const
{
    TypeSet<int> idxs;
    getLogInfo( nms, idxs );
}


void Well::odReader::getLogInfo( BufferStringSet& nms,
				 TypeSet<int>& idxs ) const
{
    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, false, break );

	double version = 0.0;
	if ( rdHdr(strm,sKeyLog(),version) )
	{
	    int bintyp = 0;
	    RefMan<Well::Log> log = rdLogHdr( strm, bintyp, idx-1 );
	    if ( nms.isPresent(log->name()) )
	    {
		BufferString msg( log->name() );
		msg += " already present in the list, won't be read";
		ErrMsg( msg );
	    }
	    else
	    {
		nms.add( log->name() );
		idxs += idx;
	    }
	}
    }
}


bool Well::odReader::getLog( const char* lognm ) const
{
    BufferStringSet nms;
    TypeSet<int> idxs;
    getLogInfo( nms, idxs );
    const int lognmidx = nms.indexOf( lognm );
    if ( lognmidx<0 ) return false;

    const int logfileidx = idxs[lognmidx];
    mGetInpStream( sExtLog(), logfileidx, true, return false );
    return addLog( strm );
}


bool Well::odReader::getLogs() const
{
    bool rv = true;
    wd_.logs().setEmpty();
    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, false, break );

	if ( !addLog(strm) )
	{
	    setStrmOperErrMsg( strm, tr("read data") );
	    ErrMsg( errmsg_.getFullString() ); errmsg_.setEmpty();
	    rv = false;
	}
    }

    return rv;
}


Well::Log* Well::odReader::rdLogHdr( od_istream& strm, int& bintype, int idx )
{
    Well::Log* newlog = new Well::Log;
    ascistream astrm( strm, false );
    bool havehdrinfo = false;
    bintype = 0;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name()) )
	    newlog->setName( astrm.value() );
	if ( astrm.hasKeyword(Well::Log::sKeyUnitLbl()) )
	    newlog->setUnitMeasLabel( astrm.value() );
	if ( astrm.hasKeyword(Well::Log::sKeyHdrInfo()) )
	    havehdrinfo = astrm.getYN();
	if ( astrm.hasKeyword(Well::Log::sKeyStorage()) )
	    bintype = *astrm.value() == 'B' ? 1
		    : (*astrm.value() == 'S' ? -1 : 0);
    }
    if ( newlog->name().isEmpty() )
    {
	BufferString nm( "[" ); nm += idx+1; nm += "]";
	newlog->setName( nm );
    }

    if ( havehdrinfo )
	newlog->pars().getFrom( astrm );

    return newlog;
}


bool Well::odReader::addLog( od_istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyLog(),version) )
	return false;

    int bintype = 0;
    Well::Log* newlog = rdLogHdr( strm, bintype, wd_.logs().size() );
    if ( !newlog )
	mErrRetStrmOper( tr("read log") )

    readLogData( *newlog, strm, bintype );
    TypeSet<float> dahvals, zvals;
    newlog->getData( dahvals, zvals );

    if ( SI().zInFeet() && version < 4.195 )
    {
	for ( int idx=0; idx<dahvals.size(); idx++ )
	    dahvals[idx] *= mToFeetFactorF;
    }

    if ( wd_.track().isEmpty() )
	getTrack();
    if ( !wd_.track().isEmpty() )
    {
	const Interval<float> trackdahrg = wd_.track().dahRange();
	for ( int idx=0; idx<dahvals.size(); idx++ )
	    if ( !trackdahrg.includes(dahvals[idx],false) )
		dahvals[idx] = mUdf(float);
    }

    newlog->setData( dahvals, zvals );
    return addToLogSet( newlog );
}


void Well::odReader::readLogData( Well::Log& wl, od_istream& strm,
				int bintype ) const
{
    float v[2];
    while ( strm.isOK() )
    {
	if ( !bintype )
	    strm >> v[0] >> v[1];
	else
	{
	    strm.getBin( v, 2 * sizeof(float) );
	    if ( (bintype > 0) != __islittle__ )
	    {
		SwapBytes( v, sizeof(float) );
		SwapBytes( v+1, sizeof(float) );
	    }
	}
	if ( !strm.isOK() ) break;

	wl.addValue( v[0], v[1] );
    }
}


bool Well::odReader::getMarkers() const
{
    mGetInpStream( sExtMarkers(), 0, true, return false );
    return getMarkers( strm );
}


bool Well::odReader::getMarkers( od_istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyMarkers(),version) )
	mErrRetStrmOper( tr("read header") )

    ascistream astrm( strm, false );

    IOPar iopar( astrm );
    if ( iopar.isEmpty() )
	mErrRetStrmOper( tr("find anything in file") )

    if ( wd_.track().isEmpty() )
    {
	if ( !getTrack() )
	    return false;
    }

    const Interval<float> trackdahrg = wd_.track().dahRange();
    const bool havetrack = !wd_.track().isEmpty();
    wd_.markers().setEmpty();
    BufferString bs;
    for ( int idx=1;  ; idx++ )
    {
	BufferString basekey; basekey += idx;
	BufferString key = IOPar::compKey( basekey, sKey::Name() );
	if ( !iopar.get(key,bs) ) break;

	Well::Marker wm( bs );

	key = IOPar::compKey( basekey, Well::Marker::sKeyDah() );
	if ( !iopar.get(key,bs) )
	    { continue; }
	const float val = bs.toFloat();
	wm.setDah( (SI().zInFeet() && version<4.195) ? (val*mToFeetFactorF)
						      : val );
	key = IOPar::compKey( basekey, sKey::StratRef() );
	Well::Marker::LevelID lvlid = Well::Marker::LevelID::getInvalid();
	iopar.get( key, lvlid );
	wm.setLevelID( lvlid );

	key = IOPar::compKey( basekey, sKey::Color() );
	if ( iopar.get(key,bs) )
	{
	    Color col( wm.color() );
	    col.use( bs.buf() );
	    wm.setColor( col );
	}

	if ( !trackdahrg.includes(wm.dah(),false) && havetrack )
	    continue;
	else
	    wd_.markers().insertNew( wm );
    }

    return true;
}


bool Well::odReader::getD2T() const	{ return doGetD2T( false ); }
bool Well::odReader::getCSMdl() const	{ return doGetD2T( true ); }
bool Well::odReader::doGetD2T( bool csmdl ) const
{
    mGetInpStream( csmdl ? sExtCSMdl() : sExtD2T(), 0, true, return false );
    return doGetD2T( strm, csmdl );
}


bool Well::odReader::getD2T( od_istream& strm ) const
{ return doGetD2T(strm,false); }
bool Well::odReader::getCSMdl( od_istream& strm ) const
{ return doGetD2T(strm,true); }
bool Well::odReader::doGetD2T( od_istream& strm, bool csmdl ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyD2T(),version) )
	mErrRetStrmOper( tr("read D/T file header") )

    ascistream astrm( strm, false );
    Well::D2TModel d2t;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name()) )
	    d2t.setName( astrm.value() );
	else if ( astrm.hasKeyword(sKey::Desc()) )
	    d2t.setDesc( astrm.value() );
	else if ( astrm.hasKeyword(Well::D2TModel::sKeyDataSrc()) )
	    d2t.setDataSource( astrm.value() );
    }

    while ( strm.isOK() )
    {
	float dah, val; strm >> dah >> val;
	if ( !strm.isOK() )
	    break;
	d2t.setValueAt( dah, val );
    }

    if ( !updateDTModel(d2t,csmdl,errmsg_) )
	mErrRetStrmOper( tr("read valid Time/Depth relation") )

    return true;
}


bool Well::odReader::getDispProps() const
{
    mGetInpStream( sExtDispProps(), 0, true, return false );
    return getDispProps( strm );
}


bool Well::odReader::getDispProps( od_istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyDispProps(),version) )
	mErrRetStrmOper( tr("read well properties header") )

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    wd_.displayProperties(true).usePar( iop );
    wd_.displayProperties(false).usePar( iop );
    return true;
}
