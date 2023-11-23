/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattribpartserv.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsetsholder.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "attribprovider.h"
#include "attribsel.h"
#include "attribsetcreator.h"

#include "arraynd.h"
#include "coltabmapper.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "executor.h"
#include "keystrs.h"
#include "ioman.h"
#include "mousecursor.h"
#include "nlamodel.h"
#include "randomlinegeom.h"
#include "rangeposprovider.h"
#include "seisbuf.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seispreload.h"
#include "seisparallelreader.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "zdomain.h"

#include "uiattr2dsel.h"
#include "uiattrdesced.h"
#include "uiattrdescseted.h"
#include "uiattrgetfile.h"
#include "uiattribcrossplot.h"
#include "uiattrsel.h"
#include "uiattrsetman.h"
#include "uiattrvolout.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicrossattrevaluatedlg.h"
#include "uievaluatedlg.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimultcomputils.h"
#include "uimultoutsel.h"
#include "uirgbattrseldlg.h"
#include "uiseispartserv.h"
#include "uisetpickdirs.h"
#include "uitaskrunner.h"


int uiAttribPartServer::evDirectShowAttr()	{ return 0; }
int uiAttribPartServer::evNewAttrSet()		{ return 1; }
int uiAttribPartServer::evAttrSetDlgClosed()	{ return 2; }
int uiAttribPartServer::evEvalAttrInit()	{ return 3; }
int uiAttribPartServer::evEvalCalcAttr()	{ return 4; }
int uiAttribPartServer::evEvalShowSlice()	{ return 5; }
int uiAttribPartServer::evEvalStoreSlices()	{ return 6; }
int uiAttribPartServer::evEvalRestore()		{ return 7; }
int uiAttribPartServer::objNLAModel2D()		{ return 100; }
int uiAttribPartServer::objNLAModel3D()		{ return 101; }

const char* uiAttribPartServer::attridstr()	{ return "Attrib ID"; }

static const int cMaxMenuSize = 150;

const char* uiAttribPartServer::sKeyUserSettingAttrErrMsg()
{ return "dTect.Display attribute positioning error messages"; }

uiAttribPartServer* uiAttribPartServer::theinst_ = nullptr;

uiAttribPartServer* uiAttribPartServer::getInst()
{ return theinst_; }

uiAttribPartServer::uiAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , needSaveNLA(this)
    , multcomp2d_(uiStrings::s2D())
    , multcomp3d_(uiStrings::s3D())
    , attrsetclosetimer_("Attrset dialog close")
{
    theinst_ = this;
    attrsetclosetimer_.tick.notify(
			mCB(this,uiAttribPartServer,attrsetDlgCloseTimTick) );

    stored2dmnuitem_.checkable = true;
    stored3dmnuitem_.checkable = true;
    calc2dmnuitem_.checkable = true;
    calc3dmnuitem_.checkable = true;
    steering2dmnuitem_.checkable = true;
    steering3dmnuitem_.checkable = true;
    multcomp3d_.checkable = true;
    multcomp2d_.checkable = true;

    bool yn;
    if ( !Settings::common().getYN( sKeyUserSettingAttrErrMsg(), yn ) )
    {
	Settings::common().setYN( sKeyUserSettingAttrErrMsg(), true );
	Settings::common().write();
    }

    mAttachCB( IOM().surveyChanged, uiAttribPartServer::survChangedCB );
    handleAutoSet();
}


uiAttribPartServer::~uiAttribPartServer()
{
    detachAllNotifiers();
    if ( theinst_ == this )
	theinst_ = nullptr;

    delete attrsetdlg_;

    deepErase( attrxplotset_ );
    if ( hasDSHolder() )
	delete &eDSHolder();
    delete manattribset2ddlg_;
    delete manattribsetdlg_;
    delete impattrsetdlg_;
    delete volattrdlg_;
    delete multiattrdlg_;
    delete dataattrdlg_;
}


uiString uiAttribPartServer::getMenuText( bool is2d,
					  bool issteering, bool endmenu )
{
    uiString menutext;
    if ( is2d )
	menutext = issteering ? toUiString("%1 %2 %3")
		   .arg(uiStrings::sSteering()).arg(uiStrings::s2D())
		   .arg(uiStrings::sData()) : toUiString("%1 %2 %3")
		   .arg(uiStrings::sStored()).arg(uiStrings::s2D())
		   .arg(uiStrings::sData());
    else
	menutext = issteering ? mJoinUiStrs(sSteering(),sCube(mPlural))
			      : mJoinUiStrs(sStored(),sCube(mPlural));

    return endmenu ? m3Dots(menutext) : menutext;
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
    DescSetMan* setmgr = eDSHolder().getDescSetMan(is2d);
    if ( SI().pars().get(idkey,id) )
    {
	PtrMan<IOObj> ioobj = IOM().get( id );
	uiString bs;
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


bool uiAttribPartServer::replaceSet( const IOPar& iopar, bool is2d )
{
    DescSet* ads = new DescSet(is2d);
    uiStringSet errmsgs;
    if ( !ads->usePar(iopar,&errmsgs) )
    {
	delete ads;
	uiMSG().errorWithDetails( errmsgs );
	return false;
    }
    eDSHolder().replaceAttribSet( ads );

    DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
    adsman->attrsetid_.setUdf();
    if ( attrsetdlg_ && attrsetdlg_->is2D()==is2d )
	attrsetdlg_->setDescSetMan( adsman );
    set2DEvent( is2d );
    sendEvent( evNewAttrSet() );
    return true;
}


bool uiAttribPartServer::addToDescSet( const char* keystr, bool is2d )
{
    //TODO: think of it: stored data can  be at 2 places: also in attrib set...
    const MultiID key( keystr );
    return eDSHolder().getDescSet(is2d,true)->getStoredID(key,-1,true)
								.isValid();
}


const DescSet* uiAttribPartServer::curDescSet( bool is2d ) const
{
    return DSHolder().getDescSetMan( is2d )->descSet();
}


void uiAttribPartServer::getDirectShowAttrSpec( SelSpec& as ) const
{
   if ( !dirshwattrdesc_ )
       as.set( nullptr, SelSpec::cNoAttrib(), false, nullptr );
   else
       as.set( *dirshwattrdesc_ );
}


void uiAttribPartServer::manageAttribSets( bool is2d )
{
    if ( is2d )
    {
	delete manattribset2ddlg_;
	manattribset2ddlg_ = new uiAttrSetMan( parent(), true );
	manattribset2ddlg_->show();
    }
    else
    {
	delete manattribsetdlg_;
	manattribsetdlg_ = new uiAttrSetMan( parent(), false );
	manattribsetdlg_->show();
    }
}


bool uiAttribPartServer::editSet( bool is2d )
{
    const DescSetMan* adsman = DSHolder().getDescSetMan( is2d );
    IOPar iop;
    if ( adsman->descSet() ) adsman->descSet()->fillPar( iop );

    delete attrsetdlg_;
    attrsetdlg_ = new uiAttribDescSetEd( parent(),
				const_cast<DescSetMan*>(adsman), nullptr );
    attrsetdlg_->applycb.notify(
		mCB(this,uiAttribPartServer,attrsetDlgApply) );
    attrsetdlg_->dirshowcb.notify(
		mCB(this,uiAttribPartServer,directShowAttr) );
    attrsetdlg_->evalattrcb.notify(
		mCB(this,uiAttribPartServer,showEvalDlg) );
    attrsetdlg_->crossevalattrcb.notify(
		mCB(this,uiAttribPartServer,showCrossEvalDlg) );
    attrsetdlg_->xplotcb.notify(
		mCB(this,uiAttribPartServer,showXPlot) );
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

    uiAttribCrossPlot* uiattrxplot = new uiAttribCrossPlot( nullptr,
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
    if ( !crossplot )
	return;

    if ( attrxplotset_.isPresent(crossplot) )
    {
	uiAttribCrossPlot* xplot =
	    attrxplotset_.removeSingle( attrxplotset_.indexOf(crossplot) );
	xplot->windowClosed.remove(
		mCB(this,uiAttribPartServer,xplotClosedCB) );
	xplot->setDeleteOnClose( true );
    }
}


void uiAttribPartServer::attrsetDlgApply( CallBacker* )
{
    const bool is2d = attrsetdlg_->getSet()->is2D();
    DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
    adsman->setDescSet( new DescSet( *attrsetdlg_->getSet() ) );
    adsman->attrsetid_ = attrsetdlg_->curSetID();
    set2DEvent( is2d );
    sendEvent( evNewAttrSet() );
}


void uiAttribPartServer::attrsetDlgClosed( CallBacker* )
{
    attrsetclosetimer_.start( 10, true );
}


void uiAttribPartServer::attrsetDlgCloseTimTick( CallBacker* )
{
    if ( attrsetdlg_->uiResult() )
    {
	bool is2d = attrsetdlg_->getSet()->is2D();
	DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
	adsman->setDescSet( new DescSet( *attrsetdlg_->getSet() ) );
	adsman->attrsetid_ = attrsetdlg_->curSetID();
	set2DEvent( is2d );
	sendEvent( evNewAttrSet() );
    }

    delete attrsetdlg_;
    attrsetdlg_ = nullptr;
    sendEvent( evAttrSetDlgClosed() );
}


const NLAModel* uiAttribPartServer::getNLAModel( bool is2d ) const
{
    const void* nlaobj = getObject( is2d ? objNLAModel2D() : objNLAModel3D() );
    const auto* nlamod = sCast(const NLAModel*,nlaobj);
    return isEmpty( nlamod ) ? nullptr : nlamod;
}


bool uiAttribPartServer::selectAttrib( SelSpec& selspec,
				       const ZDomain::Info* zdominfo,
				       Pos::GeomID geomid,
				       const uiString& seltxt )
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const bool is2d = geom && geom->is2D();
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
	TypeSet<Pos::GeomID> geomids; geomids += geomid;
	uiAttr2DSelDlg dlg( parent(), adsman->descSet(), geomids,
			    attrdata.nlamodel_ );
	if ( !dlg.go() )
	    return false;

	if ( dlg.getSelType()==0 || dlg.getSelType()==1 )
	{
	    SeisIOObjInfo info( dlg.getStoredAttrName(), Seis::Line );
	    attrdata.attribid_ = adsman->descSet()->getStoredID(
			info.ioObj() ? info.ioObj()->key() : MultiID::udf(),
			dlg.getComponent(), true );
	}
	else
	{
	    attrdata.attribid_.asInt() = dlg.getSelDescID().asInt();
	    attrdata.outputnr_ = dlg.getOutputNr();
	}
    }
    else
    {
	uiAttrSelDlg::Setup setup( mToUiStringTodo(seltxt) );
	setup.showsteeringdata(true);
	uiAttrSelDlg dlg( parent(), attrdata, setup );
	if ( !dlg.go() )
	    return false;

	attrdata.attribid_.asInt() = dlg.attribID().asInt();
	attrdata.outputnr_ = dlg.outputNr();
	attrdata.compnr_ = dlg.compNr();
	attrdata.setAttrSet( &dlg.getAttrSet() );
	const Attrib::Desc* desc =
	    attrdata.attrSet().getDesc( attrdata.attribid_ );
	if ( desc && desc->isStored() && attrdata.compnr_==-1 )
	{
	    const MultiID dbky =
		dlg.getAttrSet().getStoredKey( attrdata.attribid_ );
	    SeisIOObjInfo info( dbky );
	    TypeSet<int> selectedcomps( info.nrComponents(), 0 );
	    for ( int idx=1; idx<selectedcomps.size(); idx++ )
		selectedcomps[idx] = idx;

	    selspec.set( nullptr, Attrib::SelSpec::cOtherAttrib(),
			 false, nullptr );
	    return prepMultCompSpecs( selectedcomps, dbky, false, true );
	}
    }

    const bool isnla = !attrdata.attribid_.isValid() &&
			attrdata.outputnr_ >= 0;
    const Desc* desc = attrdata.attrSet().getDesc( attrdata.attribid_ );
    const bool isstored = desc && desc->isStored();
    BufferString objref;
    if ( isnla )
	objref = nlaname_;
    else if ( !isstored )
    {
	PtrMan<IOObj> ioobj = IOM().get( adsman->attrsetid_ );
	objref = ioobj ? ioobj->name().buf() : "";
	attrdata.attribid_.setStored( false );
    }

    selspec.set( nullptr, isnla ? DescID(attrdata.outputnr_,isstored)
			  : attrdata.attribid_, isnla, objref );
    if ( isnla && attrdata.nlamodel_ )
	selspec.setRefFromID( *attrdata.nlamodel_ );
    else if ( !isnla )
	selspec.setRefFromID( attrdata.attrSet() );
    //selspec.setZDomainKey( dlg.zDomainKey() );
    //selspec.setZDomainUnit( ?? );

    setTargetSelSpec( selspec );
    return true;
}


bool uiAttribPartServer::selectRGBAttribs( TypeSet<SelSpec>& rgbaspecs,
					   const ZDomain::Info* zinf,
					   Pos::GeomID geomid )
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const bool is2d = geom && geom->is2D();
    DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
    if ( !adsman->descSet() )
	return false;

    uiRGBAttrSelDlg dlg( parent(), *adsman->descSet(), geomid );
    dlg.setSelSpec( rgbaspecs );
    if ( !dlg.go() )
	return false;

    dlg.fillSelSpec( rgbaspecs );
    return true;
}


void uiAttribPartServer::directShowAttr( CallBacker* cb )
{
    mDynamicCastGet(uiAttribDescSetEd*,ed,cb);
    if ( !ed )
    {
	pErrMsg("cb is not uiAttribDescSetEd*");
	return;
    }

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
	    ss.set( ss.userRef(), SelSpec::cNoAttrib(), true, nullptr );
    }
    else
    {
	if ( is2d )
	    return;

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
    SelInfo attrinf( curDescSet( is2d ), nullptr, is2d );
    nms.append( attrinf.attrnms_ );
    for ( auto& id : attrinf.ioobjids_ )
	nms.add( id.toString() );
}


bool uiAttribPartServer::setSaved( bool is2d ) const
{
    return hasDSHolder() ? DSHolder().getDescSetMan( is2d )->isSaved() : true;
}


int uiAttribPartServer::use3DMode() const
{
    const DescSet* ads = getUserPrefDescSet();
    if ( !ads )
	return -1;

    return DSHolder().getDescSetMan(true)
		&& ads==DSHolder().getDescSet(true,false) ? 0 : 1;
}


const Attrib::DescSet* uiAttribPartServer::getUserPrefDescSet() const
{
    const DescSet* ds3d = DSHolder().getDescSet( false, false );
    const DescSet* ds2d = DSHolder().getDescSet( true, false );
    if ( !ds3d && !ds2d )
	return nullptr;

    if ( !(ds3d && ds2d) )
	return ds3d ? ds3d : ds2d;

    if ( !SI().has3D() )	return ds2d;
    if ( !SI().has2D() )	return ds3d;

    const int nr3d = ds3d->nrDescs( false, true );
    const int nr2d = ds2d->nrDescs( false, true );
    if ( (nr3d>0) != (nr2d>0) )
	return nr2d > 0 ? ds2d : ds3d;

    const int res =
	uiMSG().ask2D3D( tr("Which attributes do you want to use?"), true );
    return res == -1 ? nullptr : (res == 1 ? ds2d : ds3d);
}


void uiAttribPartServer::saveSet( bool is2d )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(AttribDescSet);
    ctio->ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( parent(), *ctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	ctio->ioobj_ = nullptr;
	ctio->setObj( dlg.ioObj()->clone() );
	uiString bs;
	if ( !ctio->ioobj_ )
	    uiMSG().error(tr("Cannot find attribute set in data base"));
	else if (
	    !AttribDescSetTranslator::store(*DSHolder().getDescSet(is2d,false),
					      ctio->ioobj_,bs) )
	    uiMSG().error(bs);
    }

    ctio->setObj( nullptr );
}


#define mAttrProcDlg( dlgobj ) \
    { \
	if ( !dlgobj ) \
	{\
	    dlgobj = new uiAttrVolOut( parent(), *dset, multiattrib, \
					   getNLAModel(is2d), nlaid ); \
	    mAttachCB(dlgobj->needSaveNLA,uiAttribPartServer::needSaveNLAps); \
	} \
	else \
	    dlgobj->updateAttributes(*dset,getNLAModel(is2d),nlaid ); \
	dlgobj->show(); \
    }


void uiAttribPartServer::updateMultiIdFromNLA( uiAttrVolOut* dlgobj,
		const MultiID& nlaid, bool is2d, const Attrib::DescSet* dset )
{
    if ( dlgobj )
	{
	    dlgobj->updateAttributes(*dset,getNLAModel(is2d),nlaid );
	}
}


void uiAttribPartServer::needSaveNLAps( CallBacker* )
{
    needSaveNLA.trigger();
}


void uiAttribPartServer::updateNLAInput( const MultiID& nlaid, bool is2d )
{
    const DescSet* dset = DSHolder().getDescSet( is2d, false );
    if ( !dset ) { pErrMsg("No attr set"); return; }

    if ( is2d )
	updateMultiIdFromNLA(dataattrdlg_, nlaid, is2d, dset);
    else
	updateMultiIdFromNLA(volattrdlg_, nlaid, is2d, dset);
}


void uiAttribPartServer::outputVol( const MultiID& nlaid, bool is2d,
				    bool multiattrib )
{
    const DescSet* dset = DSHolder().getDescSet( is2d, false );
    if ( !dset ) { pErrMsg("No attr set"); return; }

    if ( is2d )
	mAttrProcDlg(dataattrdlg_)
    else
    {
	if ( multiattrib )
	    mAttrProcDlg(multiattrdlg_)
	else
	    mAttrProcDlg(volattrdlg_)
    }
}


void uiAttribPartServer::setTargetSelSpec( const SelSpec& selspec )
{
    targetspecs_.erase();
    targetspecs_ += selspec;
}


Attrib::DescID uiAttribPartServer::targetID( bool for2d, int nr ) const
{
    return targetspecs_.size() <= nr ? DescID::undef()
				     : targetspecs_[nr].id();
}


EngineMan* uiAttribPartServer::createEngMan( const TrcKeyZSampling* tkzs,
					     const Pos::GeomID& geomid )
{
    if ( targetspecs_.isEmpty() ||
	 targetspecs_[0].id().asInt() == SelSpec::cNoAttrib().asInt())
	{ pErrMsg("Nothing to do"); return nullptr; }

    const bool istargetstored = targetspecs_[0].isStored();
    const bool is2d = targetspecs_[0].is2D();
    DescSet* curdescset = eDSHolder().getDescSet(is2d,istargetstored);
    if ( !curdescset )
	{ pErrMsg("No attr set"); return nullptr; }

    if ( !istargetstored )
    {
	DescID attribid = targetspecs_[0].id();
	Desc* seldesc = curdescset->getDesc( attribid );
	if ( seldesc )
	{
	    DescID multoiid = seldesc->getMultiOutputInputID();
	    if ( multoiid != DescID::undef() )
	    {
		const DescSetMan* adsman = DSHolder().getDescSetMan( is2d );
		uiAttrSelData attrdata( *adsman->descSet() );
		SelInfo attrinf( &attrdata.attrSet(), attrdata.nlamodel_, is2d,
				 DescID::undef(), false, false );
		if ( !uiMultOutSel::handleMultiCompChain( attribid, multoiid,
			    is2d, attrinf, curdescset, parent(), targetspecs_ ))
		    return nullptr;
	    }
	}
    }

    EngineMan* aem = new EngineMan;
    aem->setAttribSet( curdescset );
    aem->setNLAModel( getNLAModel(is2d) );
    aem->setAttribSpecs( targetspecs_ );
    if ( tkzs )
	aem->setTrcKeyZSampling( *tkzs );

    if ( geomid.isValid() )
	aem->setGeomID( geomid );

    return aem;
}


DataPackID uiAttribPartServer::createOutput( const TrcKeyZSampling& tkzs,
					       DataPackID cacheid )
{
    if ( tkzs.is2D() )
    {
	uiTaskRunner taskrunner( parent() );
	const Pos::GeomID geomid = tkzs.hsamp_.getGeomID();
	return create2DOutput( tkzs, geomid, taskrunner );
    }

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    ConstRefMan<RegularSeisDataPack> cache =
					dpm.get<RegularSeisDataPack>( cacheid );
    ConstRefMan<RegularSeisDataPack> newpack = createOutput(tkzs, cache.ptr() );
    if ( !newpack || !dpm.add(newpack) )
	return DataPack::cNoID();

    newpack->ref();
    return newpack->id();
}


static const Desc* getTargetDesc( const TypeSet<Attrib::SelSpec>& targetspecs )
{
    if ( targetspecs.isEmpty() )
	return nullptr;

    const bool isstortarget = targetspecs[0].isStored();
    const bool is2d = targetspecs[0].is2D();
    const DescSet* attrds = DSHolder().getDescSet( is2d, isstortarget );
    const Desc* targetdesc = !attrds || attrds->isEmpty() ? nullptr
				: attrds->getDesc( targetspecs[0].id() );
    return targetdesc;
}


ConstRefMan<RegularSeisDataPack> uiAttribPartServer::createOutput(
					    const TrcKeyZSampling& tkzs,
					    const RegularSeisDataPack* cache )
{
    return createOutputRM( tkzs, cache );
}


RefMan<RegularSeisDataPack> uiAttribPartServer::createOutputRM(
					    const TrcKeyZSampling& tkzs,
					    const RegularSeisDataPack* cache )
{
    uiMsgMainWinSetter mwsetter( sCast(uiMainWin*,parent()) );

    PtrMan<EngineMan> aem = createEngMan( &tkzs, Pos::GeomID::udf() );
    if ( !aem || targetspecs_.isEmpty() )
	return nullptr;

    const bool isnla = targetspecs_[0].isNLA();
    bool atsamplepos = true;

    bool showzprogress = true;
    Settings::common().getYN( SettingsAccess::sKeyShowZProgress(),
			      showzprogress );

    const Desc* targetdesc = getTargetDesc( targetspecs_ );
    ConstRefMan<RegularSeisDataPack> preloadeddatapack;
    const bool isz = tkzs.isFlat()&&tkzs.defaultDir() == TrcKeyZSampling::Z;
    if ( targetdesc )
    {
	if ( targetdesc->isStored() && !isnla )
	{
	    const MultiID mid( targetdesc->getStoredID().buf() );
	    preloadeddatapack = Seis::PLDM().get<RegularSeisDataPack>( mid );
	}

	BufferString defstr;
	targetdesc->getDefStr( defstr );
	if ( defstr != targetspecs_[0].defString() )
	    cache = nullptr;

	if ( !preloadeddatapack && isz )
	{
	    if ( targetdesc->isStored() )
	    {
		const MultiID mid( targetdesc->getStoredID().buf() );
		RefMan<RegularSeisDataPack> sdp = new RegularSeisDataPack(
				SeisDataPack::categoryStr(false,false) );
		PtrMan<IOObj> ioobj = IOM().get( mid );
		if ( !ioobj )
		    return nullptr;

		Seis::SequentialReader rdr( *ioobj, &tkzs );
		rdr.setDataPack( *sdp );
		uiTaskRunner uitaskr( parent() );
		TaskRunner* taskr = showzprogress ? &uitaskr : nullptr;
		if ( TaskRunner::execute(taskr, rdr) )
		    return sdp;
	    }

	    uiString errmsg;
	    Desc* nonconsttargetdesc = const_cast<Desc*>( targetdesc );
	    RefMan<Provider> tmpprov =
			Provider::create( *nonconsttargetdesc, errmsg );
	    if ( !tmpprov )
		return nullptr;

	    tmpprov->computeRefStep();
	    tmpprov->computeRefZ0();
	    const float floatres = (tkzs.zsamp_.start - tmpprov->getRefZ0()) /
				    tmpprov->getRefStep();
	    const int intres = mNINT32( floatres );
	    if ( Math::Abs(floatres-intres) > 1e-2 )
		atsamplepos = false;
	}
    }

    bool success = true;
    PtrMan<Processor> process;
    RefMan<RegularSeisDataPack> output;
    //note: 1 attrib computed at a time
    if ( !preloadeddatapack && !atsamplepos )
    {
	if ( !targetdesc )
	    return nullptr;

	Pos::RangeProvider3D rgprov3d;
	rgprov3d.setSampling( tkzs );
	DataColDef* dtcd = new DataColDef( targetdesc->userRef() );
	ManagedObjectSet<DataColDef> dtcoldefset;
	dtcoldefset += dtcd;
	uiTaskRunner taskr( parent() );
	RefMan<DataPointSet> posvals = new DataPointSet( rgprov3d.is2D() );
	if ( !posvals->extractPositions(rgprov3d,dtcoldefset,nullptr,&taskr) )
	    return nullptr;

	const int firstcolidx = 0;

	uiString errmsg;
	process = aem->getTableOutExecutor( *posvals, errmsg, firstcolidx );
	if ( !process )
	{
	    uiMSG().error( errmsg );
	    return nullptr;
	}

	if ( !TaskRunner::execute(&taskr,*process) )
	    return nullptr;

	TypeSet<float> vals;
	posvals->bivSet().getColumn( posvals->nrFixedCols()+firstcolidx, vals,
				    true );
	if ( !vals.isEmpty() )
	{
	    ArrayValueSeries<float, float> avs( vals.arr(), false, vals.size());
	    output = new RegularSeisDataPack(
				SeisDataPack::categoryStr(false,false) );
	    output->setSampling( tkzs );
	    if ( !output->addComponent(targetspecs_[0].userRef()) ||
		    !output->data(0).getStorage() )
		output = nullptr;
	    else
	    {
		ValueSeries<float>* arr3dvs = output->data(0).getStorage();
		ValueSeriesGetAll<float> copier( avs, *arr3dvs, vals.size() );
		copier.execute();
	    }
	}
    }
    else
    {
	if ( preloadeddatapack )
	{
	    ObjectSet<const RegularSeisDataPack> cubeset;
	    cubeset += preloadeddatapack;
	    return aem->getDataPackOutput( cubeset );
	}

	if ( !isnla )
	{
	    TrcKeyZSampling posstkzs( tkzs );
	    if ( !targetdesc || !targetdesc->descSet() )
		return nullptr;

	    PtrMan<DescSet> targetdescset =
		targetdesc->descSet()->optimizeClone( targetdesc->id() );
	    if ( !targetdescset )
		return nullptr;

	    const bool haspossvol = aem->getPossibleVolume( *targetdescset,
					posstkzs, nullptr, targetdesc->id() );
	    if ( !haspossvol )
		return nullptr;
	}

	uiString errmsg;
	process = aem->createDataPackOutput( errmsg, cache );
	if ( !process )
	{
	    uiMSG().error(errmsg);
	    return nullptr;
	}

	bool displayerrmsg;
	Settings::common().getYN( sKeyUserSettingAttrErrMsg(), displayerrmsg );
	process->showDataAvailabilityErrors( !aem->hasCache() && displayerrmsg);

	bool showinlprogress = true;
	bool showcrlprogress = true;
	Settings::common().getYN( SettingsAccess::sKeyShowInlProgress(),
				  showinlprogress );
	Settings::common().getYN( SettingsAccess::sKeyShowCrlProgress(),
				  showcrlprogress );

	const bool isstored =
	    targetdesc && targetdesc->isStored() && !isnla;
	const bool isinl =
		    tkzs.isFlat() && tkzs.defaultDir() == TrcKeyZSampling::Inl;
	const bool iscrl =
		    tkzs.isFlat() && tkzs.defaultDir() == TrcKeyZSampling::Crl;
	const bool hideprogress = isstored &&
	    ( (isinl&&!showinlprogress) || (iscrl&&!showcrlprogress) );

	if ( aem->getNrOutputsToBeProcessed(*process) != 0 )
	{
	    if ( !hideprogress )
	    {
		uiTaskRunner taskrunner( parent() );
		taskrunner.displayMsgOnError( false );
		success = TaskRunner::execute( &taskrunner, *process );
	    }
	    else
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		if ( !process->execute() )
		{
		    // const uiString msg( process->uiMessage() );
		    // ToDo: use this message as a tree tooltip
		    return nullptr;
		}
	    }
	}

	output = aem->getDataPackOutput( *process );
    }

    if ( output && !success )
    {
	if ( !uiMSG().askGoOn(tr("Attribute loading/calculation aborted.\n"
	    "Do you want to use the partially loaded/computed data?"), true ) )
	    output = nullptr;
    }

    return output;
}


bool uiAttribPartServer::createOutput( DataPointSet& posvals, int firstcol,
				       bool showprogress )
{
    const Desc* targetdesc = getTargetDesc( targetspecs_ );
    if ( targetdesc && targetdesc->isStored() )
    {
	const MultiID mid( targetdesc->getStoredID().buf() );
	ConstRefMan<RegularSeisDataPack> sdp =
				Seis::PLDM().get<RegularSeisDataPack>( mid );
	if ( sdp )
	{
	    const TrcKeyZSampling& seistkzs = sdp->sampling();
	    TrcKeySampling dpstks;
	    dpstks.set( posvals.bivSet().inlRange(),
			posvals.bivSet().crlRange() );

	    bool usepldata = seistkzs.hsamp_.includes( dpstks );
	    if ( !usepldata )
	    {
		uiDialog dlg( parent(),
		    uiDialog::Setup(tr("Question"),mNoDlgTitle,mNoHelpKey) );
		uiString msg( tr("Pre-loaded data does not cover the "
				"full requested area.\n"
				"Please choose one of the following options:"));
		auto* lbl = new uiLabel( &dlg, msg );
		auto* grp = new uiButtonGroup( &dlg, "Options", OD::Vertical );
		grp->attach( alignedBelow, lbl );
		new uiCheckBox( grp, tr("Use pre-loaded data (fast)") );
		new uiCheckBox( grp, tr("Read data from disk (slow)") );
		grp->selectButton( 0 );
		dlg.showAlwaysOnTop();
		if ( !dlg.go() )
		    return false;

		usepldata = grp->selectedId() == 0;
	    }

	    if ( usepldata )
	    {
		const int comp = targetdesc->selectedOutput();
		DPSFromVolumeFiller filler( posvals, firstcol, *sdp, comp );
		filler.setSampling( &sdp->sampling() );
		if ( showprogress )
		{
		    uiTaskRunner uitr( parent() );
		    return TaskRunner::execute( &uitr, filler );
		}
		else
		    return filler.execute();
	    }
	}
    }

    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem )
	return false;

    uiString errmsg;
    PtrMan<Processor> process =
			aem->getTableOutExecutor( posvals, errmsg, firstcol );
    if ( !process )
    {
	uiMSG().error(errmsg);
	return false;
    }

    if ( showprogress )
    {
	uiTaskRunner taskrunner( parent() );
	if ( !TaskRunner::execute(&taskrunner,*process) )
	    return false;
    }
    else if ( !process->execute() )
	return false;

    posvals.setName( targetspecs_[0].userRef() );
    return true;
}


class DPSOutputCreator : public ParallelTask
{
public:
DPSOutputCreator( ObjectSet<DataPointSet>& dpss, int firstcol,
		  uiAttribPartServer& aps )
: dpss_(dpss)
, aps_(aps)
, firstcol_(firstcol)
, nriter_(dpss.size())
, totalnr_(dpss.size())
{}

od_int64 nrIterations() const override		{ return nriter_; }
od_int64 totalNr() const override		{ return totalnr_; }

bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    bool res = true;
    for ( od_int64 idx=start; idx<=stop; idx++, addToNrDone(1) )
	res = res && aps_.createOutput( *dpss_[idx], firstcol_, false );
    return res;
}

protected:
    ObjectSet<DataPointSet>&	dpss_;
    uiAttribPartServer&		aps_;
    int				firstcol_;
    od_int64			nriter_;
    od_int64			totalnr_;
};


bool uiAttribPartServer::createOutput( ObjectSet<DataPointSet>& dpss,
				       int firstcol )
{
     DPSOutputCreator dpsmaker( dpss, firstcol, *this );
     uiTaskRunner taskrunner( parent() );
     return TaskRunner::execute( &taskrunner, dpsmaker );
}


RefMan<RandomSeisDataPack> uiAttribPartServer::createRdmTrcsOutputRM(
				const Interval<float>& zrg, RandomLineID rdlid )
{
    RefMan<Geometry::RandomLine> rdmline = Geometry::RLM().get( rdlid );
    if ( !rdmline )
	return nullptr;

    const bool isstortarget = targetspecs_.size() && targetspecs_[0].isStored();
    const DescSet* attrds = DSHolder().getDescSet(false,isstortarget);
    const Desc* targetdesc = !attrds || attrds->isEmpty()
			   ? nullptr
			   : attrds->getDesc(targetspecs_[0].id());

    if ( targetdesc )
    {
	const MultiID mid( targetdesc->getStoredID().buf() );
	ConstRefMan<RegularSeisDataPack> sdp =
			Seis::PLDM().get<RegularSeisDataPack>( mid );
	if ( sdp )
	{
	    BufferStringSet componentnames;
	    for ( int idx=0; idx<targetspecs_.size(); idx++ )
		componentnames.add( targetspecs_[idx].userRef() );

	    return RandomSeisDataPack::createDataPackFromRM( *sdp, rdlid, zrg,
							     &componentnames );
	}
    }

    TrcKeyPath knots, trckeys;
    rdmline->allNodePositions( knots );
    rdmline->getPathBids( knots, trckeys );

    if ( trckeys.isEmpty() )
	return nullptr;

    snapToValidRandomTraces( trckeys, targetdesc );

    BinIDValueSet bidset( 2, false );
    for ( const auto& tk : trckeys )
	bidset.add( tk.position(), zrg.start, zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput(bidset,output,knots,trckeys) || output.isEmpty() )
	return nullptr;

    RefMan<RandomSeisDataPack> newpack =
		new RandomSeisDataPack( SeisDataPack::categoryStr(true,false) );
    if ( !newpack )
	return nullptr;

    newpack->setRandomLineID( rdlid );
    newpack->setPath( trckeys );
    newpack->setZRange( output.get(0)->zRange() );
    for ( int idx=0; idx<output.get(0)->nrComponents(); idx++ )
    {
	if ( !newpack->addComponent(targetspecs_[idx].userRef()) )
	    continue;

	for ( int idy=0; idy<newpack->data(idx).info().getSize(1); idy++ )
	{
	    const int trcidx = output.find( trckeys[idy].position() );
	    const SeisTrc* trc = trcidx<0 ? nullptr : output.get( trcidx );
	    if ( !trc ) continue;
	    for ( int idz=0; idz<newpack->data(idx).info().getSize(2);idz++)
		newpack->data(idx).set( 0, idy, idz, trc->get(idz,idx) );
	}
    }

    const Attrib::SelSpec& targetspec = targetspecs_.first();
    const ZDomain::Def& zddef = ZDomain::Def::get( targetspec.zDomainKey() );
    const ZDomain::Info zdomain( zddef, targetspec.zDomainUnit() );
    newpack->setZDomain( zdomain );
    newpack->setName( targetspec.userRef() );
    return newpack;
}


DataPackID uiAttribPartServer::createRdmTrcsOutput(const Interval<float>& zrg,
						      RandomLineID rdlid )
{
    RefMan<Geometry::RandomLine> rdmline = Geometry::RLM().get( rdlid );
    if ( !rdmline )
	return DataPack::cNoID();

    const bool isstortarget = targetspecs_.size() && targetspecs_[0].isStored();
    const DescSet* attrds = DSHolder().getDescSet(false,isstortarget);
    const Desc* targetdesc = !attrds || attrds->isEmpty() ? 0
	: attrds->getDesc(targetspecs_[0].id());

    if ( targetdesc )
    {
	const MultiID mid( targetdesc->getStoredID().buf() );
	ConstRefMan<RegularSeisDataPack> sdp =
			Seis::PLDM().get<RegularSeisDataPack>( mid );
	if ( sdp )
	{
	    BufferStringSet componentnames;
	    for ( int idx=0; idx<targetspecs_.size(); idx++ )
		componentnames.add( targetspecs_[idx].userRef() );

	    return RandomSeisDataPack::createDataPackFrom( *sdp, rdlid, zrg,
							   &componentnames );
	}
    }

    TrcKeyPath knots, trckeys;
    rdmline->allNodePositions( knots );
    rdmline->getPathBids( knots, trckeys );

    if ( trckeys.isEmpty() )
	return DataPack::cNoID();

    snapToValidRandomTraces( trckeys, targetdesc );

    BinIDValueSet bidset( 2, false );
    for ( const auto& tk : trckeys )
	bidset.add( tk.position(), zrg.start, zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput(bidset,output,knots,trckeys) || output.isEmpty() )
	return DataPack::cNoID();

    RefMan<RandomSeisDataPack> newpack =
		new RandomSeisDataPack( SeisDataPack::categoryStr(true,false) );
    if ( !newpack || !DPM(DataPackMgr::SeisID()).add(newpack) )
	return DataPack::cNoID();

    newpack->setRandomLineID( rdlid );
    newpack->setPath( trckeys );
    newpack->setZRange( output.get(0)->zRange() );
    for ( int idx=0; idx<output.get(0)->nrComponents(); idx++ )
    {
	if ( !newpack->addComponent(targetspecs_[idx].userRef()) )
	    continue;

	for ( int idy=0; idy<newpack->data(idx).info().getSize(1); idy++ )
	{
	    const int trcidx = output.find( trckeys[idy].position() );
	    const SeisTrc* trc = trcidx<0 ? nullptr : output.get( trcidx );
	    if ( !trc ) continue;
	    for ( int idz=0; idz<newpack->data(idx).info().getSize(2);idz++)
		newpack->data(idx).set( 0, idy, idz, trc->get(idz,idx) );
	}
    }

    const Attrib::SelSpec& targetspec = targetspecs_.first();
    const ZDomain::Def& zddef = ZDomain::Def::get( targetspec.zDomainKey() );
    const ZDomain::Info zdomain( zddef, targetspec.zDomainUnit() );
    newpack->setZDomain( zdomain );
    newpack->setName( targetspec.userRef() );
    newpack->ref();
    return newpack->id();
}


void uiAttribPartServer::snapToValidRandomTraces( TrcKeyPath& path,
						  const Desc* targetdesc )
{
    if ( !targetdesc )
	return;

    uiString errmsg;
    Desc* nonconsttargetdesc = const_cast<Desc*>( targetdesc );
    RefMan<Provider> tmpprov = Provider::create( *nonconsttargetdesc, errmsg );

    TrcKeyZSampling tkzs( true );
    if ( !tmpprov || !tmpprov->getPossibleVolume(-1,tkzs) )
	return;

    if ( tkzs.hsamp_.step_.lineNr()==1 && tkzs.hsamp_.step_.trcNr()==1 )
	return;

    for ( auto& tk : path )
    {
	const TrcKey::IdxType linenr = const_cast<const TrcKey&>(tk).lineNr();
	const TrcKey::IdxType trcnr = const_cast<const TrcKey&>(tk).trcNr();
	if ( tkzs.hsamp_.lineRange().includes(linenr,true) &&
	     tkzs.hsamp_.trcRange().includes(trcnr,true) )
	{
	    const int shiftedtogetnearestinl = linenr +
					       tkzs.hsamp_.step_.lineNr()/2;
	    const int inlidx = tkzs.hsamp_.lineIdx( shiftedtogetnearestinl );
	    const int shiftedtogetnearestcrl = trcnr +
					       tkzs.hsamp_.step_.trcNr()/2;
	    const int crlidx = tkzs.hsamp_.trcIdx( shiftedtogetnearestcrl );
	    tk.setPosition( tkzs.hsamp_.atIndex( inlidx, crlidx ) );
	}
    }
}


DataPackID uiAttribPartServer::createRdmTrcsOutput(const Interval<float>& zrg,
			    TypeSet<BinID>& path, TypeSet<BinID>& trueknotspos )
{
    TrcKeyPath tkpath, tktrueknotspos;
    for ( const auto& bid : path )
	tkpath += TrcKey( bid );
    for ( const auto& bid : trueknotspos )
	tktrueknotspos += TrcKey( bid );

    return createRdmTrcsOutput( zrg, tkpath, tktrueknotspos );
}


DataPackID uiAttribPartServer::createRdmTrcsOutput(const Interval<float>& zrg,
				TrcKeyPath& trckeys, TrcKeyPath& trueknotspos )
{
    const bool isstortarget = targetspecs_.size() && targetspecs_[0].isStored();
    const DescSet* attrds = DSHolder().getDescSet(false,isstortarget);
    const Desc* targetdesc = !attrds || attrds->isEmpty() ? nullptr
			   : attrds->getDesc(targetspecs_[0].id());

    const MultiID mid( targetdesc->getStoredID().buf() );
    ConstRefMan<RegularSeisDataPack> sdp =
				Seis::PLDM().get<RegularSeisDataPack>( mid );
    if ( sdp )
    {
	BufferStringSet componentnames;
	for ( int idx=0; idx<targetspecs_.size(); idx++ )
	    componentnames.add( targetspecs_[idx].userRef() );

	return RandomSeisDataPack::createDataPackFrom( *sdp, trckeys, zrg,
						       &componentnames );
    }

    BinIDValueSet bidset( 2, false );
    for ( const auto& tk : trckeys )
	bidset.add( tk.position(), zrg.start, zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput(bidset,output,trueknotspos,trckeys) )
	return DataPack::cNoID();

    RefMan<RandomSeisDataPack> newpack = new RandomSeisDataPack(
				SeisDataPack::categoryStr(true,false) );
    if ( !newpack || !DPM(DataPackMgr::SeisID()).add(newpack) )
	return DataPack::cNoID();

    newpack->setPath( trckeys );
    newpack->setZRange( output.get(0)->zRange() );
    for ( int idx=0; idx<output.get(0)->nrComponents(); idx++ )
    {
	if ( !newpack->addComponent(targetspecs_[idx].userRef()) )
	    continue;

	for ( int idy=0; idy<newpack->data(idx).info().getSize(1); idy++ )
	{
	    const int trcidx = output.find( trckeys[idy].position() );
	    const SeisTrc* trc = trcidx<0 ? nullptr : output.get( trcidx );
	    if ( !trc ) continue;
	    for ( int idz=0; idz<newpack->data(idx).info().getSize(2);idz++)
		newpack->data(idx).set( 0, idy, idz, trc->get(idz,idx) );
	}
    }

    const Attrib::SelSpec& targetspec = targetspecs_.first();
    const ZDomain::Def& zddef = ZDomain::Def::get( targetspec.zDomainKey() );
    const ZDomain::Info zdomain( zddef, targetspec.zDomainUnit() );
    newpack->setZDomain( zdomain );
    newpack->setName( targetspec.userRef() );
    newpack->ref();
    return newpack->id();
}


bool uiAttribPartServer::createOutput( const BinIDValueSet& bidset,
				       SeisTrcBuf& output,
				       const TrcKeyPath& tktrueknotspos,
				       const TrcKeyPath& tksnappedpos )
{
    TypeSet<BinID> trueknotspos, snappedpos;
    for ( const auto& tk : tktrueknotspos )
	trueknotspos += tk.position();
    for ( const auto& tk : tksnappedpos )
	snappedpos += tk.position();

    return createOutput( bidset, output, trueknotspos, snappedpos );
}


bool uiAttribPartServer::createOutput( const BinIDValueSet& bidset,
				       SeisTrcBuf& output,
				       const TypeSet<BinID>& trueknotspos,
				       const TypeSet<BinID>& snappedpos )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem )
	return false;

    uiString errmsg;
    PtrMan<Processor> process =
	aem->createTrcSelOutput( errmsg, bidset, output, mUdf(float), nullptr,
				 &trueknotspos, &snappedpos );
    if ( !process )
    {
	uiMSG().error(errmsg);
	return false;
    }

    bool showprogress = true;
    Settings::common().getYN( SettingsAccess::sKeyShowRdlProgress(),
			      showprogress );

    const bool isstored = targetspecs_.size() && targetspecs_[0].isStored();
    if ( !isstored || showprogress )
    {
	uiTaskRunner taskrunner( parent() );
	return TaskRunner::execute( &taskrunner, *process );
    }

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    if ( !process->execute() )
    {
	const uiString msg( process->uiMessage() );
	if ( !msg.isEmpty() )
	    uiMSG().error( msg );
	return false;
    }

    return true;
}


class RegularSeisDataPackCreatorFor2D : public ParallelTask
{
public:
RegularSeisDataPackCreatorFor2D( const Data2DHolder& input,
				 Pos::GeomID geomid,
				 const ZDomain::Def& zdef,
				 const BufferStringSet* compnames )
    : input_(input)
    , sampling_(input.getTrcKeyZSampling())
    , zdef_(zdef)
    , refnrs_(sampling_.hsamp_.nrTrcs(),mUdf(float))
{
    sampling_.hsamp_.setGeomID( geomid );
    if ( compnames )
	compnames_ = *compnames;
}


RefMan<RegularSeisDataPack> getOutputDataPack() const
{
    return outputdp_;
}


od_int64 nrIterations() const override	{ return input_.trcinfoset_.size(); }

bool doPrepare( int nrthreads ) override
{
    if ( input_.trcinfoset_.isEmpty() || !sampling_.is2D() )
	return false;

    outputdp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,true) );
    outputdp_->setSampling( sampling_ );
    for ( int idx=0; idx<input_.dataset_[0]->validSeriesIdx().size(); idx++ )
    {
	const char* compname = compnames_.validIdx(idx) ?
		compnames_[idx]->buf() : sKey::EmptyString().buf();
	if ( !outputdp_->addComponent(compname) )
	    continue;
    }

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int threadid ) override
{
    if ( !outputdp_ ) return false;
    const StepInterval<float>& zrg = sampling_.zsamp_;
    const int nrzsamp = zrg.nrSteps()+1;
    const TypeSet<int> valididxs = input_.dataset_[0]->validSeriesIdx();

    for ( int idx=0; idx<outputdp_->nrComponents(); idx++ )
    {
	Array3DImpl<float>& data = outputdp_->data( idx );
	// trcinfoset.size() and sampling.hsamp_.nrTrcs() might differ if the
	// data has missing or duplicate trace numbers.
	const ObjectSet<SeisTrcInfo>& trcinfoset = input_.trcinfoset_;
	for ( int tidx=sCast(int,start); tidx<=sCast(int,stop); tidx++ )
	{
	    const int trcidx =
			sampling_.hsamp_.trcIdx( trcinfoset[tidx]->trcNr() );
	    if ( trcidx<0 || trcidx>sampling_.hsamp_.nrTrcs()-1 )
		continue;

	    const ValueSeries<float>* stor =
		input_.dataset_[tidx]->series( valididxs[idx] );
	    if ( !stor ) continue;

	    const bool domemcopy = stor->arr() && data.getData();
	    if ( domemcopy )
		OD::sysMemCopy( outputdp_->getTrcData(idx,trcidx),
			     stor->arr(), nrzsamp*sizeof(float) );
	    else
	    {
		OffsetValueSeries<float> trcstor =
			outputdp_->getTrcStorage( idx, trcidx );
		for ( int zidx=0; zidx<nrzsamp; zidx++ )
		    trcstor.setValue( zidx, stor->value(zidx) );
	    }

	    if ( idx == 0 )
		refnrs_[trcidx] = trcinfoset[tidx]->refnr;
	}
    }

    return true;
}

bool doFinish( bool success ) override
{
    outputdp_->setRefNrs( refnrs_ );
    outputdp_->setZDomain( zdef_ );
    if ( !compnames_.isEmpty() )
	outputdp_->setName( compnames_[0]->buf() );
    return true;
}

protected:

    const Data2DHolder&			input_;
    TrcKeyZSampling			sampling_;
    const ZDomain::Def&			zdef_;
    BufferStringSet			compnames_;
    RefMan<RegularSeisDataPack>		outputdp_;
    TypeSet<float>			refnrs_;
};

DataPackID uiAttribPartServer::create2DOutput( const TrcKeyZSampling& tkzs,
						 const Pos::GeomID& geomid,
						 TaskRunner& taskrunner )
{
    const bool isstored = targetspecs_[0].isStored();
    const DescSet* curds = DSHolder().getDescSet( true, isstored );
    if ( curds )
    {
	const Desc* targetdesc = curds->getDesc( targetID(true) );
	if ( targetdesc )
	{
	    const MultiID mid( targetdesc->getStoredID().buf() );
	    ConstRefMan<RegularSeisDataPack> regsdp =
			Seis::PLDM().get<RegularSeisDataPack>( mid, geomid );
	    if ( regsdp )
		return regsdp->id();
	}
    }

    PtrMan<EngineMan> aem = createEngMan( &tkzs, geomid );
    if ( !aem )
	return DataPack::cNoID();

    uiString errmsg;
    RefMan<Data2DHolder> data2d = new Data2DHolder;
    PtrMan<Processor> process = aem->createScreenOutput2D( errmsg, *data2d );
    if ( !process )
	{ uiMSG().error(errmsg); return DataPack::cNoID(); }

    if ( !TaskRunner::execute( &taskrunner, *process ) )
	return DataPack::cNoID();

    BufferStringSet userrefs;
    for ( int idx=0; idx<targetspecs_.size(); idx++ )
	userrefs.add( targetspecs_[idx].userRef() );

    auto dp = createDataPackFor2DRM( *data2d, tkzs,
	    ZDomain::Def::get(targetspecs_[0].zDomainKey()), &userrefs );
    if ( !dp )
	return DataPack::cNoID();

    DPM(DataPackMgr::SeisID()).add( dp );
    dp->ref();
    return dp->id();
}


RefMan<RegularSeisDataPack> uiAttribPartServer::createDataPackFor2DRM(
					const Attrib::Data2DHolder& input,
					const TrcKeyZSampling& outputsampling,
					const ZDomain::Def& zdef,
					const BufferStringSet* compnames )
{
    RegularSeisDataPackCreatorFor2D datapackcreator(
		input, outputsampling.hsamp_.getGeomID(), zdef,
		compnames );
    datapackcreator.execute();
    return datapackcreator.getOutputDataPack();
}


DataPackID uiAttribPartServer::createDataPackFor2D(
					const Attrib::Data2DHolder& input,
					const TrcKeyZSampling& outputsampling,
					const ZDomain::Def& zdef,
					const BufferStringSet* compnames )
{
    auto dp = createDataPackFor2DRM( input, outputsampling, zdef, compnames );
    if ( !dp )
	return DataPack::cNoID();

    DPM(DataPackMgr::SeisID()).add( dp );
    dp->ref();
    return dp->id();
}


bool uiAttribPartServer::extractData( ObjectSet<DataPointSet>& dpss )
{
    if ( dpss.isEmpty() )
    {
	pErrMsg("No inp data");
	return false;
    }

    const DescSet* ads = DSHolder().getDescSet( dpss[0]->is2D(), false );
    if ( !ads )
    {
	pErrMsg("No attr set");
	return false;
    }

    EngineMan aem;
    uiTaskRunner taskrunner( parent() );
    bool somesuccess = false;
    bool somefail = false;

    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	uiString err;
	DataPointSet& dps = *dpss[idx];
	Executor* tabextr = aem.getTableExtractor( dps, *ads, err );
	if ( !tabextr ) { pErrMsg(err.getFullString()); return false; }

	if ( TaskRunner::execute( &taskrunner, *tabextr ) )
	    somesuccess = true;
	else
	    somefail = true;

	delete tabextr;
    }

    if ( somefail )
    {
	return somesuccess &&
	     uiMSG().askGoOn(
	      tr("Some data extraction failed.\n\nDo you want to continue and "
		  "use the (partially) extracted data?"), true);
    }

    return true;
}


Attrib::DescID uiAttribPartServer::getStoredID( const MultiID& multiid,
						bool is2d, int selout ) const
{
    DescSet* ds = eDSHolder().getDescSet( is2d, true );
    return ds ? ds->getStoredID( multiid, selout, true ) : DescID::undef();
}


bool uiAttribPartServer::createAttributeSet( const BufferStringSet& inps,
					     DescSet* attrset )
{
    AttributeSetCreator attrsetcr( parent(), inps, attrset );
    return attrsetcr.create();
}


void uiAttribPartServer::importAttrSetFromFile()
{
    if ( !impattrsetdlg_ )
	impattrsetdlg_ = new uiImpAttrSet( parent() );

    impattrsetdlg_->show();
}


void uiAttribPartServer::importAttrSetFromOtherSurvey()
{
    uiMSG().message( tr("Not implemented yet") );
}


bool uiAttribPartServer::setPickSetDirs( Pick::Set& ps, const NLAModel* nlamod,
					 float velocity )
{
    //TODO: force 3D to avoid crash for 2D, need workaround for 2D later
    const DescSet* ds = DSHolder().getDescSet( false, false );
    if ( !ds )
	return false;

    uiSetPickDirs dlg( parent(), ps, ds, nlamod, velocity );
    return dlg.go();
}


static void insertItems( MenuItem& mnu, const BufferStringSet& nms,
	const TypeSet<MultiID>* ids, const char* cursel,
	int start, int stop, bool correcttype )
{
    const LineKey lk( cursel );
    const BufferString selnm = lk.lineName();

    mnu.removeItems();
    mnu.enabled = !nms.isEmpty();
    bool checkparent = false;
    for ( int idx=start; idx<stop; idx++ )
    {
	const BufferString& nm = nms.get( idx );
	auto* itm = new MenuItem( toUiString(nm) );
	itm->checkable = true;
	if ( ids && Seis::PLDM().isPresent(ids->get(idx)) )
	    itm->iconfnm = "preloaded";
	const bool docheck = correcttype && nm == selnm;
	if ( docheck ) checkparent = true;
	mAddMenuItem( &mnu, itm, true, docheck );
    }

    if ( checkparent )
	mnu.checked = true;
}


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
    const DescSet* nonstoredds = DSHolder().getDescSet( is2d, false );
    const Attrib::Desc* desc = nullptr;
    if ( ds && ds->getDesc(as.id()) )
	desc = ds->getDesc( as.id() );
    else if ( nonstoredds && nonstoredds->getDesc(as.id()) )
	desc = nonstoredds->getDesc( as.id() );
    const SelInfo attrinf( ds, nullptr, is2d, DescID::undef(),
			   issteer, issteer, multcomp );

    const bool isstored = desc ? desc->isStored() : false;
    const TypeSet<MultiID>& bfset =
		issteer ? attrinf.steerids_ : attrinf.ioobjids_;

    MenuItem* mnu = menu;
    if ( multcomp && needext )
    {
	MenuItem* submnu = is2d ? &multcomp2d_ : &multcomp3d_;
	mAddManagedMenuItem( menu, submnu, true, submnu->checked );
	mnu = submnu;
    }

    const int nritems = bfset.size();
    if ( nritems <= cMaxMenuSize )
    {
	const bool correcttype = desc ? isstored : true;
	const int start = 0;
	const int stop = nritems;
	if ( issteer )
	    insertItems( *mnu, attrinf.steernms_, &attrinf.steerids_,
			 as.userRef(), start, stop, correcttype );
	else
	    insertItems( *mnu, attrinf.ioobjnms_, &attrinf.ioobjids_,
			 as.userRef(), start, stop, correcttype );
    }

    menu->text = getMenuText( is2d, issteer, nritems>cMaxMenuSize );
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
	BufferString startnm = bfset.get(start);
	if ( startnm.size() > 3 ) startnm[3] = '\0';
	BufferString stopnm = bfset.get(stop-1);
	if ( stopnm.size() > 3 ) stopnm[3] = '\0';
	MenuItem* submnu = new MenuItem( toUiString("%1 - %2")
					 .arg(toUiString(startnm))
					 .arg(toUiString(stopnm)) );

	    SelInfo attrinf( DSHolder().getDescSet(false,true), nullptr, false,
			     DescID::undef() );
	    if ( issteer )
		insertItems( *submnu, attrinf.steernms_, &attrinf.steerids_,
			     as.userRef(), start, stop, correcttype );
	    else
		insertItems( *submnu, attrinf.ioobjnms_, &attrinf.ioobjids_,
			     as.userRef(), start, stop, correcttype );

	MenuItem* storedmnuitem = is2d ? issteer ? &steering2dmnuitem_
						 : &stored2dmnuitem_
				       : issteer ? &steering3dmnuitem_
						 : &stored3dmnuitem_;
	mAddManagedMenuItem( storedmnuitem, submnu, true,submnu->checked);
    }
}


MenuItem* uiAttribPartServer::calcAttribMenuItem( const SelSpec& as,
						  bool is2d, bool useext )
{
    SelInfo attrinf( DSHolder().getDescSet(is2d,false), nullptr, is2d );
    const bool isattrib = attrinf.attrids_.isPresent( as.id() );

    const int start = 0;
    const int stop = attrinf.attrnms_.size();
    MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    uiString txt = useext ? ( is2d ? tr("Attributes 2D")
				   : tr("Attributes 3D") )
			  : uiStrings::sAttribute(mPlural);
    calcmnuitem->text = txt;
    insertItems( *calcmnuitem, attrinf.attrnms_, nullptr, as.userRef(),
		 start, stop, isattrib );

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
	uiString ittxt;
	if ( !useext || is2d )
	    ittxt = toUiString( nlamodel->nlaType(false) );
	else
	    ittxt = tr("Neural Network 3D");
	if ( useext && is2d ) ittxt = toUiString("%1 %2").arg(ittxt)
							 .arg(uiStrings::s2D());

	nlamnuitem->text = ittxt;
	const DescSet* dset = DSHolder().getDescSet(is2d,false);
	SelInfo attrinf( dset, nlamodel, is2d );
	const bool isnla = as.isNLA();
	const int start = 0; const int stop = attrinf.nlaoutnms_.size();
	insertItems( *nlamnuitem, attrinf.nlaoutnms_, nullptr, as.userRef(),
		     start, stop, isnla );
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
    uiString itmtxt = toUiString("%1 %2").arg(toUiString(zdinf.key()))
			    .arg(useext ? (!is2d ? uiStrings::sCube(mPlural)
			    : mJoinUiStrs(s2D(),sLine(mPlural)))
			    : uiStrings::sData());
    zdomainmnuitem->text = itmtxt;
    zdomainmnuitem->removeItems();
    zdomainmnuitem->checkable = true;
    zdomainmnuitem->checked = false;

    BufferStringSet ioobjnms;
    SelInfo::getZDomainItems( zdinf, is2d, ioobjnms );
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const BufferString& nm = ioobjnms.get( idx );
	MenuItem* itm = new MenuItem( toUiString(nm) );
	const bool docheck = nm == as.userRef();
	mAddManagedMenuItem( zdomainmnuitem, itm, true, docheck );
	if ( docheck ) zdomainmnuitem->checked = true;
    }

    zdomainmnuitem->enabled = zdomainmnuitem->nrItems();
    return zdomainmnuitem;
}


void uiAttribPartServer::filter2DMenuItems(
	MenuItem& subitem, const Attrib::SelSpec& as, const Pos::GeomID& geomid,
	bool isstored, int steerpol )
{
    if ( geomid == Survey::GM().cUndefGeomID() )
	return;

    BufferStringSet childitemnms;
    for ( int idx=0; idx<subitem.nrItems(); idx++ )
	childitemnms.add( subitem.getItem(idx)->text.getFullString() );

    subitem.removeItems();
    StringView linenm( Survey::GM().getName(geomid) );
    BufferStringSet attribnms;
    uiSeisPartServer::get2DStoredAttribs( linenm, attribnms, steerpol );
    for ( int idx=0; idx<childitemnms.size(); idx++ )
    {
	StringView childnm( childitemnms.get(idx).buf() );
	if ( isstored )
	{
	    if ( attribnms.isPresent(childnm) )
	    {
		MenuItem* item = new MenuItem( mToUiStringTodo(childnm) );
		const bool docheck = childnm==as.userRef();
		mAddMenuItem(&subitem,item,true,docheck);
	    }
	}
	else
	{
	    const Attrib::DescSet* ds =
		Attrib::DSHolder().getDescSet( true, as.isStored() );
	    const Attrib::DescSet* activeds = ds;
	    const Attrib::DescSet* altds =
		Attrib::DSHolder().getDescSet( true, !as.isStored() );
	    int descidx = ds->indexOf( childnm );
	    if ( descidx<0 && altds )
	    {
		activeds = altds;
		descidx = altds->indexOf( childnm );
	    }

	    if ( descidx<0 )
		continue;

	    const Attrib::Desc* desc = activeds->desc( descidx );
	    if ( !desc )
		continue;

	    MultiID mid( desc->getStoredID(true).buf() );
	    PtrMan<IOObj> seisobj = IOM().get( mid );
	    if ( !seisobj || attribnms.isPresent(seisobj->name()) )
	    {
		MenuItem* item = new MenuItem( mToUiStringTodo(childnm) );
		const bool docheck = childnm==as.userRef();
		mAddMenuItem(&subitem,item,true,docheck);
	    }
	}
    }
}


bool uiAttribPartServer::handleAttribSubMenu( int mnuid, SelSpec& as,
					      bool& dousemulticomp )
{
    if ( stored3dmnuitem_.id == mnuid )
	return selectAttrib( as, nullptr, Survey::GM().cUndefGeomID(),
			    uiStrings::phrSelect( uiStrings::sAttribute()) );

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

    DescID attribid = SelSpec::cAttribNotSel();
    int outputnr = -1;
    bool isnla = false;
    bool isstored = false;
    MultiID multiid = MultiID::udf();

    if ( stored3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = stored3dmnuitem_.findItem(mnuid);
	const int idx = attrinf.ioobjnms_.indexOf(item->text.getFullString());
	multiid = attrinf.ioobjids_.get( idx );
	attribid =
	    eDSHolder().getDescSet(false,true)->getStoredID( multiid, -1, true);
	isstored = true;
    }
    else if ( steering3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = steering3dmnuitem_.findItem( mnuid );
	const int idx = attrinf.steernms_.indexOf( item->text.getFullString() );
	multiid = attrinf.steerids_.get( idx );
	attribid =
	    eDSHolder().getDescSet(false,true)->getStoredID( multiid, -1, true);
	isstored = true;
    }
    else if ( stored2dmnuitem_.findItem(mnuid) ||
	      steering2dmnuitem_.findItem(mnuid) )
    {
	    const MenuItem* item = stored2dmnuitem_.findItem(mnuid);
	    if ( !item ) item = steering2dmnuitem_.findItem(mnuid);
	if ( !item ) return false;

	const BufferString& itmnm = item->text.getFullString();
	const int idx = issteering ? attrinf.steernms_.indexOf( itmnm )
				   : attrinf.ioobjnms_.indexOf( itmnm );
	multiid = issteering ? attrinf.steerids_.get(idx)
			     : attrinf.ioobjids_.get(idx);
	const int selout = issteering ? 1 : -1;
	attribid = eDSHolder().getDescSet(true,true)->getStoredID( multiid,
								selout, true );
	isstored = true;
    }
    else if ( calcmnuitem->findItem(mnuid) )
    {
	const MenuItem* item = calcmnuitem->findItem(mnuid);
	int idx = attrinf.attrnms_.indexOf(item->text.getFullString());
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
	IOM().to( IOObjContext::Seis );
	ConstPtrMan<IOObj> ioobj =
			IOM().getLocal( item->text.getFullString(), nullptr );
	if ( ioobj )
	{
	    multiid = ioobj->key();
	    auto* ds = eDSHolder().getDescSet(false,true);
	    attribid = ds->getStoredID( multiid, -1, true );
	    isstored = true;
	}
    }
    else
	return false;

    const bool nocompsel = is2d && issteering;
    if ( isstored && !nocompsel )
    {
	BufferStringSet complist;
	SeisIOObjInfo::getCompNames( multiid, complist );
	if ( complist.size()>1 )
	{
	    TypeSet<int> selcomps;
	    if ( !handleMultiComp( multiid, is2d, issteering, complist,
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
	objref = ioobj ? ioobj->name().buf() : "";
    }

    DescID did = isnla ? DescID(outputnr,false) : attribid;
    as.set( nullptr, did, isnla, objref );

    BufferString bfs;
    if ( attribid.asInt() != SelSpec::cAttribNotSel().asInt() )
    {
	const DescSet* attrset = DSHolder().getDescSet(is2d, isstored);
	const Desc* desc = attrset ? attrset->getDesc( attribid ) : nullptr;
	if ( desc  )
	{
	    desc->getDefStr( bfs );
	    as.setDefString( bfs.buf() );
	}
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
	attbnm = item->text.getFullString();
    }
    else if ( calc2dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = calc2dmnuitem_.findItem( mnuid );
	attbnm = item->text.getFullString();
    }
}


#define mFakeCompName( searchfor, replaceby ) \
{ \
    LineKey lkey( desc->userRef() ); \
    if ( lkey.attrName() == searchfor ) \
	lkey.setAttrName( replaceby );\
    desc->setUserRef( lkey.buf() ); \
}

bool uiAttribPartServer::handleMultiComp( const MultiID& multiid, bool is2d,
					  bool issteering,
					  BufferStringSet& complist,
					  DescID& attribid,
					  TypeSet<int>& selectedcomps )
{
    //Trick for old steering cubes: fake good component names
    if ( !is2d && issteering && complist.isPresent("Component 1") )
    {
	complist.erase();
	complist.add( Desc::sKeyInlDipComp() );
	complist.add( Desc::sKeyCrlDipComp() );
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
	    attribid = ads->getStoredID( multiid, selectedcomps[0] );
	    //Trick for old steering cubes: fake good component names
	    if ( !is2d && issteering )
	    {
		Desc* desc = ads->getDesc(attribid);
		if ( !desc ) return false;
		mFakeCompName( "Component 1", Desc::sKeyInlDipComp() );
		mFakeCompName( "Component 2", Desc::sKeyCrlDipComp() );
	    }

	    return true;
	}
	prepMultCompSpecs( selectedcomps, multiid, is2d, issteering );
    }
    else
	return false;

    return true;
}


bool uiAttribPartServer::prepMultCompSpecs( TypeSet<int> selectedcomps,
					    const MultiID& multiid, bool is2d,
					    bool issteering )
{
    targetspecs_.erase();
    DescSet* ads = eDSHolder().getDescSet( is2d, true );
    for ( int idx=0; idx<selectedcomps.size(); idx++ )
    {
	DescID did = ads->getStoredID( multiid, selectedcomps[idx], true );
	SelSpec as( nullptr, did );
	BufferString bfs;
	Desc* desc = ads->getDesc(did);
	if ( !desc ) return false;

	desc->getDefStr(bfs);
	as.setDefString(bfs.buf());
	//Trick for old steering cubes: fake good component names
	if ( !is2d && issteering )
	{
	    mFakeCompName( "Component 1", Desc::sKeyInlDipComp() );
	    mFakeCompName( "Component 2", Desc::sKeyCrlDipComp() );
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


IOObj* uiAttribPartServer::getIOObj( const SelSpec& as ) const
{
    if ( as.isNLA() ) return nullptr;

    const DescSet* attrset = DSHolder().getDescSet( as.is2D(), true );
    const Desc* desc = attrset ? attrset->getDesc( as.id() ) : nullptr;
    if ( !desc )
    {
	attrset = DSHolder().getDescSet( as.is2D(), false );
	desc = attrset ? attrset->getDesc( as.id() ) : nullptr;
	if ( !desc )
	    return nullptr;
    }

    BufferString storedid = desc->getStoredID();
    if ( !desc->isStored() || storedid.isEmpty() ) return nullptr;

    return IOM().get( MultiID(storedid.buf()) );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiAttribPartServer::processEvalDlg( bool iscrossevaluate )
{
    if ( !attrsetdlg_ )
	return;

    const Desc* curdesc = attrsetdlg_->curDesc();
    if ( !curdesc )
	mErrRet( tr("Please add this attribute first") );

    uiAttrDescEd* ade = attrsetdlg_->curDescEd();
    if ( !ade )
	return;

    sendEvent( evEvalAttrInit() );
    //if ( !alloweval_ ) mErrRet( "Evaluation of attributes only possible on\n"
//			       "Inlines, Crosslines, Timeslices and Surfaces.");

    if ( !iscrossevaluate )
    {
	auto* evaldlg = new uiEvaluateDlg( attrsetdlg_, *ade, allowevalstor_ );
	if ( !evaldlg->evaluationPossible() )
	    mErrRet( tr("This attribute has no parameters to evaluate") );

	evaldlg->calccb.notify( mCB(this,uiAttribPartServer,calcEvalAttrs) );
	evaldlg->showslicecb.notify(
		mCB(this,uiAttribPartServer,showSliceCB) );
	evaldlg->windowClosed.notify(
		mCB(this,uiAttribPartServer,evalDlgClosed) );
	evaldlg->go();
    }
    else
    {
	auto* crossevaldlg = new uiCrossAttrEvaluateDlg(
				attrsetdlg_, *attrsetdlg_, allowevalstor_ );
	if ( !crossevaldlg->evaluationPossible() )
	    mErrRet( tr("This attribute has no parameters to evaluate") );

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


void uiAttribPartServer::showEvalDlg( CallBacker* )
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
	    const TypeSet<DescID>& cids = crossevaldlg->evaluateChildIds();
	    BufferString ds = crossevaldlg->acceptedDefStr();
	    DescSet* ads = attrsetdlg_->getSet();
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
    DescSet* ads = evaldlg ? evaldlg->getEvalSet()
			   : crossevaldlg->getEvalSet();
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
    if ( sel < 0 )
	return;

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


void uiAttribPartServer::fillPar( IOPar& iopar, bool is2d,
				  bool isstored ) const
{
    const DescSet* ads = DSHolder().getDescSet( is2d, isstored );
    if ( ads && !ads->isEmpty() )
	ads->fillPar( iopar );

    const MultiID mid = DSHolder().getDescSetMan( is2d )->attrsetid_;
    if ( IOM().isPresent(mid) )
	iopar.set( sKey::ID(), mid );
}


void uiAttribPartServer::usePar( const IOPar& iopar, bool is2d,
				 bool isstored, uiStringSet& errmsgs )
{
    DescSet* ads = eDSHolder().getDescSet( is2d, isstored );
    if ( !ads )
	return;

    const int odversion = iopar.odVersion();
    uiStringSet locmsg;
    if ( isstored && odversion<411 ) // backward compatibility v<4.1.1
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
	ads->usePar( iopar, &locmsg );

    if ( !locmsg.isEmpty() )
    {
	errmsgs.add( tr("Error during restore of %1 Attribute Set")
						 .arg(is2d ? uiStrings::s2D()
						   : uiStrings::s3D()) );
	errmsgs.add( locmsg );
    }

    MultiID mid;
    if ( iopar.get(sKey::ID(), mid) && IOM().isPresent(mid) )
	eDSHolder().getDescSetMan( is2d )->attrsetid_ = mid;

    set2DEvent( is2d );
    sendEvent( evNewAttrSet() );
}


void uiAttribPartServer::usePar( const IOPar& iopar, bool is2d,
				 bool isstored )
{
    uiStringSet errmsgs;
    usePar( iopar, is2d, isstored, errmsgs );

    if ( !errmsgs.isEmpty() )
	uiMSG().errorWithDetails( errmsgs );
}


void uiAttribPartServer::setEvalBackupColTabMapper(
			const ColTab::MapperSetup* mp )
{
    if ( evalmapperbackup_ && mp )
	*evalmapperbackup_ = *mp;
    else if ( !mp )
	deleteAndNullPtr( evalmapperbackup_ );
    else if ( mp )
	evalmapperbackup_ = new ColTab::MapperSetup( *mp );
}


const ColTab::MapperSetup*
	uiAttribPartServer::getEvalBackupColTabMapper() const
{
    return evalmapperbackup_;
}


void uiAttribPartServer::survChangedCB( CallBacker* )
{
    closeAndNullPtr( manattribset2ddlg_ );
    closeAndNullPtr( manattribsetdlg_ );
    closeAndNullPtr( attrsetdlg_ );
    closeAndNullPtr( impattrsetdlg_ );
    closeAndNullPtr( volattrdlg_);
    closeAndNullPtr( multiattrdlg_);
    closeAndNullPtr( dataattrdlg_);
}


void uiAttribPartServer::setSelAttr( const char* attrnm )
{
    if ( attrsetdlg_ )
	attrsetdlg_->setSelAttr( attrnm );
}


void uiAttribPartServer::loadDefaultAttrSet( const char* attribsetnm )
{
    if ( attrsetdlg_ )
	attrsetdlg_->loadDefaultAttrSet( attribsetnm );
}
