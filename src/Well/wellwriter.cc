/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "wellwriter.h"
#include "wellodwriter.h"
#include "wellioprov.h"
#include "welltransl.h"

#include "welldata.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "ascstream.h"
#include "od_ostream.h"
#include "keystrs.h"
#include "envvars.h"
#include "settings.h"
#include "ioobj.h"
#include "ioman.h"


bool Well::Writer::isFunctional( const DBKey& ky )
{
    PtrMan<IOObj> ioobj = IOM().get( ky );
    return ioobj ? isFunctional(*ioobj) : false;
}


bool Well::Writer::isFunctional( const IOObj& ioobj )
{
    RefMan<Well::Data> wd = new Well::Data;
    Well::Writer wrr( ioobj, *wd );
    return wrr.isFunctional();
}


Well::Writer::Writer( const IOObj& ioobj, const Well::Data& wd )
    : wa_(0)
{
    init( ioobj, wd );
}


Well::Writer::Writer( const DBKey& ky, const Well::Data& wd )
    : wa_(0)
{
    IOObj* ioobj = IOM().get( ky );
    if ( !ioobj )
	errmsg_ = tr( "Cannot find well ID %1 in data store." ).arg( ky );
    else
    {
	init( *ioobj, wd );
	delete ioobj;
    }
}


void Well::Writer::init( const IOObj& ioobj, const Well::Data& wd )
{
    if ( ioobj.group() != mTranslGroupName(Well) )
        errmsg_ = tr( "%1 is a %2 - not a Well" )
		    .arg( ioobj.name() ).arg( ioobj.group() );
    else
    {
	wa_ = WDIOPF().getWriteAccess( ioobj, wd, errmsg_ );
	if ( !wa_ && errmsg_.isEmpty() )
	    errmsg_ = tr( "Cannot create writer of type %1" )
		   .arg( ioobj.translator() );
    }
}


Well::Writer::~Writer()
{
    delete wa_;
}

#define mImplWWFn(rettyp,fnnm,typ,arg,udf) \
rettyp Well::Writer::fnnm( typ arg ) const \
{ return wa_ ? wa_->fnnm(arg) : udf; }
#define mImplSimpleWWFn(fnnm) \
bool Well::Writer::fnnm() const { return wa_ ? wa_->fnnm() : false; }

mImplSimpleWWFn(put)
mImplSimpleWWFn(putInfoAndTrack)
mImplSimpleWWFn(putLogs)
mImplSimpleWWFn(putMarkers)
mImplSimpleWWFn(putD2T)
mImplSimpleWWFn(putCSMdl)
mImplSimpleWWFn(putDispProps)
mImplSimpleWWFn(isFunctional)

mImplWWFn(bool,putLog,const Log&,wl,false)


#define mErrStrmOper(oper,todo) \
{ setStrmErrMsg(strm,oper); todo; }
#define mErrRetStrmOper(oper) mErrStrmOper(oper,return false)

#define mGetOutStream(ext,nr,todo) \
    errmsg_.setEmpty(); \
    od_ostream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) mErrStrmOper(startWriteStr(),todo)


Well::odWriter::odWriter( const char* f, const Well::Data& w, uiString& e )
    : Well::odIO(f,e)
    , Well::WriteAccess(w)
{
    init();
}


Well::odWriter::odWriter( const IOObj& ioobj, const Well::Data& w,
			  uiString& e )
    : Well::odIO(ioobj.fullUserExpr(false),e)
    , Well::WriteAccess(w)
{
    wd_.setDBKey( ioobj.key() );
    init();
    ioobj.pars().getYN( sKeyLogStorage(), binwrlogs_ );
}


void Well::odWriter::init()
{
    binwrlogs_ = true;
    mSettUse(getYN,"dTect.Well logs","Binary format",binwrlogs_);
}


void Well::odWriter::setStrmErrMsg( od_stream& strm,
				    const uiString& oper ) const
{
    errmsg_ = tr( "Cannot %1 for %2." ).arg( oper ).arg( strm.fileName() );
    strm.addErrMsgTo( errmsg_ );
}


uiString Well::odWriter::startWriteStr() const
{
    return tr("start writing");
}


bool Well::odWriter::isFunctional() const
{
    return !basenm_.isEmpty();
}


bool Well::odWriter::wrHdr( od_ostream& strm, const char* fileky ) const
{
    ascostream astrm( strm );
    return astrm.putHeader( fileky );
}


bool Well::odWriter::put() const
{
    return putInfoAndTrack()
	&& putLogs()
	&& putMarkers()
	&& putD2T()
	&& putCSMdl()
	&& putDispProps();
}


bool Well::odWriter::putInfoAndTrack() const
{
    mGetOutStream( sExtWell(), 0, return false )
    return putInfoAndTrack( strm );
}


bool Well::odWriter::putInfoAndTrack( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWell()) )
	mErrRetStrmOper(tr("write header (info/track)"))

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
    astrm.put( Well::Info::sKeyUwid(), wd_.info().UWI() );
    astrm.put( Well::Info::sKeyOper(), wd_.info().wellOperator() );
    astrm.put( Well::Info::sKeyState(), wd_.info().getState() );
    astrm.put( Well::Info::sKeyCounty(), wd_.info().getCounty() );
    astrm.put( Well::Info::sKeyWellType(), (int)wd_.info().wellType() );
    if ( wd_.info().surfaceCoord() != Coord(0,0) )
	astrm.put( Well::Info::sKeyCoord(),
			wd_.info().surfaceCoord().toString());
    astrm.put( Well::Info::sKeyReplVel(), wd_.info().replacementVelocity() );
    astrm.put( Well::Info::sKeyGroundElev(), wd_.info().groundElevation() );
    astrm.newParagraph();

    return putTrack( strm );
}


bool Well::odWriter::putTrack( od_ostream& strm ) const
{
    TrackIter iter( wd_.track() );
    while ( iter.next() )
    {
	const Coord3 c = iter.pos();
	    // don't try to do the following in one statement
	    // (unless for educational purposes)
	strm << c.x << od_tab;
	strm << c.y << od_tab;
	strm << c.z << od_tab;
	strm << iter.dah() << od_newline;
    }
    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write track data"))
    return true;
}


bool Well::odWriter::putLogs() const
{
    removeAll( sExtLog() );
    Well::LogSetIter iter( wd_.logs() );
    int idx = 0;
    while ( iter.next() )
    {
	idx++;
	mGetOutStream( sExtLog(), idx, return false )

	const Well::Log& wl = iter.log();
	if ( !putLog(strm,wl) )
	    mErrRetStrmOper(tr("write log"))
    }

    return true;
}


bool Well::odWriter::putLog( const Well::Log& wl ) const
{
    const int logidx = wd_.logs().indexOf( wl.name() );
    if ( logidx<0 )
    {
	pErrMsg( "First add Log to Well::Data" );
	return false;
    }

    const BufferString logfnm = getFileName( Well::odIO::sExtLog(), logidx+1 );
    od_ostream strm( logfnm );
    if ( !putLog(strm,wl) )
	return false;

    return true;
}


bool Well::odWriter::putLog( od_ostream& strm, const Well::Log& wl ) const
{
    if ( !wrHdr(strm,sKeyLog()) )
	mErrRetStrmOper(tr("write header (log)"))

    MonitorLock ml( wl );
    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
    astrm.put( sKey::Name(), wl.name() );
    const BufferString uomlbl = wl.unitMeasLabel();
    const bool haveunits = !uomlbl.isEmpty();
    const bool havepars = !wl.pars().isEmpty();
    if ( haveunits )
	astrm.put( Well::Log::sKeyUnitLbl(), uomlbl );
    astrm.putYN( Well::Log::sKeyHdrInfo(), havepars );
    const char* stortyp = binwrlogs_ ? (__islittle__ ? "Binary" : "Swapped")
				     : "Ascii";
    astrm.put( Well::Log::sKeyStorage(), stortyp );
    astrm.newParagraph();
    if ( havepars )
	wl.pars().putTo( astrm );

    Interval<int> wrintv( 0, wl.size()-1 );
    float dah, val;
    for ( ; wrintv.start<wl.size(); wrintv.start++ )
    {
	dah = wl.dahByIdx(wrintv.start); val = wl.valueByIdx(wrintv.start);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }
    for ( ; wrintv.stop>=0; wrintv.stop-- )
    {
	dah = wl.dahByIdx(wrintv.stop); val = wl.valueByIdx(wrintv.stop);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }

    float v[2];
    for ( int idx=wrintv.start; idx<=wrintv.stop; idx++ )
    {
	v[0] = wl.dahByIdx( idx );
	if ( mIsUdf(v[0]) )
	    continue;

	v[1] = wl.valueByIdx( idx );
	if ( binwrlogs_ )
	    strm.addBin( v );
	else
	{
	    strm << v[0] << od_tab;
	    if ( mIsUdf(v[1]) )
		strm << sKey::FloatUdf();
	    else
		strm << v[1];

	    strm << od_newline;
	}
    }

    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write log data"))
    return true;
}


bool Well::odWriter::putMarkers() const
{
    mGetOutStream( sExtMarkers(), 0, return false )
    return putMarkers( strm );
}


bool Well::odWriter::putMarkers( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyMarkers()) )
	mErrRetStrmOper(tr("write header (markers)"))

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
    for ( int idx=0; idx<wd_.markers().size(); idx++ )
    {
	BufferString basekey; basekey += idx+1;
	const Well::Marker& wm = *wd_.markers()[idx];
	const float dah = wm.dah();
	if ( mIsUdf(dah) )
	    continue;

	astrm.put( IOPar::compKey(basekey,sKey::Name()), wm.name() );
	astrm.put( IOPar::compKey(basekey,Well::Marker::sKeyDah()), dah );
	astrm.put( IOPar::compKey(basekey,sKey::StratRef()),
						wm.levelID().getI() );
	BufferString bs; wm.color().fill( bs );
	astrm.put( IOPar::compKey(basekey,sKey::Color()), bs );
    }

    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write markers"))
    return true;
}


bool Well::odWriter::putD2T() const	{ return doPutD2T( false ); }
bool Well::odWriter::putCSMdl() const	{ return doPutD2T( true ); }
bool Well::odWriter::doPutD2T( bool csmdl ) const
{
    if ( (csmdl && wd_.checkShotModel().isEmpty())
     || (!csmdl && wd_.d2TModel().isEmpty()) )
	return true;

    mGetOutStream( csmdl ? sExtCSMdl() : sExtD2T(), 0, return false )
    return doPutD2T( strm, csmdl );
}


bool Well::odWriter::putD2T( od_ostream& strm ) const
{ return doPutD2T( strm, false ); }
bool Well::odWriter::putCSMdl( od_ostream& strm ) const
{ return doPutD2T( strm, true ); }
bool Well::odWriter::doPutD2T( od_ostream& strm, bool csmdl ) const
{
    if ( !wrHdr(strm,sKeyD2T()) )
	mErrRetStrmOper(tr("write header (D2T model)"))

    ascostream astrm( strm );
    const Well::D2TModel& d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    astrm.put( sKey::Name(), d2t.name() );
    astrm.put( sKey::Desc(), d2t.desc() );
    astrm.put( D2TModel::sKeyDataSrc(), d2t.dataSource() );
    astrm.put( Well::Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
    astrm.newParagraph();

    D2TModelIter iter( d2t );
    while ( iter.next() )
    {
	const float dah = iter.dah();
	if ( mIsUdf(dah) )
	    continue;

	strm.addPrecise( dah );
	strm << od_tab;
	strm.addPrecise( iter.t() );
	strm << od_newline;
    }

    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write Depth/Time data"))
    return true;
}


bool Well::odWriter::putDispProps() const
{
    mGetOutStream( sExtDispProps(), 0, return false )
    return putDispProps( strm );
}


bool Well::odWriter::putDispProps( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyDispProps()) )
	mErrRetStrmOper(tr("write header (display parameters)"))

    ascostream astrm( strm );
    IOPar iop;
    wd_.displayProperties(true).fillPar( iop );
    wd_.displayProperties(false).fillPar( iop );
    iop.putTo( astrm );
    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write well display parameters"))
    return true;
}
