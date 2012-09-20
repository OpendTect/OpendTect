/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril 
 * DATE     : 2-12-2005

 Reading the SEG-Y files downloadable from the ga.gov.au web site.
 The files are indicated to be SEG-Y Rev. 0, but it just looks like that.
 The main problem is that the coordinates are written in a weird place, as
 floating point numbers rather than integers + scaling as the standard requires.
 Further, we need to apply a false easting and northing, only indcated in the
 text header.

 Additionally, we want to extract the grav/mag data (although these are also
 in the various spreadsheets, but this makes it easier to import).

-*/

static const char* rcsID = "$Id$";

#include "ioobj.h"
#include "iopar.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "segytr.h"
#include "segyhdr.h"
#include "ibmformat.h"
#include "file.h"
#include "strmprov.h"
#include "progressmeter.h"
#include "seisselectionimpl.h"

#include "batchprog.h"
#include <math.h>

static const int nrsamples2load = 2501; // 10 seconds
static const DataCharacteristics sampledc( DataCharacteristics::SI16 );
static const float samplescale = 5000;


class TerrexSEGYSeisTrcTranslator : public SEGYSeisTrcTranslator
{
				  isTranslator(TerrexSEGY,SeisTrc)
public:

TerrexSEGYSeisTrcTranslator( const char* s1="TerrexSEGY",
			     const char* s2="Terrex SEG-Y" )
    : SEGYSeisTrcTranslator(s1,s2)
{
    airmag_ = gravity_ = mUdf(float);
    fe_ = fn_ = 0;
}

void interpretBuf( SeisTrcInfo& ti )
{
    SEGYSeisTrcTranslator::interpretBuf( ti );

	// byte 21 (CDP) works, byte 181 is not OK (rather irregular):
    ti.nr = IbmFormat::asInt( trchead_.buf+20 );

	// As indicated in tape/text header:
    ti.coord.x = fe_ + IbmFormat::asFloat( trchead_.buf+190 );
    ti.coord.y = fn_ + IbmFormat::asFloat( trchead_.buf+194 );
    airmag_ = IbmFormat::asFloat( trchead_.buf+198 );
    gravity_ = IbmFormat::asFloat( trchead_.buf+202 );

    ti.offset = 0;
}

    float	airmag_;
    float	gravity_;
    double	fe_;
    double	fn_;
};


#define mErrRet(s) { strm << s << std::endl; return false; }

bool BatchProgram::go( std::ostream& strm )
{
    const char* segyfnm = pars().find( "Input Seismics.File name" );
    if ( !segyfnm || !File::exists(segyfnm) )
	mErrRet("Need valid 'Input Seismics.File name'")

    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    ctxt.deftransl = "2D";
    PtrMan<IOObj> outioobj = getIOObjFromPars( "Output Seismics.Line Set", true,
						ctxt, true );
    if ( !outioobj ) return false;

    const char* linenm = pars().find( "Output Seismics.Line name" );
    if ( !linenm && !*linenm )
	mErrRet("Need 'Output Seismics.Line name'")

    const char* gravmagfnm = pars().find( "Output.GravMag.File name" );
    StreamData gravmagsd;
    if ( gravmagfnm && *gravmagfnm )
    {
	gravmagsd = StreamProvider(gravmagfnm).makeOStream();
	if ( !gravmagsd.usable() )
	    mErrRet(BufferString("Cannot open '",gravmagfnm,"'"))
    }

    TerrexSEGYSeisTrcTranslator transl;
    pars().get( "False Easting", transl.fe_ );
    pars().get( "False Northing", transl.fn_ );
    if ( !transl.initRead(new StreamConn(segyfnm,Conn::Read)) )
	mErrRet(transl.errMsg())

    SeisTrcWriter wrr( outioobj );
    Seis::RangeSelData* rsd = new Seis::RangeSelData;
    rsd->lineKey().setLineName( linenm ); rsd->setIsAll( true );
    wrr.setSelData( rsd );

    SeisTrc trcin, trcout( nrsamples2load, sampledc );
    TextStreamProgressMeter pm( strm );
    while ( transl.read(trcin) )
    {
	for ( int idx=0; idx<2501; idx++ )
	    trcout.set( idx, samplescale*trcin.get(idx,0), 0 );
	trcout.info() = trcin.info();

	// if ( wrr.put(trcout) )
	if ( true )
	    ++pm;
	else
	    mErrRet(wrr.errMsg())

	if ( gravmagsd.usable() )
	{
	    *gravmagsd.ostrm << '"' << linenm << "\"\t"
			     << trcin.info().nr << '\t'
			     << toString(trcin.info().coord.x) << '\t';
	    *gravmagsd.ostrm << toString(trcin.info().coord.y) << '\t'
			     << transl.gravity_ << '\t' << transl.airmag_
			     << '\n';
	}
    }
    gravmagsd.close();

    return true;
}
