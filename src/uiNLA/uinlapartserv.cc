/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uinlapartserv.cc,v 1.9 2004-05-12 15:03:49 bert Exp $
________________________________________________________________________

-*/

#include "uinlapartserv.h"
#include "nlacrdesc.h"
#include "picksettr.h"
#include "welltransl.h"
#include "wellextractdata.h"
#include "featset.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "debug.h"

const int uiNLAPartServer::evPrepareWrite	= 0;
const int uiNLAPartServer::evPrepareRead	= 1;
const int uiNLAPartServer::evReadFinished	= 2;
const int uiNLAPartServer::evGetInputNames	= 3;
const int uiNLAPartServer::evGetStoredInput	= 4;
const int uiNLAPartServer::evGetData		= 5;
const int uiNLAPartServer::evSaveMisclass	= 6;
const int uiNLAPartServer::evCreateAttrSet	= 7;


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


void uiNLAPartServer::getBinIDValues(
			  ObjectSet< TypeSet<BinIDValue> >& bivsets ) const
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( !crdesc.isdirect )
	PickSetGroupTranslator::createBinIDValues( crdesc.outids, bivsets );
    else
    {
	Executor* ex = WellTranslator::createBinIDValues( crdesc.outids,
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
	ObjectSet< TypeSet<BinIDValue> > bivsets;
	for ( int idx=0; idx<fss.size(); idx++ )
	{
	    FeatureSet& fs = *fss[idx];
	    TypeSet<BinIDValue>* bivset = new TypeSet<BinIDValue>;
	    bivsets += bivset;
	    for ( int ivec=0; ivec<fs.size(); ivec++ )
	    {
		const FVPos& fvp = fs[ivec]->fvPos();
		*bivset += BinIDValue( fvp.inl, fvp.crl, fvp.ver );
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

    return crdesc.transferData( fss, fsTrain(), fsTest(), &fswrite );
}
