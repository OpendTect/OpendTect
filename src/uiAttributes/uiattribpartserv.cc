/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiattribpartserv.cc,v 1.200 2012-08-23 21:47:45 cvsnanne Exp $";

#include "uiattribpartserv.h"

#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "attribdatapack.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsetsholder.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribposvecoutput.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "uiattrsetman.h"
#include "attribsetcreator.h"
#include "attribstorprovider.h"

#include "arraynd.h"
#include "binidvalset.h"
#include "coltabmapper.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datapointset.h"
#include "executor.h"
#include "iodir.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "nlacrdesc.h"
#include "nlamodel.h"
#include "posvecdataset.h"
#include "datapointset.h"
#include "ptrman.h"
#include "randcolor.h"
#include "seisbuf.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "settings.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "zdomain.h"

#include "uiattrdesced.h"
#include "uiattrdescseted.h"
#include "uiattrsel.h"
#include "uiattr2dsel.h"
#include "uiattrvolout.h"
#include "uiattribcrossplot.h"
#include "uievaluatedlg.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uimultcomputils.h"
#include "uiseisioobjinfo.h"
#include "uisetpickdirs.h"
#include "uitaskrunner.h"
#include "uivolprocbatchsetup.h"
#include "uivolprocchain.h"
#include "uicrossattrevaluatedlg.h"

#include <math.h>

using namespace Attrib;

int uiAttribPartServer::evDirectShowAttr()	    { return 0; }
int uiAttribPartServer::evNewAttrSet()	   	    { return 1; }
int uiAttribPartServer::evAttrSetDlgClosed() 	    { return 2; }
int uiAttribPartServer::evEvalAttrInit()	    { return 3; }
int uiAttribPartServer::evEvalCalcAttr()	    { return 4; }
int uiAttribPartServer::evEvalShowSlice()	    { return 5; }
int uiAttribPartServer::evEvalStoreSlices()	    { return 6; }
int uiAttribPartServer::evEvalRestore()      	    { return 7; }
int uiAttribPartServer::objNLAModel2D()	   	    { return 100; }
int uiAttribPartServer::objNLAModel3D()	  	    { return 101; }

const char* uiAttribPartServer::attridstr()	    { return "Attrib ID"; }

static const int cMaxNrClasses = 100;


uiAttribPartServer::uiAttribPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, dirshwattrdesc_(0)
        , attrsetdlg_(0)
        , is2devsent_(false)
    	, attrsetclosetim_("Attrset dialog close")
	, stored2dmnuitem_("&Stored 2D Data")
	, stored3dmnuitem_("Stored &Cubes")
	, steering2dmnuitem_("Stee&ring 2D Data")
	, steering3dmnuitem_("Steer&ing Cubes")
	, multcomp3d_("3D")
	, multcomp2d_("2D")
	, volprocchain_( 0 )
	, dpsdispmgr_( 0 )
        , evalmapperbackup_( 0 )
        , attrsneedupdt_(true)
{
    attrsetclosetim_.tick.notify( 
			mCB(this,uiAttribPartServer,attrsetDlgCloseTimTick) );

    stored2dmnuitem_.checkable = true;
    stored3dmnuitem_.checkable = true;
    calc2dmnuitem_.checkable = true;
    calc3dmnuitem_.checkable = true;
    steering2dmnuitem_.checkable = true;
    steering3dmnuitem_.checkable = true;
    multcomp3d_.checkable = true;
    multcomp2d_.checkable = true;

    handleAutoSet();
}


uiAttribPartServer::~uiAttribPartServer()
{
    delete attrsetdlg_;
    if ( volprocchain_ ) volprocchain_->unRef();
    deepErase( linesets2dstoredmnuitem_ );
    deepErase( linesets2dsteeringmnuitem_ );
    deepErase( attrxplotset_ );
    delete &eDSHolder();
    deepErase( attrxplotset_ );
}


void uiAttribPartServer::doVolProc( const MultiID* mid )
{
    if ( !volprocchain_ )
    {
	volprocchain_ = new VolProc::Chain;
	volprocchain_->ref();
    }

    PtrMan<IOObj> ioobj = mid ? IOM().get( *mid ) : 0;
    if ( ioobj )
    {
	BufferString errmsg;
	if ( !VolProcessingTranslator::retrieve( *volprocchain_, ioobj,
		errmsg ) )
	{
	    FileMultiString fms( "Cannot read volume builder setup" );
	    if ( errmsg.buf() )
		fms += errmsg;

	    uiMSG().error( fms.buf() );
	    volprocchain_->unRef();
	    volprocchain_ = new VolProc::Chain;
	    volprocchain_->ref();
	}
    }

    VolProc::uiChain dlg( parent(), *volprocchain_, true );
    if ( dlg.go() && dlg.saveButtonChecked() )
    {
	ioobj = IOM().get( volprocchain_->storageID() );
	createVolProcOutput( ioobj );
    }
}


void uiAttribPartServer::createVolProcOutput( const IOObj* sel )
{
    VolProc::uiBatchSetup dlg( parent(), sel );
    dlg.go();
}


void uiAttribPartServer::handleAutoSet()
{
    bool douse = false;
    Settings::common().getYN( uiAttribDescSetEd::sKeyUseAutoAttrSet, douse );
    if ( douse )
    {
	if ( SI().has2D() ) useAutoSet( true );
	if ( SI().has3D() ) useAutoSet( false );
    }
}


void uiAttribPartServer::useAutoSet( bool is2d )
{
    MultiID id;
    const char* idkey = is2d ? uiAttribDescSetEd::sKeyAuto2DAttrSetID
			     : uiAttribDescSetEd::sKeyAuto3DAttrSetID;
    Attrib::DescSetMan* setmgr = eDSHolder().getDescSetMan(is2d);
    if ( SI().pars().get(idkey,id) )
    {
	PtrMan<IOObj> ioobj = IOM().get( id );
	BufferString bs;
	DescSet* attrset = new DescSet( is2d );
	if ( !ioobj || !AttribDescSetTranslator::retrieve(*attrset,ioobj,bs)
		|| attrset->is2D()!=is2d )
	    delete attrset;
	else
	{
	    setmgr->setDescSet( attrset );
	    setmgr->attrsetid_ = id;
	}
    }
}


bool uiAttribPartServer::replaceSet( const IOPar& iopar, bool is2d,
				     float versionnr )
{
    DescSet* ads = new DescSet(is2d);
    if ( !ads->usePar(iopar, versionnr) )
    {
	delete ads;
	return false;
    }
    eDSHolder().replaceAttribSet( ads );

    DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
    adsman->attrsetid_ = "";
    set2DEvent( is2d );
    sendEvent( evNewAttrSet() );
    return true;
}


bool uiAttribPartServer::addToDescSet( const char* key, bool is2d )
{
    //TODO: think of it: stored data can  be at 2 places: also in attrib set...
    return eDSHolder().getDescSet(is2d,true)->getStoredID( key ).isValid();
}


const DescSet* uiAttribPartServer::curDescSet( bool is2d ) const
{
    return DSHolder().getDescSetMan( is2d )->descSet();
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
    uiAttrSetMan dlg( parent() );
    dlg.go();
}


bool uiAttribPartServer::editSet( bool is2d )
{
    const DescSetMan* adsman = DSHolder().getDescSetMan( is2d );
    IOPar iop;
    if ( adsman->descSet() ) adsman->descSet()->fillPar( iop );

    delete attrsetdlg_;
    attrsetdlg_ = new uiAttribDescSetEd( parent(),
	    				 const_cast<DescSetMan*>(adsman),0,
	   				 attrsneedupdt_ );
    attrsetdlg_->dirshowcb.notify( mCB(this,uiAttribPartServer,directShowAttr));
    attrsetdlg_->evalattrcb.notify( mCB(this,uiAttribPartServer,showEvalDlg) );
    attrsetdlg_->crossevalattrcb.notify( 
	    mCB(this,uiAttribPartServer,showCrossEvalDlg) );
    attrsetdlg_->xplotcb.notify( mCB(this,uiAttribPartServer,showXPlot) );
    if ( attrsneedupdt_ )
    {
	attrsetdlg_->updtAllEntries();
	attrsneedupdt_ = false;
    }

    attrsetdlg_->windowClosed.notify( 
	    			mCB(this,uiAttribPartServer,attrsetDlgClosed) );
    return attrsetdlg_->go();
}


void uiAttribPartServer::showXPlot( CallBacker* cb )
{
    bool is2d = false;
    if ( !cb )
	is2d = is2DEvent();
    else if ( attrsetdlg_ )
	is2d = attrsetdlg_->getSet()->is2D();
    uiAttribCrossPlot* uiattrxplot = new uiAttribCrossPlot( 0,
	    	 *(attrsetdlg_ ? attrsetdlg_->getSet() : curDescSet(is2d)) );
    uiattrxplot->windowClosed.notify(
	    mCB(this,uiAttribPartServer,xplotClosedCB) );
    uiattrxplot->setDisplayMgr( dpsdispmgr_ );
    uiattrxplot->setDeleteOnClose( false );
    uiattrxplot->show();
    attrxplotset_ += uiattrxplot;
}


void uiAttribPartServer::xplotClosedCB( CallBacker* cb )
{
    mDynamicCastGet(uiAttribCrossPlot*,crossplot,cb);
    if ( !crossplot ) return;
    if ( attrxplotset_.isPresent(crossplot) )
    {
	uiAttribCrossPlot* xplot =
	    attrxplotset_.remove( attrxplotset_.indexOf(crossplot) );
	xplot->windowClosed.remove(
		mCB(this,uiAttribPartServer,xplotClosedCB) );
	xplot->setDeleteOnClose( true );
    }
}


void uiAttribPartServer::attrsetDlgClosed( CallBacker* )
{
    attrsetclosetim_.start( 10, true );
}


void uiAttribPartServer::attrsetDlgCloseTimTick( CallBacker* )
{
    if ( attrsetdlg_->uiResult() )
    {
	bool is2d = attrsetdlg_->getSet()->is2D();
	DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
	adsman->setDescSet( new Attrib::DescSet( *attrsetdlg_->getSet() ) );
	adsman->attrsetid_ = attrsetdlg_->curSetID();
	set2DEvent( is2d );
	sendEvent( evNewAttrSet() );
    }

    delete attrsetdlg_;
    attrsetdlg_ = 0;
    sendEvent( evAttrSetDlgClosed() );
}


const NLAModel* uiAttribPartServer::getNLAModel( bool is2d ) const
{
    return (NLAModel*)getObject( is2d ? objNLAModel2D() : objNLAModel3D() );
}


bool uiAttribPartServer::selectAttrib( SelSpec& selspec,
				       const ZDomain::Info* zdominfo, bool is2d)
{
    DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
    if ( !adsman->descSet() )
	return false;

    uiAttrSelData attrdata( *adsman->descSet() );
    attrdata.attribid_ = selspec.isNLA() ? SelSpec::cNoAttrib() : selspec.id();
    attrdata.outputnr_ = selspec.isNLA() ? selspec.id().asInt() : -1;
    attrdata.nlamodel_ = getNLAModel(is2d);
    attrdata.zdomaininfo_ = zdominfo;

    if ( is2d )
    {
	MultiID mid( selspec.objectRef() );
	uiAttr2DSelDlg dlg( parent(), adsman->descSet(),
			    mid, attrdata.nlamodel_ );
	if ( !dlg.go() )
	    return false;


	if ( dlg.getSelType() == 0 )
	{
	    LineKey lk( mid, dlg.getStoredAttrName() );
	    attrdata.attribid_ = adsman->descSet()->getStoredID( lk, 0, true );
	}
	else
	    attrdata.attribid_.asInt() = dlg.getSelDescID().asInt();

    }
    else
    {
	uiAttrSelDlg::Setup setup( "View Data" );
	setup.showsteeringdata(true);
	uiAttrSelDlg dlg( parent(), attrdata, setup );
	if ( !dlg.go() )
	    return false;

	attrdata.attribid_.asInt() = dlg.attribID().asInt();
	attrdata.outputnr_ = dlg.outputNr();
	attrdata.setAttrSet( &dlg.getAttrSet() );
    }

    const bool isnla = !attrdata.attribid_.isValid() && attrdata.outputnr_ >= 0;
    const Desc* desc = attrdata.attrSet().getDesc( attrdata.attribid_ );
    const bool isstored = desc && desc->isStored();
    BufferString objref;
    if ( isnla )
	objref = nlaname_;
    else if ( !isstored )
    {
	PtrMan<IOObj> ioobj = IOM().get( adsman->attrsetid_ );
	objref = ioobj ? ioobj->name() : "";
	attrdata.attribid_.setStored( false );
    }

    selspec.set( 0, isnla ? DescID(attrdata.outputnr_,false)
	    		  : attrdata.attribid_, isnla, objref );
    if ( isnla && attrdata.nlamodel_ )
	selspec.setRefFromID( *attrdata.nlamodel_ );
    else if ( !isnla )
	selspec.setRefFromID( attrdata.attrSet() );
    //selspec.setZDomainKey( dlg.zDomainKey() );

    return true;
}


void uiAttribPartServer::directShowAttr( CallBacker* cb )
{
    mDynamicCastGet(uiAttribDescSetEd*,ed,cb);
    if ( !ed ) { pErrMsg("cb is not uiAttribDescSetEd*"); return; }

    dirshwattrdesc_ = ed->curDesc();
    DescSetMan* kpman = eDSHolder().getDescSetMan( ed->is2D() );
    DescSet* edads = const_cast<DescSet*>(dirshwattrdesc_->descSet());
    PtrMan<DescSetMan> tmpadsman = new DescSetMan( ed->is2D(), edads, false );
    eDSHolder().replaceADSMan(tmpadsman);
    sendEvent( evDirectShowAttr() );
    eDSHolder().replaceADSMan(kpman);
}


void uiAttribPartServer::updateSelSpec( SelSpec& ss ) const
{
    bool is2d = ss.is2D();
    if ( ss.isNLA() )
    {
	const NLAModel* nlamod = getNLAModel( is2d );
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
	if ( is2d ) return;
	bool isstored = ss.isStored();
	const bool isother = ss.id().asInt() == SelSpec::cOtherAttrib().asInt();
	const DescSet* ads = DSHolder().getDescSet( false, isstored );
	ss.setIDFromRef( *ads );

	const bool notfound = ss.id() == DescID( -1, false );
	if ( isother && notfound )	//Could be multi-components stored cube
	{
	    ss.setIDFromRef( *DSHolder().getDescSet( false, true ) );
	    isstored = ss.isStored();
	}
	
	if ( !isstored )
	{
	    IOObj* ioobj = IOM().get(
		    		DSHolder().getDescSetMan(false)->attrsetid_ );
	    if ( ioobj ) ss.setObjectRef( ioobj->name() );
	}
    }
}


void uiAttribPartServer::getPossibleOutputs( bool is2d, 
					     BufferStringSet& nms ) const
{
    nms.erase();
    SelInfo attrinf( curDescSet( is2d ), 0, is2d );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
	nms.add( attrinf.attrnms_.get(idx) );

    BufferStringSet storedoutputs;
    for ( int idx=0; idx<attrinf.ioobjids_.size(); idx++ )
    {
	const char* ioobjid = attrinf.ioobjids_.get( idx );
	uiSeisIOObjInfo* sii = new uiSeisIOObjInfo( MultiID(ioobjid), false );
	sii->getDefKeys( storedoutputs, true );
	delete sii;
    }

    for ( int idx=0; idx<storedoutputs.size(); idx++ )
	nms.add( storedoutputs.get(idx) );
}


bool uiAttribPartServer::setSaved( bool is2d ) const
{
    return DSHolder().getDescSetMan( is2d )->isSaved();
}


int uiAttribPartServer::use3DMode() const
{
    const Attrib::DescSet* ads = getUserPrefDescSet();
    if ( !ads ) return -1;
    return DSHolder().getDescSetMan(true)
		&& ads==DSHolder().getDescSet(true,false) ? 0 : 1;
}


const Attrib::DescSet* uiAttribPartServer::getUserPrefDescSet() const
{
    const Attrib::DescSet* ds3d = DSHolder().getDescSet( false, false );
    const Attrib::DescSet* ds2d = DSHolder().getDescSet( true, false );
    if ( !ds3d && !ds2d ) return 0;
    if ( !(ds3d && ds2d) ) return ds3d ? ds3d : ds2d;
    if ( !SI().has3D() ) return ds2d;
    if ( !SI().has2D() ) return ds3d;

    const int nr3d = ds3d->nrDescs( false, true );
    const int nr2d = ds2d->nrDescs( false, true );
    if ( (nr3d>0) != (nr2d>0) ) return nr2d > 0 ? ds2d : ds3d;
    
    int res = uiMSG().askGoOnAfter( "Which attributes do you want to use?",
	   			    0, "&3D", "&2D" );
    if ( res == 2 ) return 0;
    return res == 0 ? ds3d : ds2d;
}


void uiAttribPartServer::saveSet( bool is2d )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(AttribDescSet);
    ctio->ctxt.forread = false;
    uiIOObjSelDlg dlg( parent(), *ctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	ctio->ioobj = 0;
	ctio->setObj( dlg.ioObj()->clone() );
	BufferString bs;
	if ( !ctio->ioobj )
	    uiMSG().error("Cannot find attribute set in data base");
	else if (
	    !AttribDescSetTranslator::store(*DSHolder().getDescSet(is2d,false),
					      ctio->ioobj,bs) )
	    uiMSG().error(bs);
    }
    ctio->setObj( 0 );
}


void uiAttribPartServer::outputVol( MultiID& nlaid, bool is2d )
{
    const DescSet* dset = DSHolder().getDescSet( is2d, false );
    if ( !dset ) { pErrMsg("No attr set"); return; }

    uiAttrVolOut dlg( parent(), *dset, getNLAModel(is2d), nlaid );
    dlg.go();
}


void uiAttribPartServer::setTargetSelSpec( const SelSpec& selspec )
{
    targetspecs_.erase();
    targetspecs_ += selspec;
}


Attrib::DescID uiAttribPartServer::targetID( bool for2d, int nr ) const
{
    return targetspecs_.size() <= nr ? Attrib::DescID::undef()
				     : targetspecs_[nr].id();
}


EngineMan* uiAttribPartServer::createEngMan( const CubeSampling* cs, 
					     const char* linekey )
{
    if ( targetspecs_.isEmpty() ||
	 targetspecs_[0].id().asInt() == SelSpec::cNoAttrib().asInt())
	{ pErrMsg("Nothing to do"); return false; }
    
    const bool istargetstored = targetspecs_[0].isStored();
    const bool is2d = targetspecs_[0].is2D();
    const DescSet* ads = DSHolder().getDescSet( is2d, istargetstored );
    if ( !ads )
	{ pErrMsg("No attr set"); return false; }

    EngineMan* aem = new EngineMan;
    aem->setAttribSet( ads );
    aem->setNLAModel( getNLAModel(is2d) );
    aem->setAttribSpecs( targetspecs_ );
    if ( cs )
	aem->setCubeSampling( *cs );
    if ( linekey )
	aem->setLineKey( linekey );

    return aem;
}


DataPack::ID uiAttribPartServer::createOutput( const CubeSampling& cs,
					       DataPack::ID cacheid )
{
    DataPack* datapack = DPM( DataPackMgr::FlatID() ).obtain( cacheid, true );
    if ( !datapack )
	datapack = DPM( DataPackMgr::CubeID() ).obtain( cacheid, true );

    const Attrib::DataCubes* cache = 0;
    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack);
    if ( cdp ) cache = &cdp->cube();
    mDynamicCastGet(const Attrib::Flat3DDataPack*,fdp,datapack);
    if ( fdp ) cache = &fdp->cube();
    const DataCubes* output = createOutput( cs, cache );
    if ( !output || !output->nrCubes() )  return DataPack::cNoID();

    const bool isflat = cs.isFlat();
    DataPack* newpack;
    if ( isflat )
	newpack = new Attrib::Flat3DDataPack( targetID(false), *output, 0 );
    else
	newpack = new Attrib::CubeDataPack( targetID(false), *output, 0 );
    newpack->setName( targetspecs_[0].userRef() );

    DataPackMgr& dpman = DPM( isflat ? DataPackMgr::FlatID()
	    			     : DataPackMgr::CubeID() );
    dpman.add( newpack );
    return newpack->id();
}


const Attrib::DataCubes* uiAttribPartServer::createOutput(
				const CubeSampling& cs, const DataCubes* cache )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, 0 );
    if ( !aem ) return 0;

    BufferString defstr;
    const bool isstortarget = targetspecs_.size() && targetspecs_[0].isStored();
    const DescSet* attrds = DSHolder().getDescSet( false, isstortarget );
    const Desc* targetdesc = !attrds || attrds->isEmpty() ? 0
			   : attrds->getDesc( targetspecs_[0].id() );
    if ( targetdesc )
    {
	attrds->getDesc(targetspecs_[0].id())->getDefStr(defstr);
	if ( strcmp(defstr,targetspecs_[0].defString()) )
	    cache = 0;
    }

    BufferString errmsg;
    Processor* process = aem->createDataCubesOutput( errmsg, cache );
    if ( !process )
	{ uiMSG().error(errmsg); return 0; }

    bool showinlprogress = true;
    bool showcrlprogress = true;
    Settings::common().getYN( "dTect.Show inl progress", showinlprogress );
    Settings::common().getYN( "dTect.Show crl progress", showcrlprogress );

    const bool isstored =
	targetdesc && targetdesc->isStored() && !targetspecs_[0].isNLA();
    const bool isinl = cs.isFlat() && cs.defaultDir() == CubeSampling::Inl;
    const bool iscrl = cs.isFlat() && cs.defaultDir() == CubeSampling::Crl;
    const bool hideprogress = isstored &&
	( (isinl&&!showinlprogress) || (iscrl&&!showcrlprogress) );

    bool success = true;
    if ( aem->getNrOutputsToBeProcessed(*process) != 0 )
    {
	if ( !hideprogress )
	{
	    uiTaskRunner taskrunner( parent() );
	    success = taskrunner.execute( *process );
	}
	else
	{
	    MouseCursorChanger cursorchgr( MouseCursor::Wait );
	    if ( !process->execute() )
	    {
		BufferString msg( process->message() );
		if ( !msg.isEmpty() )
		    uiMSG().error( msg );
		delete process;
		return 0;
	    }
	}
    }

    const DataCubes* output = aem->getDataCubesOutput( *process );
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


bool uiAttribPartServer::createOutput( DataPointSet& posvals, int firstcol )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return false;

    BufferString errmsg;
    PtrMan<Processor> process =
			aem->getTableOutExecutor( posvals, errmsg, firstcol );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiTaskRunner taskrunner( parent() );
    if ( !taskrunner.execute(*process) ) return false;

    posvals.setName( targetspecs_[0].userRef() );
    return true;
}


bool uiAttribPartServer::createOutput( ObjectSet<DataPointSet>& dpss,
				       int firstcol )
{
    ExecutorGroup execgrp( "Calculating Attribute", true );
    BufferString errmsg;

    ObjectSet<EngineMan> aems;
    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	EngineMan* aem = createEngMan();
	if ( !aem ) continue;

	execgrp.add( aem->getTableOutExecutor(*dpss[idx],errmsg,firstcol) );
	aems += aem;
    }

    bool res = true;
    uiTaskRunner taskrunner( parent() );
    res = taskrunner.execute( execgrp );

    deepErase( aems );
    return res;
}


DataPack::ID uiAttribPartServer::createRdmTrcsOutput(
				const Interval<float>& zrg,
				TypeSet<BinID>* path,
				TypeSet<BinID>* trueknotspos )
{
    BinIDValueSet bidset( 2, false );
    for ( int idx=0; idx<path->size(); idx++ )
	bidset.add( (*path)[idx], zrg.start, zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput( bidset, output, trueknotspos, path ) )
	return -1;

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    DataPack* newpack = new Attrib::FlatRdmTrcsDataPack( targetID(false),
							 output, path );
    newpack->setName( targetspecs_[0].userRef() );
    dpman.add( newpack );
    return newpack->id();
}


bool uiAttribPartServer::createOutput( const BinIDValueSet& bidset,
				       SeisTrcBuf& output,
				       TypeSet<BinID>* trueknotspos,
       				       TypeSet<BinID>* snappedpos )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return 0;

    BufferString errmsg;
    PtrMan<Processor> process = aem->createTrcSelOutput( errmsg, bidset, 
	    						 output, mUdf(float), 0,
							 trueknotspos,
							 snappedpos );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiTaskRunner taskrunner( parent() );
    if ( !taskrunner.execute(*process) ) return false;

    return true;
}


DataPack::ID uiAttribPartServer::create2DOutput( const CubeSampling& cs,
						 const LineKey& linekey,
						 TaskRunner& tr )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, linekey );
    if ( !aem ) return -1;

    BufferString errmsg;
    RefMan<Data2DHolder> data2d = new Data2DHolder;
    PtrMan<Processor> process = aem->createScreenOutput2D( errmsg, *data2d );
    if ( !process )
	{ uiMSG().error(errmsg); return -1; }

    if ( !tr.execute(*process) )
	return -1;

    int component = 0;
    const bool isstored = targetspecs_[0].isStored();
    Attrib::DescID adid = targetID(true);
    const Attrib::DescSet* curds =
			DSHolder().getDescSet( true, isstored );
    if ( curds )
    {
	const Attrib::Desc* targetdesc = curds->getDesc( adid );
	if ( targetdesc )
	    component = targetdesc->selectedOutput();
    }

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    mDeclareAndTryAlloc( Flat2DDHDataPack*, newpack, 
	Attrib::Flat2DDHDataPack( adid, *data2d, false, component ) );
    if ( !newpack || !newpack->isOK() )
	return DataPack::cNoID();

    newpack->setName( linekey.attrName() );
    newpack->setLineName( linekey.lineName() );
    dpman.add( newpack );
    return newpack->id();
}


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
		const int ival = mNINT32(val);
		if ( !mIsEqual(val,ival,mDefEps) || abs(ival)>cMaxNrClasses )
		    return false;
//		nrint++;
//		if ( nrint > sMaxNrVals )
//		    break;
	    }

    return true;
}


bool uiAttribPartServer::extractData( ObjectSet<DataPointSet>& dpss ) 
{
    if ( dpss.isEmpty() ) { pErrMsg("No inp data"); return 0; }
    const DescSet* ads = DSHolder().getDescSet( dpss[0]->is2D(), false );
    if ( !ads ) { pErrMsg("No attr set"); return 0; }

    Attrib::EngineMan aem;
    uiTaskRunner taskrunner( parent() );
    bool somesuccess = false;
    bool somefail = false;

    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	BufferString err;
	DataPointSet& dps = *dpss[idx];
	Executor* tabextr = aem.getTableExtractor( dps, *ads, err );
	if ( !tabextr ) { pErrMsg(err); return 0; }

	if ( taskrunner.execute(*tabextr) )
	    somesuccess = true;
	else
	    somefail = true;

	delete tabextr;
    }

    if ( somefail )
    {
	return somesuccess &&
	     uiMSG().askGoOn(
	      "Some data extraction failed. Do you want to continue and use"
	      " the (partially) extracted data?", true);
    }

    return true;
}


Attrib::DescID uiAttribPartServer::getStoredID( const LineKey& lkey, bool is2d,
       						int selout )
{
    DescSet* ds = eDSHolder().getDescSet( is2d, true );
    return ds ? ds->getStoredID( lkey, selout ) : DescID::undef();
}


bool uiAttribPartServer::createAttributeSet( const BufferStringSet& inps,
					     DescSet* attrset )
{
    AttributeSetCreator attrsetcr( parent(), inps, attrset );
    return attrsetcr.create();
}


bool uiAttribPartServer::setPickSetDirs( Pick::Set& ps, const NLAModel* nlamod,
					 float velocity )
{
    //TODO: force 3D to avoid crash for 2D, need workaround for 2D later
    const Attrib::DescSet* ds = DSHolder().getDescSet( false, false );
    if ( !ds )
	return false;

    uiSetPickDirs dlg( parent(), ps, ds, nlamod, velocity );
    return dlg.go();
}


//TODO may require a linekey in the as ( for docheck )
void uiAttribPartServer::insert2DStoredItems( const BufferStringSet& bfset, 
					      int start, int stop, 
					      bool correcttype, MenuItem* mnu,
       					      const SelSpec& as, bool usesubmnu,
					      bool issteer, bool onlymultcomp ) 
{
    mnu->checkable = true;
    mnu->enabled = bfset.size();
    ObjectSet<MenuItem>* lsets2dmnuitem = issteer ? &linesets2dsteeringmnuitem_
						  : &linesets2dstoredmnuitem_;
    if ( usesubmnu )
    {
	while ( lsets2dmnuitem->size() )
	{
	    MenuItem* olditm = lsets2dmnuitem->remove(0);
	    delete olditm;
	}
    }	    
    for ( int idx=start; idx<stop; idx++ )
    {
	BufferString key = bfset.get(idx);
	MenuItem* itm = new MenuItem( key );
	itm->checkable = true;
	const bool docheck =  correcttype && key == as.userRef();
	mAddManagedMenuItem( mnu, itm, true, docheck );
	if ( docheck ) mnu->checked = true;
	if ( usesubmnu )
	{
	    SelInfo attrinf( DSHolder().getDescSet(true,true), 0, true,
		    	     DescID::undef(), issteer, issteer );
	    const MultiID mid( attrinf.ioobjids_.get(idx) );
	    BufferStringSet nms = get2DStoredItems( mid, issteer, onlymultcomp);
	    if ( nms.isEmpty() ) continue;
	    *lsets2dmnuitem += itm;
	    insert2DStoredItems( nms, 0, nms.size(), correcttype,
				 itm, as, false, issteer, onlymultcomp );
	}
    }
}


BufferStringSet uiAttribPartServer::get2DStoredItems( const MultiID& mid, 
						      bool issteer,
       						      bool onlymultcomp ) const
{
    BufferStringSet nms;
    SelInfo::getAttrNames( mid, nms, issteer, onlymultcomp );
    return nms;
}


BufferStringSet uiAttribPartServer::get2DStoredLSets( const SelInfo& sinf) const
{
    BufferStringSet linesets;
    for ( int idlset=0; idlset<sinf.ioobjids_.size(); idlset++ )
    {
	const char* lsetid = sinf.ioobjids_.get(idlset);
	const MultiID mid( lsetid );
	const BufferString& lsetnm = IOM().get(mid)->name();
	linesets.add( lsetnm );
    }

    return linesets;
}

	
#define mInsertItems(list,mnu,correcttype) \
(mnu)->removeItems(); \
(mnu)->enabled = attrinf.list.size(); \
for ( int idx=start; idx<stop; idx++ ) \
{ \
    const BufferString& nm = attrinf.list.get(idx); \
    MenuItem* itm = new MenuItem( nm ); \
    itm->checkable = true; \
    const bool docheck = correcttype && nm == as.userRef(); \
    mAddMenuItem( mnu, itm, true, docheck );\
    if ( docheck ) (mnu)->checked = true; \
}


static int cMaxMenuSize = 150;

MenuItem* uiAttribPartServer::storedAttribMenuItem( const SelSpec& as, 
						    bool is2d, bool issteer )
{
    MenuItem* storedmnuitem = is2d ? issteer ? &steering2dmnuitem_
					     : &stored2dmnuitem_ 
				   : issteer ? &steering3dmnuitem_
				   	     : &stored3dmnuitem_;
    fillInStoredAttribMenuItem( storedmnuitem, is2d, issteer, as, false );

    return storedmnuitem;
}


void uiAttribPartServer::fillInStoredAttribMenuItem(
					MenuItem* menu, bool is2d, bool issteer,
					const SelSpec& as, bool multcomp,
       					bool needext )
{
    const DescSet* ds = DSHolder().getDescSet( is2d, true );
    SelInfo attrinf( ds, 0, is2d, DescID::undef(), issteer, issteer, multcomp );

    const bool isstored = ds && ds->getDesc( as.id() ) 
	? ds->getDesc( as.id() )->isStored() : false;
    BufferStringSet bfset = is2d ? get2DStoredLSets( attrinf )
		: (issteer ? attrinf.steerids_ : attrinf.ioobjids_);

    MenuItem* mnu = menu;
    if ( multcomp && needext )
    {
	MenuItem* submnu = is2d ? &multcomp2d_ : &multcomp3d_;
	mAddManagedMenuItem( menu, submnu, true, submnu->checked );
	mnu = submnu;
    }
    
    int nritems = bfset.size();
    if ( nritems <= cMaxMenuSize )
    {
	if ( is2d )
	{
	    if ( nritems == 1 )
	    {
		const MultiID mid( attrinf.ioobjids_.get(0) );
		BufferStringSet nmsset = get2DStoredItems(mid,issteer,multcomp);

		insert2DStoredItems( nmsset, 0, nmsset.size(), isstored,
				     mnu, as, false, issteer, multcomp );
	    }
	    else
		insert2DStoredItems( bfset, 0, nritems, isstored,
				     mnu, as, true, issteer, multcomp );
	}
	else
	{
	    const int start = 0; const int stop = nritems;
	    if ( issteer )
	    { mInsertItems(steernms_,mnu,isstored); }
	    else
	    { mInsertItems(ioobjnms_,mnu,isstored); }
	}
    }
    else
	insertNumerousItems( bfset, as, isstored, is2d, issteer );

    mnu->enabled = mnu->nrItems();
}



void uiAttribPartServer::insertNumerousItems( const BufferStringSet& bfset,
					      const SelSpec& as,
       					      bool correcttype, bool is2d,
					      bool issteer )
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
	    insert2DStoredItems( bfset, start, stop, correcttype,
		    		 submnu, as, true, issteer );
	else
	{
	    SelInfo attrinf( DSHolder().getDescSet(false,true), 0, false,
		    	     DescID::undef() );
	    if ( issteer )
	    { mInsertItems(steernms_,submnu,correcttype); }
	    else
	    { mInsertItems(ioobjnms_,submnu,correcttype); }
	}
	
	MenuItem* storedmnuitem = is2d ? issteer ? &steering2dmnuitem_
						 : &stored2dmnuitem_ 
				       : issteer ? &steering3dmnuitem_
						 : &stored3dmnuitem_;
	mAddManagedMenuItem( storedmnuitem, submnu, true,submnu->checked);
    }
}


MenuItem* uiAttribPartServer::stored2DAttribMenuItem( const SelSpec& as,
		const MultiID& mid, const char* linenm, bool issteer )
{
    MenuItem* storedmnuitem = issteer ? &steering2dmnuitem_
	                              : &stored2dmnuitem_;
    storedmnuitem->removeItems();

    BufferStringSet attribnames;
    uiSeisIOObjInfo objinfo( mid );
    SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = issteer ? 1 : 0;
    objinfo.ioObjInfo().getAttribNamesForLine( linenm, attribnames, o2d );

    bool docheckparent = false;
    for ( int idx=0; idx<attribnames.size(); idx++ )
    {
	const BufferString& nm = attribnames.get( idx );
	MenuItem* item = new MenuItem( nm );
	const bool docheck = nm == as.userRef();
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( storedmnuitem, item, true, docheck );
    }

    storedmnuitem->checked = docheckparent;

    return storedmnuitem;
}


MenuItem* uiAttribPartServer::calcAttribMenuItem( const SelSpec& as,
						  bool is2d, bool useext )
{
    SelInfo attrinf( DSHolder().getDescSet(is2d,false) );
    const bool isattrib = attrinf.attrids_.indexOf( as.id() ) >= 0; 

    const int start = 0; const int stop = attrinf.attrnms_.size();
    MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    BufferString txt = useext ? ( is2d? "Attributes &2D" : "Attributes &3D" )
			      : "&Attributes";
    calcmnuitem->text = txt;
    mInsertItems(attrnms_,calcmnuitem,isattrib);

    calcmnuitem->enabled = calcmnuitem->nrItems();
    return calcmnuitem;
}


MenuItem* uiAttribPartServer::nlaAttribMenuItem( const SelSpec& as, bool is2d,
       						 bool useext )
{
    const NLAModel* nlamodel = getNLAModel(is2d);
    MenuItem* nlamnuitem = is2d ? &nla2dmnuitem_ : &nla3dmnuitem_;
    if ( nlamodel )
    {
	BufferString ittxt;
	if ( !useext || is2d )
	    { ittxt = "&"; ittxt += nlamodel->nlaType(false); }
	else
	    ittxt = "N&eural Network 3D";
	if ( useext && is2d ) ittxt += " 2D";
	
	nlamnuitem->text = ittxt;
	const DescSet* dset = DSHolder().getDescSet(is2d,false);
	SelInfo attrinf( dset, nlamodel );
	const bool isnla = as.isNLA();
	const int start = 0; const int stop = attrinf.nlaoutnms_.size();
	mInsertItems(nlaoutnms_,nlamnuitem,isnla);
    }

    nlamnuitem->enabled = nlamnuitem->nrItems();
    return nlamnuitem;
}


MenuItem* uiAttribPartServer::zDomainAttribMenuItem( const SelSpec& as,
						     const ZDomain::Info& zdinf,
						     bool is2d, bool useext)
{
    MenuItem* zdomainmnuitem = is2d ? &zdomain2dmnuitem_ 
				    : &zdomain3dmnuitem_;
    BufferString itmtxt = zdinf.userName();
    itmtxt += useext ? (!is2d ? " Cubes" : " 2D Lines") : " Data";
    zdomainmnuitem->text = itmtxt;
    zdomainmnuitem->removeItems();
    zdomainmnuitem->checkable = true;
    zdomainmnuitem->checked = false;

    BufferStringSet ioobjnms;
    SelInfo::getZDomainItems( zdinf, ioobjnms );
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const BufferString& nm = ioobjnms.get( idx );
	MenuItem* itm = new MenuItem( nm );
	const bool docheck = nm == as.userRef();
	mAddManagedMenuItem( zdomainmnuitem, itm, true, docheck );
	if ( docheck ) zdomainmnuitem->checked = true;
    }

    zdomainmnuitem->enabled = zdomainmnuitem->nrItems();
    return zdomainmnuitem;
}


bool uiAttribPartServer::handleAttribSubMenu( int mnuid, SelSpec& as,
       					      bool& dousemulticomp )
{
    const bool is3d = stored3dmnuitem_.findItem(mnuid) ||
		      calc3dmnuitem_.findItem(mnuid) ||
		      nla3dmnuitem_.findItem(mnuid) ||
		      zdomain3dmnuitem_.findItem(mnuid) ||
		      steering3dmnuitem_.findItem(mnuid);
    //look at 3D =trick: extra menus available in 2D cannot be reached from here
    const bool is2d = !is3d;

    const bool issteering = steering2dmnuitem_.findItem(mnuid) ||
			    steering3dmnuitem_.findItem(mnuid);
    const DescSetMan* adsman = DSHolder().getDescSetMan( is2d );
    uiAttrSelData attrdata( *adsman->descSet() );
    attrdata.nlamodel_ = getNLAModel(is2d);
    SelInfo attrinf( &attrdata.attrSet(), attrdata.nlamodel_, is2d,
	    	     DescID::undef(), issteering, issteering );
    const MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    const MenuItem* nlamnuitem = is2d ? &nla2dmnuitem_ : &nla3dmnuitem_;
    const MenuItem* zdomainmnuitem = is2d ? &zdomain2dmnuitem_
					      : &zdomain3dmnuitem_;
    ObjectSet<MenuItem>* ls2dmnuitm = issteering ? &linesets2dsteeringmnuitem_
						 : &linesets2dstoredmnuitem_;

    DescID attribid = SelSpec::cAttribNotSel();
    int outputnr = -1;
    bool isnla = false;
    bool isstored = false;
    LineKey idlkey;

    if ( stored3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = stored3dmnuitem_.findItem(mnuid);
	const int idx = attrinf.ioobjnms_.indexOf(item->text);
	const char* objidstr = attrinf.ioobjids_.get(idx);
	attribid = eDSHolder().getDescSet(false,true)->getStoredID( objidstr );
	idlkey = LineKey( objidstr );
	isstored = true;
    }
    else if ( steering3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = steering3dmnuitem_.findItem( mnuid );
	const int idx = attrinf.steernms_.indexOf( item->text );
	const char* objidstr = attrinf.steerids_.get( idx );
	attribid = eDSHolder().getDescSet(false,true)->getStoredID( objidstr );
	idlkey = LineKey( objidstr );
	isstored = true;
    }
    else if ( stored2dmnuitem_.findItem(mnuid) ||
	      steering2dmnuitem_.findItem(mnuid) )
    {
	if ( ls2dmnuitm->isEmpty() )
	{
	    const MenuItem* item = stored2dmnuitem_.findItem(mnuid);
	    if ( !item ) item = steering2dmnuitem_.findItem(mnuid);
	    const MultiID mid( attrinf.ioobjids_.get(0) );
	    idlkey = LineKey( mid, item->text );
	    attribid = eDSHolder().getDescSet(true,true)->getStoredID( idlkey );
	    isstored = true;
	}
	else
	{
	    for ( int idx=0; idx<ls2dmnuitm->size(); idx++ )
	    {
		if ( (*ls2dmnuitm)[idx]->findItem(mnuid) )
		{
		    const MenuItem* item = (*ls2dmnuitm)[idx]->findItem(mnuid);
		    const MultiID mid( attrinf.ioobjids_.get(idx) );
		    idlkey = LineKey( mid, item->text );
		    attribid = eDSHolder().getDescSet( true, true )
					->getStoredID( idlkey );
		    isstored = true;
		}
	    }
	}
    }
    else if ( calcmnuitem->findItem(mnuid) )
    {
	const MenuItem* item = calcmnuitem->findItem(mnuid);
	int idx = attrinf.attrnms_.indexOf(item->text);
	attribid = attrinf.attrids_[idx];
    }
    else if ( nlamnuitem->findItem(mnuid) )
    {
	outputnr = nlamnuitem->itemIndex(nlamnuitem->findItem(mnuid));
	isnla = true;
    }
    else if ( zdomainmnuitem->findItem(mnuid) )
    {
	if ( is2d )
	    return false;
	const MenuItem* item = zdomainmnuitem->findItem( mnuid );
	IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id));
	PtrMan<IOObj> ioobj = IOM().getLocal( item->text );
	if ( ioobj )
	{
	    attribid = eDSHolder().getDescSet( false, true )
					->getStoredID( ioobj->key() );
	    isstored = true;
	}
    }
    else
	return false;

    if ( isstored )
    {
	BufferStringSet complist;
	SeisIOObjInfo::getCompNames( idlkey, complist );
	if ( complist.size()>1 )
	{
	    TypeSet<int> selcomps;
	    if ( !handleMultiComp( idlkey, is2d, issteering, complist,
				   attribid, selcomps ) )
		return false;

	    dousemulticomp = selcomps.size()>1;
	    if ( dousemulticomp )
		return true;
	}
    }
    

    BufferString objref;
    if ( isnla )
	objref = nlaname_;
    else if ( !isstored )
    {
	PtrMan<IOObj> ioobj = IOM().get( adsman->attrsetid_ );
	objref = ioobj ? ioobj->name() : "";
    }

    DescID did = isnla ? DescID(outputnr,false) : attribid;
    as.set( 0, did, isnla, objref );

    BufferString bfs;
    if ( attribid.asInt() != SelSpec::cAttribNotSel().asInt() )
    {
	DSHolder().getDescSet(is2d,isstored)->getDesc(attribid)->getDefStr(bfs);
	as.setDefString(bfs.buf());
    }

    if ( isnla )
	as.setRefFromID( *attrdata.nlamodel_ );
    else
	as.setRefFromID( *DSHolder().getDescSet(is2d, isstored) );

    as.set2DFlag( is2d );

    return true;
}


void uiAttribPartServer::info2DAttribSubMenu( int mnuid, BufferString& attbnm,
					      bool& steering, bool& stored )
{
    steering = steering2dmnuitem_.findItem(mnuid);
    stored = false;

    if ( stored2dmnuitem_.findItem(mnuid) || steering2dmnuitem_.findItem(mnuid))
    {
	stored = true;
	const MenuItem* item = stored2dmnuitem_.findItem( mnuid );
	if ( !item ) item = steering2dmnuitem_.findItem( mnuid );
	attbnm = item->text;
    }
    else if ( calc2dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = calc2dmnuitem_.findItem( mnuid );
	attbnm = item->text;
    }
}


#define mFakeCompName( searchfor, replaceby ) \
{ \
    LineKey lkey( desc->userRef() ); \
    if ( lkey.attrName() == searchfor ) \
	lkey.setAttrName( replaceby );\
    desc->setUserRef( lkey.buf() ); \
}

bool uiAttribPartServer::handleMultiComp( const LineKey& idlkey, bool is2d,
					  bool issteering,
					  BufferStringSet& complist,
					  DescID& attribid,
					  TypeSet<int>& selectedcomps )
{
    //Trick for old steering cubes: fake good component names
    if ( !is2d && issteering && complist.indexOf("Component 1")>=0 )
    {
	complist.erase();
	complist.add( "Inline Dip" );
	complist.add( "Crossline Dip" );
    }

    uiMultCompDlg compdlg( parent(), complist );
    if ( compdlg.go() )
    {
	selectedcomps.erase();
	compdlg.getCompNrs( selectedcomps );
	if ( !selectedcomps.size() ) return false;

	DescSet* ads = eDSHolder().getDescSet( is2d, true );
	if ( selectedcomps.size() == 1 )
	{
	    //Using const_cast for compiler but ads won't be modified anyway
	    attribid = const_cast<DescSet*>(ads)
			->getStoredID( idlkey, selectedcomps[0], false );
	    //Trick for old steering cubes: fake good component names
	    if ( !is2d && issteering )
	    {
		Attrib::Desc* desc = ads->getDesc(attribid);
		if ( !desc ) return false;
		mFakeCompName( "Component 1", "Inline Dip" );
		mFakeCompName( "Component 2", "Crossline Dip" );
	    }

	    return true;
	}
	prepMultCompSpecs( selectedcomps, idlkey, is2d, issteering );
    }
    else
	return false;

    return true;
}


bool uiAttribPartServer::prepMultCompSpecs( TypeSet<int> selectedcomps,
					    const LineKey& idlkey, bool is2d,
					    bool issteering )
{
    targetspecs_.erase();
    DescSet* ads = eDSHolder().getDescSet( is2d, true );
    for ( int idx=0; idx<selectedcomps.size(); idx++ )
    {
	DescID did = ads->getStoredID( idlkey, selectedcomps[idx], true );
	SelSpec as( 0, did );
	BufferString bfs;
	Attrib::Desc* desc = ads->getDesc(did);
	if ( !desc ) return false;

	desc->getDefStr(bfs);
	as.setDefString(bfs.buf());
	//Trick for old steering cubes: fake good component names
	if ( !is2d && issteering )
	{
	    mFakeCompName( "Component 1", "Inline Dip" );
	    mFakeCompName( "Component 2", "Crossline Dip" );
	}

	//Trick for PreStack offsets displayed on the fly
	if ( desc->isStored() && desc->userRef()[0] == '{' )
	{
	    LineKey lkey( desc->userRef() );
	    BufferString newnm = "offset index "; newnm += selectedcomps[idx];
	    lkey.setAttrName( newnm );
	    desc->setUserRef( lkey.buf() );
	}

	as.setRefFromID( *ads );
	as.set2DFlag( is2d );
	targetspecs_ += as;
    }
    set2DEvent( is2d );
    return true;
}


IOObj* uiAttribPartServer::getIOObj( const Attrib::SelSpec& as ) const
{
    if ( as.isNLA() ) return 0;

    const Attrib::DescSet* attrset = DSHolder().getDescSet( as.is2D(), true );
    if ( !attrset ) return 0;

    const Attrib::Desc* desc = attrset->getDesc( as.id() );
    if ( !desc ) return 0;

    BufferString storedid = desc->getStoredID();
    if ( !desc->isStored() || storedid.isEmpty() ) return 0;

    return IOM().get( MultiID(storedid.buf()) );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiAttribPartServer::processEvalDlg( bool iscrossevaluate )
{
    if ( !attrsetdlg_ ) return;
    const Desc* curdesc = attrsetdlg_->curDesc();
    if ( !curdesc )
	mErrRet( "Please add this attribute first" )

    uiAttrDescEd* ade = attrsetdlg_->curDescEd();
    if ( !ade ) return;

    sendEvent( evEvalAttrInit() );
    //if ( !alloweval_ ) mErrRet( "Evaluation of attributes only possible on\n"
//			       "Inlines, Crosslines, Timeslices and Surfaces.");

    if ( !iscrossevaluate )
    {
    	uiEvaluateDlg* evaldlg = 
	    new uiEvaluateDlg( attrsetdlg_, *ade, allowevalstor_ );
    
	if ( !evaldlg->evaluationPossible() )
    	    mErrRet( "This attribute has no parameters to evaluate" )
    
	evaldlg->calccb.notify( mCB(this,uiAttribPartServer,calcEvalAttrs) );
    	evaldlg->showslicecb.notify( mCB(this,uiAttribPartServer,showSliceCB) );
    	evaldlg->windowClosed.notify(
		mCB(this,uiAttribPartServer,evalDlgClosed) );
	evaldlg->go();
    }
    else
    {
    	uiCrossAttrEvaluateDlg* crossevaldlg = 
	    new uiCrossAttrEvaluateDlg(attrsetdlg_,*attrsetdlg_,allowevalstor_);
	crossevaldlg->calccb.notify( 
		mCB(this,uiAttribPartServer,calcEvalAttrs) );
	crossevaldlg->showslicecb.notify( 
		mCB(this,uiAttribPartServer,showSliceCB) );
	crossevaldlg->windowClosed.notify(
		mCB(this,uiAttribPartServer,evalDlgClosed) );
	crossevaldlg->go();
    }

    attrsetdlg_->setSensitive( false );
}


void uiAttribPartServer::showCrossEvalDlg( CallBacker* )
{ processEvalDlg( true ); }


void uiAttribPartServer::showEvalDlg( CallBacker* cb )
{ processEvalDlg( false ); }


void uiAttribPartServer::evalDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    mDynamicCastGet(uiCrossAttrEvaluateDlg*,crossevaldlg,cb);
    if ( !evaldlg && !crossevaldlg )
       return;	
    
    if ( (evaldlg && evaldlg->storeSlices()) ||
	 (crossevaldlg && crossevaldlg->storeSlices()) )
	sendEvent( evEvalStoreSlices() );
    
    Desc* curdesc = attrsetdlg_->curDesc();
    BufferString curusrref = curdesc->userRef();

    const Desc* evad = evaldlg ? evaldlg->getAttribDesc() 
			       : crossevaldlg->getAttribDesc();
    if ( evad )
    {
	BufferString defstr;
	evad->getDefStr( defstr );
	curdesc->parseDefStr( defstr );
	curdesc->setUserRef( curusrref );
	attrsetdlg_->updateCurDescEd();

	if ( crossevaldlg )
	{
	    const TypeSet<Attrib::DescID>& cids = 
		crossevaldlg->evaluateChildIds();
	    BufferString ds = crossevaldlg->acceptedDefStr();
	    Attrib::DescSet* ads = attrsetdlg_->getSet();
	    for ( int idx=0; idx<cids.size(); idx++ )
	    {
		Desc* ad = ads->getDesc( cids[idx] );
		if ( ad ) ad->parseDefStr( ds );
	    }
	}
    }

    sendEvent( evEvalRestore() );
    attrsetdlg_->setSensitive( true );
}


void uiAttribPartServer::calcEvalAttrs( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    mDynamicCastGet(uiCrossAttrEvaluateDlg*,crossevaldlg,cb);
    if ( !evaldlg && !crossevaldlg )
       return;	

    const bool is2d = attrsetdlg_->is2D();
    DescSetMan* kpman = eDSHolder().getDescSetMan( is2d );
    DescSet* ads = evaldlg ? evaldlg->getEvalSet() : crossevaldlg->getEvalSet();
    if ( evaldlg )
    	evaldlg->getEvalSpecs( targetspecs_ );
    else
	crossevaldlg->getEvalSpecs( targetspecs_ );
    
    PtrMan<DescSetMan> tmpadsman = new DescSetMan( is2d, ads, false );
    eDSHolder().replaceADSMan( tmpadsman );
    set2DEvent( is2d );
    sendEvent( evEvalCalcAttr() );
    eDSHolder().replaceADSMan( kpman );
}


void uiAttribPartServer::showSliceCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( sel < 0 ) return;

    sliceidx_ = sel;
    sendEvent( evEvalShowSlice() );
}


#define mCleanMenuItems(startstr)\
{\
    startstr##mnuitem_.removeItems();\
    startstr##mnuitem_.checked = false;\
}

void uiAttribPartServer::resetMenuItems()
{
    mCleanMenuItems(stored2d)
    mCleanMenuItems(steering2d)
    mCleanMenuItems(calc2d)
    mCleanMenuItems(nla2d)
    mCleanMenuItems(stored3d)
    mCleanMenuItems(steering3d)
    mCleanMenuItems(calc3d)
    mCleanMenuItems(nla3d)
}


void uiAttribPartServer::fillPar( IOPar& iopar, bool is2d, bool isstored ) const
{
    const DescSet* ads = DSHolder().getDescSet( is2d, isstored );
    if ( ads && !ads->isEmpty() )
	ads->fillPar( iopar );
}


void uiAttribPartServer::usePar( const IOPar& iopar, bool is2d, bool isstored )
{
    DescSet* ads = eDSHolder().getDescSet( is2d, isstored );
    if ( ads )
    {
	BufferStringSet errmsgs;
	BufferString versionstr;
	float versionnr = iopar.get( sKey::Version(), versionstr )
	    			? toFloat( versionstr.buf() ) : 0 ;
	if ( isstored && versionnr<4.05 )	//backward compatibility v<4.1.1
	{
	    DescSet* adsnonstored = eDSHolder().getDescSet( is2d, false );
	    if ( adsnonstored )
	    {
		TypeSet<DescID> allstoredids;
		adsnonstored->getStoredIds( allstoredids );
		ads = adsnonstored->optimizeClone( allstoredids );
		ads->setContainStoredDescOnly( true );
		eDSHolder().replaceStoredAttribSet( ads );
	    }
	}
	else
	    ads->usePar( iopar, versionnr, &errmsgs );

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

	set2DEvent( is2d );
	sendEvent( evNewAttrSet() );
    }
}


void uiAttribPartServer::setEvalBackupColTabMapper(
			const ColTab::MapperSetup* mp )
{
    if ( evalmapperbackup_ && mp )
	*evalmapperbackup_ = *mp;
    else if ( !mp )
    {
	delete evalmapperbackup_;
	evalmapperbackup_ = 0;
    }
    else if ( mp )
    {
	evalmapperbackup_ = new ColTab::MapperSetup( *mp );
    }
}


const ColTab::MapperSetup* uiAttribPartServer::getEvalBackupColTabMapper() const
{ return evalmapperbackup_; }
