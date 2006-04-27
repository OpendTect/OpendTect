/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiattribpartserv.cc,v 1.30 2006-04-27 21:43:52 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattribpartserv.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribdataholder.h"
#include "attribdescsetman.h"
#include "attribprocessor.h"
#include "attribstorprovider.h"
#include "attribposvecoutput.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribinit.h"
#include "attribsel.h"
#include "attribdatacubes.h"
#include "executor.h"
#include "ctxtioobj.h"
#include "binidselimpl.h"
#include "cubesampling.h"
#include "binidvalset.h"
#include "nlacrdesc.h"
#include "nlamodel.h"
#include "iodir.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"
#include "survinfo.h"
#include "seisinfo.h"
#include "keystrs.h"
#include "posvecdataset.h"

#include "uiattrsel.h"
#include "uiattrvolout.h"
#include "uiattrdescseted.h"
#include "uiioobjsel.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uisetpickdirs.h"
#include "attribsetcreator.h"
#include "uiseisioobjinfo.h"

#include "uievaluatedlg.h"
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
	, storedmnuitem("Stored Cubes")
	, calcmnuitem("Attributes")
{
    Attrib::initAttribClasses();
    StorageProvider::initClass();
    attrsetclosetim.tick.notify( 
			mCB(this,uiAttribPartServer,attrsetDlgCloseTimTick) );
}


uiAttribPartServer::~uiAttribPartServer()
{
    delete adsman;
    delete attrsetdlg;
}


bool uiAttribPartServer::replaceSet( const IOPar& iopar )
{
    Attrib::DescSet* ads = new Attrib::DescSet;
    if ( !ads->usePar(iopar) )
    {
	delete ads;
	return false;
    }

    delete adsman;
    adsman = new DescSetMan( ads, true );
    adsman->attrsetid_ = "";
    sendEvent( evNewAttrSet );
    return true;
}


bool uiAttribPartServer::addToDescSet( const char* key )
{
    DescID id = adsman->descSet()->getStoredID( key );
    return id < 0 ? false : true;
}


const DescSet* uiAttribPartServer::curDescSet() const
{
    return adsman->descSet();
}


void uiAttribPartServer::getDirectShowAttrSpec( SelSpec& as ) const
{
   if ( !dirshwattrdesc )
       as.set( 0, SelSpec::cNoAttrib(), false, 0 );
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
	adsman->setDescSet( attrsetdlg->getSet()->clone() );
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
    attrdata.attribid = selspec.isNLA() ? SelSpec::cNoAttrib() : selspec.id();
    attrdata.outputnr = selspec.isNLA() ? selspec.id().asInt() : -1;
    attrdata.nlamodel = getNLAModel();
    uiAttrSelDlg dlg( appserv().parent(), "View Data", attrdata, No2D );
    if ( !dlg.go() )
	return false;

    attrdata.attribid = dlg.attribID();
    attrdata.outputnr = dlg.outputNr();
    const bool isnla = attrdata.attribid < 0 && attrdata.outputnr >= 0;
    IOObj* ioobj = IOM().get( adsman->attrsetid_ );
    BufferString attrsetnm = ioobj ? ioobj->name() : "";
    selspec.set( 0, isnla ? DescID(attrdata.outputnr,true) : attrdata.attribid,
	         isnla, isnla ? (const char*)nlaname : (const char*)attrsetnm );
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
	    ss.set( ss.userRef(), SelSpec::cNoAttrib(), true, 0 );
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

    BufferStringSet storedoutputs;
    for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
    {
	const char* ioobjid = attrinf.ioobjids.get( idx );
	uiSeisIOObjInfo* sii = new uiSeisIOObjInfo( MultiID(ioobjid), false );
	sii->getDefKeys( storedoutputs, true );
	delete sii;
    }

    for ( int idx=0; idx<storedoutputs.size(); idx++ )
	nms.add( storedoutputs.get(idx) );
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


void uiAttribPartServer::setTargetSelSpec( const SelSpec& selspec )
{
    targetspecs.erase();
    targetspecs += selspec;
}


EngineMan* uiAttribPartServer::createEngMan( const CubeSampling* cs, 
					     const char* linekey,
					     const DescSet* ads )
{
    if ( !adsman->descSet() )
	{ pErrMsg("No attr set"); return false; }
    else if ( !targetspecs.size() || targetspecs[0].id()==SelSpec::cNoAttrib() )
	{ pErrMsg("Nothing to do"); return false; }

    EngineMan* aem = new EngineMan;
    aem->setAttribSet( ads ? ads : adsman->descSet() );
    aem->setNLAModel( getNLAModel() );
    aem->setAttribSpecs( targetspecs );
    if ( cs )
	aem->setCubeSampling( *cs );
    if ( linekey )
	aem->setLineKey( linekey );

    return aem;
}


const Attrib::DataCubes* uiAttribPartServer::createOutput(
	const CubeSampling& cs, const DataCubes* cache, const DescSet* ads )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, 0, ads );
    if ( !aem ) return 0;

    BufferString defstr;
    const Attrib::DescSet* attrds = ads ? ads : adsman->descSet();
    if ( attrds && attrds->nrDescs() && attrds->getDesc(targetspecs[0].id()) )
    {
	attrds->getDesc(targetspecs[0].id())->getDefStr(defstr);
	if ( strcmp (defstr, targetspecs[0].defString()) )
	    cache = 0;
    }

    BufferString errmsg;
    Processor* process = aem->createDataCubesOutput( errmsg, cache );
    if ( !process )
	{ uiMSG().error(errmsg); return 0; }

    if ( aem->getNrOutputsToBeProcessed(*process) != 0 )
    {
	uiExecutor dlg( appserv().parent(), *process );
	if ( !dlg.go() ) { delete process; return 0; }
    }

    const Attrib::DataCubes* output = aem->getDataCubesOutput( *process );
    if ( !output )
    {
	delete process;
	return 0;
    }
    output->ref();
    delete process;
    output->unRefNoDelete();

    return output;
}


bool uiAttribPartServer::createOutput( ObjectSet<BinIDValueSet>& values )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return false;

    BufferString errmsg;
    aem->computeIntersect2D(values);
    PtrMan<Processor> process = aem->createLocationOutput( errmsg, values );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( appserv().parent(), *process );
    if ( !dlg.go() ) return false;

    return true;
}


bool uiAttribPartServer::createOutput( const BinIDValueSet& bidvalset,
				       SeisTrcBuf& output )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return 0;

    BufferString errmsg;
    PtrMan<Processor> process = aem->createTrcSelOutput( errmsg, bidvalset, 
	    						 output );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( appserv().parent(), *process );
    if ( !dlg.go() ) return false;

    return true;
}


bool uiAttribPartServer::extractData( const NLACreationDesc& desc,
				      const ObjectSet<BinIDValueSet>& bivsets,
				      ObjectSet<PosVecDataSet>& outvds )
{
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return 0; }

    if ( desc.doextraction )
    {
	PosVecOutputGen pvog( *adsman->descSet(), desc.design.inputs,
				     bivsets, outvds );
	uiExecutor dlg( appserv().parent(), pvog );
	if ( !dlg.go() )
	    return false;
	else if ( outvds.size() != bivsets.size() )
	{
	    // Earlier error
	    ErrMsg( "Error during data extraction (not all data extracted)." );
	    return false;
	}
    }
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( desc.vdsid );
	PosVecDataSet* vds = new PosVecDataSet;
	BufferString errmsg;
	if ( !vds->getFrom(ioobj->fullUserExpr(true),errmsg) )
	    { uiMSG().error( errmsg ); delete vds; return false; }
	if ( !vds->pars().size() || vds->data().isEmpty() )
	    { uiMSG().error( "Invalid Training set specified" );
		delete vds; return false; }
	outvds += vds;
    }

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
	    uiMSG().error( errmsg );

	sendEvent( evNewAttrSet );
    }
}


bool uiAttribPartServer::create2DOutput( const CubeSampling& cs,
					 const char* linekey,
					 ObjectSet<DataHolder>& dataset,
					 ObjectSet<SeisTrcInfo>& trcinfoset )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, linekey );
    if ( !aem ) return false;

    BufferString errmsg;
    PtrMan<Processor> process = aem->createScreenOutput2D( errmsg, dataset,
							   trcinfoset );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( appserv().parent(), *process );
    return dlg.go();
}


bool uiAttribPartServer::createAttributeSet( const BufferStringSet& inps,
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


#define mInsertItems(list,mnu,correcttype) \
(mnu)->enabled = attrinf.list.size(); \
for ( int idx=start; idx<stop; idx++ ) \
{ \
    const BufferString& nm = attrinf.list.get(idx); \
    MenuItem* itm = new MenuItem( nm ); \
    const bool docheck = correcttype && nm == as.userRef(); \
    mAddManagedMenuItem( mnu, itm, true, docheck );\
    if ( docheck ) (mnu)->checked = true; \
}


static int cMaxMenuSize = 150;

MenuItem* uiAttribPartServer::storedAttribMenuItem( const SelSpec& as )
{
    storedmnuitem.removeItems();
    storedmnuitem.checked = false;
    SelInfo attrinf( adsman->descSet(), 0, No2D, DescID::undef() );
    const bool isnla = as.isNLA();
    const bool hasid = as.id() >= 0;
    const int nritems = attrinf.ioobjnms.size();
    if ( nritems <= cMaxMenuSize )
    {
	const int start = 0; const int stop = attrinf.ioobjnms.size();
	mInsertItems(ioobjnms,&storedmnuitem,!isnla&&hasid);
    }
    else
    {
	const int nrsubmnus = (nritems-1)/cMaxMenuSize + 1;
	for ( int mnuidx=0; mnuidx<nrsubmnus; mnuidx++ )
	{
	    const int start = mnuidx * cMaxMenuSize;
	    int stop = (mnuidx+1) * cMaxMenuSize;
	    if ( stop > nritems ) stop = nritems;
	    const char* startnm = attrinf.ioobjnms.get(start);
	    const char* stopnm = attrinf.ioobjnms.get(stop-1);
	    BufferString str; strncat(str.buf(),startnm,3);
	    str += " - "; strncat(str.buf(),stopnm,3);
	    MenuItem* submnu = new MenuItem( str );
	    mInsertItems(ioobjnms,submnu,!isnla&&hasid);
	    mAddManagedMenuItem( &storedmnuitem, submnu, true,submnu->checked);
	}
    }

    storedmnuitem.enabled = storedmnuitem.nrItems();
    return &storedmnuitem;
}
	

MenuItem* uiAttribPartServer::calcAttribMenuItem( const SelSpec& as )
{
    calcmnuitem.removeItems();
    calcmnuitem.checked = false;
    SelInfo attrinf( adsman->descSet() );
    const bool isattrib = attrinf.attrids.indexOf( as.id() ) >= 0;
    const int start = 0; const int stop = attrinf.attrnms.size();
    mInsertItems(attrnms,&calcmnuitem,isattrib);

    calcmnuitem.enabled = calcmnuitem.nrItems();
    return &calcmnuitem;
}


MenuItem* uiAttribPartServer::nlaAttribMenuItem( const SelSpec& as )
{
    nlamnuitem.removeItems();
    nlamnuitem.checked = false;
    const NLAModel* nlamodel = getNLAModel();
    if ( nlamodel )
    {
	nlamnuitem.text = nlamodel->nlaType(false);
	SelInfo attrinf( adsman->descSet(), nlamodel );
	const bool isnla = as.isNLA();
	const bool hasid = as.id() >= 0;
	const int start = 0; const int stop = attrinf.nlaoutnms.size();
	mInsertItems(nlaoutnms,&nlamnuitem,isnla);
    }

    nlamnuitem.enabled = nlamnuitem.nrItems();
    return &nlamnuitem;
}


// TODO: create more general function, for now it does what we need
MenuItem* uiAttribPartServer::depthdomainAttribMenuItem( const SelSpec& as,
							 const char* key )
{
    BufferString itmtxt = key; itmtxt += " Cubes";
    depthdomainmnuitem.text = itmtxt;
    depthdomainmnuitem.removeItems();
    depthdomainmnuitem.checked = false;

    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();

    BufferStringSet ioobjnms;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	const char* res = ioobj.pars().find( sKey::DepthDomain );
	if ( res && !strcmp(res,key) )
	    ioobjnms.add( ioobj.name() );
    }

    ioobjnms.sort();
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const BufferString& nm = ioobjnms.get( idx );
	MenuItem* itm = new MenuItem( nm );
	const bool docheck = nm == as.userRef();
	mAddManagedMenuItem( &depthdomainmnuitem, itm, true, docheck );
	if ( docheck ) depthdomainmnuitem.checked = true;
    }

    depthdomainmnuitem.enabled = depthdomainmnuitem.nrItems();
    return &depthdomainmnuitem;
}


bool uiAttribPartServer::handleAttribSubMenu( int mnuid, SelSpec& as ) const
{
    uiAttrSelData attrdata( adsman->descSet() );
    attrdata.nlamodel = getNLAModel();
    SelInfo attrinf( attrdata.attrset, attrdata.nlamodel, No2D );

    DescID attribid = SelSpec::cAttribNotSel();
    int outputnr = -1;
    bool isnla = false;

    if ( storedmnuitem.findItem(mnuid) )
    {
	const MenuItem* item = storedmnuitem.findItem(mnuid);
	int idx = attrinf.ioobjnms.indexOf(item->text);
	attribid = adsman->descSet()->getStoredID( attrinf.ioobjids.get(idx) );
    }
    else if ( calcmnuitem.findItem(mnuid) )
    {
	const MenuItem* item = calcmnuitem.findItem(mnuid);
	int idx = attrinf.attrnms.indexOf(item->text);
	attribid = attrinf.attrids[idx];
    }
    else if ( nlamnuitem.findItem(mnuid) )
    {
	outputnr = nlamnuitem.itemIndex(nlamnuitem.findItem(mnuid));
	isnla = true;
    }
    else if ( depthdomainmnuitem.findItem(mnuid) )
    {
	const MenuItem* item = depthdomainmnuitem.findItem( mnuid );
	IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id));
	PtrMan<IOObj> ioobj = IOM().getLocal( item->text );
	if ( ioobj )
	    attribid = adsman->descSet()->getStoredID( ioobj->key() );
    }
    else
	return false;

    IOObj* ioobj = IOM().get( adsman->attrsetid_ );
    BufferString attrsetnm = ioobj ? ioobj->name() : "";
    as.set( 0, isnla ? DescID(outputnr,true) : attribid, isnla,
	    isnla ? (const char*)nlaname : (const char*)attrsetnm );
    BufferString bfs;
    if ( attribid != SelSpec::cAttribNotSel() )
    {
	adsman->descSet()->getDesc(attribid)->getDefStr(bfs);
	as.setDefString(bfs.buf());
    }

    if ( isnla )
	as.setRefFromID( *attrdata.nlamodel );
    else
	as.setRefFromID( *adsman->descSet() );

    return true;
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiAttribPartServer::showEvalDlg( CallBacker* )
{
    if ( !attrsetdlg ) return;
    const Desc* curdesc = attrsetdlg->curDesc();
    if ( !curdesc )
	mErrRet( "Please add this attribute first" )

    uiAttrDescEd* ade = attrsetdlg->curDescEd();
    if ( !ade ) return;

    sendEvent( evEvalAttrInit );
    if ( !alloweval ) mErrRet( "Evaluation of attributes only possible on\n"
			       "Inlines, Crosslines, Timeslices and Surfaces.");

    uiEvaluateDlg* evaldlg = new uiEvaluateDlg( appserv().parent(), *ade, 
						allowevalstor );
    if ( !evaldlg->evaluationPossible() )
	mErrRet( "This attribute has no parameters to evaluate" )

    evaldlg->calccb.notify( mCB(this,uiAttribPartServer,calcEvalAttrs) );
    evaldlg->showslicecb.notify( mCB(this,uiAttribPartServer,showSliceCB) );
    evaldlg->windowClosed.notify( mCB(this,uiAttribPartServer,evalDlgClosed) );
    evaldlg->go();
    attrsetdlg->setSensitive( false );
}


void uiAttribPartServer::evalDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    if ( !evaldlg ) { pErrMsg("cb is not uiEvaluateDlg*"); return; }

    if ( evaldlg->storeSlices() )
	sendEvent( evEvalStoreSlices );
    
    Desc* curdesc = attrsetdlg->curDesc();
    BufferString curusrref = curdesc->userRef();
    uiAttrDescEd* ade = attrsetdlg->curDescEd();

    DescSet* curattrset = attrsetdlg->getSet();
    const Desc* evad = evaldlg->getAttribDesc();
    if ( evad )
    {
	BufferString defstr;
	evad->getDefStr( defstr );
	curdesc->parseDefStr( defstr );
	attrsetdlg->updateCurDescEd();
    }

    attrsetdlg->setSensitive( true );
}


void uiAttribPartServer::calcEvalAttrs( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    if ( !evaldlg ) { pErrMsg("cb is not uiEvaluateDlg*"); return; }

    DescSetMan* kpman = adsman;
    DescSet* ads = evaldlg->getEvalSet();
    evaldlg->getEvalSpecs( targetspecs );
    DescSetMan tmpadsman( ads, false );
    adsman = &tmpadsman;
    sendEvent( evEvalCalcAttr );
    adsman = kpman;
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
