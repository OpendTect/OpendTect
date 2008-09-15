/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: segytr.cc,v 1.64 2008-09-15 10:10:36 cvsbert Exp $";

#include "segytr.h"
#include "seistrc.h"
#include "segyhdr.h"
#include "segyfiledef.h"
#include "datainterp.h"
#include "conn.h"
#include "settings.h"
#include "strmprov.h"
#include "errh.h"
#include "iopar.h"
#include "timefun.h"
#include "scaler.h"
#include "survinfo.h"
#include "seisselection.h"
#include "envvars.h"
#include "separstr.h"
#include "keystrs.h"
#include <math.h>
#include <ctype.h>
# include <sstream>

const char* SEGY::FileSpec::sKeyFileNrs = "File numbers";
const char* SEGYSeisTrcTranslator::sNumberFormat = "Number format";
const char* SEGYSeisTrcTranslator::sExternalNrSamples = "Nr samples overrule";
const char* SEGYSeisTrcTranslator::sExternalTimeShift = "Start time overrule";
const char* SEGYSeisTrcTranslator::sExternalSampleRate = "Sample rate overrule";
const char* SEGYSeisTrcTranslator::sForceRev0 = "Force Rev0";
const char* SEGYSeisTrcTranslator::sExternalCoordScaling
	= "Coordinate scaling overrule";

static const char* allsegyfmtoptions[] = {
    "From file header",
    "1 - Floating point",
    "2 - Integer (4 byte)",
    "3 - Integer (2 byte)",
    "5 - IEEE float (4 byte)",
    "8 - Signed char (1 byte)",
    0
};
const char** SEGY::FilePars::getFmts( bool forread )
{
    return forread ? allsegyfmtoptions : allsegyfmtoptions+1;
}


void SEGY::FileSpec::fillPar( IOPar& iop ) const
{
    iop.set( sKey::FileName, fname_ );
    if ( mIsUdf(nrs_.start) )
	iop.removeWithKey( sKeyFileNrs );
    else
    {
	FileMultiString fms;
	fms += nrs_.start; fms += nrs_.stop; fms += nrs_.step;
	if ( zeropad_ )
	    fms += zeropad_;
	iop.set( sKeyFileNrs, fms );
    }
}


void SEGY::FileSpec::usePar( const IOPar& iop )
{
    iop.get( sKey::FileName, fname_ );
    getMultiFromString( iop.find(sKeyFileNrs) );
}


void SEGY::FileSpec::getMultiFromString( const char* str )
{
    FileMultiString fms( str );
    const int len = fms.size();
    nrs_.start = len > 0 ? atoi( fms[0] ) : mUdf(int);
    if ( len > 1 )
	nrs_.stop = atoi( fms[1] );
    if ( len > 2 )
	nrs_.step = atoi( fms[2] );
    if ( len > 3 )
	zeropad_ = atoi( fms[3] );
}


void SEGY::FilePars::fillPar( IOPar& iop ) const
{
    iop.set( SEGYSeisTrcTranslator::sExternalNrSamples, ns_ );
    iop.set( SEGYSeisTrcTranslator::sNumberFormat, nameOfFmt(fmt_,forread_) );
    iop.setYN( SegylikeSeisTrcTranslator::sKeyBytesSwapped, byteswapped_ );
}


void SEGY::FilePars::usePar( const IOPar& iop )
{
    iop.get( SEGYSeisTrcTranslator::sExternalNrSamples, ns_ );
    iop.getYN( SegylikeSeisTrcTranslator::sKeyBytesSwapped, byteswapped_ );
    fmt_ = fmtOf( iop.find(SEGYSeisTrcTranslator::sNumberFormat), forread_ );
}


const char* SEGY::FilePars::nameOfFmt( int fmt, bool forread )
{
    if ( fmt > 0 && fmt < 4 )
	return allsegyfmtoptions[fmt];
    if ( fmt == 5 )
	return allsegyfmtoptions[4];
    if ( fmt == 8 )
	return allsegyfmtoptions[5];

    return forread ? allsegyfmtoptions[0] : nameOfFmt( 1, false );
}


int SEGY::FilePars::fmtOf( const char* str, bool forread )
{
    if ( !str || !*str || !isdigit(*str) )
	return forread ? 0 : 1;

    return (int)(*str - '0');
}


SEGYSeisTrcTranslator::SEGYSeisTrcTranslator( const char* nm, const char* unm )
	: SegylikeSeisTrcTranslator(nm,unm)
	, numbfmt(0)
	, itrc(0)
	, trchead(*new SEGY::TrcHeader(headerbuf,true,hdef))
	, txthead(*new SEGY::TxtHeader)
	, binhead(*new SEGY::BinHeader)
	, trcscale(0)
	, curtrcscale(0)
	, ext_nr_samples(-1)
	, ext_coord_scaling(mUdf(float))
	, ext_time_shift(mUdf(float))
	, ext_sample_rate(mUdf(float))
	, force_rev0(false)
{
}


SEGYSeisTrcTranslator::~SEGYSeisTrcTranslator()
{
    SegylikeSeisTrcTranslator::cleanUp();

    delete &txthead;
    delete &binhead;
    delete &trchead;
}


int SEGYSeisTrcTranslator::dataBytes() const
{
    return SEGY::BinHeader::formatBytes( numbfmt > 0 ? numbfmt : 1 );
}


#define mErrRet(s) { fillErrMsg(s); return false; }


bool SEGYSeisTrcTranslator::readTapeHeader()
{
    if ( !sConn().doIO(txthead.txt,SegyTxtHeaderLength) )
	mErrRet( "Cannot read EBCDIC header" )
    txthead.setAscii();

    binhead.needswap = bytesswapped;
    unsigned char binheaderbuf[400];
    if ( !sConn().doIO( binheaderbuf, SegyBinHeaderLength ) )
	mErrRet( "Cannot read EBCDIC header" )
    binhead.getFrom( binheaderbuf );

    trchead.needswap = bytesswapped;
    trchead.isrev1 = force_rev0 ? false : binhead.isrev1;
    if ( trchead.isrev1 )
    {
	if ( binhead.nrstzs > 100 ) // protect against wild values
	    binhead.nrstzs = 0;
	for ( int idx=0; idx<binhead.nrstzs; idx++ )
	{
	    if ( !sConn().doIO(txthead.txt,SegyTxtHeaderLength) )
		mErrRet( "No traces found in the SEG-Y file" )
	}
    }

    if ( numbfmt == 0 )
    {
	numbfmt = binhead.format;
	if ( numbfmt == 4 && read_mode != Seis::PreScan )
	    mErrRet( "SEG-Y format '4' (fixed point/gain code) not supported" )
	else if ( numbfmt < 1 || numbfmt > 8 || numbfmt == 6 || numbfmt == 7 )
	{
	    BufferString msg = "SEG-Y format '"; msg += numbfmt;
	    msg += "' found. Will try '1' (4-byte floating point)";
	    ErrMsg( msg ); numbfmt = 1;
	}
    }

    txthead.getText( pinfo.usrinfo );
    pinfo.nr = binhead.lino;
    pinfo.zrg.step = binhead.hdt * (0.001 / SI().zFactor());
    insd.step = binhead_dpos = pinfo.zrg.step;
    innrsamples = binhead_ns = binhead.hns;
    return true;
}


void SEGYSeisTrcTranslator::updateCDFromBuf()
{
    SeisTrcInfo info;
    trchead.fill( info, ext_coord_scaling );
    insd = info.sampling;
    if ( !insd.step ) insd.step = binhead_dpos;
    if ( !mIsUdf(ext_time_shift) )
	insd.start = ext_time_shift;
    if ( !mIsUdf(ext_sample_rate) )
	insd.step = ext_sample_rate;

    innrsamples = ext_nr_samples;
    if ( innrsamples <= 0 )
    {
	innrsamples = binhead_ns;
	if ( innrsamples <= 0 )
	{
	    innrsamples = trchead.nrSamples();
	    if ( innrsamples <= 0 )
		innrsamples = SI().zRange(false).nrSteps() + 1;
	}
    }

    addComp( getDataChar(numbfmt)  );
    DataCharacteristics& dc = tarcds[0]->datachar;
    dc.fmt = DataCharacteristics::Ieee;
    const float scfac = trchead.postScale( numbfmt ? numbfmt : 1 );
    if ( !mIsEqual(scfac,1,mDefEps)
      || (dc.isInteger() && dc.nrBytes() == BinDataDesc::N4) )
	dc = DataCharacteristics();
}


int SEGYSeisTrcTranslator::nrSamplesRead() const
{
    int ret = trchead.nrSamples();
    return ret ? ret : binhead_ns;
}


void SEGYSeisTrcTranslator::interpretBuf( SeisTrcInfo& ti )
{
    itrc++;
    trchead.fill( ti, ext_coord_scaling );
    if ( is_prestack && offsazimopt == 1 )
    {
	Coord c1( trchead.getCoord(true,ext_coord_scaling) );
	Coord c2( trchead.getCoord(false,ext_coord_scaling) );
	ti.setPSFlds( c1, c2 );
	ti.coord = Coord( (c1.x+c2.x)*.5, (c1.y+c2.y)*.5 );
    }
    float scfac = trchead.postScale( numbfmt ? numbfmt : 1 );
    if ( mIsEqual(scfac,1,mDefEps) )
	curtrcscale = 0;
    else
    {
	if ( !trcscale ) trcscale = new LinScaler( 0, scfac );
	else		 trcscale->factor = scfac;
	curtrcscale = trcscale;
    }

    if ( !mIsUdf(ext_time_shift) )
	ti.sampling.start = ext_time_shift;
    if ( !mIsUdf(ext_sample_rate) )
	ti.sampling.step = ext_sample_rate;
}


bool SEGYSeisTrcTranslator::writeTapeHeader()
{
    if ( numbfmt == 0 )
	// Auto-detect
	numbfmt = nrFormatFor( storinterp->dataChar() );

    trchead.isrev1 = !force_rev0;

    SEGY::TxtHeader txthead( trchead.isrev1 );
    txthead.setUserInfo( pinfo.usrinfo );
    hdef.linename = curlinekey.buf();
    hdef.pinfo = &pinfo;
    txthead.setPosInfo( hdef );
    txthead.setStartPos( outsd.start );
    if ( GetEnvVarYN("OD_WRITE_EBCDIC_SEGY_HDR" ) )
	txthead.setEbcdic();
    if ( !sConn().doIO( txthead.txt, SegyTxtHeaderLength ) )
	mErrRet("Cannot write SEG-Y textual header")

    SEGY::BinHeader binhead( trchead.isrev1 );
    binhead.format = numbfmt < 2 ? 1 : numbfmt;
    numbfmt = binhead.format;
    binhead.lino = pinfo.nr;
    static int jobid = 0;
    binhead.jobid = ++jobid;
    binhead.hns = (short)outnrsamples;
    binhead.hdt = (short)(outsd.step * SI().zFactor() * 1e3 + .5);
    binhead.tsort = is_prestack ? 0 : 4; // To make Strata users happy
    unsigned char binheadbuf[400];
    binhead.putTo( binheadbuf );
    if ( !sConn().doIO( binheadbuf, SegyBinHeaderLength ) )
	mErrRet("Cannot write SEG-Y binary header")

    return true;
}


void SEGYSeisTrcTranslator::fillHeaderBuf( const SeisTrc& trc )
{
    trchead.use( trc.info() );
    if ( useinpsd )
	trchead.putSampling( trc.info().sampling, trc.size() );
    else
	trchead.putSampling( outsd, outnrsamples );
}


void SEGYSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SegylikeSeisTrcTranslator::usePar( iopar );

    const char* res = iopar[ sNumberFormat ];
    if ( *res )
	numbfmt = isdigit(*res) ? *res - '0' : 0;

    iopar.get( sExternalNrSamples, ext_nr_samples );
    iopar.get( sExternalCoordScaling, ext_coord_scaling );
    iopar.get( sExternalTimeShift, ext_time_shift );
    iopar.get( sExternalSampleRate, ext_sample_rate );
    iopar.getYN( sForceRev0, force_rev0 );
}


bool SEGYSeisTrcTranslator::isRev1() const
{
    return trchead.isrev1;
}


void SEGYSeisTrcTranslator::toPreferred( DataCharacteristics& dc ) const
{
    dc = getDataChar( nrFormatFor(dc) );
}


void SEGYSeisTrcTranslator::toSupported( DataCharacteristics& dc ) const
{
    if ( dc.isInteger() || !dc.isIeee() )
	dc = getDataChar( nrFormatFor(dc) );
}


void SEGYSeisTrcTranslator::toPreSelected( DataCharacteristics& dc ) const
{
    if ( numbfmt > 0 ) dc = getDataChar( numbfmt );
}


int SEGYSeisTrcTranslator::nrFormatFor( const DataCharacteristics& dc ) const
{
    int nrbytes = dc.nrBytes();
    if ( nrbytes > 4 ) nrbytes = 4;
    else if ( !dc.isSigned() && nrbytes < 4 )
	nrbytes *= 2;

    int nf = 1;
    if ( nrbytes == 1 )
	nf = 8;
    else if ( nrbytes == 2 )
	nf = 3;
    else
	nf = dc.isInteger() ? 2 : 1;

    return nf;
}


DataCharacteristics SEGYSeisTrcTranslator::getDataChar( int nf ) const
{
    DataCharacteristics dc( true, true, BinDataDesc::N4,
			    DataCharacteristics::Ibm,
			    bytesswapped ? !__islittle__ : __islittle__ );

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
	dc.fmt = DataCharacteristics::Ieee;
	dc.setInteger( false );
	dc.littleendian = false;
    break;
    default:
	dc.setInteger( false );
    break;
    }

    return dc;
}
