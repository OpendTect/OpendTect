/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: segytr.cc,v 1.3 2001-02-28 14:59:18 bert Exp $";

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
#include <math.h>
#include <ctype.h>


SEGYSeisTrcTranslator::SEGYSeisTrcTranslator( const char* nm )
	: SegylikeSeisTrcTranslator(nm)
	, numbfmt(0)
	, dumpsd(*new StreamData)
	, itrc(0)
	, trhead((unsigned char*)(char*)headerbuf,hdef)
{
}


SEGYSeisTrcTranslator::~SEGYSeisTrcTranslator()
{
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

    unsigned char headerbuf[400];
    if ( !sConn().doIO( headerbuf, SegyBinHeaderLength ) )
	{ errmsg = "Cannot read binary header"; return false; }
    SegyBinHeader binhead;
    binhead.getFrom( headerbuf );

    if ( binhead.format == 4 )
    {
	errmsg = "SEG-Y format 4 (fixed point with gain code) not supported";
	return false;
    }
    else if ( binhead.format > 0 && binhead.format < 7 )
	numbfmt = binhead.format;
    if ( numbfmt < 1 || numbfmt > 6 )
	numbfmt = 1;

    pinfo.nr = binhead.lino;
    pinfo.client = txthead.getClient();
    pinfo.company = txthead.getCompany();
    binhead_ns = binhead.hns;
    binhead_dpos = binhead.hdt * 1e-6;

    if ( itrc < 5 )
    {
	const char* res = Settings::common()[ "SEG-Y.Header dump" ];
	if ( !res || !*res )
	    res = Settings::common()[ "Seg-Y headers" ];
	if ( res && *res )
	{
	    dumpsd.close();
	    StreamProvider sp( res );
	    dumpsd = sp.makeOStream();
	    if ( dumpsd.usable() )
	    {
		txthead.print( *dumpsd.ostrm );
		binhead.print( *dumpsd.ostrm );
	    }
	}
    }

    return true;
}


void SEGYSeisTrcTranslator::updateCDFromBuf()
{
    SeisTrcInfo info;
    trhead.fill( info );
    SamplingData<float> sd( info.sampling.start, info.sampling.step );
    if ( !sd.step ) sd.step = binhead_dpos;

    int nrsamples = trhead.nrSamples();
    if ( !nrsamples ) nrsamples = binhead_ns;

    addComp( getDataChar(numbfmt), sd, nrsamples );
    DataCharacteristics& dc = tarcds[0]->datachar;
    dc.fmt = DataCharacteristics::Ieee;
    if ( dc.isInteger() && dc.nrBytes() == BinDataDesc::N4 )
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
    if ( itrc < 5 && dumpsd.usable() )
    {
	if ( itrc == 1 )
	    *dumpsd.ostrm << "\n\n\tField\tByte\tValue\n\n";
	*dumpsd.ostrm << "\nTrc " << itrc << "\n";
	trhead.print( *dumpsd.ostrm );
	if ( itrc == 4 ) dumpsd.close();
	else		 *dumpsd.ostrm << endl;
    }

    trhead.fill( ti );
    tracescale = trhead.postScale( numbfmt );
}


bool SEGYSeisTrcTranslator::writeTapeHeader()
{
    numbfmt = nrFormatFor( storinterp->dataChar() );

    SegyTxtHeader txthead;
    txthead.setClient( pinfo.client );
    txthead.setCompany( pinfo.company[0] ? (const char*)pinfo.company : "dGB" );
    txthead.setAuxInfo( pinfo.auxinfo );
    txthead.setPosInfo( hdef.xcoord, hdef.ycoord, hdef.inl, hdef.crl );
    txthead.setEbcdic();
    if ( !sConn().doIO( txthead.txt, SegyTxtHeaderLength ) )
	{ errmsg = "Cannot write SEG-Y EBCDIC header"; return false; }

    SegyBinHeader binhead;
    binhead.format = numbfmt < 2 ? 1 : numbfmt;
    binhead.lino = pinfo.nr;
    binhead.reno = 1;
    binhead.hns = (short)outcd->nrsamples;
    binhead.hdt = (short)(outcd->sd.step*1e6 + .5);
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


void SEGYSeisTrcTranslator::usePar( const IOPar* iopar )
{
    if ( !iopar ) return;
    SegylikeSeisTrcTranslator::usePar( iopar );

    const char* res = (*iopar)[ mSegyFmt ];
    if ( *res )
	numbfmt = isdigit(*res) ? *res - '0' : 0;
}


void SEGYSeisTrcTranslator::toSupported( DataCharacteristics& dc ) const
{
    dc = getDataChar( nrFormatFor(dc) );
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
