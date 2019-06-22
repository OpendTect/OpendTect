/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jul 2016
-*/


#include "waveletio.h"
#include "waveletmanager.h"
#include "uistrings.h"
#include "keystrs.h"
#include "ioobj.h"
#include "separstr.h"
#include "ascstream.h"
#include "survinfo.h"
#include "tabledef.h"

defineTranslatorGroup(Wavelet,"Wavelet");
defineTranslator(dgb,Wavelet,mDGBKey);
mDefSimpleTranslatorSelector(Wavelet);
mDefSimpleTranslatorioContext(Wavelet,Seis)

uiString WaveletTranslatorGroup::sTypeName( int num )
{ return uiStrings::sWavelet(num); }


WaveletLoader::WaveletLoader( const DBKey& id )
    : ioobj_(id.getIOObj())
{
}


WaveletLoader::WaveletLoader( const IOObj* ioobj )
    : ioobj_(ioobj ? ioobj->clone() : 0)
{
}


WaveletLoader::~WaveletLoader()
{
    delete ioobj_;
}


uiRetVal WaveletLoader::read( Wavelet*& wvlt )
{
    uiRetVal uirv;
    if ( !ioobj_ )
    {
	uirv = uiStrings::phrCannotFindDBEntry( uiStrings::sWavelet() );
	return uirv;
    }

    PtrMan<WaveletTranslator> transl =
		(WaveletTranslator*)ioobj_->createTranslator();
    if ( !transl )
    {
	uirv = uiStrings::phrSelectObjectWrongType( uiStrings::sWavelet() );
	return uirv;
    }

    wvlt = 0;
    Conn* connptr = ioobj_->getConn( Conn::Read );
    if ( !connptr || connptr->isBad() )
	uirv = ioobj_->phrCannotOpenObj();
    else
    {
	wvlt = new Wavelet;
	if ( transl->read(wvlt,*connptr) )
	    wvlt->setName( ioobj_->name() );
	else
	{
	    uirv = ioobj_->phrCannotReadObj();
	    wvlt->unRef(); wvlt = 0;
	}
    }

    delete connptr;
    return uirv;
}


uiRetVal WaveletLoader::load()
{
    Wavelet* wvlt;

    uiRetVal uirv = read( wvlt );
    if ( uirv.isOK() )
	addToMGR( wvlt, ioobj_->key() );


    return uirv;
}


bool WaveletLoader::addToMGR( Wavelet* wvlt, const DBKey& ky )
{
    WaveletManager& mgr = WaveletMGR();
    if ( mgr.isLoaded(ioobj_->key()) )
	{ wvlt->unRef(); return false; }

    mgr.addNew( *wvlt, ky, &ioobj_->pars(), true );
    return true;
}


mDefineInstanceCreatedNotifierAccess(WaveletSaver)


WaveletSaver::WaveletSaver( const Wavelet& wvlt )
    : Saveable(wvlt)
{
    mTriggerInstanceCreatedNotifier();
}


WaveletSaver::WaveletSaver( const WaveletSaver& oth )
    : Saveable(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


WaveletSaver::~WaveletSaver()
{
    sendDelNotif();
}


mImplMonitorableAssignmentWithNoMembers(WaveletSaver,Saveable)


ConstRefMan<Wavelet> WaveletSaver::wavelet() const
{
    return ConstRefMan<Wavelet>( static_cast<const Wavelet*>( object() ) );
}


void WaveletSaver::setWavelet( const Wavelet& wvlt )
{
    setObject( wvlt );
}



uiRetVal WaveletSaver::doStore( const IOObj& ioobj,
				const TaskRunnerProvider& ) const
{
    uiRetVal uirv;
    PtrMan<WaveletTranslator> transl =
        (WaveletTranslator*)ioobj.createTranslator();
    if ( !transl )
    {
	uirv.add( uiStrings::phrSelectObjectWrongType(uiStrings::sWavelet()) );
	return uirv;
    }

    ConstRefMan<Wavelet> wvlt = wavelet();
    if ( !wvlt )
	return uirv;

    Conn* connptr = ioobj.getConn( Conn::Write );
    if ( !connptr || connptr->isBad() )
	uirv.add( ioobj.phrCannotOpenObj() );
    else
    {
	RefMan<Wavelet> copiedwvlt = new Wavelet( *wvlt );
	if ( !transl->write(copiedwvlt,*connptr) )
	{
	    connptr->rollback();
	    uirv.add( ioobj.phrCannotWriteObj() );
	}
    }

    delete connptr;
    return uirv;
}


static const char* sLength	= "Length";
static const char* sIndex	= "Index First Sample";
static const char* sSampRate	= "Sample Rate";

bool dgbWaveletTranslator::read( Wavelet* wv, Conn& conn )
{
    if ( !wv || !conn.forRead() || !conn.isStream() )	return false;

    ascistream astream( ((StreamConn&)conn).iStream() );
    if ( !astream.isOfFileType(mTranslGroupName(Wavelet)) )
	return false;

    int cidx = 0; Wavelet::ZType sr = (Wavelet::ZType)SI().zStep();
    int sz = 0;
    while ( !atEndOfSection( astream.next() ) )
    {
        if ( astream.hasKeyword( sLength ) )
	    sz = astream.getIValue();
        else if ( astream.hasKeyword( sIndex ) )
	    cidx = -1 * astream.getIValue();
        else if ( astream.hasKeyword(sKey::Name()) )
	    wv->setName( astream.value() );
        else if ( astream.hasKeyword( sSampRate ) )
	    sr = astream.getFValue() / SI().showZ2UserFactor();
    }
    if ( sz < 1 )
	return false;

    wv->reSize( sz );
    wv->setSampleRate( sr );
    wv->setCenterSample( cidx );

    TypeSet<Wavelet::ValueType> samps( sz, 0.f );
    for ( int idx=0; idx<sz; idx++ )
	astream.stream() >> samps[idx];
    wv->setSamples( samps );

    wv->trimPaddedZeros();
    return astream.isOK();
}


bool dgbWaveletTranslator::write( const Wavelet* inwv, Conn& conn )
{
    if ( !inwv || !conn.forWrite() || !conn.isStream() )
	return false;

    RefMan<Wavelet> wv = new Wavelet( *inwv );
    wv->trimPaddedZeros();

    ascostream astream( ((StreamConn&)conn).oStream() );
    const BufferString head( mTranslGroupName(Wavelet), " file" );
    if ( !astream.putHeader( head ) )
	return false;

    if ( !wv->name().isEmpty() )
	astream.put( sKey::Name(), wv->name() );
    astream.put( sLength, wv->size() );
    astream.put( sIndex, -wv->centerSample() );
    astream.put( IOPar::compKey(sSampRate,sKey::Unit()),
		 SI().zDomain().fileUnitStr() );
    astream.put( sSampRate, wv->sampleRate() * SI().zDomain().userFactor() );

    astream.newParagraph();
    TypeSet<Wavelet::ValueType> samps;
    wv->getSamples( samps );
    const Wavelet::size_type sz = samps.size();
    for ( int idx=0; idx<sz; idx++ )
	astream.stream() << samps[idx] << od_newline;

    astream.newParagraph();
    return astream.isOK();
}


Table::FormatDesc* WaveletAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Wavelet" );
    fd->headerinfos_ += new Table::TargetInfo( uiStrings::sSampleIntrvl(),
			FloatInpSpec(SI().zRange().step), Table::Required,
			PropertyRef::surveyZType() );
    fd->headerinfos_ += new Table::TargetInfo( tr("Center Sample"),
					    IntInpSpec(), Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( tr("Data Samples"), FloatInpSpec(),
					     Table::Required );
    return fd;
}


#define mErrRet(s) { if ( !s.isEmpty() ) errmsg_ = s; return 0; }

Wavelet* WaveletAscIO::get( od_istream& strm ) const
{
    if ( !getHdrVals(strm) )
	return 0;

    float sr = getFValue( 0 );
    if ( sr == 0 || mIsUdf(sr) )
	sr = SI().zStep();
    else if ( sr < 0 )
	sr = -sr;

    int centersmp = getIntValue( 1 );
    if ( !mIsUdf(centersmp) )
    {
	if ( centersmp <= 0 )
	    centersmp = -centersmp;
	else
	    centersmp--;
    }

    TypeSet<Wavelet::ValueType> samps;
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 )
	    mErrRet(uiString::empty())
	if ( ret == 0 )
	    break;

	Wavelet::ValueType val = getFValue( 0 );
	if ( !mIsUdf(val) )
	    samps += val;
    }

    if ( samps.isEmpty() )
	mErrRet( tr("No valid data samples found") )
    if ( mIsUdf(centersmp) || centersmp > samps.size() )
	centersmp = samps.size() / 2;

    Wavelet* ret = new Wavelet;
    ret->setSamples( samps );
    ret->setCenterSample( centersmp  );
    ret->setSampleRate( sr );
    ret->trimPaddedZeros();
    return ret;
}


bool WaveletAscIO::put( od_ostream& ) const
{
    errmsg_ = mINTERNAL( "TODO: WaveletAscIO::put not implemented" );
    return false;
}
