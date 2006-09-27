/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: segytr.cc,v 1.50 2006-09-27 20:24:34 cvskris Exp $";

#include "segytr.h"
#include "seistrc.h"
#include "segyhdr.h"
#include "datainterp.h"
#include "conn.h"
#include "settings.h"
#include "strmprov.h"
#include "errh.h"
#include "iopar.h"
#include "timefun.h"
#include "scaler.h"
#include "survinfo.h"
#include "seistrcsel.h"
#include "envvars.h"
#include <math.h>
#include <ctype.h>
# include <sstream>

const char* SEGYSeisTrcTranslator::sNumberFormat = "Number format";
const char* SEGYSeisTrcTranslator::sExternalNrSamples = "Nr samples overrule";
const char* SEGYSeisTrcTranslator::sExternalTimeShift = "Start time overrule";
const char* SEGYSeisTrcTranslator::sExternalSampleRate = "Sample rate overrule";
const char* SEGYSeisTrcTranslator::sForceRev0 = "Force Rev0";
const char* SEGYSeisTrcTranslator::sExternalCoordScaling
	= "Coordinate scaling overrule";
const char* SEGYSeisTrcTranslator::sUseLiNo
	= "Use tape header line number for inline";
const char* SEGYSeisTrcTranslator::sUseOffset
	= "Use tape header offset";


SEGYSeisTrcTranslator::SEGYSeisTrcTranslator( const char* nm, const char* unm )
	: SegylikeSeisTrcTranslator(nm,unm)
	, numbfmt(0)
	, dumpsd(*new StreamData)
	, itrc(0)
	, trhead(headerbuf,true,hdef)
	, trcscale(0)
	, curtrcscale(0)
	, ext_nr_samples(-1)
	, ext_coord_scaling(mUdf(float))
	, ext_time_shift(mUdf(float))
	, ext_sample_rate(mUdf(float))
	, use_lino(false)
	, use_offset(false)
	, force_rev0(false)
	, dumpmode(None)
    	, maxnrdump(5)
{
}


SEGYSeisTrcTranslator::~SEGYSeisTrcTranslator()
{
    SegylikeSeisTrcTranslator::cleanUp();
    closeTraceDump();
    delete &dumpsd;
}


int SEGYSeisTrcTranslator::dataBytes() const
{
    return SegyBinHeader::formatBytes( numbfmt > 0 ? numbfmt : 1 );
}


bool SEGYSeisTrcTranslator::readTapeHeader()
{
    SegyTxtHeader txthead;
    if ( !sConn().doIO(txthead.txt,SegyTxtHeaderLength) )
	{ errmsg = "Cannot read EBCDIC header"; return false; }
    txthead.setAscii();

    unsigned char binheaderbuf[400];
    if ( !sConn().doIO( binheaderbuf, SegyBinHeaderLength ) )
	{ errmsg = "Cannot read binary header"; return false; }
    SegyBinHeader binhead;
    binhead.getFrom( binheaderbuf );
    trhead.isrev1 = force_rev0 ? false : binhead.isrev1;

    if ( binhead.isrev1 )
    {
	for ( int idx=0; idx<binhead.nrstzs; idx++ )
	{
	    if ( !sConn().doIO(txthead.txt,SegyTxtHeaderLength) )
		{ errmsg = "Cannot SEG-Y REV1 extended headers"; return false; }
	}
    }

    if ( numbfmt == 0 )
    {
	numbfmt = binhead.format;
	if ( numbfmt < 1 || numbfmt > 8 || numbfmt == 6 || numbfmt == 7 )
	{
	    BufferString msg = "SEG-Y format '";
	    msg += numbfmt;
	    msg += "' found. Will try '1' (4-byte floating point)";
	    ErrMsg( msg );
	    numbfmt = 1;
	}
	else if ( numbfmt == 4 && read_mode != Seis::PreScan )
	{
	    errmsg = "SEG-Y format '4' (fixed point/gain code) not supported";
	    return false;
	}
    }

    txthead.getText( pinfo.usrinfo );
    pinfo.nr = binhead.lino;
    pinfo.zrg.step = binhead.hdt * (0.001 / SI().zFactor());
    insd.step = binhead_dpos = pinfo.zrg.step;
    innrsamples = binhead_ns = binhead.hns;

    if ( dumpmode != None )
    {
	dumpsd.close();
	if ( dumpmode == String )
	    dumpsd.ostrm = new std::ostringstream;
	else
	{
	    const char* res = Settings::common()[ "SEG-Y.Header dump" ];
	    if ( !res || !*res )
		res = Settings::common()[ "Seg-Y headers" ];
	    if ( res && *res )
	    {
		StreamProvider sp( res );
		dumpsd = sp.makeOStream();
	    }
	}
	if ( dumpsd.usable() )
	{
	    *dumpsd.ostrm << "SEG-Y Text header:\n\n-------\n";
	    txthead.print( *dumpsd.ostrm );
	    *dumpsd.ostrm << "-------\n\n\nSEG-Y Binary header "
				"(non-zero values displayed only):\n\n";
	    *dumpsd.ostrm << "\tField\tByte\tValue\n\n";
	    binhead.print( *dumpsd.ostrm );
	}
    }

    return true;
}


void SEGYSeisTrcTranslator::updateCDFromBuf()
{
    SeisTrcInfo info;
    trhead.fill( info, ext_coord_scaling );
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
	    innrsamples = trhead.nrSamples();
	    if ( innrsamples <= 0 )
		innrsamples = SI().zRange(false).nrSteps() + 1;
	}
    }

    addComp( getDataChar(numbfmt)  );
    DataCharacteristics& dc = tarcds[0]->datachar;
    dc.fmt = DataCharacteristics::Ieee;
    const float scfac = trhead.postScale( numbfmt ? numbfmt : 1 );
    if ( !mIsEqual(scfac,1,mDefEps)
      || (dc.isInteger() && dc.nrBytes() == BinDataDesc::N4) )
	dc = DataCharacteristics();
}


int SEGYSeisTrcTranslator::nrSamplesRead() const
{
    int ret = trhead.nrSamples();
    return ret ? ret : binhead_ns;
}


const char* SEGYSeisTrcTranslator::dumpFileName() const
{
    return dumpsd.fileName();
}


void SEGYSeisTrcTranslator::closeTraceDump()
{
    if ( dumpsd.usable() )
    {
	if ( dumpmode == String )
	{
	    mDynamicCastGet(std::ostringstream*,sstrm,dumpsd.ostrm)
	    dumpstr = sstrm->str();
	}
	dumpsd.close();
    }
}


void SEGYSeisTrcTranslator::interpretBuf( SeisTrcInfo& ti )
{
    itrc++;
    if ( dumpsd.usable() )
    {
	if ( itrc == 1 )
	    *dumpsd.ostrm << "\n\n\n";
	*dumpsd.ostrm << "\nTrace header " << itrc << ":\n\n";
	trhead.print( *dumpsd.ostrm );
	if ( itrc <= maxnrdump )
	    *dumpsd.ostrm << std::endl;
	else
	    closeTraceDump();
    }

    trhead.fill( ti, ext_coord_scaling );
    if ( is_prestack )
    {
	if ( use_offset )
	{
	    if ( !mIsUdf(ext_coord_scaling) && !mIsUdf(ti.offset) )
		ti.offset *= ext_coord_scaling;
	    ti.coord = SI().transform( ti.binid );
	}
	else
	{
	    Coord c1( trhead.getCoord(true,ext_coord_scaling) );
	    Coord c2( trhead.getCoord(false,ext_coord_scaling) );
	    fillOffsAzim( ti, c1, c2 );
	    ti.coord = Coord( (c1.x+c2.x)*.5, (c1.y+c2.y)*.5 );
	}
    }
    if ( use_lino ) ti.binid.inl = pinfo.nr;
    float scfac = trhead.postScale( numbfmt ? numbfmt : 1 );
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

    trhead.isrev1 = !force_rev0;

    SegyTxtHeader txthead( trhead.isrev1 );
    txthead.setUserInfo( pinfo.usrinfo );
    hdef.linename = curlinekey.buf();
    hdef.pinfo = &pinfo;
    txthead.setPosInfo( hdef );
    txthead.setStartPos( outsd.start );
    if ( !sConn().doIO( txthead.txt, SegyTxtHeaderLength ) )
	{ errmsg = "Cannot write SEG-Y textual header"; return false; }

    SegyBinHeader binhead( trhead.isrev1 );
    binhead.format = numbfmt < 2 ? 1 : numbfmt;
    numbfmt = binhead.format;
    binhead.lino = pinfo.nr;
    static int jobid = 0;
    binhead.jobid = ++jobid;
    binhead.hns = (short)outnrsamples;
    binhead.hdt = (short)(outsd.step * SI().zFactor() * 1e3 + .5);
    unsigned char binheadbuf[400];
    binhead.putTo( binheadbuf );
    if ( !sConn().doIO( binheadbuf, SegyBinHeaderLength ) )
	{ errmsg = "Cannot write SEG-Y binary header"; return false; }

    return true;
}


void SEGYSeisTrcTranslator::fillHeaderBuf( const SeisTrc& trc )
{
    trhead.use( trc.info() );
    if ( useinpsd )
	trhead.putSampling( trc.info().sampling, trc.size() );
    else
	trhead.putSampling( outsd, outnrsamples );
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
    iopar.getYN( sUseOffset, use_offset );
    iopar.getYN( sUseLiNo, use_lino );
    iopar.getYN( sForceRev0, force_rev0 );
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
    static bool isswppd = GetEnvVarYN( "DTECT_SEGY_SWAPPED_DATA" );
    DataCharacteristics dc( true, true, BinDataDesc::N4,
			    DataCharacteristics::Ibm,
			    isswppd ? !__islittle__ : __islittle__ );

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
