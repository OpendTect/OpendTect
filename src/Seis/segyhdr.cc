/*+ * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 1995
 * FUNCTION : Seg-Y headers
-*/

#include "segyhdr.h"

#include "envvars.h"
#include "linekey.h"
#include "math2.h"
#include "msgh.h"
#include "od_iostream.h"
#include "odver.h"
#include "posimpexppars.h"
#include "seisinfo.h"
#include "seispacketinfo.h"
#include "settings.h"
#include "string2.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "timefun.h"
#include "trckeyzsampling.h"


static const int sTxtHeadNrLines = 40;
static const int sTxtHeadCharsPerLine = 80;
static const int sDefStartPos = 7;

bool& SEGY::TxtHeader::info2D()
{ mDefineStaticLocalObject( bool, is2d, = false ); return is2d; }

bool& SEGY::TxtHeader::isPS()
{ mDefineStaticLocalObject( bool, isps, = false ); return isps; }

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


static const char* sSeparatorLine()
{
    return "----------------------------------------"
	   "------------------------------------";
}


SEGY::TxtHeader::TxtHeader( int rev )
    : revision_(rev)
{
    clear();
}


void SEGY::TxtHeader::clearText()
{
    OD::memSet( txt_, ' ', SegyTxtHeaderLength );
}


int SEGY::TxtHeader::setInfo( const char* datanm,
			      const Coords::CoordSystem* crs,
			      const TrcHeaderDef& def )
{
    int lnr = setGeneralInfo( datanm );
    lnr = setSurveySetupInfo( ++lnr, crs );
    lnr = setPosInfo( ++lnr, def );
    return lnr;
}


int SEGY::TxtHeader::setGeneralInfo( const char* datanm )
{
    int lnr = 0;
    BufferString str;
    str = "SEG-Y exported from OpendTect "; str += GetFullODVersion();
    str += " at "; str += Time::getDateTimeString();
    putAt( ++lnr, sDefStartPos, 75, str );
    putAt( ++lnr, sDefStartPos, 75, "Survey:" );
    putAt( lnr, 21, 75, SI().name() );

    const StringPair sp( datanm );
    putAt( ++lnr, sDefStartPos, 75, "Data name:" );
    putAt( lnr, 21, 75, sp.first() );
    if ( sp.hasSecond() )
    {
	putAt( ++lnr, sDefStartPos, 75, "Component:" );
	putAt( lnr, 21, 75, sp.second() );
    }

    if ( info2D() )
	putAt( ++lnr, sDefStartPos, 75, "Line name:" );

    putAt( ++lnr, sDefStartPos-1, 75, sSeparatorLine() );
    return lnr;
}


static BufferString pointTxt( int idx, const BinID& bid,
			      const Coords::CoordSystem* crs )
{
    const bool needsconversion = crs && !(*crs == *SI().getCoordSystem());
    Coord crd = SI().transform( bid );
    if ( needsconversion )
	crd = crs->convertFrom( crd, *SI().getCoordSystem() );
    BufferString txt( "Corner ", idx, ":  " );
    txt.add( "X: " ).add( crd.x, 2 ).add( "  Y: " ).add( crd.y, 2 );
    txt.add( "  IL: " ).add( bid.inl() ).add( "  XL: " ).add( bid.crl() );
    return txt;
}


int SEGY::TxtHeader::setSurveySetupInfo( int firstlnr,
					 const Coords::CoordSystem* tocrs )
{
    int lnr = firstlnr-1;
    const Coords::CoordSystem* crs =
		tocrs ? tocrs : SI().getCoordSystem().ptr();

    BufferString str;
    if ( !info2D() )
    {
	BinID bid = SI().sampling(false).hsamp_.start_;
	putAt( ++lnr, sDefStartPos, 75, pointTxt(1,bid,crs).buf() );

	bid.crl() = SI().sampling(false).hsamp_.stop_.crl();
	putAt( ++lnr, sDefStartPos, 75, pointTxt(2,bid,crs).buf() );

	bid = SI().sampling(false).hsamp_.stop_;
	putAt( ++lnr, sDefStartPos, 75, pointTxt(3,bid,crs).buf() );

	bid.crl() = SI().sampling(false).hsamp_.start_.crl();
	putAt( ++lnr, sDefStartPos, 75, pointTxt(4,bid,crs).buf() );

	const float inldist = SI().inlDistance(), crldist = SI().crlDistance();
	str.set("Bin size:  ").add(inldist,2).add(" x ").add(crldist,2)
	   .addSpace().add( SI().getXYUnitString() );
	putAt( ++lnr, sDefStartPos, 75, str );
    }

    if ( crs && crs->isProjection() )
	str = crs->summary();
    else
	str = "CRS: Not set";

    putAt( ++lnr, sDefStartPos, 75, str );

    putAt( ++lnr, sDefStartPos-1, 75, sSeparatorLine() );
    return lnr;
}


int SEGY::TxtHeader::setPosInfo( int firstlinenr, const SEGY::TrcHeaderDef& thd)
{
    int zrglinenr = -1;
    int xlinenr = -1;
    int ylinenr = -1;
    BufferString txt;
    short bytepos;
    SeisPacketInfo* info = thd.pinfo;
    int lnr = firstlinenr;
    if ( info2D() )
    {
	putAt( lnr, sDefStartPos, 20, BufferString(sKey::TraceNr(),":") );
	txt.set(info->crlrg.start).add("-").add(info->crlrg.stop);
	putAt( lnr, 21, 32, txt );
	putAt( lnr, 33, 75, BufferString("inc: ",info->crlrg.step) );

	Interval<float> sprg( 0, 0 );
	const Survey::Geometry2D& geom2d =
			Survey::GM().get2D( info->inlrg.start );
	if ( !geom2d.spnrs().isEmpty() )
	    sprg.set( geom2d.spnrs().first(), geom2d.spnrs().last() );
	putAt( ++lnr, sDefStartPos, 20, BufferString(sKey::Shotpoint(),":") );
	txt.set(sprg.start).add("-").add(sprg.stop);
	putAt( lnr, 21, 32, txt );

	zrglinenr = ++lnr;

	putAt( ++lnr, sDefStartPos-1, 75, sSeparatorLine() );

	bytepos = thd.trnr_.bytepos_;
	putAt( ++lnr, sDefStartPos, 25,
			BufferString(sKey::TraceNr()," byte:") );
	txt.set(bytepos+1).add("-").add(bytepos+4);
	putAt( lnr, 30, 75, txt.buf() );

	bytepos = thd.refnr_.bytepos_;
	putAt( ++lnr, sDefStartPos, 25,
			BufferString(sKey::Shotpoint()," byte:"));
	txt.set(bytepos+1).add("-").add(bytepos+4);
	putAt( lnr, 30, 75, txt.buf() );

	xlinenr = ++lnr;
	ylinenr = ++lnr;
    }
    else
    {
	putAt( lnr, sDefStartPos, 19, BufferString(sKey::Inline(),":") );
	txt.set(info->inlrg.start).add("-").add(info->inlrg.stop);
	putAt( lnr, 21, 31, txt );
	putAt( lnr, 33, 75, BufferString("inc: ",info->inlrg.step) );

	putAt( ++lnr, sDefStartPos, 19, BufferString(sKey::Crossline(),":") );
	txt.set(info->crlrg.start).add("-").add(info->crlrg.stop);
	putAt( lnr, 21, 31, txt );
	putAt( lnr, 33, 75, BufferString("inc: ",info->crlrg.step) );

	zrglinenr = ++lnr;

	putAt( ++lnr, sDefStartPos-1, 75, sSeparatorLine() );

	bytepos = thd.inl_.bytepos_;
	putAt( ++lnr, sDefStartPos, 25,
				BufferString(sKey::Inline()," byte:") );
	txt.set(bytepos+1).add("-").add(bytepos+4);
	putAt( lnr, 30, 75, txt.buf() );

	bytepos = thd.crl_.bytepos_;
	putAt( ++lnr, sDefStartPos, 25,
				BufferString(sKey::Crossline()," byte:") );
	txt.set(bytepos+1).add("-").add(bytepos+4);
	putAt( lnr, 30, 75, txt.buf() );

	xlinenr = ++lnr;
	ylinenr = ++lnr;
    }

    if ( isPS() )
    {
	bytepos = thd.offs_.bytepos_;
	putAt( ++lnr, sDefStartPos, 25,
				BufferString(sKey::Offset()," byte:") );
	txt.set(bytepos+1).add("-").add(bytepos+4);
	putAt( lnr, 30, 75, txt.buf() );

	bytepos = thd.azim_.bytepos_;
	putAt( ++lnr, sDefStartPos, 25,
				BufferString(sKey::Azimuth()," byte:") );
	txt.set(bytepos+1).add("-").add(bytepos+2);
	putAt( lnr, 30, 75, txt.buf() );
    }

    putAt( ++lnr, sDefStartPos-1, 75, sSeparatorLine() );

// Common entries
    StepInterval<float> zrg = info->zrg;
    zrg.scale( SI().zDomain().userFactor() );
    const int nrdec = SI().nrZDecimals();
    BufferString key = sKey::Z();
    key.addSpace().add( SI().getZUnitString() ).add(":");
    putAt( zrglinenr, sDefStartPos, 20, key );
    txt.set(zrg.start,nrdec).add("-").add(zrg.stop,nrdec);
    putAt( zrglinenr, 21, 32, txt );
    putAt( zrglinenr, 33, 75, BufferString("inc: ",zrg.step) );

    bytepos = thd.xcoord_.bytepos_;
    putAt( xlinenr, sDefStartPos, 25, BufferString(sKey::XCoord()," byte:") );
    txt.set(bytepos+1).add("-").add(bytepos+4);
    putAt( xlinenr, 30, 75, txt.buf() );

    bytepos = thd.ycoord_.bytepos_;
    putAt( ylinenr, sDefStartPos, 25, BufferString(sKey::YCoord()," byte:") );
    txt.set(bytepos+1).add("-").add(bytepos+4);
    putAt( ylinenr, 30, 75, txt.buf() );

    return lnr;
}


void SEGY::TxtHeader::setUserInfo( int firstlinenr, const char* infotxt )
{
    if ( !infotxt || !*infotxt )
	return;

    BufferString buf;
    int lnr = firstlinenr;
    while ( lnr < 35 )
    {
	char* ptr = buf.getCStr();
	int idx = 0;
	while ( *infotxt && *infotxt != '\n' && ++idx < 75 )
	    *ptr++ = *infotxt++;
	*ptr = '\0';
	putAt( lnr, 5, sTxtHeadCharsPerLine, buf );

	if ( !*infotxt ) break;
	lnr++;
	infotxt++;
    }
}


void SEGY::TxtHeader::setGeomID( const Pos::GeomID& geomid )
{
    if ( !info2D() )
	return;

    const BufferString linenm = Survey::GM().getName( geomid );
    putAt( 4, 21, 75, linenm );
}


void SEGY::TxtHeader::setLineStarts()
{
    for ( int iln=0; iln<sTxtHeadNrLines; iln++ )
    {
	const int i80 = iln*sTxtHeadCharsPerLine;
	const BufferString lnrstr( iln < 9 ? "0" : "", iln+1 );
	txt_[i80] = 'C'; txt_[i80+1] = lnrstr[0]; txt_[i80+2] = lnrstr[1];
    }

    BufferString rvstr( "SEG-Y REV" );
    rvstr += revision_;
    putAt( sTxtHeadNrLines-1, sDefStartPos, 75, rvstr.buf() );
    putAt( sTxtHeadNrLines, sDefStartPos, 75, "END TEXTUAL HEADER" );
}


void SEGY::TxtHeader::setAscii()
{
    if ( !isAscii() )
	Ebcdic2Ascii( txt_, SegyTxtHeaderLength );
}


void SEGY::TxtHeader::setEbcdic()
{
    if ( isAscii() )
	Ascii2Ebcdic( txt_, SegyTxtHeaderLength );
}


bool SEGY::TxtHeader::isAscii() const
{
    return txt_[0]!=0xC3 && txt_[0]!=0x83;
}


void SEGY::TxtHeader::setSurveySetupInfo( const Coords::CoordSystem* crs )
{
    BufferString str;
    const char* res = Settings::common().find( "Company" );
    if ( !res ) res = "OpendTect";
    str = "Created by: "; str += res;
    str += "     ("; str += Time::getDateTimeString(); str += ")";
    putAt( 1, 6, 75, str );
    putAt( 2, 6, 75, BufferString("Survey: '", SI().name(),"'") );
    BinID bid = SI().sampling(false).hsamp_.start_;
    Coord coord = SI().transform( bid );
    if ( crs && !(*crs == *SI().getCoordSystem()) )
	coord = crs->convertFrom( coord, *SI().getCoordSystem() );
    coord.x = fabs(coord.x); coord.y = fabs(coord.y);
    if ( !mIsEqual(bid.inl(),coord.x,mDefEps)
      || !mIsEqual(bid.crl(),coord.x,mDefEps)
      || !mIsEqual(bid.inl(),coord.y,mDefEps)
      || !mIsEqual(bid.crl(),coord.y,mDefEps) )
    {
	putAt( 13, 6, 75, "Survey setup:" );
	coord = SI().transform( bid );
	str.set( bid.toString() ).add( " = " ).add( coord.toPrettyString() );
	putAt( 14, 6, 75, str );
	bid.crl() = SI().sampling(false).hsamp_.stop_.crl();
	coord = SI().transform( bid );
	str.set( bid.toString() ).add( " = " ).add( coord.toPrettyString() );
	putAt( 15, 6, 75, str );
	bid.inl() = SI().sampling(false).hsamp_.stop_.inl();
	bid.crl() = SI().sampling(false).hsamp_.start_.crl();
	coord = SI().transform( bid );
	str.set( bid.toString() ).add( " = " ).add( coord.toPrettyString() );
	putAt( 16, 6, 75, str );

	if ( crs )
	{
	    str = crs->summary();
	    putAt( 17, 6, 75, str );
	}

    }

    if ( !SI().zIsTime() )
    {
	str = "Depth survey: 1 SEG-Y millisec = 1 ";
	str += SI().getZUnitString(false);
	putAt( 18, 6, 75, str );
    }
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
	putAt( lnr, 5, sTxtHeadCharsPerLine, buf );

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
	LineKey lk( thd.linename );
	putAt( 4, 6, 20, "Line name:" );
	putAt( 4, 20, 75, lk.lineName() );
	putAt( 4, 45, 75, lk.attrName() );
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
    if ( mIsZero(sp,mDefEps) ) return;

    BufferString txt( "First sample at:", sp );
    putAt( 10, 6, 75, txt );
    putAt( 11, 10, 75, "(in bytes 109 (+) and 105 (-) of the trace header)");
}


void SEGY::TxtHeader::getText( BufferString& bs ) const
{
    char buf[sTxtHeadCharsPerLine+1];
    getFrom( 1, 1, sTxtHeadCharsPerLine, buf );
    bs = buf;
    for ( int iln=2; iln<=sTxtHeadNrLines; iln++ )
    {
	bs += "\n";
	getFrom( iln, 1, sTxtHeadCharsPerLine, buf );
	bs += buf;
    }
}


void SEGY::TxtHeader::setText( const char* txt )
{
    clearText();

    BufferString bs( txt );
    char* ptr = bs.getCStr();
    for ( int iln=1; iln<=sTxtHeadNrLines; iln++ )
    {
	char* endptr = firstOcc( ptr, '\n' );
	if ( !endptr ) break;
	*endptr = '\0';

	putAt( iln, 1, sTxtHeadCharsPerLine, ptr );
	ptr = endptr + 1; if ( !*ptr ) break;
    }

    setLineStarts();
}


void SEGY::TxtHeader::getFrom( int line, int pos, int endpos, char* str ) const
{
    if ( !str ) return;

    int charnr = (line-1)*sTxtHeadCharsPerLine + pos - 1;
    if ( endpos > sTxtHeadCharsPerLine ) endpos = sTxtHeadCharsPerLine;
    int maxcharnr = (line-1)*sTxtHeadCharsPerLine + endpos;

    while ( iswspace(txt_[charnr]) && charnr < maxcharnr ) charnr++;
    while ( charnr < maxcharnr ) *str++ = txt_[charnr++];
    *str = '\0';
    removeTrailingBlanks( str );
}


void SEGY::TxtHeader::putAt( int line, int pos, int endpos, const char* str )
{
    if ( !str || !*str ) return;

    int charnr = (line-1)*sTxtHeadCharsPerLine + pos - 1;
    if ( endpos > sTxtHeadCharsPerLine ) endpos = sTxtHeadCharsPerLine;
    int maxcharnr = (line-1)*sTxtHeadCharsPerLine + endpos;

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


// SEGY::BinHeader
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


// SEGY::TrcHeader
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
	    { delete [] buf_; buf_ = nullptr; }
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
    mDefineStaticLocalObject( PtrMan<SEGY::HdrDef>, trcdef, = nullptr );
    if ( trcdef ) return *(trcdef.ptr());

    trcdef = new SEGY::HdrDef( false );
    if ( !trcdef )
    {
	pFreeFnErrMsg( "Could not instantiate SEG-Y Trace Header definition" );
    }

    return *(trcdef.ptr());
}


void SEGY::TrcHeader::fillRev1Def( TrcHeaderDef& thd )
{
    const SEGY::HdrDef& defs = SEGY::TrcHeader::hdrDef();
    thd.inl_ = *defs[SEGY::TrcHeader::EntryInline()];		// 189-192
    thd.crl_ = *defs[SEGY::TrcHeader::EntryCrossline()];	// 193-196
    thd.xcoord_ = *defs[SEGY::TrcHeader::EntryXcdp()];		// 181-184
    thd.ycoord_ = *defs[SEGY::TrcHeader::EntryYcdp()];		// 185-188
    thd.trnr_ = *defs[SEGY::TrcHeader::EntryCdp()];		// 21-24
    thd.refnr_ = *defs[SEGY::TrcHeader::EntrySP()];		// 197-200
    thd.offs_ = *defs[SEGY::TrcHeader::EntryOffset()];		// 37-40
}


void SEGY::TrcHeader::initRead()
{
    const short trid = sCast(short,entryVal(EntryTrid()));	// 29-30
    isusable = trid < 2 || trid > 10;

    const short counit = sCast(short,entryVal(EntryCoUnit()));	// 89-90
    nonrectcoords = counit > 1 && counit < 5;
}


unsigned short SEGY::TrcHeader::nrSamples() const
{
    return sCast(unsigned short,entryVal(EntryNs()));		// 115-116
}


void SEGY::TrcHeader::putSampling( SamplingData<float> sdin, unsigned short ns )
{
    SamplingData<float> sd( sdin );
    PosImpExpPars::SVY().adjustZ( sd.start, false );
    PosImpExpPars::SVY().adjustZ( sd.step, false );

    const float zfac = sCast( float, SI().zDomain().userFactor() );
    const float drt = sd.start * zfac;
    const short delrt = sCast(short,mNINT32(drt));
    setEntryVal( EntryLagA(), -delrt ); // For HRS and Petrel	// 105-106
    setEntryVal( EntryDelRt(), delrt );				// 109-110

    const int sr_us = sCast(int,sd.step * 1e3f * zfac + .5f);
    setEntryVal( EntryDt(), sr_us );				// 117-118
    if ( ns == 0 )
	ns = SI().zRange(false).nrSteps() + 1;

    setEntryVal( EntryNs(), ns );				// 115-116
}


void SEGY::TrcHeader::putRev1Flds( const SeisTrcInfo& ti ) const
{
    Coord crd( ti.coord );
    PosImpExpPars::SVY().adjustCoord( crd, false );
    const int icx = mNINT32(crd.x*10);
    const int icy = mNINT32(crd.y*10);
    setEntryVal( EntryXcdp(), icx );				// 181-184
    setEntryVal( EntryYcdp(), icy );				// 185-188

    if ( SEGY::TxtHeader::info2D() )
    {
	int tnr = ti.nr;
	PosImpExpPars::SVY().adjustTrcNr( tnr, false );
	if ( !mIsUdf(ti.refnr) )
	{
	    tnr = mNINT32(ti.refnr*100);
	    setEntryVal( EntrySPscale(), -100 );		// 201-202
	}
	setEntryVal( EntrySP(), tnr );				// 197-200
    }
    else
    {
	BinID bid( ti.binid );
	PosImpExpPars::SVY().adjustBinID( bid, false );
	setEntryVal( EntryInline(), bid.inl() );		// 189-192
	setEntryVal( EntryCrossline(), bid.crl() );		// 193-196
    }
}


void SEGY::TrcHeader::use( const SeisTrcInfo& ti )
{
    if ( isrev0_ ) // For rev0, we initially fill with Rev 1 defaults
	putRev1Flds( ti );

    setEntryVal( EntryTrid(), 1 );				// 29-30
    setEntryVal( EntryDUse(), 1 );				// 35-36
    setEntryVal( EntryCoUnit(), 1 );				// 89-90

    const bool is2d = SEGY::TxtHeader::info2D();
    if ( !is2d && ti.binid.inl() != previnl_ )
	lineseqnr_ = 1;

    previnl_ = ti.binid.inl();
    const int nr2put = is2d ? seqnr_ : lineseqnr_;
    setEntryVal( EntryTracl(), nr2put );			// 1-4
    setEntryVal( EntryTracr(), seqnr_ );			// 5-8
    seqnr_++;
    lineseqnr_++;

    if ( is2d )
    {
	int trcnr = ti.nr;
	PosImpExpPars::SVY().adjustTrcNr( trcnr, false );
	setEntryVal( EntryCdp(), trcnr );			// 21-24
	setEntryVal( EntryOldSP(), int(ti.refnr) );		// 17-20

	hdef_.trnr_.putValue( buf_, trcnr );
    }
    else
    {
	BinID bid = ti.binid;
	PosImpExpPars::SVY().adjustBinID( bid, false );
	setEntryVal( EntryFldr(), bid.inl() );			// 9-12
	setEntryVal( EntryCdp(), bid.crl() );			// 21-24

	hdef_.inl_.putValue( buf_, ti.binid.inl() );
	hdef_.crl_.putValue( buf_, ti.binid.crl() );
    }

    Coord crd( ti.coord );
    if ( mIsUdf(crd.x) )
	crd.x = crd.y = 0;

    PosImpExpPars::SVY().adjustCoord( crd, false );
    static bool noscalco = GetEnvVarYN( "OD_SEGY_NO_SCALCO" );
    int iscalco, icx, icy;
    if ( noscalco )
    {
	iscalco = 1;
	icx = mNINT32(crd.x);
	icy = mNINT32(crd.y);
    }
    else
    {
	iscalco = -10;
	icx = mNINT32(crd.x*10);
	icy = mNINT32(crd.y*10);
    }

    setEntryVal( EntryScalco(), iscalco );			// 71-72
    setEntryVal( EntrySx(), icx );				// 73-76
    setEntryVal( EntrySy(), icy );				// 77-80
    hdef_.xcoord_.putValue( buf_, icx );
    hdef_.ycoord_.putValue( buf_, icy );

    float tioffs = ti.offset;
    PosImpExpPars::SVY().adjustOffset( tioffs, false );
    int intval = mNINT32( tioffs );
    hdef_.offs_.putValue( buf_, intval );
    intval = mNINT32( ti.azimuth * 360 / M_PI );
    hdef_.azim_.putValue( buf_, intval );

    const float zfac = sCast( float, SI().zDomain().userFactor() );
#define mSetScaledMemb(nm,fac) \
    if ( !mIsUdf(ti.nm) ) \
	{ intval = mNINT32(ti.nm*fac); hdef_.nm##_.putValue( buf_, intval ); }
    mSetScaledMemb(pick,zfac)

    // Absolute priority, therefore possibly overwriting previous
    putSampling( ti.sampling, 0 ); // 0=ns must be set elsewhere

    if ( !isrev0_ ) // Now this overrules everything
	putRev1Flds( ti );
}


float SEGY::TrcHeader::postScale( int numbfmt ) const
{
    // There seems to be software (Paradigm?) putting this on byte 189
    // Then we'd expect this to be 4 byte. Sigh. How far do we need to go
    // to support crap from SEG-Y vandals?
    HdrEntry he( *hdrDef()[EntryTrwf()] );			// 169-170
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
    const short trwf = sCast(short,he.getValue(buf_,needswap_));
    if ( trwf == 0 || trwf > 50 || trwf < -50 )
	return 1;

    return Math::IntPowerOf( ((float)2), trwf );
}


void SEGY::TrcHeader::getRev1Flds( SeisTrcInfo& ti ) const
{
    ti.coord.x = entryVal( EntryXcdp() );			// 181-184
    ti.coord.y = entryVal( EntryYcdp() );			// 185-188
    ti.binid.inl() = entryVal( EntryInline() );			// 189-192
    ti.binid.crl() = entryVal( EntryCrossline() );		// 193-196
    ti.refnr = sCast( float, entryVal(EntrySP()) );		// 197-200
    if ( !isrev0_ )
    {
	const short scalnr = sCast(short,entryVal(EntrySPscale())); // 201-202
	if ( scalnr != 0 )
	{
	    ti.refnr *= (scalnr > 0 ? scalnr : -1.0f/scalnr);
	    ti.nr = mNINT32(ti.refnr);
	}
    }

    PosImpExpPars::SVY().adjustCoord( ti.coord, true );
    PosImpExpPars::SVY().adjustBinID( ti.binid, true );
    PosImpExpPars::SVY().adjustTrcNr( ti.nr, true );
}


void SEGY::TrcHeader::fill( SeisTrcInfo& ti, float extcoordsc ) const
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
	getRev1Flds( ti ); // if rev 0, start with rev 1 as default

    const float zfac = 1.0f / SI().zDomain().userFactor();
    short delrt = sCast( short, entryVal(EntryDelRt()) );	// 109-110
    if ( delrt == 0 )
    {								// 105-106
	delrt = - sCast( short, entryVal(EntryLagA()) ); // HRS and Petrel
	mDefineStaticLocalObject( const bool, smt_bad_laga,
				  = GetEnvVarYN("OD_SEGY_BAD_LAGA") );
	if ( smt_bad_laga )
	    delrt = -delrt;
	float startz = delrt * zfac;
	if ( startz < -5000 || startz > 10000 )
	    delrt = 0;
    }
    ti.sampling.start = delrt * zfac;
    ti.sampling.step = entryVal(EntryDt()) * zfac * 0.001f;	// 117-118
    PosImpExpPars::SVY().adjustZ( ti.sampling.start, true );
    PosImpExpPars::SVY().adjustZ( ti.sampling.step, true );

    ti.pick = ti.refnr = mUdf(float);
    ti.nr = entryVal( EntryTracl() );				// 1-4

    if ( !hdef_.pick_.isUdf() )
    {
	const float val = hdef_.pick_.getValue( buf_, needswap_ );
	ti.pick = val * 0.001f;
    }

    PosImpExpPars::SVY().adjustZ( ti.pick, true );

    if ( !hdef_.refnr_.isUdf() )
    {
	ti.refnr = hdef_.refnr_.getValue( buf_, needswap_ );
	short bp = hdef_.refnr_.bytepos_;
	if ( !hdef_.refnr_.isInternal() )
	    bp--;
	if ( bp == hdrDef()[EntrySP()]->bytepos_ )		// 197-200
	{							// 201-202
	    const short spscale = sCast(short,entryVal(EntrySPscale()) );
	    const float scalnr =
		spscale==0 ? 1 : (spscale>0 ? spscale : -1.f/spscale);
	    ti.refnr *= scalnr;
	}
    }

    ti.coord.x = ti.coord.y = 0;
    ti.coord.x = hdef_.xcoord_.getValue(buf_,needswap_);
    ti.coord.y = hdef_.ycoord_.getValue(buf_,needswap_);
    ti.binid.inl() = ti.binid.crl() = 0;
    ti.binid.inl() = hdef_.inl_.getValue(buf_,needswap_);
    ti.binid.crl() = hdef_.crl_.getValue(buf_,needswap_);
    PosImpExpPars::SVY().adjustBinID( ti.binid, true );

    ti.offset = sCast( float, hdef_.offs_.getValue(buf_,needswap_) );
    if ( ti.offset < 0 ) ti.offset = -ti.offset;
    ti.azimuth = sCast( float, hdef_.azim_.getValue(buf_,needswap_) );
    ti.azimuth *= M_PI / 360;
    if ( hdef_.trnr_.bytepos_ >= 0 )
	ti.nr = hdef_.trnr_.getValue(buf_,needswap_);
    else
    {
	// Trick to set trace number to sequence number when no trnr_ defined
	mDefineStaticLocalObject( Threads::Atomic<int>, seqnr, (0) );
	if ( hdef_.trnr_.bytepos_ == -5 )
	    seqnr++;
	else
	    { seqnr = 1;
		const_cast<SEGY::TrcHeaderDef&>(hdef_).trnr_.bytepos_ = -5; }
	ti.nr = seqnr;
    }
    PosImpExpPars::SVY().adjustTrcNr( ti.nr, true );

    if ( !isrev0_ )
    {
	const int oldnr = ti.nr;
	getRev1Flds( ti ); // if >= rev 1, then those fields are holy
	if ( oldnr ) ti.nr = oldnr;
    }

    const double scale = getCoordScale( extcoordsc );
    ti.coord.x *= scale;
    ti.coord.y *= scale;
    PosImpExpPars::SVY().adjustCoord( ti.coord, true );
}


double SEGY::TrcHeader::getCoordScale( float extcoordsc ) const
{
    if ( !mIsUdf(extcoordsc) )
	return double(extcoordsc);

    const short scalco = sCast( short, entryVal(EntryScalco()) ); // 71-72
    return scalco ? (scalco > 0 ? scalco : -1./scalco) : 1;
}


Coord SEGY::TrcHeader::getCoord( bool rcv, float extcoordsc ) const
{
    const double scale = getCoordScale( extcoordsc );
    Coord ret(	entryVal( rcv?EntryGx():EntrySx() ),		// 81-84, 73-76
		entryVal( rcv?EntryGy():EntrySy() ) );		// 85-88, 77-80
    ret.x *= scale; ret.y *= scale;
    return ret;
}
