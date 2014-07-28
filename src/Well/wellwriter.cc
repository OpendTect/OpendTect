/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellwriter.h"
#include "wellodwriter.h"
#include "wellioprov.h"
#include "welltransl.h"

#include "welldata.h"
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


Well::Writer::Writer( const IOObj& ioobj, const Well::Data& wd )
    : wa_(0)
{
    init( ioobj, wd );
}


Well::Writer::Writer( const MultiID& ky, const Well::Data& wd )
    : wa_(0)
{
    IOObj* ioobj = IOM().get( ky );
    if ( !ioobj )
	errmsg_.set( "Cannot find well ID " ).add( ky ).add( " in data store" );
    else
    {
	init( *ioobj, wd );
	delete ioobj;
    }
}


/* DEPRECATED: will only write to OD internal data store! */
Well::Writer::Writer( const char* filenm, const Well::Data& wd )
    : wa_( new Well::odWriter( filenm, wd, errmsg_ ) )
{
}


void Well::Writer::init( const IOObj& ioobj, const Well::Data& wd )
{
    if ( ioobj.group() != mTranslGroupName(Well) )
	errmsg_.set( ioobj.name() ).add( " is for a " ).add( ioobj.group() )
	       .add( " - not for a Well" );
    else
    {
	wa_ = WDIOPF().getWriteAccess( ioobj, wd, errmsg_ );
	if ( !wa_ )
	    errmsg_.set( "Cannot create writer of type " )
		   .add( ioobj.translator() );
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
mImplSimpleWWFn(putTrack)
mImplSimpleWWFn(putLogs)
mImplSimpleWWFn(putMarkers)
mImplSimpleWWFn(putD2T)
mImplSimpleWWFn(putCSMdl)
mImplSimpleWWFn(putDispProps)

mImplWWFn(bool,putLog,const Log&,wl,false)


#define mErrStrmOper(oper,todo) \
{ errmsg_.set( "Cannot " ).add( oper ).add( " for " ).add( strm.fileName() ); \
    strm.addErrMsgTo( errmsg_ ); todo; }
#define mErrRetStrmOper(oper) mErrStrmOper(oper,return false)

#define mGetOutStream(ext,nr,todo) \
    errmsg_.setEmpty(); \
    od_ostream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) mErrStrmOper("start writing",todo)


Well::odWriter::odWriter( const char* f, const Well::Data& w, BufferString& e )
    : Well::odIO(f,e)
    , Well::WriteAccess(w)
{
    init();
}

Well::odWriter::odWriter( const IOObj& ioobj, const Well::Data& w,
			  BufferString& e )
    : Well::odIO(ioobj.fullUserExpr(false),e)
    , Well::WriteAccess(w)
{
    init();
}


void Well::odWriter::init()
{
    binwrlogs_ = true;
    mSettUse(getYN,"dTect.Well logs","Binary format",binwrlogs_);
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
	mErrRetStrmOper("write header (info/track)")

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyuwid(), wd_.info().uwid );
    astrm.put( Well::Info::sKeyoper(), wd_.info().oper );
    astrm.put( Well::Info::sKeystate(), wd_.info().state );
    astrm.put( Well::Info::sKeycounty(), wd_.info().county );
    if ( wd_.info().surfacecoord != Coord(0,0) )
	astrm.put( Well::Info::sKeycoord(), wd_.info().surfacecoord.toString());
    astrm.put( Well::Info::sKeySRD(), wd_.info().srdelev );
    astrm.put( Well::Info::sKeyreplvel(), wd_.info().replvel );
    astrm.put( Well::Info::sKeygroundelev(), wd_.info().groundelev );
    astrm.newParagraph();

    return putTrack( strm );
}


bool Well::odWriter::putTrack( od_ostream& strm ) const
{
    for ( int idx=0; idx<wd_.track().size(); idx++ )
    {
	const Coord3& c = wd_.track().pos(idx);
	    // don't try to do the following in one statement
	    // (unless for educational purposes)
	strm << c.x << od_tab;
	strm << c.y << od_tab;
	strm << c.z << od_tab;
	strm << wd_.track().dah(idx) << od_newline;
    }
    if ( !strm.isOK() )
	mErrRetStrmOper("write track data")
    return true;
}


bool Well::odWriter::putTrack() const
{
    mGetOutStream( sExtTrack(), 0, return false )
    return putTrack( strm );
}


bool Well::odWriter::putLogs() const
{
    removeAll( sExtLog() );
    for ( int idx=0; idx<wd_.logs().size(); idx++ )
    {
	mGetOutStream( sExtLog(), idx+1, return false )

	const Well::Log& wl = wd_.logs().getLog(idx);
	if ( !putLog(strm,wl) )
	    mErrRetStrmOper("write log")
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
	mErrRetStrmOper("write header (log)")

    ascostream astrm( strm );
    astrm.put( sKey::Name(), wl.name() );
    const bool haveunits = *wl.unitMeasLabel();
    const bool havepars = !wl.pars().isEmpty();
    if ( haveunits )
	astrm.put( Well::Log::sKeyUnitLbl(), wl.unitMeasLabel() );
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
	dah = wl.dah(wrintv.start); val = wl.value(wrintv.start);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }
    for ( ; wrintv.stop>=0; wrintv.stop-- )
    {
	dah = wl.dah(wrintv.stop); val = wl.value(wrintv.stop);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }

    float v[2];
    for ( int idx=wrintv.start; idx<=wrintv.stop; idx++ )
    {
	v[0] = wl.dah( idx );
	if ( mIsUdf(v[0]) )
	    continue;

	v[1] = wl.value( idx );
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
	mErrRetStrmOper("write log data")
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
	mErrRetStrmOper("write header (markers)")

    ascostream astrm( strm );
    for ( int idx=0; idx<wd_.markers().size(); idx++ )
    {
	BufferString basekey; basekey += idx+1;
	const Well::Marker& wm = *wd_.markers()[idx];
	const float dah = wm.dah();
	if ( mIsUdf(dah) )
	    continue;

	astrm.put( IOPar::compKey(basekey,sKey::Name()), wm.name() );
	astrm.put( IOPar::compKey(basekey,Well::Marker::sKeyDah()), dah );
	astrm.put( IOPar::compKey(basekey,sKey::StratRef()), wm.levelID() );
	BufferString bs; wm.color().fill( bs );
	astrm.put( IOPar::compKey(basekey,sKey::Color()), bs );
    }

    if ( !strm.isOK() )
	mErrRetStrmOper("write markers")
    return true;
}


bool Well::odWriter::putD2T() const	{ return doPutD2T( false ); }
bool Well::odWriter::putCSMdl() const	{ return doPutD2T( true ); }
bool Well::odWriter::doPutD2T( bool csmdl ) const
{
    if ( (csmdl && !wd_.checkShotModel()) || (!csmdl && !wd_.d2TModel()) )
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
	mErrRetStrmOper("write header (D2T model)")

    ascostream astrm( strm );
    const Well::D2TModel& d2t = *(csmdl ? wd_.checkShotModel(): wd_.d2TModel());
    astrm.put( sKey::Name(), d2t.name() );
    astrm.put( sKey::Desc(), d2t.desc );
    astrm.put( D2TModel::sKeyDataSrc(), d2t.datasource );
    astrm.newParagraph();

    for ( int idx=0; idx<d2t.size(); idx++ )
    {
	const float dah = d2t.dah( idx );
	if ( mIsUdf(dah) )
	    continue;

	strm << dah << od_tab << d2t.t(idx) << od_newline;
    }

    if ( !strm.isOK() )
	mErrRetStrmOper("write Depth/Time data")
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
	mErrRetStrmOper("write header (display parameters)")

    ascostream astrm( strm );
    IOPar iop;
    wd_.displayProperties(true).fillPar( iop );
    wd_.displayProperties(false).fillPar( iop );
    iop.putTo( astrm );
    if ( !strm.isOK() )
	mErrRetStrmOper("write well display parameters")
    return true;
}

