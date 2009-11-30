/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril 
 * DATE     : 2-12-2005
-*/

static const char* rcsID = "$Id: read_terrex_segy.cc,v 1.1 2009-11-30 15:23:46 cvsbert Exp $";

#include "ioobj.h"
#include "iopar.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "segytr.h"
#include "segyhdr.h"
#include "cubesampling.h"
#include "seisselectionimpl.h"
#include "ibmformat.h"
#include "filegen.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "linekey.h"
#include "progressmeter.h"

#include "batchprog.h"
#include <math.h>


class TerrexSEGYSeisTrcTranslator : public SEGYSeisTrcTranslator
{
				  isTranslator(TerrexSEGY,SeisTrc)
public:
    		TerrexSEGYSeisTrcTranslator( const char* s1, const char* s2 )
		    : SEGYSeisTrcTranslator(s1,s2)		{ init(); }
    		TerrexSEGYSeisTrcTranslator()
		    : SEGYSeisTrcTranslator("TerrexSEGY","Terrex SEG-Y")
								{ init(); }

    void	init()
    		{
		    airmag_ = gravity_ = mUdf(float);
		    fe_ = fn_ = 0;
		}

    void	interpretBuf(SeisTrcInfo&);

    float	airmag_;
    float	gravity_;
    double	fe_;
    double	fn_;
};

class TerrexLKProv : public LineKeyProvider
{
public:

		TerrexLKProv( const char* lnm ) : lk_(lnm)	{}
    LineKey	lineKey() const					{ return lk_; }

    LineKey	lk_;

};



void TerrexSEGYSeisTrcTranslator::interpretBuf( SeisTrcInfo& ti )
{
    SEGYSeisTrcTranslator::interpretBuf( ti );
    ti.nr = IbmFormat::asInt( trchead_.buf+20 ); // CDP is good, byte 181 sucks
    ti.coord.x = fe_ + IbmFormat::asFloat( trchead_.buf+190 );
    ti.coord.y = fn_ + IbmFormat::asFloat( trchead_.buf+194 );
    ti.offset = 0;
    airmag_ = IbmFormat::asFloat( trchead_.buf+198 );
    gravity_ = IbmFormat::asFloat( trchead_.buf+202 );
}


#define mErrRet(s) { strm << s << std::endl; return false; }

bool BatchProgram::go( std::ostream& strm )
{
    const char* segyfnm = pars().find( "Input Seismics.File name" );
    if ( !segyfnm || !File_exists(segyfnm) )
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
    CubeSampling cs( false );
    if ( !transl.initRead(new StreamConn(segyfnm,Conn::Read)) )
	mErrRet(transl.errMsg())

    SeisTrcWriter wrr( outioobj );
    Seis::RangeSelData* rsd = new Seis::RangeSelData;
    rsd = new Seis::RangeSelData;
    rsd->setIsAll( true );
    rsd->lineKey().setLineName( linenm );
    wrr.setSelData( rsd );


    pars().get( "False Easting", transl.fe_ );
    pars().get( "False Northing", transl.fn_ );

    SeisTrc trcin, trcout(2501,DataCharacteristics(DataCharacteristics::SI16));
    TextStreamProgressMeter pm( strm );
    while ( transl.read(trcin) )
    {
	for ( int idx=0; idx<2501; idx++ )
	    trcout.set( idx, 5000*trcin.get(idx,0), 0 );
	trcout.info() = trcin.info();

	if ( wrr.put(trcout) )
	    ++pm;
	else
	    mErrRet(wrr.errMsg())
	if ( gravmagsd.usable() )
	{
	    *gravmagsd.ostrm << trcin.info().nr << '\t'
			     << toString(trcin.info().coord.x) << '\t';
	    *gravmagsd.ostrm << toString(trcin.info().coord.y) << '\t'
			     << transl.gravity_ << '\t' << transl.airmag_
			     << '\n';
	}
    }
    gravmagsd.close();

    return true;
}
