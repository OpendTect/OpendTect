/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/

static const char* rcsID = "$Id: segydirecttr.cc,v 1.1 2008-11-20 13:24:21 cvsbert Exp $";

#include "segydirecttr.h"
#include "segydirectdef.h"
#include "posinfo.h"
#include "filepath.h"


class SEGYDirectPSIOProvider : public SeisPSIOProvider
{
public:
			SEGYDirectPSIOProvider()
			    	: SeisPSIOProvider("SEGYDirect")
    			{}
    SeisPS3DReader*	make3DReader( const char* fnm, int ) const
			{ return new SEGYDirect3DPSReader(fnm); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm, const char* lnm ) const
			{ return new SEGYDirect2DPSReader(dirnm,lnm); }
    SeisPSWriter*	make2DWriter( const char* dirnm, const char* lnm ) const
			{ return 0; }
    bool		getLineNames(const char*,BufferStringSet&) const
			{ return false; }
    static int		factid;
};

// This adds the Multicube type pre-stack seismics data storage to the factory
int SEGYDirectPSIOProvider::factid = SPSIOPF().add(new SEGYDirectPSIOProvider);


SEGYDirect3DPSReader::SEGYDirect3DPSReader( const char* fnm )
    : posdata_(*new PosInfo::CubeData)
    , def_(*new SEGY::DirectDef(fnm))
{
}


SEGYDirect3DPSReader::~SEGYDirect3DPSReader()
{
    delete &posdata_;
    delete &def_;
}


SeisTrc* SEGYDirect3DPSReader::getTrace( const BinID& bid, int nr ) const
{
    return 0;
}


bool SEGYDirect3DPSReader::getGather( const BinID& bid, SeisTrcBuf& tb ) const
{
    return false;
}


SEGYDirect2DPSReader::SEGYDirect2DPSReader( const char* dirnm, const char* lnm )
    : SeisPS2DReader(lnm)
    , posdata_(*new PosInfo::Line2DData)
    , def_(*new SEGY::DirectDef(fileName(dirnm,lnm)))
{
}


SEGYDirect2DPSReader::~SEGYDirect2DPSReader()
{
    delete &posdata_;
    delete &def_;
}


SeisTrc* SEGYDirect2DPSReader::getTrace( const BinID& bid, int nr ) const
{
    return 0;
}


bool SEGYDirect2DPSReader::getGather( const BinID& bid, SeisTrcBuf& tb ) const
{
    return false;
}


const char* SEGYDirect2DPSReader::fileName( const char* dirnm, const char* lnm )
{
    FilePath fp( dirnm ); fp.add( lnm );
    static BufferString ret; ret = fp.fullPath();
    FilePath::mkCleanPath( ret.buf(), FilePath::Local );
    return ret.buf();
}

