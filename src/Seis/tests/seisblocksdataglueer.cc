/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2019
-*/


#include "testprog.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "moddepmgr.h"
#include "plugins.h"
#include "seisblocksdataglueer.h"
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "seisstorer.h"
#include "seistype.h"


#define mErrRetIfNotOK( uirv ) \
    if ( !uirv.isOK() ) \
    { \
	tstStream(true) << toString(uirv) << od_endl; \
	return false; \
    }

#define mAddBlock( inl, crl, z ) \
    setVals( bid, arr, inl, crl, z ); \
    uirv = dg.addData( bid, z, arr ); \
    mErrRetIfNotOK( uirv )

#define mAddBlocks( inl, crl ) \
    mAddBlock( inl, crl, 1.0f ) \
    mAddBlock( inl, crl, 1.02f ) \
    mAddBlock( inl, crl, 1.04f )

static DBKey dbky_;


static void setVals( BinID& bid, Array3D<float>& arr, int inl, int crl, float z)
{
    bid = BinID( inl, crl );
    const auto& arrinf = arr.info();
    for ( int iinl=0; iinl<arrinf.getSize(0); iinl++ )
    {
	for ( int icrl=0; icrl<arrinf.getSize(1); icrl++ )
	{
	    const float posval = (inl+iinl) * 10 + crl + icrl;
	    for ( int iz=0; iz<arrinf.getSize(2); iz++ )
	    {
		const float val = posval + z*1000000.f + iz*10000.f;
		arr.set( iinl, icrl, iz, val );
	    }
	}
    }
}


static bool testCreate()
{
    IOObjContext* ctxt = getIOObjContext( Seis::Vol, false );
    CtxtIOObj ctio( *ctxt );
    ctio.setName( "Blocks Glueing Test" );
    ctio.fillObj( true );
    dbky_ = ctio.ioobj_->key();
    Seis::Storer strr( *ctio.ioobj_ );

    const CubeSubSel css( StepInterval<int>(400, 410, 1 ),
			  StepInterval<int>(600, 620, 1 ),
			  ZSampling(1.0f,1.5f,0.004f) );
    const Seis::RangeSelData sd( css );
    Seis::Blocks::DataGlueer dg( sd, strr, Coord3f(0.f, 0.f, 0.f),
				 Seis::Blocks::DataGlueer::Average );
    dg.setSteps( BinID(2,1), 0.004f );
    uiRetVal uirv;
    Array3DImpl<float> arr( 6, 10, 14 );
    BinID bid;

    mAddBlocks( 400, 600 )
    mAddBlocks( 400, 604 )
    mAddBlocks( 400, 608 )
    mAddBlocks( 404, 602 )
    mAddBlocks( 404, 606 )
    mAddBlocks( 404, 610 )
    mAddBlocks( 408, 604 )
    mAddBlocks( 408, 608 )
    mAddBlocks( 408, 612 )

    uirv = dg.finish();
    mErrRetIfNotOK( uirv )

    return true;
}


static bool testReadBack()
{
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    PIM().loadAuto( true );
    OD::ModDeps().ensureLoaded("Seis");
    PIM().loadAuto( false );

    if ( !testCreate() )
	return 1;
    if ( !testReadBack() )
	return 1;

    return 0;
}
