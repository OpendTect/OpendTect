/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jul 2016
-*/


#include "waveletio.h"
#include "wavelet.h"
#include "uistrings.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "separstr.h"
#include "ascstream.h"
#include "survinfo.h"
#include "tabledef.h"

defineTranslatorGroup(Wavelet,"Wavelet");
defineTranslator(dgb,Wavelet,mDGBKey);
mDefSimpleTranslatorSelector(Wavelet);

uiString WaveletTranslatorGroup::sTypeName( int num )
{ return uiStrings::sWavelet(num); }

static const char* sKeyScaled = "Scaled";


IOObj* Wavelet::getIOObj( const char* waveletnm )
{
    if ( !waveletnm || !*waveletnm )
	return 0;

    IOObjContext ctxt( mIOObjContext(Wavelet) );
    IOM().to( ctxt.getSelKey() );
    return IOM().getLocal( waveletnm, mTranslGroupName(Wavelet) );
}


Wavelet* Wavelet::get( const IOObj* ioobj )
{
    if ( !ioobj ) return 0;
    PtrMan<WaveletTranslator> tr =
		(WaveletTranslator*)ioobj->createTranslator();
    if ( !tr ) return 0;
    Wavelet* newwv = 0;

    Conn* connptr = ioobj->getConn( Conn::Read );
    if ( !connptr || connptr->isBad() )
	ErrMsg( "Cannot open Wavelet file" );
    else
    {
	newwv = new Wavelet;
	if ( tr->read( newwv, *connptr ) )
	    newwv->setName( ioobj->name() );
	else
	{
	    ErrMsg( "Problem reading Wavelet from file (format error?)" );
	    delete newwv;
	    newwv = 0;
	}
    }

    delete connptr;
    return newwv;
}


bool Wavelet::put( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    PtrMan<WaveletTranslator> trans =
        (WaveletTranslator*)ioobj->createTranslator();
    if ( !trans ) return false;
    bool retval = false;

    Conn* connptr = ioobj->getConn( Conn::Write );
    if ( connptr && !connptr->isBad() )
    {
	if ( trans->write(this,*connptr) )
	    retval = true;
	else
	{
	    connptr->rollback();
	    ErrMsg( "Cannot write Wavelet" );
	}
    }
    else
	ErrMsg( "Cannot open Wavelet file for write" );

    delete connptr;
    return retval;
}


static void markWaveletScaled( const MultiID& id, const char* val )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;
    ioobj->pars().set( sKeyScaled, val );
    IOM().commitChanges( *ioobj );
}


void Wavelet::markScaled( const MultiID& id )
{
    markWaveletScaled( id, "External" );
}


void Wavelet::markScaled( const MultiID& id, const MultiID& orgid,
			  const MultiID& horid, const MultiID& seisid,
			  const char* lvlnm )
{
    FileMultiString fms( orgid.buf() );
    fms += horid; fms += seisid; fms += lvlnm;
    markWaveletScaled( id, fms );
}


static BufferString waveletScaleStr( const MultiID& id )
{
    BufferString ret;
    IOObj* ioobj = IOM().get( id );
    if ( !ioobj ) return ret;
    ret = ioobj->pars().find( sKeyScaled );
    delete ioobj;
    return ret;
}


bool Wavelet::isScaled( const MultiID& id )
{
    return !waveletScaleStr(id).isEmpty();
}


bool Wavelet::isScaled( const MultiID& id, MultiID& orgid, MultiID& horid,
			MultiID& seisid, BufferString& lvlnm )
{
    BufferString val( waveletScaleStr(id) );
    if ( val.isEmpty() ) return false;
    FileMultiString fms( val );
    const int fmssz = fms.size();
    if ( fmssz < 3 )
	{ orgid = "0"; return true; }

    orgid = fms[0]; horid = fms[1]; seisid = fms[2]; lvlnm = fms[3];
    return true;
}


mDefSimpleTranslatorioContext(Wavelet,Seis)


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

    Wavelet wv( *inwv );
    wv.trimPaddedZeros();

    ascostream astream( ((StreamConn&)conn).oStream() );
    const BufferString head( mTranslGroupName(Wavelet), " file" );
    if ( !astream.putHeader( head ) )
	return false;

    if ( !wv.name().isEmpty() )
	astream.put( sKey::Name(), wv.name() );
    astream.put( sLength, wv.size() );
    astream.put( sIndex, -wv.centerSample() );
    astream.put( IOPar::compKey(sSampRate,sKey::Unit()),
		 SI().zDomain().unitStr() );
    astream.put( sSampRate, wv.sampleRate() * SI().zDomain().userFactor() );

    astream.newParagraph();
    TypeSet<Wavelet::ValueType> samps;
    wv.getSamples( samps );
    const Wavelet::size_type sz = samps.size();
    for ( int idx=0; idx<sz; idx++ )
	astream.stream() << samps[idx] << od_newline;

    astream.newParagraph();
    return astream.isOK();
}


Table::FormatDesc* WaveletAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Wavelet" );
    fd->headerinfos_ += new Table::TargetInfo( "Sample interval",
			FloatInpSpec(SI().zRange(true).step), Table::Required,
			PropertyRef::surveyZType() );
    fd->headerinfos_ += new Table::TargetInfo( "Center sample",
						IntInpSpec(), Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( "Data samples", FloatInpSpec(),
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
	    mErrRet(uiString::emptyString())
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

    Wavelet* ret = new Wavelet( "" );
    ret->setSamples( samps );
    ret->setCenterSample( centersmp  );
    ret->setSampleRate( sr );
    ret->trimPaddedZeros();
    return ret;
}


bool WaveletAscIO::put( od_ostream& ) const
{
    errmsg_ = tr("TODO: WaveletAscIO::put not implemented");
    return false;
}
