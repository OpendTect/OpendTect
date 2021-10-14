/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "wellwriter.h"
#include "wellodwriter.h"
#include "wellodreader.h"
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


Well::Writer::Writer( const IOObj& ioobj, const Well::Data& wd )
    : wa_(0)
{
    init( ioobj, wd );
}


Well::Writer::Writer( const DBKey& ky, const Well::Data& wd )
    : wa_(0)
{
    IOObj* ioobj = ky.getIOObj();
    if ( !ioobj )
	errmsg_.appendPhrase( uiStrings::phrCannotFindDBEntry(ky) );
    else
    {
	init( *ioobj, wd );
	delete ioobj;
    }
}


void Well::Writer::init( const IOObj& ioobj, const Well::Data& wd )
{
    if ( ioobj.group() != mTranslGroupName(Well) )
	errmsg_ = tr("%1 is for a %2- not for a Well")
			.arg(ioobj.name()).arg(ioobj.group());
    else
    {
	wa_ = WDIOPF().getWriteAccess( ioobj, wd, errmsg_ );
	if ( !wa_ )
	    errmsg_ = uiStrings::phrCannotCreate(tr("writer of type %1"))
		   .arg(ioobj.translator());
    }
}


Well::Writer::~Writer()
{
    delete wa_;
}


bool Well::Writer::isFunctional( const DBKey& ky )
{
    PtrMan<IOObj> ioobj = ky.getIOObj();
    return ioobj ? isFunctional(*ioobj) : false;
}


bool Well::Writer::isFunctional( const IOObj& ioobj )
{
    RefMan<Well::Data> wd = new Well::Data;
    Well::Writer wrr( ioobj, *wd );
    return wrr.isFunctional();
}


const Well::Data* Well::Writer::data() const
{
    return wa_ ? &wa_->data() : 0;
}


#define mImplWWFn(rettyp,fnnm,typ,arg,udf) \
rettyp Well::Writer::fnnm( typ arg ) const \
{ return wa_ ? wa_->fnnm(arg) : udf; }
#define mImplSimpleWWFn(fnnm) \
bool Well::Writer::fnnm() const { return wa_ ? wa_->fnnm() : false; }

mImplSimpleWWFn(put)
mImplSimpleWWFn(putInfoAndTrack)
mImplSimpleWWFn(putMarkers)
mImplSimpleWWFn(putD2T)
mImplSimpleWWFn(putCSMdl)
mImplSimpleWWFn(putLogs)
mImplSimpleWWFn(putDispProps)
mImplSimpleWWFn(isFunctional)


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
    : Well::odIO(ioobj.mainFileName(),e)
    , Well::WriteAccess(w)
{
    init();
    ioobj.pars().getYN( sKeyLogStorage(), binwrlogs_ );
}


void Well::odWriter::init()
{
    binwrlogs_ = true;
    mSettUse(getYN,"dTect.Well logs","Binary format",binwrlogs_);
}


void Well::odWriter::putDepthUnit( ascostream& astrm ) const
{
    astrm.put( sKey::DepthUnit(),
		UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
}


void Well::odWriter::setStrmErrMsg( od_stream& strm,
				    const uiString& oper ) const
{
    errmsg_ = tr("Cannot %1 for %2").arg( oper ).arg( strm.fileName() );
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
	&& putD2T()
	&& putLogs()
	&& putMarkers()
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

    IOPar iop;
    wd_.info().fillPar( iop );

    // Name can be changed in obj management. Don't want it in file.
    iop.removeWithKey( Well::Info::sKeyWellName() );

    ascostream astrm( strm );
    putDepthUnit( astrm );
    iop.putTo( astrm );

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
	strm << c.x_ << od_tab;
	strm << c.y_ << od_tab;
	strm << c.z_ << od_tab;
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
    while ( iter.next() )
    {
	mGetOutStream( sExtLog(), iter.curIdx()+1, return false )
	const Well::Log& wl = iter.log();
	if ( !putLog(strm,wl) )
	    mErrRetStrmOper(tr("write log"))
    }

    return true;
}


bool Well::odWriter::putLog( od_ostream& strm, const Well::Log& wl ) const
{
    if ( !wrHdr(strm,sKeyLog()) )
	mErrRetStrmOper(tr("write header (log)"))

    MonitorLock ml( wl );
    ascostream astrm( strm );
    putDepthUnit( astrm );
    astrm.put( sKey::Name(), wl.name() );
    const BufferString uomlbl = wl.unitMeasLabel();
    const BufferString mnemlbl = wl.mnemLabel();
    const bool haveunits = !uomlbl.isEmpty();
    const bool havemnemonics = !mnemlbl.isEmpty();
    const bool havepars = !wl.pars().isEmpty();
    if ( havemnemonics )
	astrm.put( Well::Log::sKeyMnemLbl(), mnemlbl );
    if ( haveunits )
	astrm.put( Well::Log::sKeyUnitLbl(), uomlbl );
    astrm.putYN( Well::Log::sKeyHdrInfo(), havepars );
    const char* stortyp = binwrlogs_ ? (__islittle__ ? "Binary" : "Swapped")
				     : "Ascii";
    astrm.put( Well::Log::sKeyStorage(), stortyp );
    astrm.put( Well::Log::sKeyDahRange(),
	       wl.dahRange().start, wl.dahRange().stop );
    const Interval<float>& logrange = wl.valueRange();
    if ( !logrange.isUdf() )
	astrm.put( Well::Log::sKeyLogRange(), logrange.start, logrange.stop );

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
	    strm.addBin( v, 2*sizeof(float) );
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
    putDepthUnit( astrm );

    const Well::MarkerSet&  markerset = wd_.markers();
    Well::MarkerSetIter miter( markerset );
    while( miter.next() )
    {
	BufferString basekey; basekey += miter.curIdx()+1;
	const Well::Marker wm = miter.get();
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
    IOPar iop;
    d2t.fillHdrPar( iop );
    putDepthUnit( astrm );
    iop.putTo( astrm );

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
    wd_.displayProperties3d().fillPar( iop );
    wd_.displayProperties2d().fillPar( iop );
    iop.putTo( astrm );
    putDepthUnit( astrm );
    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write well display parameters"))
    return true;
}
