/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uinlapartserv.cc,v 1.16 2005-02-08 16:57:12 bert Exp $
________________________________________________________________________

-*/

#include "uinlapartserv.h"
#include "nlacrdesc.h"
#include "picksettr.h"
#include "welltransl.h"
#include "wellextractdata.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "binidvalset.h"
#include "uiexecutor.h"
#include "uiposdataedit.h"
#include "uimsg.h"
#include "debug.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "featset.h"
#include "featsettr.h"

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
	, trainvds(*new PosVecDataSet("Training data"))
	, testvds(*new PosVecDataSet("Test data"))
	, mcvds(*new PosVecDataSet("Misclassified"))
	, storepars(*new IOPar)
{
}


uiNLAPartServer::~uiNLAPartServer()
{
    deepErase( inpnms );
    delete &trainvds;
    delete &testvds;
    delete &mcvds;
    delete &storepars;
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


static void addFSToSets( ObjectSet<BinIDValueSet>& bivsets, FeatureSet& fs )
{
    const int nrdescs = fs.descs().size();
    BinIDValueSet* bvs = new BinIDValueSet( nrdescs + 1, true );
    bivsets += bvs;
    float vals[nrdescs + 1];
    for ( int idx=0; idx<fs.size(); idx++ )
    {
	const FeatureVec& vec = *fs[idx];
	vals[0] = vec.fvPos().ver;
	for ( int iv=0; iv<vec.size(); iv++ )
	    vals[iv+1] = vec[iv];
	bvs->add( vec.fvPos(), vals );
    }
    fs.erase();
}


static void putBivSetToFS( BinIDValueSet& bvs, FeatureSet& fs )
{
    BinIDValueSet::Pos pos;
    const int nrdescs = fs.descs().size();
    float vals[nrdescs+1];
    BinID binid;
    while ( bvs.next(pos) )
    {
	bvs.get( pos, binid, vals );
        FVPos fvp(binid.inl, binid.crl, vals[0]);
	FeatureVec* vec = new FeatureVec( fvp );
	for ( int idx=1; idx<=nrdescs; idx++ )
	    (*vec) += vals[idx];
	fs += vec;
    }
}

const char* uiNLAPartServer::prepareInputData(
		const ObjectSet<PosVecDataSet>& inpvdss )
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( crdesc.doextraction && crdesc.isdirect )
    {
	// Direct prediction: we need to fetch the well data
	if ( inpvdss.size() != crdesc.outids.size() )
	{
	    if ( DBG::isOn() )
		DBG::message( "uiNLAPartServer::prepareInputData: "
			      "Nr BinIDValue Sets != Nr. well IDs" );
	    return 0;
	}

	// Put the positions in new BinIDValueSets
	ObjectSet<BinIDValueSet> bivsets;
	for ( int idx=0; idx<inpvdss.size(); idx++ )
	{
	    BinIDValueSet* newbvs = new BinIDValueSet( 1, true );
	    bivsets += newbvs;
	    newbvs->append( inpvdss[idx]->data() );
	}

	// Fetch the well data
	Well::LogDataExtracter lde( crdesc.outids, bivsets );
	lde.usePar( crdesc.pars );
	uiExecutor uiex( appserv().parent(), lde );
	if ( uiex.go() )
	{
	    // Add a column to the input data
	    const BufferString outnm = crdesc.design.outputs.get(0);
	    for ( int idx=0; idx<inpvdss.size(); idx++ )
	    {
		PosVecDataSet& vds = *inpvdss[idx];
		DataColDef* newdcd = new DataColDef( outnm );
		newdcd->ref_ = outnm;
		vds.add( newdcd );
		TypeSet<float>& res = *lde.results()[idx];
		const int ressz = res.size();

		BinIDValueSet::Pos pos;
		const int lastidx = vds.data().nrVals() - 1;
		BinID bid; float vals[lastidx+1];
		int ivec = 0;
		while ( vds.data().next(pos) )
		{
		    vds.data().get( pos, bid, vals );
		    vals[lastidx] = ivec >= ressz ? mUndefValue : res[ivec];
		    vds.data().set( pos, vals );
		    ivec++;
		}
	    }
	}

	deepErase( bivsets );
    }

    const char* res = crdesc.prepareData( inpvdss, trainvds, testvds );
    if ( res ) return res;

    // allow user to view and edit data
    ObjectSet<PosVecDataSet> vdss;
    vdss += &trainvds; vdss += &testvds;
    uiPosDataEdit dlg( appserv().parent(), vdss );
    return dlg.go() ? 0 : "User cancel";
}


#define mErrRet(s) { return; }

void uiNLAPartServer::writeSets( CallBacker* )
{
    uiMSG().error( "TODO: Write sets must be from pos data editor" );
    /*
    FeatureSet fswrite( trainvds );
    fswrite.addData( testvds.data() );
    fswrite.pars() = storepars;

    PtrMan<IOObj> ioobj = IOM().get( fsid );
    if ( !ioobj )
	return "Cannot initialise training set storage ...";
    ioobj->pars().setYN( FeatureSetTranslator::sKeyDoVert, true );
    if ( !fswrite.put(ioobj) )
	return "Cannot put training set data ...";

    fswrite.close();
    */
}
