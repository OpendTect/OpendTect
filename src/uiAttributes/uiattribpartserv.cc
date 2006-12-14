/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiattribpartserv.cc,v 1.43 2006-12-14 14:30:52 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattribpartserv.h"

#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribinit.h"
#include "attribposvecoutput.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "uiattrsetman.h"
#include "attribsetcreator.h"
#include "attribstorprovider.h"

#include "arraynd.h"
#include "binidselimpl.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "executor.h"
#include "iodir.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "nlacrdesc.h"
#include "nlamodel.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "seisinfo.h"
#include "survinfo.h"

#include "uiattrdesced.h"
#include "uiattrdescseted.h"
#include "uiattrsel.h"
#include "uiattrvolout.h"
#include "uievaluatedlg.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uisetpickdirs.h"

#include <math.h>

using namespace Attrib;

const int uiAttribPartServer::evDirectShowAttr	 = 0;
const int uiAttribPartServer::evNewAttrSet	 = 1;
const int uiAttribPartServer::evAttrSetDlgClosed = 2;
const int uiAttribPartServer::evEvalAttrInit 	 = 3;
const int uiAttribPartServer::evEvalCalcAttr	 = 4;
const int uiAttribPartServer::evEvalShowSlice	 = 5;
const int uiAttribPartServer::evEvalStoreSlices	 = 6;
const int uiAttribPartServer::objNLAModel2D	 = 100;
const int uiAttribPartServer::objNLAModel3D	 = 101;

const char* uiAttribPartServer::attridstr_ = "Attrib ID";


uiAttribPartServer::uiAttribPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, adsman2d_(new DescSetMan(true))
    	, adsman3d_(new DescSetMan(false))
	, dirshwattrdesc_(0)
        , attrsetdlg_(0)
    	, attrsetclosetim_("Attrset dialog close")
	, stored2dmnuitem_("Stored 2D Data")
	, stored3dmnuitem_("Stored Cubes")
	, calc2dmnuitem_("Attributes 2D")
	, calc3dmnuitem_("Attributes 3D")
{
    initAttribClasses();
    StorageProvider::initClass();
    attrsetclosetim_.tick.notify( 
			mCB(this,uiAttribPartServer,attrsetDlgCloseTimTick) );
}


uiAttribPartServer::~uiAttribPartServer()
{
    delete adsman2d_;
    delete adsman3d_;
    delete attrsetdlg_;
}


bool uiAttribPartServer::replaceSet( const IOPar& iopar, bool is2d )
{
    DescSet* ads = new DescSet;
    if ( !ads->usePar(iopar) )
    {
	delete ads;
	return false;
    }

    DescSetMan* adsman = getAdsMan( is2d );
    delete adsman;
    adsman = new DescSetMan( is2d, ads, true );
    adsman->attrsetid_ = "";
    sendEvent( evNewAttrSet );
    return true;
}


bool uiAttribPartServer::addToDescSet( const char* key, bool is2d )
{
    DescID id = getAdsMan( is2d )->descSet()->getStoredID( key );
    return id < 0 ? false : true;
}


const DescSet* uiAttribPartServer::curDescSet( bool is2d ) const
{
    return getAdsMan( is2d )->descSet();
}


void uiAttribPartServer::getDirectShowAttrSpec( SelSpec& as ) const
{
   if ( !dirshwattrdesc_ )
       as.set( 0, SelSpec::cNoAttrib(), false, 0 );
   else
       as.set( *dirshwattrdesc_ );
}


void uiAttribPartServer::manageAttribSets()
{
    uiAttrSetMan dlg( appserv().parent() );
    dlg.go();
}


bool uiAttribPartServer::editSet( bool is2d )
{
    DescSetMan* adsman = getAdsMan( is2d );
    IOPar iop;
    if ( adsman->descSet() ) adsman->descSet()->fillPar( iop );

    DescSet* oldset = adsman->descSet();
    delete attrsetdlg_;
    attrsetdlg_ = new uiAttribDescSetEd( appserv().parent(), adsman );
    attrsetdlg_->dirshowcb.notify( mCB(this,uiAttribPartServer,directShowAttr));
    attrsetdlg_->evalattrcb.notify( mCB(this,uiAttribPartServer,showEvalDlg) );
    attrsetdlg_->windowClosed.notify( 
	    			mCB(this,uiAttribPartServer,attrsetDlgClosed) );
    return attrsetdlg_->go();
}


void uiAttribPartServer::attrsetDlgClosed( CallBacker* )
{
    attrsetclosetim_.start( 10, true );
}

void uiAttribPartServer::attrsetDlgCloseTimTick( CallBacker* )
{
    if ( attrsetdlg_->uiResult() )
    {
	DescSetMan* adsman = getAdsMan( attrsetdlg_->getSet()->is2D() );
	adsman->setDescSet( attrsetdlg_->getSet()->clone() );
	adsman->attrsetid_ = attrsetdlg_->curSetID();
	sendEvent( evNewAttrSet );
    }

    delete attrsetdlg_;
    attrsetdlg_ = 0;
    sendEvent( evAttrSetDlgClosed );
}


const NLAModel* uiAttribPartServer::getNLAModel( bool is2d ) const
{
    return (NLAModel*)getObject( is2d ? objNLAModel2D : objNLAModel3D );
}


bool uiAttribPartServer::selectAttrib( SelSpec& selspec, const char* depthkey,
       				       bool is2d )
{
    DescSetMan* adsman = getAdsMan( is2d );
    uiAttrSelData attrdata( adsman->descSet() );
    attrdata.attribid = selspec.isNLA() ? SelSpec::cNoAttrib() : selspec.id();
    attrdata.outputnr = selspec.isNLA() ? selspec.id().asInt() : -1;
    attrdata.nlamodel = getNLAModel(is2d);
    attrdata.depthdomainkey = depthkey;
    uiAttrSelDlg dlg( appserv().parent(), "View Data", attrdata, No2D );
    if ( !dlg.go() )
	return false;

    attrdata.attribid = dlg.attribID();
    attrdata.outputnr = dlg.outputNr();
    const bool isnla = attrdata.attribid < 0 && attrdata.outputnr >= 0;
    IOObj* ioobj = IOM().get( adsman->attrsetid_ );
    BufferString attrsetnm = ioobj ? ioobj->name() : "";
    selspec.set( 0, isnla ? DescID(attrdata.outputnr,true) : attrdata.attribid,
	         isnla, isnla ? (const char*)nlaname_ : (const char*)attrsetnm);
    if ( isnla && attrdata.nlamodel )
	selspec.setRefFromID( *attrdata.nlamodel );
    else if ( !isnla )
	selspec.setRefFromID( *adsman->descSet() );
    selspec.setDepthDomainKey( dlg.depthDomainKey() );

    return true;
}


void uiAttribPartServer::directShowAttr( CallBacker* cb )
{
    mDynamicCastGet(uiAttribDescSetEd*,ed,cb);
    if ( !ed ) { pErrMsg("cb is not uiAttribDescSetEd*"); return; }
    DescSetMan* adsman = getAdsMan( ed->is2D() );
    dirshwattrdesc_ = ed->curDesc();
    DescSetMan* kpman = adsman;
    DescSet* edads = const_cast<DescSet*>(dirshwattrdesc_->descSet());
    DescSetMan tmpadsman( edads, false );
    adsman = &tmpadsman;
    sendEvent( evDirectShowAttr );
    adsman = kpman;
}


void uiAttribPartServer::updateSelSpec( SelSpec& ss ) const
{
    //TODO how to get real is2d?
    bool is2d = false;//TODO remove!!!!!
    if ( ss.isNLA() )
    {
	const NLAModel* nlamod = getNLAModel(is2d);
	if ( nlamod )
	{
	    ss.setIDFromRef( *nlamod );
	    ss.setObjectRef( nlaname_ );
	}
	else
	    ss.set( ss.userRef(), SelSpec::cNoAttrib(), true, 0 );
    }
    else
    {
	if ( ss.is2D() ) return;
	const DescSet& ads = *adsman3d_->descSet();
	ss.setIDFromRef( ads );
	IOObj* ioobj = IOM().get( adsman3d_->attrsetid_ );
	if ( ioobj ) ss.setObjectRef( ioobj->name() );
    }
}


void uiAttribPartServer::getPossibleOutputs( bool is2d, 
					     BufferStringSet& nms ) const
{
    nms.erase();
    SelInfo attrinf( curDescSet( is2d ) );
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


bool uiAttribPartServer::setSaved( bool is2d ) const
{
    return getAdsMan( is2d )->isSaved();
}


void uiAttribPartServer::saveSet( bool is2d )
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
	else if ( !AttribDescSetTranslator::store(*getAdsMan(is2d)->descSet(),
						  ctio->ioobj,bs) )
	    uiMSG().error(bs);
    }
    ctio->setObj( 0 );
}


void uiAttribPartServer::outputVol( MultiID& nlaid, bool is2d )
{
    DescSet* dset = getAdsMan( is2d )->descSet();
    if ( !dset ) { pErrMsg("No attr set"); return; }

    uiAttrVolOut dlg( appserv().parent(), *dset, getNLAModel(is2d), nlaid );
    dlg.go();
}


void uiAttribPartServer::setTargetSelSpec( const SelSpec& selspec )
{
    targetspecs_.erase();
    targetspecs_ += selspec;
}


EngineMan* uiAttribPartServer::createEngMan( const CubeSampling* cs, 
					     const char* linekey )
{
    if ( targetspecs_.isEmpty() || targetspecs_[0].id() == SelSpec::cNoAttrib())
	{ pErrMsg("Nothing to do"); return false; }
    
    const bool is2d = targetspecs_[0].is2D();
    DescSetMan* adsman = getAdsMan( is2d );
    if ( !adsman->descSet() )
	{ pErrMsg("No attr set"); return false; }

    EngineMan* aem = new EngineMan;
    aem->setAttribSet( adsman->descSet() );
    aem->setNLAModel( getNLAModel(is2d) );
    aem->setAttribSpecs( targetspecs_ );
    if ( cs )
	aem->setCubeSampling( *cs );
    if ( linekey )
	aem->setLineKey( linekey );

    return aem;
}


const Attrib::DataCubes* uiAttribPartServer::createOutput(
				const CubeSampling& cs, const DataCubes* cache )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, 0 );
    if ( !aem ) return 0;

    BufferString defstr;
    const DescSet* attrds = adsman3d_->descSet();
    if ( attrds && attrds->nrDescs() && attrds->getDesc(targetspecs_[0].id()) )
    {
	attrds->getDesc(targetspecs_[0].id())->getDefStr(defstr);
	if ( strcmp (defstr, targetspecs_[0].defString()) )
	    cache = 0;
    }

    BufferString errmsg;
    Processor* process = aem->createDataCubesOutput( errmsg, cache );
    if ( !process )
	{ uiMSG().error(errmsg); return 0; }

    bool success = true;
    if ( aem->getNrOutputsToBeProcessed(*process) != 0 )
    {
	uiExecutor dlg( appserv().parent(), *process );
	success = dlg.go();
    }

    const Attrib::DataCubes* output = aem->getDataCubesOutput( *process );
    if ( !output )
    {
	delete process;
	return 0;
    }
    output->ref();
    delete process;

    if ( !success )
    {
	if ( !uiMSG().askGoOn("Attribute loading/calculation aborted.\n"
	    "Do you want to use the partially loaded/computed data?", true ) )
	{
	    output->unRef();
	    output = 0;
	}
    }

    if ( output )
	output->unRefNoDelete();

    return output;
}


bool uiAttribPartServer::createOutput( ObjectSet<BinIDValueSet>& values )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return false;

    BufferString errmsg;
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


bool uiAttribPartServer::isDataAngles( bool is2ddesc ) const
{
    DescSetMan* adsman = getAdsMan( is2ddesc );
    if ( !adsman->descSet() || targetspecs_.isEmpty() )
	return false;
	    
    const Desc* desc = adsman->descSet()->getDesc(targetspecs_[0].id());
    if ( !desc )
	return false;

    return Seis::isAngle( desc->dataType() );
}


static const int sMaxNrClasses = 100;
//static const int sMaxNrVals = 100;

bool uiAttribPartServer::isDataClassified( const Array3D<float>& array ) const
{
    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    const int sz2 = array.info().getSize( 2 );
//    int nrint = 0;
    for ( int x0=0; x0<sz0; x0++ )
	for ( int x1=0; x1<sz1; x1++ )
	    for ( int x2=0; x2<sz2; x2++ )
	    {
		const float val = array.get( x0, x1, x2 );
		if ( mIsUdf(val) ) continue;
		const int ival = mNINT(val);
		if ( !mIsEqual(val,ival,mDefEps) || abs(ival)>sMaxNrClasses )
		    return false;
//		nrint++;
//		if ( nrint > sMaxNrVals )
//		    break;
	    }

    return true;
}


//TODO: would NLACreationDesc need a is2d field?
bool uiAttribPartServer::extractData( const NLACreationDesc& desc,
				      const ObjectSet<BinIDValueSet>& bivsets,
				      ObjectSet<PosVecDataSet>& outvds )
{
    /*
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
	if ( vds->pars().isEmpty() || vds->data().isEmpty() )
	    { uiMSG().error( "Invalid Training set specified" );
		delete vds; return false; }
	outvds += vds;
    }
*/
    return true;
}


void uiAttribPartServer::fillPar( IOPar& iopar, bool is2d ) const
{
    DescSetMan* adsman = getAdsMan( is2d );
    if ( adsman->descSet() && adsman->descSet()->nrDescs() )
	adsman->descSet()->fillPar( iopar );
}


void uiAttribPartServer::usePar( const IOPar& iopar, bool is2d )
{
    DescSetMan* adsman = getAdsMan( is2d );
    if ( adsman->descSet() )
    {
	BufferStringSet errmsgs;
	adsman->descSet()->usePar( iopar, &errmsgs );
	BufferString errmsg;
	for ( int idx=0; idx<errmsgs.size(); idx++ )
	{
	    if ( !idx )
	    {
		errmsg = "Error during restore of ";
		errmsg += is2d ? "2D " : "3D "; errmsg += "Attribute Set:";
	    }
	    errmsg += "\n";
	    errmsg += errmsgs.get( idx );
	}
	if ( !errmsg.isEmpty() )
	    uiMSG().error( errmsg );

	sendEvent( evNewAttrSet );
    }
}


Attrib::DescID uiAttribPartServer::createStored2DAttrib(const MultiID& lineset,
							const char* attribname)
{
    return adsman2d_->descSet()->getStoredID( LineKey(lineset,attribname) );
}


bool uiAttribPartServer::create2DOutput( const CubeSampling& cs,
					 const char* linekey,
					 Attrib::Data2DHolder& dataset )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, linekey );
    if ( !aem ) return false;

    BufferString errmsg;
    PtrMan<Processor> process = aem->createScreenOutput2D( errmsg, dataset);
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


//TODO check what is the exact role of the descset
bool uiAttribPartServer::setPickSetDirs( Pick::Set& ps, const NLAModel* nlamod )
{
//    uiSetPickDirs dlg( appserv().parent(), ps, curDescSet(), nlamod );
//	return dlg.go();
    return true;// extra, to make it compile
}


//TODO may require a linekey in the as ( for docheck )
void uiAttribPartServer::insert2DStoredItems( const BufferStringSet& bfset, 
					      int start, int stop, 
					      bool correcttype, MenuItem* mnu,
       					      const SelSpec& as	) 
{
    mnu->enabled = bfset.size();
    for ( int idx=start; idx<stop; idx++ )
    {
	BufferString lkey = bfset.get(idx);
	MenuItem* itm = new MenuItem( lkey );
	const bool docheck =  correcttype && lkey == as.userRef();
	mAddManagedMenuItem( mnu, itm, true, docheck );
	if ( docheck ) mnu->checked = true;
    }
}


BufferStringSet uiAttribPartServer::get2DStoredItems( const SelInfo& sinf) const
{
    BufferStringSet lkeyset;
    for ( int idlset=0; idlset<sinf.ioobjids.size(); idlset++ )
    {
	const char* lsetid = sinf.ioobjids.get(idlset);
	const MultiID mid( lsetid );
	const BufferString& lsetnm = IOM().get(mid)->name();
	BufferStringSet nms;
	SelInfo::getAttrNames( lsetid, nms );
	for ( int idx=0; idx<nms.size(); idx++ )
	{
	    const LineKey lkey( lsetnm.buf(), nms.get(idx), true ); 
	    lkeyset.add( lkey );
	}
    }

    return lkeyset;
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


#define mCleanMenuItems(startstr,mnuitem_)\
{\
    startstr##mnuitem_.removeItems();\
    startstr##mnuitem_.checked = false;\
}

    
static int cMaxMenuSize = 150;

MenuItem* uiAttribPartServer::storedAttribMenuItem( const SelSpec& as, 
						    bool is2d )
{
    SelInfo attrinf( adsman3d_->descSet(), 0, No2D, DescID::undef() );
    if ( is2d ) 
    {
	mCleanMenuItems(stored2d,mnuitem_)
	attrinf = SelInfo( adsman2d_->descSet(), 0, Only2D, DescID::undef() );
    }
    else
	mCleanMenuItems(stored3d,mnuitem_);
    
    const bool isnla = as.isNLA();
    const bool hasid = as.id() >= 0;
    const BufferStringSet bfset = is2d ? get2DStoredItems( attrinf )
				       : attrinf.ioobjnms;
    int nritems = bfset.size();
    if ( nritems <= cMaxMenuSize )
    {
	if ( is2d )
	    insert2DStoredItems( bfset, 0, nritems, !isnla&&hasid,
		    		 &stored2dmnuitem_, as );
	else
	{
	    const int start = 0; const int stop = nritems;
	    mInsertItems(ioobjnms,&stored3dmnuitem_,!isnla&&hasid);
	}
    }
    else
	insertNumerousItems( bfset, as, !isnla&&hasid, is2d );

    MenuItem* storedmnuitem = is2d ? &stored2dmnuitem_ : &stored3dmnuitem_;
    storedmnuitem->enabled = storedmnuitem->nrItems();
    return storedmnuitem;
}



void uiAttribPartServer::insertNumerousItems( const BufferStringSet& bfset,
					      const SelSpec& as,
       					      bool correcttype, bool is2d )
{
    int nritems = bfset.size();
    const int nrsubmnus = (nritems-1)/cMaxMenuSize + 1;
    for ( int mnuidx=0; mnuidx<nrsubmnus; mnuidx++ )
    {
	const int start = mnuidx * cMaxMenuSize;
	int stop = (mnuidx+1) * cMaxMenuSize;
	if ( stop > nritems ) stop = nritems;
	const char* startnm = bfset.get(start);
	const char* stopnm = bfset.get(stop-1);
	BufferString str; strncat(str.buf(),startnm,3);
	str += " - "; strncat(str.buf(),stopnm,3);
	MenuItem* submnu = new MenuItem( str );
	if ( is2d )
	    insert2DStoredItems( bfset, start, stop, correcttype, submnu, as );
	else
	{
	    SelInfo attrinf( adsman3d_->descSet(), 0, No2D, DescID::undef() );
	    mInsertItems(ioobjnms,submnu,correcttype);
	}
	
	MenuItem* storedmnuitem = is2d ? &stored2dmnuitem_ : &stored3dmnuitem_;
	mAddManagedMenuItem( storedmnuitem, submnu, true,submnu->checked);
    }
}


MenuItem* uiAttribPartServer::calcAttribMenuItem( const SelSpec& as, bool is2d )
{
    if ( is2d ) 
	mCleanMenuItems(calc2d,mnuitem_)
    else
	mCleanMenuItems(calc3d,mnuitem_);
    SelInfo attrinf( is2d ? adsman2d_->descSet() : adsman3d_->descSet() );
    const bool isattrib = attrinf.attrids.indexOf( as.id() ) >= 0; 

    const int start = 0; const int stop = attrinf.attrnms.size();
    MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    mInsertItems(attrnms,calcmnuitem,isattrib);

    calcmnuitem->enabled = calcmnuitem->nrItems();
    return calcmnuitem;
}


//TODO 2/3D list
MenuItem* uiAttribPartServer::nlaAttribMenuItem( const SelSpec& as, bool is2d )
{
    if ( is2d )
	mCleanMenuItems(nla2d,mnuitem_)
    else
	mCleanMenuItems(nla3d,mnuitem_);
    const NLAModel* nlamodel = getNLAModel(is2d);
    MenuItem* nlamnuitem = is2d ? &nla2dmnuitem_ : &nla3dmnuitem_;
    if ( nlamodel )
    {
	nlamnuitem->text = nlamodel->nlaType(false);
	DescSet* dset = is2d ? adsman2d_->descSet() : adsman3d_->descSet();
	SelInfo attrinf( dset, nlamodel );
	const bool isnla = as.isNLA();
	const bool hasid = as.id() >= 0;
	const int start = 0; const int stop = attrinf.nlaoutnms.size();
	mInsertItems(nlaoutnms,nlamnuitem,isnla);
    }

    nlamnuitem->enabled = nlamnuitem->nrItems();
    return nlamnuitem;
}


// TODO: create more general function, for now it does what we need
MenuItem* uiAttribPartServer::depthdomainAttribMenuItem( const SelSpec& as,
							 const char* key,
							 bool is2d )
{
    MenuItem* depthdomainmnuitem = is2d ? &depthdomain2dmnuitem_ 
					: &depthdomain3dmnuitem_;
    BufferString itmtxt = key; itmtxt += " Cubes";
    depthdomainmnuitem->text = itmtxt;
    depthdomainmnuitem->removeItems();
    depthdomainmnuitem->checked = false;

    BufferStringSet ioobjnms;
    SelInfo::getSpecialItems( key, ioobjnms );
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const BufferString& nm = ioobjnms.get( idx );
	MenuItem* itm = new MenuItem( nm );
	const bool docheck = nm == as.userRef();
	mAddManagedMenuItem( depthdomainmnuitem, itm, true, docheck );
	if ( docheck ) depthdomainmnuitem->checked = true;
    }

    depthdomainmnuitem->enabled = depthdomainmnuitem->nrItems();
    return depthdomainmnuitem;
}


bool uiAttribPartServer::handleAttribSubMenu( int mnuid, SelSpec& as ) const
{
    bool is2d = stored2dmnuitem_.findItem(mnuid) ||
		calc2dmnuitem_.findItem(mnuid) ||
		nla2dmnuitem_.findItem(mnuid) ||
		depthdomain2dmnuitem_.findItem(mnuid);

    DescSetMan* adsman = getAdsMan( is2d );
    uiAttrSelData attrdata( adsman->descSet() );
    attrdata.nlamodel = getNLAModel(is2d);
    SelInfo attrinf( attrdata.attrset, attrdata.nlamodel, is2d ? Only2D: No2D );
    const MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    const MenuItem* nlamnuitem = is2d ? &nla2dmnuitem_ : &nla3dmnuitem_;
    const MenuItem* depthdomainmnuitem = is2d ? &depthdomain2dmnuitem_ 
					      : &depthdomain3dmnuitem_;

    DescID attribid = SelSpec::cAttribNotSel();
    int outputnr = -1;
    bool isnla = false;

    if ( stored3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = stored3dmnuitem_.findItem(mnuid);
	int idx = attrinf.ioobjnms.indexOf(item->text);
	attribid = adsman->descSet()->getStoredID( attrinf.ioobjids.get(idx) );
    }
    else if ( stored2dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = stored2dmnuitem_.findItem(mnuid);
	const BufferStringSet stored2d = get2DStoredItems( attrinf );
	int idx = stored2d.indexOf(item->text);
	LineKey nmlkey(stored2d.get(idx));
	MultiID mid = IOM().getByName(nmlkey.lineName(), "Seismics" )->key();
	LineKey idlkey(mid, nmlkey.attrName() );
	attribid = adsman->descSet()->getStoredID( idlkey );
    }
    else if ( calcmnuitem->findItem(mnuid) )
    {
	const MenuItem* item = calcmnuitem->findItem(mnuid);
	int idx = attrinf.attrnms.indexOf(item->text);
	attribid = attrinf.attrids[idx];
    }
    else if ( nlamnuitem->findItem(mnuid) )
    {
	outputnr = nlamnuitem->itemIndex(nlamnuitem->findItem(mnuid));
	isnla = true;
    }
    else if ( depthdomainmnuitem->findItem(mnuid) )
    {
	const MenuItem* item = depthdomainmnuitem->findItem( mnuid );
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
	    isnla ? (const char*)nlaname_ : (const char*)attrsetnm );
    
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
    
    if ( is2d )	as.set2DFlag();

    return true;
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiAttribPartServer::showEvalDlg( CallBacker* )
{
    if ( !attrsetdlg_ ) return;
    const Desc* curdesc = attrsetdlg_->curDesc();
    if ( !curdesc )
	mErrRet( "Please add this attribute first" )

    uiAttrDescEd* ade = attrsetdlg_->curDescEd();
    if ( !ade ) return;

    sendEvent( evEvalAttrInit );
    if ( !alloweval_ ) mErrRet( "Evaluation of attributes only possible on\n"
			       "Inlines, Crosslines, Timeslices and Surfaces.");

    uiEvaluateDlg* evaldlg = new uiEvaluateDlg( appserv().parent(), *ade, 
						allowevalstor_ );
    if ( !evaldlg->evaluationPossible() )
	mErrRet( "This attribute has no parameters to evaluate" )

    evaldlg->calccb.notify( mCB(this,uiAttribPartServer,calcEvalAttrs) );
    evaldlg->showslicecb.notify( mCB(this,uiAttribPartServer,showSliceCB) );
    evaldlg->windowClosed.notify( mCB(this,uiAttribPartServer,evalDlgClosed) );
    evaldlg->go();
    attrsetdlg_->setSensitive( false );
}


void uiAttribPartServer::evalDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    if ( !evaldlg ) { pErrMsg("cb is not uiEvaluateDlg*"); return; }

    if ( evaldlg->storeSlices() )
	sendEvent( evEvalStoreSlices );
    
    Desc* curdesc = attrsetdlg_->curDesc();
    BufferString curusrref = curdesc->userRef();
    uiAttrDescEd* ade = attrsetdlg_->curDescEd();

    DescSet* curattrset = attrsetdlg_->getSet();
    const Desc* evad = evaldlg->getAttribDesc();
    if ( evad )
    {
	BufferString defstr;
	evad->getDefStr( defstr );
	curdesc->parseDefStr( defstr );
	attrsetdlg_->updateCurDescEd();
    }

    attrsetdlg_->setSensitive( true );
}

//TODO : how do I know which adsman to use?
void uiAttribPartServer::calcEvalAttrs( CallBacker* cb )
{
    /*
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    if ( !evaldlg ) { pErrMsg("cb is not uiEvaluateDlg*"); return; }

    DescSetMan* kpman = adsman;
    DescSet* ads = evaldlg->getEvalSet();
    evaldlg->getEvalSpecs( targetspecs_ );
    DescSetMan tmpadsman( ads, false );
    adsman = &tmpadsman;
    sendEvent( evEvalCalcAttr );
    adsman = kpman;*/
}


void uiAttribPartServer::showSliceCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( sel < 0 ) return;

    sliceidx_ = sel;
    sendEvent( evEvalShowSlice );
}


DescSetMan* uiAttribPartServer::getAdsMan( bool is2d )
{
    return is2d ? adsman2d_ : adsman3d_;
}


DescSetMan* uiAttribPartServer::getAdsMan( bool is2d ) const 
{
    return is2d ? adsman2d_ : adsman3d_;
}
