/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uinlapartserv.cc,v 1.12 2005-01-31 16:03:19 bert Exp $
________________________________________________________________________

-*/

#include "uinlapartserv.h"
#include "nlacrdesc.h"
#include "picksettr.h"
#include "welltransl.h"
#include "wellextractdata.h"
#include "featset.h"
#include "featsettr.h"
#include "binidvalset.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "debug.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

const int uiNLAPartServer::evPrepareWrite	= 0;
const int uiNLAPartServer::evPrepareRead	= 1;
const int uiNLAPartServer::evReadFinished	= 2;
const int uiNLAPartServer::evGetInputNames	= 3;
const int uiNLAPartServer::evGetStoredInput	= 4;
const int uiNLAPartServer::evGetData		= 5;
const int uiNLAPartServer::evSaveMisclass	= 6;
const int uiNLAPartServer::evCreateAttrSet	= 7;
const int uiNLAPartServer::evIs2D		= 8;


uiNLAPartServer::uiNLAPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, fstrain(*new FeatureSet)
	, fstest(*new FeatureSet)
	, fsmc(*new FeatureSet)
{
}


uiNLAPartServer::~uiNLAPartServer()
{
    deepErase( inpnms );
    delete &fstrain;
    delete &fstest;
    delete &fsmc;
}


bool uiNLAPartServer::willDoExtraction() const
{
    return creationDesc().doextraction;
}


const BufferStringSet& uiNLAPartServer::modelInputs() const
{
    return getModel().design().inputs;
}


void uiNLAPartServer::getBinIDValueSets(
				  ObjectSet<BinIDValueSet>& bivsets ) const
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( !crdesc.isdirect )
	PickSetGroupTranslator::createBinIDValueSets( crdesc.outids, bivsets );
    else
    {
	Executor* ex = WellTranslator::createBinIDValueSets( crdesc.outids,
							     crdesc.pars,
							     bivsets );
	if ( !ex ) return;
	uiExecutor uiex( appserv().parent(), *ex );
	if ( !uiex.go() )
	    deepErase( bivsets );
    }
}


const char* uiNLAPartServer::transferData( const ObjectSet<FeatureSet>& fss,
					   FeatureSet& fswrite )
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( crdesc.doextraction && crdesc.isdirect )
    {
	// Direct prediction: we need to fetch the well data
	if ( fss.size() != crdesc.outids.size() )
	{
	    if ( DBG::isOn() )
		DBG::message( "uiNLAPartServer::transferData: "
			      "Nr Feature Sets != Nr. well IDs" );
	    return 0;
	}

	// Put the positions in BinIDValueSets
	ObjectSet<BinIDValueSet> bivsets;
	for ( int idx=0; idx<fss.size(); idx++ )
	{
	    FeatureSet& fs = *fss[idx];
	    BinIDValueSet* bivset = new BinIDValueSet( 1, true );
	    bivsets += bivset;
	    for ( int ivec=0; ivec<fs.size(); ivec++ )
	    {
		const FVPos& fvp = fs[ivec]->fvPos();
		bivset->add( BinID(fvp.inl,fvp.crl), fvp.ver );
	    }
	}

	// Fetch the well data
	Well::LogDataExtracter lde( crdesc.outids, bivsets );
	lde.usePar( crdesc.pars );
	uiExecutor uiex( appserv().parent(), lde );
	if ( uiex.go() )
	{
	    const BufferString outnm = crdesc.design.outputs.get(0);
	    for ( int idx=0; idx<fss.size(); idx++ )
	    {
		FeatureSet& fs = *fss[idx];
		TypeSet<float>& res = *lde.results()[idx];
		const int ressz = res.size();
		fs.descs() += new FeatureDesc( outnm );

		for ( int ivec=0; ivec<fs.size(); ivec++ )
		{
		    FeatureVec& fv = *fs[ivec];
		    fv += ivec >= ressz ? mUndefValue : res[ivec];
		}
	    }
	}

	deepErase( bivsets );
    }

    const char* res = crdesc.transferData( fss, fsTrain(), fsTest() );
    if ( res ) return res;

    //TODO allow user to view and edit data here

    if ( crdesc.doextraction && crdesc.fsid != "" )
	writeToFS( fswrite, crdesc.fsid );

    return 0;
}


#define mErrRet(s) { ErrMsg(s); return; }

void uiNLAPartServer::writeToFS( FeatureSet& fswrite, const MultiID& fsid )
{
    const FeatureSet& fstrain = fsTrain();
    fswrite.copyStructure( fstrain );

    PtrMan<IOObj> ioobj = IOM().get( fsid );
    if ( !ioobj )
	mErrRet( "Cannot initialise training set storage ..." );
    ioobj->pars().setYN( FeatureSetTranslator::sKeyDoVert, true );
    if ( !fswrite.startPut(ioobj) )
	mErrRet( "Cannot open training set storage ..." );

    FeatureVec fv( FVPos(0,0) );
    for ( int idx=0; idx<fstrain.size(); idx++ )
	{ fstrain.get( fv ); fswrite.put( fv ); }

    const FeatureSet& fstest = fsTest();
    for ( int idx=0; idx<fstest.size(); idx++ )
	{ fstest.get( fv ); fswrite.put( fv ); }

    fswrite.close();
}
