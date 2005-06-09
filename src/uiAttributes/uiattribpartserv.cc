/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiattribpartserv.cc,v 1.1 2005-06-09 13:11:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattribpartserv.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribdescsetman.h"
#include "attribengman.h"
#include "attribsel.h"
#include "attribslice.h"
#include "executor.h"
#include "ctxtioobj.h"
#include "binidselimpl.h"
#include "cubesampling.h"
#include "binidvalset.h"
#include "nlacrdesc.h"
#include "nlamodel.h"
#include "featset.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uiattrvolout.h"
#include "uiattrdescseted.h"
#include "uiioobjsel.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uisetpickdirs.h"
#include "uicolorattrsel.h"
#include "attribsetcreator.h"
#include "uiseisioobjinfo.h"

//#include "uievaluatedlg.h"
#include "uiattrdesced.h"

using namespace Attrib;

const int uiAttribPartServer::evDirectShowAttr	 = 0;
const int uiAttribPartServer::evNewAttrSet	 = 1;
const int uiAttribPartServer::evAttrSetDlgClosed = 2;
const int uiAttribPartServer::evEvalAttrInit 	 = 3;
const int uiAttribPartServer::evEvalCalcAttr	 = 4;
const int uiAttribPartServer::evEvalShowSlice	 = 5;
const int uiAttribPartServer::evEvalStoreSlices	 = 6;
const int uiAttribPartServer::objNLAModel	 = 100;

const char* uiAttribPartServer::attridstr = "Attrib ID";


uiAttribPartServer::uiAttribPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, adsman(new DescSetMan)
	, dirshwattrdesc(0)
        , attrsetdlg(0)
    	, attrsetclosetim("Attrset dialog close")
{
    attrsetclosetim.tick.notify( 
			mCB(this,uiAttribPartServer,attrsetDlgCloseTimTick) );
}


uiAttribPartServer::~uiAttribPartServer()
{
    delete adsman;
    delete attrsetdlg;
}


void uiAttribPartServer::replaceSet( const IOPar& iopar )
{
    delete adsman;
    adsman = new DescSetMan;
    adsman->descSet()->usePar( iopar );
    adsman->attrsetid_ = "";
    sendEvent( evNewAttrSet );
}


bool uiAttribPartServer::addToDescSet( const char* id )
{
    int res = adsman->descSet()->getStoredID( id );
    return res < 0 ? false : true;
}


const DescSet* uiAttribPartServer::curDescSet() const
{
    return adsman->descSet();
}


void uiAttribPartServer::getDirectShowAttrSpec( SelSpec& as ) const
{
   if ( !dirshwattrdesc )
       as.set( 0, -2, false, 0 );
   else
       as.set( *dirshwattrdesc );
}


bool uiAttribPartServer::editSet()
{
    IOPar iop;
    if ( adsman->descSet() ) adsman->descSet()->fillPar( iop );

    DescSet* oldset = adsman->descSet();
    delete attrsetdlg;
    attrsetdlg = new uiAttribDescSetEd( appserv().parent(), adsman );
    attrsetdlg->dirshowcb.notify( mCB(this,uiAttribPartServer,directShowAttr) );
    attrsetdlg->evalattrcb.notify( mCB(this,uiAttribPartServer,showEvalDlg) );
    attrsetdlg->windowClosed.notify( 
	    			mCB(this,uiAttribPartServer,attrsetDlgClosed) );
    return attrsetdlg->go();
}


void uiAttribPartServer::attrsetDlgClosed( CallBacker* )
{
    attrsetclosetim.start( 10, true );
}


void uiAttribPartServer::attrsetDlgCloseTimTick( CallBacker* )
{
    if ( attrsetdlg->uiResult() )
    {
	adsman->setDescSet( attrsetdlg->getSet() );
	adsman->attrsetid_ = attrsetdlg->curSetID();
	sendEvent( evNewAttrSet );
    }

    delete attrsetdlg;
    attrsetdlg = 0;
    sendEvent( evAttrSetDlgClosed );
}


const NLAModel* uiAttribPartServer::getNLAModel() const
{
    return (NLAModel*)getObject( objNLAModel );
}


bool uiAttribPartServer::selectAttrib( SelSpec& selspec )
{
    uiAttrSelData attrdata( adsman->descSet() );
    attrdata.attribid = selspec.isNLA() ? -2 : selspec.id();
    attrdata.outputnr = selspec.isNLA() ? selspec.id() : -1;
    attrdata.nlamodel = getNLAModel();
    uiAttrSelDlg dlg( appserv().parent(), "View Data", attrdata, No2D );
    if ( !dlg.go() )
	return false;

    attrdata.attribid = dlg.attribID();
    attrdata.outputnr = dlg.outputNr();
    const bool isnla = attrdata.attribid < 0 && attrdata.outputnr >= 0;
    IOObj* ioobj = IOM().get( adsman->attrsetid_ );
    BufferString attrsetnm = ioobj ? ioobj->name() : "";
    selspec.set( 0, isnla ? attrdata.outputnr : attrdata.attribid, isnla,
	         isnla ? (const char*)nlaname : (const char*)attrsetnm );
    if ( isnla && attrdata.nlamodel )
	selspec.setRefFromID( *attrdata.nlamodel );
    else if ( !isnla )
	selspec.setRefFromID( *adsman->descSet() );

    return true;
}


void uiAttribPartServer::directShowAttr( CallBacker* cb )
{
    mDynamicCastGet(uiAttribDescSetEd*,ed,cb);
    if ( !ed ) { pErrMsg("cb is not uiAttribDescSetEd*"); return; }
    dirshwattrdesc = ed->curDesc();
    DescSetMan* kpman = adsman;
    DescSet* edads = const_cast<DescSet*>(dirshwattrdesc->descSet());
    DescSetMan tmpadsman( edads, false );
    adsman = &tmpadsman;
    sendEvent( evDirectShowAttr );
    adsman = kpman;
}


void uiAttribPartServer::updateSelSpec( SelSpec& ss ) const
{
    if ( ss.isNLA() )
    {
	const NLAModel* nlamod = getNLAModel();
	if ( nlamod )
	{
	    ss.setIDFromRef( *nlamod );
	    ss.setObjectRef( nlaname );
	}
	else
	    ss.set( ss.userRef(), -2, true, 0 );
    }
    else
    {
	const DescSet& ads = *adsman->descSet();
	if ( ads.is2D() ) return;
	ss.setIDFromRef( ads );
	IOObj* ioobj = IOM().get( adsman->attrsetid_ );
	if ( ioobj ) ss.setObjectRef( ioobj->name() );
    }
}


void uiAttribPartServer::getPossibleOutputs( BufferStringSet& nms ) const
{
    nms.erase();
    SelInfo attrinf( curDescSet() );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
	nms.add( attrinf.attrnms.get(idx) );
    for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
    {
	const char* ioobjid = attrinf.ioobjids.get( idx );
	uiSeisIOObjInfo* sii = new uiSeisIOObjInfo( MultiID(ioobjid), false );
	sii->getDefKeys( nms, true );
	delete sii;
    }
}


bool uiAttribPartServer::setSaved() const
{
    return adsman->isSaved();
}


void uiAttribPartServer::saveSet()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(AttribDescSet);
    ctio->ctxt.forread = false;
    uiIOObjSelDlg dlg( appserv().parent(), *ctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	ctio->ioobj = 0;
	ctio->setObj( dlg.ioObj()->clone() );
	BufferString bs;
	if ( !ctio->ioobj )
	    uiMSG().error("Cannot find attribute set in data base");
	else if ( !AttribDescSetTranslator::store(*adsman->descSet(),
						  ctio->ioobj,bs) )
	    uiMSG().error(bs);
    }
    ctio->setObj( 0 );
}


void uiAttribPartServer::outputVol( MultiID& nlaid )
{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return; }

    uiAttrVolOut dlg( appserv().parent(), *adsman->descSet(),
	    	      getNLAModel(), nlaid );
    dlg.go();
}


Attrib::SliceSet* uiAttribPartServer::createOutput( const CubeSampling& cs,
						  const SelSpec& selspec,
						  const SliceSet* cache,
       						  const DescSet* ads )
{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return 0; }
    else if ( selspec.id() == -2 ) { pErrMsg("selspec id == -2"); return 0; }
/*
    EngineMan aem;
    aem.setAttribSet( ads ? ads : adsman->descSet() );
    aem.setNLAModel( getNLAModel() );
    aem.setAttribSpec( selspec );
    aem.setCubeSampling( cs );

    BufferString errmsg;
    PtrMan<Executor> outex = aem.cubeOutputCreater( errmsg, cache );
    if ( !outex )
	{ uiMSG().error(errmsg); return 0; }

    uiExecutor dlg( appserv().parent(), *outex );
    if ( !dlg.go() ) return 0;

    return aem.getCubeOutput( outex );
*/
    return 0;
}


bool uiAttribPartServer::createOutput( ObjectSet<BinIDValueSet>& values,
					const SelSpec& selspec )
{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return 0; }
    else if ( selspec.id() == -2 ) { pErrMsg("selspec id == -2"); return 0; }

/*
    EngineMan aem;
    aem.setAttribSet( adsman->descSet() );
    aem.setNLAModel( getNLAModel() );
    aem.setAttribSpec( selspec );

    BufferString errmsg;
    PtrMan<Executor> outex = aem.tableOutputCreator( errmsg, values );
    if ( !outex )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( appserv().parent(), *outex );
    if ( !dlg.go() ) return false;
*/

    return true;
}


bool uiAttribPartServer::createOutput( const TypeSet<BinID>& bids,
				       const Interval<float>& zrg,
				       SeisTrcBuf& output,
				       const SelSpec& selspec )
{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return 0; }
    else if ( selspec.id() == -2 ) { pErrMsg("selspec id == -2"); return 0; }

/*
    EngineMan aem;
    aem.setAttribSet( adsman->descSet() );
    aem.setNLAModel( getNLAModel() );
    aem.setAttribSpec( selspec );

    BufferString errmsg;
    PtrMan<Executor> outex =
	aem.trcSelOutputCreator( errmsg, bids, zrg, output );
    if ( !outex )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( appserv().parent(), *outex );
    if ( !dlg.go() ) return false;
*/

    return true;
}


class FeatSetAttribOutGen : public Executor
{
public:

FeatSetAttribOutGen( DescSet* as, const NLAModel* m,
    		     const BufferStringSet& lnms, const BufferStringSet& in,
		     const ObjectSet<BinIDValueSet>& b,
		     ObjectSet<FeatureSet>& f )
	: Executor("Extracting")
	, ads(as)
	, nlamodel(m)
	, linenames(lnms)
	, inps(in)
	, bvss(b)
	, fss(f)
	, curlnr(0)
	, aem(0)
	, outex(0)
{
    nextExec();
}

~FeatSetAttribOutGen()
{
    cleanUp();
}

void cleanUp()
{
    delete aem; delete outex; deepErase( workfss );
}

const char* message() const
{
    if ( !outex ) return "Scanning data";

    msg = outex->message();
    if ( curlnr < linenames.size() )
    {
	const char* lnm = linenames.get(curlnr);
	if ( lnm && *lnm )
	{
	    msg += " for ";
	    msg += linenames.get(curlnr);
	}
    }
    return msg.buf();
}

const char* nrDoneText() const
{ return outex ? outex->nrDoneText() : "Positions handled"; }
int nrDone() const
{ return outex ? outex->nrDone() : 0; }
int totalNr() const
{ return outex ? outex->totalNr() : -1; }


void nextExec()
{
/*
    cleanUp();
    aem = new EngineMan;
    aem->setAttribSet( ads );
    aem->setNLAModel( nlamodel );
    aem->setLineKey( linenames.get(curlnr) );
    outex = aem->featureOutputCreator( inps, bvss, workfss );
    setName( outex->name() );
*/
}


void addResults()
{
    if ( curlnr == 0 )
    {
	while ( workfss.size() )
	    { FeatureSet* fs = workfss[0]; fss += fs; workfss -= fs; }
    }
    else
    {
	for ( int ifs=0; ifs<workfss.size(); ifs++ )
	{
	    FeatureSet& fsnew = *workfss[ifs];
	    FeatureSet& fs = *fss[ifs];
	    while ( fsnew.size() )
		fs += fsnew.releaseLast();
	}
    }
}


int nextStep()
{
    if ( !outex ) return -1;

    int res = outex->doStep();
    if ( res < 0 ) return res;
    if ( res == 0 )
    {
	addResults();
	curlnr++;
	if ( curlnr >= linenames.size() )
	    return 0;
	nextExec();
	return 1;
    }

    return 1;
}

    ObjectSet<FeatureSet>	workfss;
    const BufferStringSet&	inps;
    const BufferStringSet&	linenames;
    const DescSet*	ads;
    const NLAModel*		nlamodel;
    const ObjectSet<BinIDValueSet>&	bvss;
    ObjectSet<FeatureSet>&	fss;
    int				curlnr;
    EngineMan*			aem;
    Executor*			outex;
    mutable BufferString	msg;
};


bool uiAttribPartServer::extractFeatures( const NLACreationDesc& desc,
					const ObjectSet<BinIDValueSet>& bivsets,
					ObjectSet<FeatureSet>& outfss )
{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return 0; }

/*
    if ( !desc.doextraction )
    {
	PtrMan<IOObj> ioobj = IOM().get( desc.fsid );
	FeatureSet* fs = FeatureSet::get( ioobj );
	if ( !fs || !fs->pars().size() || !fs->size() )
	{
	    uiMSG().error( "Invalid Training set specified" );
	    return false;
	}

	outfss += fs;
	return true;
    }

    bool foundpos = false;
    PtrMan<BinIDValueSet> allbivs = 0;
    for ( int idx=0; idx<bivsets.size(); idx++ )
    {
	if ( !idx )
	    allbivs = new BinIDValueSet( *bivsets[idx] );
	else
	    allbivs->append( *bivsets[idx] );
    }
    if ( allbivs->isEmpty() )
	{ uiMSG().error( "No data extraction points" ); return false; }

    BufferStringSet linenames;
    if ( !adsman->descSet()->is2D() )
	linenames.add( "" );
    else
    {
	MultiID key;
	if ( !adsman->descSet()->getFirstStored(Only2D,key) )
	{
	    ErrMsg( "Internal: Cannot find line set in attribute set." );
	    return false;
	}
	uiSeisIOObjInfo oi( key );
	if ( !oi.isOK() ) return false;
	oi.getLineNames( linenames, false, allbivs );
	if ( linenames.size() < 1 )
	{
	    uiMSG().error( "No line with any extraction position found" );
	    return false;
	}
    }

    FeatSetAttribOutGen fsag( adsman->descSet(), getNLAModel(), linenames,
	    		      desc.design.inputs, bivsets, outfss );
    uiExecutor dlg( appserv().parent(), fsag );
    if ( !dlg.go() )
	return false;
    else if ( outfss.size() != bivsets.size() )
    {
	// Earlier error
	ErrMsg( "Error during data extraction (not all data extracted)." );
	return false;
    }
*/

    return true;
}


void uiAttribPartServer::fillPar( IOPar& iopar ) const
{
    if ( adsman->descSet() && adsman->descSet()->nrDescs() )
	adsman->descSet()->fillPar( iopar );
}


void uiAttribPartServer::usePar( const IOPar& iopar )
{
    if ( adsman->descSet() )
    {
	BufferStringSet errmsgs;
	adsman->descSet()->usePar( iopar, &errmsgs );
	BufferString errmsg;
	for ( int idx=0; idx<errmsgs.size(); idx++ )
	{
	    if ( !idx )
		errmsg = "Error during restore of Attribute Set:";
	    errmsg += "\n";
	    errmsg += errmsgs.get( idx );
	}
	if ( errmsg != "" )
	    uiMSG().warning( errmsg );

	sendEvent( evNewAttrSet );
    }
}


bool uiAttribPartServer::createOutput( const CubeSampling& cs,
				       SliceSet* sliceset )
{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return false; }
    else if ( !targetspecs.size() ) { pErrMsg("Nothing to do"); return 0; }

    sliceset->direction = (Slice::Dir)cs.defaultDir();
/*
    EngineMan aem;
    aem.setAttribSet( adsman->descSet() );
    aem.setNLAModel( getNLAModel() );
    aem.setCubeSampling( cs );
    aem.setAttribSpec( targetspecs[0] );

    for ( int idx=1; idx<targetspecs.size(); idx++ )
	aem.addOutputAttrib( targetspecs[idx].id() );

    BufferString errmsg;
    PtrMan<Executor> outex = aem.cubeOutputCreater( errmsg, 0 );
    if ( !outex )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( appserv().parent(), *outex );
    if ( !dlg.go() ) return false; 

    SliceSet* slcset = aem.getCubeOutput( outex );
    if ( !slcset ) return false;

    sliceset->sampling = slcset->sampling;
    for ( int idx=0; idx<slcset->size(); idx++ )
	(*sliceset) += (*slcset)[idx];
*/
    return true;
}


bool uiAttribPartServer::createOutput( ObjectSet<BinIDValueSet>& values )					
{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return 0; }
    else if ( !targetspecs.size() ) { pErrMsg("Nothing to do"); return 0; }
/*
    EngineMan aem;
    aem.setAttribSet( adsman->descSet() );
    aem.setNLAModel( getNLAModel() );
    aem.setAttribSpec( targetspecs[0] );

    for ( int idx=1; idx<targetspecs.size(); idx++ )
	aem.addOutputAttrib( targetspecs[idx].id() );
    
    BufferString errmsg;
    PtrMan<Executor> outex = aem.tableOutputCreator( errmsg, values );
    if ( !outex )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( appserv().parent(), *outex );
    if ( !dlg.go() ) return false;
*/
    return true;
}


SeisTrcBuf* uiAttribPartServer::create2DOutput( const CubeSampling& cs,
					        const SelSpec& selspec,
						const char* linekey )

{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return 0; }
    else if ( selspec.id() == -2 ) { pErrMsg("selspec id == -2"); return 0; }
/*
    EngineMan aem;
    aem.setAttribSet( adsman->descSet() );
    aem.setNLAModel( getNLAModel() );
    aem.setAttribSpec( selspec );
    aem.setCubeSampling( cs );
    aem.setLineKey( linekey );

    BufferString errmsg;
    PtrMan<Executor> outex = aem.cubeOutputCreater( errmsg, 0 );
    if ( !outex )
	{ uiMSG().error(errmsg); return 0; }

    uiExecutor dlg( appserv().parent(), *outex );
    if ( !dlg.go() ) return 0;

    return aem.get2DLineOutput( outex );
*/
    return 0;
}


bool uiAttribPartServer::createAttributeSet(const BufferStringSet& inps,
					    DescSet* attrset )
{
    AttributeSetCreator attrsetcr( appserv().parent(), inps, attrset );
    return attrsetcr.create();
}




bool uiAttribPartServer::setPickSetDirs( PickSet& ps, const NLAModel* nlamod )
{
    uiSetPickDirs dlg( appserv().parent(), ps, curDescSet(), nlamod );
	return dlg.go();
}


bool uiAttribPartServer::selectColorAttrib( ColorSelSpec& selspec )
{
    uiAttrSelData attrdata( adsman->descSet() );
    attrdata.attribid = selspec.as.isNLA() ? -2 : selspec.as.id();
    attrdata.outputnr = selspec.as.isNLA() ? selspec.as.id() : -1;
    attrdata.nlamodel = getNLAModel();

    uiColorAttrSel dlg( appserv().parent(), attrdata );
    dlg.setPars( selspec );

    if ( !dlg.go() )
	return false;

    dlg.getPars( selspec );
    attrdata.attribid = dlg.attribID();
    attrdata.outputnr = dlg.outputNr();
    const bool isnla = attrdata.outputnr >= 0;
    IOObj* ioobj = IOM().get( adsman->attrsetid_ );
    BufferString attrsetnm = ioobj ? ioobj->name() : "";
    selspec.as.set( 0, isnla ? attrdata.outputnr : attrdata.attribid, isnla,
	            isnla ? (const char*)nlaname : (const char*)attrsetnm );
    if ( isnla )
	selspec.as.setRefFromID( *attrdata.nlamodel );
    else
	selspec.as.setRefFromID( *adsman->descSet() );

    return true;
}

#define mInsertItems(list,mnu,correcttype) \
mnu->setEnabled( attrinf.list.size() ); \
for ( int idx=start; idx<stop; idx++ ) \
{ \
    const BufferString& nm = attrinf.list.get(idx); \
    uiMenuItem* itm = new uiMenuItem( nm ); \
    mnu->insertItem( itm, mnuid, idx ); \
    const bool docheck = correcttype && nm == as.userRef(); \
    itm->setChecked( docheck ); \
    if ( docheck ) mnu->setChecked( true ); \
    mnuid++; \
}


static int cMaxMenuSize = 150;

uiPopupMenu* uiAttribPartServer::createStoredCubesSubMenu( int& mnuid,
						   const SelSpec& as )
{
    uiPopupMenu* cubemnu = new uiPopupMenu( appserv().parent(), "Stored Cubes");
    SelInfo attrinf( adsman->descSet(), 0, No2D, -1 );
    const bool isnla = as.isNLA();
    const bool hasid = as.id() >= 0;
    const int nritems = attrinf.ioobjnms.size();
    if ( nritems <= cMaxMenuSize )
    {
	const int start = 0; const int stop = attrinf.ioobjnms.size();
	mInsertItems(ioobjnms,cubemnu,!isnla&&hasid);
    }
    else
    {
	const int nrsubmnus = (nritems-1)/cMaxMenuSize + 1;
	bool cubemnuchecked = false;
	for ( int mnuidx=0; mnuidx<nrsubmnus; mnuidx++ )
	{
	    const int start = mnuidx * cMaxMenuSize;
	    int stop = (mnuidx+1) * cMaxMenuSize;
	    if ( stop > nritems ) stop = nritems;
	    const char* startnm = attrinf.ioobjnms.get(start);
	    const char* stopnm = attrinf.ioobjnms.get(stop-1);
	    BufferString str; strncat(str.buf(),startnm,3); 
	    str += " - "; strncat(str.buf(),stopnm,3);
	    uiPopupMenu* submnu = new uiPopupMenu( appserv().parent(), str );
	    cubemnu->insertItem( submnu );
	    mInsertItems(ioobjnms,submnu,!isnla&&hasid);
	    if ( !cubemnuchecked && submnu->isChecked() )
	    {
		cubemnu->setChecked( true ); 
		cubemnuchecked = true;
	    }
	}
    }

    return cubemnu;
}


uiPopupMenu* uiAttribPartServer::createAttribSubMenu( int& mnuid,
						      const SelSpec& as )
{
    uiPopupMenu* attrmnu = new uiPopupMenu( appserv().parent(), "Attributes" );
    SelInfo attrinf( adsman->descSet() );
    const bool isattrib = attrinf.attrids.indexOf( as.id() ) >= 0;
    const int start = 0; const int stop = attrinf.attrnms.size();
    mInsertItems(attrnms,attrmnu,isattrib);
    return attrmnu;
}


uiPopupMenu* uiAttribPartServer::createNLASubMenu( int& mnuid,
						   const SelSpec& as )
{
    const NLAModel* nlamodel = getNLAModel();
    if ( !nlamodel ) return 0;
    uiPopupMenu* nlamnu = new uiPopupMenu( appserv().parent(), 
	    				   nlamodel->nlaType(false) );
    SelInfo attrinf( adsman->descSet(), nlamodel );
    const bool isnla = as.isNLA();
    const bool hasid = as.id() >= 0;
    const int start = 0; const int stop = attrinf.nlaoutnms.size();
    mInsertItems(nlaoutnms,nlamnu,isnla);
    return nlamnu;
}


bool uiAttribPartServer::handleAttribSubMenu( int mnuid, int type,
					      SelSpec& as )
{
    uiAttrSelData attrdata( adsman->descSet() );
    attrdata.nlamodel = getNLAModel();
    SelInfo attrinf( attrdata.attrset, attrdata.nlamodel, No2D );

    const bool isstored = !type;
    const bool isattr = type==1;
    const bool isnla = type==2;

    int selidx = mnuid;
    int attribid = -2;
    int outputnr = -1;
    if ( isattr )
	attribid = attrinf.attrids[selidx];
    else if ( isnla )
	outputnr = selidx;
    else if ( isstored )
    {
	attribid = adsman->descSet()->getStoredID( 
						attrinf.ioobjids.get(selidx) );
    }

    IOObj* ioobj = IOM().get( adsman->attrsetid_ );
    BufferString attrsetnm = ioobj ? ioobj->name() : "";
    as.set( 0, isnla ? outputnr : attribid, isnla,
	    isnla ? (const char*)nlaname : (const char*)attrsetnm );

    if ( isnla )
	as.setRefFromID( *attrdata.nlamodel );
    else
	as.setRefFromID( *adsman->descSet() );

    return true;
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiAttribPartServer::showEvalDlg( CallBacker* )
{
    /*
    if ( !attrsetdlg ) return;
    const Desc* curdesc = attrsetdlg->curDesc();
    if ( !curdesc )
	mErrRet( "Please add this attribute first" )

    uiAttrDescEd* ade = attrsetdlg->curDescEd();
    if ( !ade ) return;
    const DescSet* curattrset = attrsetdlg->getSet();
    if ( !curattrset || !ade->attrparset.size() )
	mErrRet( "Cannot evaluate this attribute" );

    sendEvent( evEvalAttrInit );
    if ( !alloweval ) mErrRet( "Evaluation of attributes only possible on\n"
			       "Inlines, Crosslines, Timeslices and Surfaces.");

    attrsetdlg->setSensitive( false );
    uiEvaluateDlg* evaldlg = new uiEvaluateDlg( appserv().parent(), *ade, 
	    					*curattrset, curdesc->id(), 
						allowevalstor );
    evaldlg->calccb.notify( mCB(this,uiAttribPartServer,calcEvalAttrs) );
    evaldlg->showslicecb.notify( mCB(this,uiAttribPartServer,showSliceCB) );
    evaldlg->windowClosed.notify( mCB(this,uiAttribPartServer,evalDlgClosed) );
    evaldlg->go();
    */
}


void uiAttribPartServer::evalDlgClosed( CallBacker* cb )
{
    /*
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    if ( !evaldlg ) { pErrMsg("cb is not uiEvaluateDlg*"); return; }

    if ( evaldlg->storeSlices() )
	sendEvent( evEvalStoreSlices );
    
    Desc* curdesc = attrsetdlg->curDesc();
    BufferString curusrref = curdesc->userRef();
    uiAttrDescEd* ade = attrsetdlg->curDescEd();

    DescSet* curattrset = attrsetdlg->getSet();
    const Desc* evad = evaldlg->getAttribDesc();
    Desc* ad = evad ? evad->clone( curattrset ) : 0;
    if ( ad && ade ) 
    {
	ad->setUserRef( curusrref );
	ade->setDesc( ad, adsman );
	IOPar iopar;
	ad->fillPar( iopar );
	curdesc->usePar( iopar );
    }

    */
    attrsetdlg->setSensitive( true );
}


void uiAttribPartServer::calcEvalAttrs( CallBacker* cb )
{
/*
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    if ( !evaldlg ) { pErrMsg("cb is not uiEvaluateDlg*"); return; }

    DescSetMan* kpman = adsman;
    DescSet* ads = evaldlg->getEvalSet();
    evaldlg->getEvalSpecs( targetspecs );
    DescSetMan tmpadsman( ads, false );
    adsman = &tmpadsman;
    sendEvent( evEvalCalcAttr );
    adsman = kpman;
*/
}


void uiAttribPartServer::showSliceCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( sel < 0 ) return;

    sliceidx = sel;
    sendEvent( evEvalShowSlice );
}


void uiAttribPartServer::getTargetAttribNames( BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<targetspecs.size(); idx++ )
	nms.add( targetspecs[idx].userRef() );
}
