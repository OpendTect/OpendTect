/*+ * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 1995
 * FUNCTION : Seg-Y headers
-*/



#include "segyhdr.h"
#include "string2.h"
#include "survinfo.h"
#include "seisinfo.h"
#include "seispacketinfo.h"
#include "msgh.h"
#include "math2.h"
#include "envvars.h"
#include "timefun.h"
#include "od_iostream.h"
#include "odver.h"
#include "coordsystem.h"
#include "posimpexppars.h"


static const int cTxtHeadNrLines = 40;
static const int cTxtHeadCharsPerLine = 80;

bool& SEGY::TxtHeader::info2D()
{ mDefineStaticLocalObject( bool, is2d, = false ); return is2d; }


    static unsigned char segyhdre2a[256] = {
0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
0x80,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
0x20,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0x5B,0x2E,0x3C,0x28,0x2B,0x21,
0x26,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0x5D,0x24,0x2A,0x29,0x3B,0x5E,
0x2D,0x2F,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0x7C,0x2C,0x25,0x5F,0x3E,0x3F,
0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,0xC1,0xC2,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
0xC3,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,
0xCA,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
0xD1,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,
0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,
0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,
0x5C,0x9F,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
    };


static void Ebcdic2Ascii( unsigned char *chbuf, int len )
{
    int i;

    for ( i=0; i<len; i++ ) chbuf[i] = segyhdre2a[chbuf[i]];
}


static unsigned char segyhdra2e[256] = {
0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,0x16,0x05,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
0x40,0x4F,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0x4A,0xE0,0x5A,0x5F,0x6D,
0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xC0,0x6A,0xD0,0xA1,0x07,
0x20,0x21,0x22,0x23,0x24,0x15,0x06,0x17,0x28,0x29,0x2A,0x2B,0x2C,0x09,0x0A,0x1B,
0x30,0x31,0x1A,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3A,0x3B,0x04,0x14,0x3E,0xE1,
0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
0x58,0x59,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x70,0x71,0x72,0x73,0x74,0x75,
0x76,0x77,0x78,0x80,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x9A,0x9B,0x9C,0x9D,0x9E,
0x9F,0xA0,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xDA,0xDB,
0xDC,0xDD,0xDE,0xDF,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
    };


static void Ascii2Ebcdic( unsigned char *chbuf, int len )
{
    int i;

    for ( i=0; i<len; i++ ) chbuf[i] = segyhdra2e[chbuf[i]];
}


SEGY::TxtHeader::TxtHeader( int rev )
    : revision_(rev)
{
    clear();

    BufferString str( "Created using OpendTect ", GetFullODVersion() );
    str.add( " on " ).add( Time::getISOUTCDateTimeString() );
    putAt( 1, 6, 75, str );
    putAt( 2, 6, 75, BufferString("Survey: '", SI().name(),"'") );
    const auto inlrg = SI().inlRange();
    const auto crlrg = SI().crlRange();
    BinID bid = BinID( inlrg.start, crlrg.start );
    Coord coord = SI().transform( bid );
    coord.x_ = fabs(coord.x_); coord.y_ = fabs(coord.y_);
    if ( !mIsEqual(bid.inl(),coord.x_,mDefEps)
      || !mIsEqual(bid.crl(),coord.x_,mDefEps)
      || !mIsEqual(bid.inl(),coord.y_,mDefEps)
      || !mIsEqual(bid.crl(),coord.y_,mDefEps) )
    {
	putAt( 13, 6, 75, "Survey setup:" );
	coord = SI().transform( bid );
	str.set( bid.toString() ).add( " = " ).add( coord.toPrettyString() );
	putAt( 14, 6, 75, str );
	bid.crl() = crlrg.stop;
	coord = SI().transform( bid );
	str.set( bid.toString() ).add( " = " ).add( coord.toPrettyString() );
	putAt( 15, 6, 75, str );
	bid.inl() = inlrg.stop;
	bid.crl() = crlrg.start;
	coord = SI().transform( bid );
	str.set( bid.toString() ).add( " = " ).add( coord.toPrettyString() );
	putAt( 16, 6, 75, str );

	ConstRefMan<Coords::CoordSystem> coordsys = SI().getCoordSystem();
	if ( coordsys && coordsys->isWorthMentioning() )
	    putAt( 17, 6, 75, mFromUiStringTodo(coordsys->summary()) );
    }

    BufferString comments = SI().comments();
    char* lineptr = comments.getCStr();
    for ( int iln=20; lineptr && *lineptr && iln<35; iln++ )
    {
	char* nlptr = firstOcc( lineptr, '\n' );
	if ( nlptr )
	    *nlptr++ = '\0';
	putAt( iln, 6, 75, lineptr );
	lineptr = nlptr;
    }

    if ( !SI().zIsTime() )
    {
	str = "Depth survey: 1 SEG-Y millisec = 1 ";
	str += SI().fileZUnitString( false );
	putAt( 18, 6, 75, str );
    }
}


void SEGY::TxtHeader::clearText()
{
    OD::memSet( txt_, ' ', SegyTxtHeaderLength );
}


void SEGY::TxtHeader::setLineStarts()
{
    for ( int iln=0; iln<cTxtHeadNrLines; iln++ )
    {
	const int i80 = iln*cTxtHeadCharsPerLine;
	const BufferString lnrstr( iln < 9 ? "0" : "", iln+1 );
	txt_[i80] = 'C'; txt_[i80+1] = lnrstr[0]; txt_[i80+2] = lnrstr[1];
    }

    BufferString rvstr( "SEG Y REV" );
    rvstr += revision_;
    putAt( cTxtHeadNrLines-1, 6, 75, rvstr.buf() );
    putAt( cTxtHeadNrLines, 6, 75, "END TEXTUAL HEADER" );
}


void SEGY::TxtHeader::setAscii()
{ if ( !isAscii() ) Ebcdic2Ascii( txt_, SegyTxtHeaderLength ); }
void SEGY::TxtHeader::setEbcdic()
{ if ( isAscii() ) Ascii2Ebcdic( txt_, SegyTxtHeaderLength ); }


bool SEGY::TxtHeader::isAscii() const
{
    return txt_[0]!=0xC3 && txt_[0]!=0x83;
}


void SEGY::TxtHeader::setUserInfo( const char* infotxt )
{
    if ( !infotxt || !*infotxt ) return;

    BufferString buf;
    int lnr = 16;
    while ( lnr < 35 )
    {
	char* ptr = buf.getCStr();
	int idx = 0;
	while ( *infotxt && *infotxt != '\n' && ++idx < 75 )
	    *ptr++ = *infotxt++;
	*ptr = '\0';
	putAt( lnr, 5, cTxtHeadCharsPerLine, buf );

	if ( !*infotxt ) break;
	lnr++;
	infotxt++;
    }
}


void SEGY::TxtHeader::setPosInfo( const SEGY::TrcHeaderDef& thd )
{

    putAt( 7, 6, 75, "Positions according to the SEG-Y Rev. 1 standard." );
    if ( !info2D() ) return;

    if ( !thd.trnr_.isUdf() )
    {
	BufferString txt( "Trace number at byte: ", thd.trnr_.bytepos_+1 );
	if ( thd.trnr_.issmall_ ) txt += " (2-byte)";
	putAt( 8, 6, 6+txt.size(), txt.buf() );
    }

    if ( !thd.linename.isEmpty() )
    {
	putAt( 4, 6, 20, "Line name:" );
	putAt( 4, 20, 75, thd.linename );
    }
    if ( thd.pinfo )
    {
	BufferString txt( "Trace number range: ",
			  thd.pinfo->crlrg.start, " - " );
	txt += thd.pinfo->crlrg.stop;
	putAt( 5, 6, 75, txt );
    }

    putAt( 36, 6, 75, "2-D line" );
}


void SEGY::TxtHeader::setStartPos( float sp )
{
    if ( mIsZero(sp,Seis::cDefZEps()) ) return;

    BufferString txt( "First sample at:", sp );
    putAt( 10, 6, 75, txt );
    putAt( 11, 10, 75, "(in bytes 109 (+) and 105 (-) of the trace header)");
}


void SEGY::TxtHeader::getText( BufferString& bs ) const
{
    char buf[cTxtHeadCharsPerLine+1];
    getFrom( 1, 1, cTxtHeadCharsPerLine, buf );
    bs = buf;
    for ( int iln=2; iln<=cTxtHeadNrLines; iln++ )
    {
	bs += "\n";
	getFrom( iln, 1, cTxtHeadCharsPerLine, buf );
	bs += buf;
    }
}


void SEGY::TxtHeader::setText( const char* txt )
{
    clearText();

    BufferString bs( txt );
    char* ptr = bs.getCStr();
    for ( int iln=1; iln<=cTxtHeadNrLines; iln++ )
    {
	char* endptr = firstOcc( ptr, '\n' );
	if ( !endptr ) break;
	*endptr = '\0';

	putAt( iln, 1, cTxtHeadCharsPerLine, ptr );
	ptr = endptr + 1; if ( !*ptr ) break;
    }

    setLineStarts();
}


void SEGY::TxtHeader::getFrom( int line, int pos, int endpos, char* str ) const
{
    if ( !str ) return;

    int charnr = (line-1)*cTxtHeadCharsPerLine + pos - 1;
    if ( endpos > cTxtHeadCharsPerLine ) endpos = cTxtHeadCharsPerLine;
    int maxcharnr = (line-1)*cTxtHeadCharsPerLine + endpos;

    while ( iswspace(txt_[charnr]) && charnr < maxcharnr ) charnr++;
    while ( charnr < maxcharnr ) *str++ = txt_[charnr++];
    *str = '\0';
    removeTrailingBlanks( str );
}


void SEGY::TxtHeader::putAt( int line, int pos, int endpos, const char* str )
{
    if ( !str || !*str ) return;

    int charnr = (line-1)*cTxtHeadCharsPerLine + pos - 1;
    if ( endpos > cTxtHeadCharsPerLine ) endpos = cTxtHeadCharsPerLine;
    int maxcharnr = (line-1)*cTxtHeadCharsPerLine + endpos;

    while ( charnr < maxcharnr && *str )
    {
	txt_[charnr] = *str;
	charnr++; str++;
    }
}


void SEGY::TxtHeader::dump( od_ostream& stream ) const
{
    BufferString buf; getText( buf );
    stream << buf << od_endl;
}


SEGY::BinHeader::BinHeader()
	: needswap_(false)
	, forwrite_(false)
{
}


void SEGY::BinHeader::setForWrite()
{
    forwrite_ = true;
    needswap_ = false;
    OD::memZero( buf_, SegyBinHeaderLength );
    setEntryVal( EntryMFeet(), SI().xyInFeet() ? 2 : 1 );
    setEntryVal( EntryRevCode(), 256 );
    setEntryVal( EntryRevCode()+1, 1 );
}


void SEGY::BinHeader::setInput( const void* inp, bool needswap )
{
    forwrite_ = false;
    needswap_ = needswap;
    OD::memCopy( buf_, inp, SegyBinHeaderLength );
}


void SEGY::BinHeader::guessIsSwapped()
{
    needswap_ = false;
    int ns = nrSamples(), fmt = format();
    if ( ns < 0 || fmt > 100 || fmt < 0 )
    {
	needswap_ = true;
	ns = nrSamples(); fmt = format();
	if ( ns < 0 || fmt < 1 || fmt > 8 )
	    needswap_ = false;
    }
}


void SEGY::BinHeader::unSwap()
{
    if ( needswap_ )
    {
	hdrDef().swapValues( buf_ );
	needswap_ = false;
    }
}


DataCharacteristics SEGY::BinHeader::getDataChar( int nf, bool swpd )
{
    DataCharacteristics dc( true, true, BinDataDesc::N4,
			DataCharacteristics::Ibm,
			swpd ? !__islittle__ : __islittle__ );

    switch ( nf )
    {
    case 3:
        dc.setNrBytes( 2 );
    case 2:
    break;
    case 8:
        dc.setNrBytes( 1 );
    break;
    case 5:
	dc.fmt_ = DataCharacteristics::Ieee;
	dc.setInteger( false );
	dc.littleendian_ = swpd;
    break;
    default:
	dc.setInteger( false );
    break;
    }

    return dc;
}



const SEGY::HdrDef& SEGY::BinHeader::hdrDef()
{
    mDefineStaticLocalObject( PtrMan<SEGY::HdrDef>, bindef, = 0 );
    if ( bindef ) return *(bindef.ptr());

    bindef = new SEGY::HdrDef( true );
    if ( !bindef )
    {
	pFreeFnErrMsg( "Could not instantiate SEG-Y Binary Header definition" );
    }

    return *(bindef.ptr());
}


float SEGY::BinHeader::sampleRate( bool isdepth ) const
{
    float sr = entryVal( EntryDt() ) * 0.001f;
    if ( !isdepth )
	sr *= 0.001;
    return sr;
}


int SEGY::BinHeader::revision() const
{
    const int nr = entryVal( EntryRevCode() );
    if ( nr == 1 || nr == 256 )
	return 1;
    else if ( nr == 2 || nr == 512 )
	return 2;
    return 0;
}


int SEGY::BinHeader::skipRev1Stanzas( od_istream& strm )
{
    // Never seen any file with these abominations, so we'll only support
    // them if a user explicitly tells us to.
    mDefineStaticLocalObject( bool, support_stanzas,
			= GetEnvVarYN("OD_SEIS_SEGY_REV1_STANZAS") );
    if ( !support_stanzas )
	return 0;

    int nrstzs = entryVal( EntryRevCode() + 2 );
    if ( nrstzs > 0 && nrstzs < 100 )
	strm.ignore( nrstzs * SegyTxtHeaderLength );
    else
	nrstzs = 0;

    return nrstzs;
}


void SEGY::BinHeader::setSampleRate( float sr, bool isdepth )
{
    sr *= 1000;
    if ( !isdepth )
	sr *= 1000;
    setEntryVal( EntryDt(), mNINT32(sr) );
}


void SEGY::BinHeader::dump( od_ostream& strm ) const
{
    const HdrDef& hdef = hdrDef();
    strm << "Field\tByte\tValue\tDescription\n\n";
    for ( int idx=0; idx<hdef.size() ; idx++ )
    {
	const HdrEntry& he = *hdef[idx];
	const int value = he.getValue( buf_, needswap_ );
	if ( !value ) continue;

	strm << he.name() << od_tab << (int)(he.bytepos_+1)
	     << od_tab << value << od_tab << he.description() << od_newline;
    }
    strm << od_endl;
}


SEGY::TrcHeader::TrcHeader( unsigned char* b, const SEGY::TrcHeaderDef& hd,
				bool isrev0, bool ismine )
    : buf_(b)
    , mybuf_(ismine)
    , hdef_(hd)
    , needswap_(false)
    , isrev0_(isrev0)
    , seqnr_(1)
    , lineseqnr_(1)
    , previnl_(-1)
    , isusable(true)
    , nonrectcoords(false)
{
}


SEGY::TrcHeader::~TrcHeader()
{
    if ( mybuf_ )
	delete [] buf_;
}


SEGY::TrcHeader& SEGY::TrcHeader::operator =( const SEGY::TrcHeader& oth )
{
    if ( this != &oth )
    {
	needswap_ = oth.needswap_;
	isrev0_ = oth.isrev0_;
	seqnr_ = oth.seqnr_;
	lineseqnr_ = oth.lineseqnr_;
	previnl_ = oth.previnl_;
	isusable = oth.isusable;
	nonrectcoords = oth.nonrectcoords;

	if ( mybuf_ )
	    { delete [] buf_; buf_ = 0; }
	mybuf_ = oth.mybuf_;
	if ( !mybuf_ )
	    buf_ = oth.buf_;
	else if ( oth.buf_ )
	{

	    buf_ = new unsigned char [ SegyTrcHeaderLength ];
	    OD::memCopy( buf_, oth.buf_, SegyTrcHeaderLength );
	}
    }
    return *this;
}


const SEGY::HdrDef& SEGY::TrcHeader::hdrDef()
{
    mDefineStaticLocalObject( PtrMan<SEGY::HdrDef>, trcdef, = 0 );
    if ( trcdef ) return *(trcdef.ptr());

    trcdef = new SEGY::HdrDef( false );
    if ( !trcdef )
    {
	pFreeFnErrMsg( "Could not instantiate SEG-Y Trace Header definition" );
    }

    return *(trcdef.ptr());
}


void SEGY::TrcHeader::initRead()
{
    const short trid = (short)entryVal( EntryTrid() );
    isusable = trid < 2 || trid > 10;

    const short counit = (short)entryVal( EntryCoUnit() );
    nonrectcoords = counit > 1 && counit < 5;
}


unsigned short SEGY::TrcHeader::nrSamples() const
{
    return (unsigned short)entryVal( EntryNs() );
}


void SEGY::TrcHeader::putSampling( SamplingData<float> sdin, unsigned short ns )
{
    SamplingData<float> sd( sdin );
    mPIEPAdj(Z,sd.start,false); mPIEPAdj(Z,sd.step,false);

    const float zfac = mCast( float, SI().zDomain().userFactor() );
    float drt = sd.start * zfac;
    short delrt = (short)mNINT32(drt);
    setEntryVal( EntryLagA(), -delrt ); // For HRS and Petrel
    setEntryVal( EntryDelRt(), delrt );
    int sr_us = (int)( sd.step * 1e3 * zfac + .5 );
    setEntryVal( EntryDt(), sr_us );
    if ( ns != 0 )
	setEntryVal( EntryNs(), ns );
    else
	setEntryVal( EntryNs(), SI().zRange().nrSteps() + 1 );
}


void SEGY::TrcHeader::putRev1Flds( const SeisTrcInfo& ti ) const
{
    Coord crd( ti.coord_ ); mPIEPAdj(Coord,crd,false);
    const int icx = mNINT32(crd.x_*10); const int icy = mNINT32(crd.y_*10);
    setEntryVal( EntryXcdp(), icx );
    setEntryVal( EntryYcdp(), icy );
    BinID bid( ti.binID() ); mPIEPAdj(BinID,bid,false);
    setEntryVal( EntryInline(), bid.inl() );
    setEntryVal( EntryCrossline(), bid.crl() );
    int tnr = ti.trcNr(); mPIEPAdj(TrcNr,tnr,false);
    if ( ti.refnr_ != 0 && !mIsUdf(ti.refnr_) )
    {
	tnr = mNINT32(ti.refnr_*10);
	setEntryVal( EntrySPscale(), -10 );
    }
    setEntryVal( EntrySP(), tnr );
}


void SEGY::TrcHeader::use( const SeisTrcInfo& ti )
{
    if ( isrev0_ ) // For rev0, we initially fill with Rev 1 defaults
	putRev1Flds( ti );

    setEntryVal( EntryTrid(), 1 );
    setEntryVal( EntryDUse(), 1 );
    setEntryVal( EntryCoUnit(), 1 );

    const bool is2d = SEGY::TxtHeader::info2D();
    if ( !is2d && ti.lineNr() != previnl_ )
	lineseqnr_ = 1;
    previnl_ = ti.lineNr();
    int nr2put = is2d ? seqnr_ : lineseqnr_;
    setEntryVal( EntryTracl(), nr2put );
    setEntryVal( EntryTracr(), seqnr_ );
    seqnr_++; lineseqnr_++;
    nr2put = ti.trcNr();
    if ( is2d )
	mPIEPAdj(TrcNr,nr2put,false);
    else
	mPIEPAdj(Crl,nr2put,false);
    setEntryVal( EntryCdp(), nr2put );
    Coord crd( ti.coord_ );
    if ( mIsUdf(crd.x_) ) crd.x_ = crd.y_ = 0;
    mPIEPAdj(Coord,crd,false);
    int iscalco, icx, icy;
    mDefineStaticLocalObject( const bool, noscalco,
			      = GetEnvVarYN("OD_SEGY_NO_SCALCO") );
    if ( noscalco )
	{ iscalco = 1; icx = mNINT32(crd.x_); icy = mNINT32(crd.y_); }
    else
	{ iscalco = -10; icx = mNINT32(crd.x_*10); icy = mNINT32(crd.y_*10); }

    setEntryVal( EntryScalco(), iscalco );
    hdef_.xcoord_.putValue( buf_, icx );
    hdef_.ycoord_.putValue( buf_, icy );

    BinID bid( ti.binID() ); mPIEPAdj(BinID,bid,false);
    hdef_.inl_.putValue( buf_, ti.lineNr() );
    hdef_.crl_.putValue( buf_, ti.trcNr() );
    int intval = ti.trcNr();
    if ( is2d )
    {
	mPIEPAdj(TrcNr,intval,false);
	hdef_.trnr_.putValue( buf_, intval );
    }

    float tioffs = ti.offset_; mPIEPAdj(Offset,tioffs,false);
    intval = mNINT32( tioffs );
    hdef_.offs_.putValue( buf_, intval );
    intval = mNINT32( ti.azimuth_ * 360 / M_PI );
    hdef_.azim_.putValue( buf_, intval );

    const float zfac = mCast( float, SI().zDomain().userFactor() );
#define mSetScaledMemb(nm,fac) \
    if ( !mIsUdf(ti.nm##_) ) \
    { intval = mNINT32(ti.nm##_*fac); hdef_.nm##_.putValue( buf_, intval ); }

    mSetScaledMemb(pick,zfac)
    mSetScaledMemb(refnr,10)

    // Absolute priority, therefore possibly overwriting previous
    putSampling( ti.sampling_, 0 ); // 0=ns must be set elsewhere

    if ( !isrev0_ ) // Now this overrules everything
	putRev1Flds( ti );
}


float SEGY::TrcHeader::postScale( int numbfmt ) const
{
    // There seems to be software (Paradigm?) putting this on byte 189
    // Then we'd expect this to be 4 byte. Sigh. How far do we need to go
    // to support crap from SEG-Y vandals?
    HdrEntry he( *hdrDef()[EntryTrwf()] );
    mDefineStaticLocalObject( bool, postscale_byte_established, = false );
    mDefineStaticLocalObject( int, bnr, = he.bytepos_ );
    mDefineStaticLocalObject( bool, smallbtsz, = he.issmall_ );
    if ( !postscale_byte_established )
    {
	postscale_byte_established = true;
	bnr = GetEnvVarIVal( "OD_SEGY_TRCSCALE_BYTE", he.bytepos_ );
	if ( bnr > 0 && bnr < 255 )
	    smallbtsz = !GetEnvVarYN( "OD_SEGY_TRCSCALE_4BYTE" );
    }

    he.bytepos_ = (HdrEntry::BytePos)bnr;
    he.issmall_ = smallbtsz;
    const short trwf = (short)he.getValue( buf_, needswap_ );
    if ( trwf == 0 || trwf > 50 || trwf < -50 ) return 1;

    return Math::IntPowerOf( ((float)2), trwf );
}


void SEGY::TrcHeader::getRev1Flds( SeisTrcInfo& ti, bool is2d ) const
{
    ti.coord_.x_ = entryVal( EntryXcdp() );
    ti.coord_.y_ = entryVal( EntryYcdp() );
    BinID tibid( entryVal( EntryInline() ), entryVal( EntryCrossline() ) );
    if ( is2d )
	ti.setPos( Pos::GeomID(0), EntryTracr() );
    else
    {
	mPIEPAdj(BinID,tibid,true);
	ti.setPos( tibid );
    }

    ti.refnr_ = entryfVal( EntrySP() );
    if ( !isrev0_ )
    {
	const short scalnr = (short)entryVal( EntrySPscale() );
	if ( scalnr )
	    ti.refnr_ *= (scalnr > 0 ? scalnr : -1.0f/scalnr);
    }

    mPIEPAdj(Coord,ti.coord_,true);
}


void SEGY::TrcHeader::fill( SeisTrcInfo& ti, bool is2d, float extcoordsc ) const
{
    if ( mIsZero(extcoordsc,1e-8)
	    && !GetEnvVarYN("OD_ALLOW_ZERO_COORD_SCALING") )
    {
	mDefineStaticLocalObject( bool, warningdone, = false );
	if ( !warningdone )
	{
	    ErrMsg( "Replacing requested zero scaling with 1" );
	    warningdone = true;
	}
	extcoordsc = 1;
    }

    if ( isrev0_ )
	getRev1Flds( ti, is2d ); // if rev 0, start with rev 1 as default

    const float zfac = 1.0f / SI().zDomain().userFactor();
    short delrt = mCast( short, entryVal( EntryDelRt() ) );
    if ( delrt == 0 )
    {
	delrt = - (short)entryVal( EntryLagA() ); // HRS and Petrel
	mDefineStaticLocalObject( const bool, smt_bad_laga,
				  = GetEnvVarYN("OD_SEGY_BAD_LAGA") );
	if ( smt_bad_laga )
	    delrt = -delrt;
	float startz = delrt * zfac;
	if ( startz < -5000 || startz > 10000 )
	    delrt = 0;
    }
    ti.sampling_.start = delrt * zfac;
    ti.sampling_.step = entryVal(EntryDt()) * zfac * 0.001f;
    mPIEPAdj(Z,ti.sampling_.start,true);
    mPIEPAdj(Z,ti.sampling_.step,true);

    ti.pick_ = ti.refnr_ = mUdf(float);

#define mGetFloatVal(memb,fac) \
    if ( !hdef_.memb##_.isUdf() ) \
    {\
	float ftemp = hdef_.memb##_.getfValue(buf_,needswap_); \
	ti.memb##_ = ftemp * fac; \
    }
    mGetFloatVal(pick,0.001f);
    mPIEPAdj(Z,ti.pick_,true);
    float nrfac = 1.f;
    short scalnr = (short)entryVal( EntrySPscale() );
    if ( scalnr == -10 || scalnr == -100 || scalnr == -1000 )
	nrfac = 1.f / ((float)(-scalnr));
    mGetFloatVal(refnr,nrfac);

    ti.coord_.x_ = ti.coord_.y_ = 0;
    ti.coord_.x_ = hdef_.xcoord_.getValue(buf_,needswap_);
    ti.coord_.y_ = hdef_.ycoord_.getValue(buf_,needswap_);
    ti.offset_ = hdef_.offs_.getfValue(buf_,needswap_);
    if ( ti.offset_ < 0 ) ti.offset_ = -ti.offset_;
    ti.azimuth_ = hdef_.azim_.getfValue(buf_,needswap_);
    ti.azimuth_ *= M_PI / 360;

    if ( !is2d )
    {
	BinID tibid( hdef_.inl_.getValue(buf_,needswap_),
		     hdef_.crl_.getValue(buf_,needswap_) );
	mPIEPAdj(BinID,tibid,true);
	ti.setPos( tibid );
    }

    if ( !isrev0_ )
	getRev1Flds( ti, is2d ); // if >= rev 1, then those fields are holy

    if ( is2d )
    {
	int trcnr = ti.trcNr();
	if ( hdef_.trnr_.bytepos_ >= 0 )
	    trcnr = hdef_.trnr_.getValue(buf_,needswap_);
	else
	{
	    //Trick to set trace number to sequence number when no trnr_ defined
	    mDefineStaticLocalObject( Threads::Atomic<int>, seqnr, (0) );
	    if ( hdef_.trnr_.bytepos_ == -5 )
		seqnr++;
	    else
	    {
		seqnr = 1;
		const_cast<SEGY::TrcHeaderDef&>(hdef_).trnr_.bytepos_ = -5;
	    }

	    trcnr = seqnr;
	}

	mPIEPAdj(TrcNr,trcnr,true);
	ti.setPos( Pos::GeomID(0), trcnr );
    }

    const double scale = getCoordScale( extcoordsc );
    ti.coord_.x_ *= scale; ti.coord_.y_ *= scale;
    mPIEPAdj(Coord,ti.coord_,true);
}


double SEGY::TrcHeader::getCoordScale( float extcoordsc ) const
{
    if ( !mIsUdf(extcoordsc) ) return extcoordsc;
    const short scalco = mCast( short, entryVal( EntryScalco() ) );
    return scalco ? (scalco > 0 ? scalco : -1./scalco) : 1;
}


Coord SEGY::TrcHeader::getCoord( bool rcv, float extcoordsc ) const
{
    double scale = getCoordScale( extcoordsc );
    Coord ret(	entryVal( rcv?EntryGx():EntrySx() ),
		entryVal( rcv?EntryGy():EntrySy() ) );
    ret.x_ *= scale; ret.y_ *= scale;
    return ret;
}
