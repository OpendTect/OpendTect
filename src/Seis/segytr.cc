/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: segytr.cc,v 1.27 2004-04-28 14:37:18 bert Exp $";

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
#include <math.h>
#include <ctype.h>
# include <sstream>

const char* SEGYSeisTrcTranslator::sNumberFormat = "Number format";
const char* SEGYSeisTrcTranslator::sExternalNrSamples = "Nr samples overrule";
const char* SEGYSeisTrcTranslator::sExternalTimeShift = "Start time overrule";
const char* SEGYSeisTrcTranslator::sExternalSampleRate = "Sample rate overrule";
const char* SEGYSeisTrcTranslator::sExternalCoordScaling
	= "Coordinate scaling overrule";
const char* SEGYSeisTrcTranslator::sUseLiNo
	= "Use tape header line number for inline";

static const char* nth_to_dump_str = getenv("DTECT_NR_SEGY_HEADERS_TO_DUMP");
static int nr_trc_headers_to_dump = nth_to_dump_str ? atoi(nth_to_dump_str) : 5;


SEGYSeisTrcTranslator::SEGYSeisTrcTranslator( const char* nm, const char* unm )
	: SegylikeSeisTrcTranslator(nm,unm)
	, numbfmt(0)
	, dumpsd(*new StreamData)
	, itrc(0)
	, trhead(headerbuf,hdef)
	, trcscale(0)
	, curtrcscale(0)
	, ext_nr_samples(-1)
	, ext_coord_scaling(mUndefValue)
	, ext_time_shift(mUndefValue)
	, ext_sample_rate(mUndefValue)
	, use_lino(false)
	, do_string_dump(false)
{
}


SEGYSeisTrcTranslator::~SEGYSeisTrcTranslator()
{
    SegylikeSeisTrcTranslator::cleanUp();
    dumpsd.close();
    delete &dumpsd;
}


int SEGYSeisTrcTranslator::dataBytes() const
{
    return SegyBinHeader::formatBytes( numbfmt );
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

    if ( binhead.format == 4 )
    {
	errmsg = "SEG-Y format 4 (fixed point with gain code) not supported";
	return false;
    }
    else if ( binhead.format > 0 && binhead.format < 7 )
	numbfmt = binhead.format;
    if ( numbfmt < 1 || numbfmt > 6 )
	numbfmt = 1;

    txthead.getText( pinfo->usrinfo );
    pinfo->nr = binhead.lino;
    binhead_ns = binhead.hns;
    binhead_dpos = binhead.hdt * (0.001 / SI().zFactor());

    if ( itrc <= nr_trc_headers_to_dump )
    {
	dumpsd.close();
	if ( do_string_dump )
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
	    *dumpsd.ostrm << "SEG-Y text header:\n\n";
	    txthead.print( *dumpsd.ostrm );
	    *dumpsd.ostrm << "\n\nInfo present in SEG-Y binary header:\n\n";
	    binhead.print( *dumpsd.ostrm );
	}
    }

    return true;
}


void SEGYSeisTrcTranslator::updateCDFromBuf()
{
    SeisTrcInfo info;
    trhead.fill( info, ext_coord_scaling );
    SamplingData<float> sd( info.sampling );
    if ( !sd.step ) sd.step = binhead_dpos;
    if ( !mIsUndefined(ext_time_shift) )
	sd.start = ext_time_shift;
    if ( !mIsUndefined(ext_sample_rate) )
	sd.step = ext_sample_rate;

    int nrsamples = ext_nr_samples;
    if ( nrsamples <= 0 )
    {
	nrsamples = trhead.nrSamples();
	if ( !nrsamples ) nrsamples = binhead_ns;
    }

    addComp( getDataChar(numbfmt), sd, nrsamples );
    DataCharacteristics& dc = tarcds[0]->datachar;
    dc.fmt = DataCharacteristics::Ieee;
    const float scfac = trhead.postScale( numbfmt );
    if ( !mIS_ZERO(1-scfac)
      || (dc.isInteger() && dc.nrBytes() == BinDataDesc::N4) )
	dc.setInteger( false );
}


int SEGYSeisTrcTranslator::nrSamplesRead() const
{
    int ret = trhead.nrSamples();
    return ret ? ret : binhead_ns;
}


void SEGYSeisTrcTranslator::interpretBuf( SeisTrcInfo& ti )
{
    itrc++;
    if ( itrc <= nr_trc_headers_to_dump && dumpsd.usable() )
    {
	if ( itrc == 1 )
	    *dumpsd.ostrm << "\n\n\n\tField\tByte\tValue\n\n";
	*dumpsd.ostrm << "\nTrace " << itrc << ":\n";
	trhead.print( *dumpsd.ostrm );
	if ( itrc != nr_trc_headers_to_dump )
	    *dumpsd.ostrm << std::endl;
	else
	{
	    if ( do_string_dump )
	    {
		mDynamicCastGet(std::ostringstream*,sstrm,dumpsd.ostrm)
		dumpstr = sstrm->str();
	    }
	    dumpsd.close();
	}
    }

    trhead.fill( ti, ext_coord_scaling );
    if ( dofilloffsazim )
	fillOffsAzim( ti, trhead.getCoord(true,ext_coord_scaling),
			  trhead.getCoord(false,ext_coord_scaling) );
    if ( use_lino ) ti.binid.inl = pinfo->nr;
    float scfac = trhead.postScale( numbfmt );
    if ( mIS_ZERO(1-scfac) )
	curtrcscale = 0;
    else
    {
	if ( !trcscale ) trcscale = new LinScaler( 0, scfac );
	else		 trcscale->factor = scfac;
	curtrcscale = trcscale;
    }

    if ( !mIsUndefined(ext_time_shift) )
	ti.sampling.start = ext_time_shift;
    if ( !mIsUndefined(ext_sample_rate) )
	ti.sampling.step = ext_sample_rate;
}


bool SEGYSeisTrcTranslator::writeTapeHeader()
{
    numbfmt = nrFormatFor( storinterp->dataChar() );

    SegyTxtHeader txthead;
    txthead.setUserInfo( pinfo->usrinfo );
    txthead.setPosInfo( hdef.xcoord, hdef.ycoord, hdef.inl, hdef.crl );
    txthead.setStartPos( outcd->sd.start );
    txthead.setEbcdic();
    if ( !sConn().doIO( txthead.txt, SegyTxtHeaderLength ) )
	{ errmsg = "Cannot write SEG-Y EBCDIC header"; return false; }

    SegyBinHeader binhead;
    binhead.format = numbfmt < 2 ? 1 : numbfmt;
    binhead.lino = pinfo->nr;
    binhead.reno = 1;
    binhead.hns = (short)outcd->nrsamples;
    binhead.hdt = (short)(outcd->sd.step * SI().zFactor() * 1e3 + .5);
    binhead.fold = 1;
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
	trhead.putSampling( trc.samplingData(selComp()), trc.size(selComp()) );
    else
	trhead.putSampling( outcd->sd, outcd->nrsamples );
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
    iopar.getYN( sUseLiNo, use_lino );
}


void SEGYSeisTrcTranslator::toSupported( DataCharacteristics& dc ) const
{
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
	nf = 5;
    else if ( nrbytes == 2 )
	nf = 3;
    else
	nf = dc.isIeee() ? 6 : (dc.isInteger() ? 2 : 1);
    return nf;
}


DataCharacteristics SEGYSeisTrcTranslator::getDataChar( int nf ) const
{
    DataCharacteristics dc( true, true, BinDataDesc::N4,
			    DataCharacteristics::Ibm, __islittle__ );

    switch ( nf )
    {
    case 3:
        dc.setNrBytes( 2 );
    case 2:
    break;
    case 5:
        dc.setNrBytes( 1 );
    break;
    case 6:
	dc.fmt = DataCharacteristics::Ieee;
    default:
	dc.setInteger( false );
    break;
    }

    return dc;
}
