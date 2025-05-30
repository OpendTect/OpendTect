/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodapplmgr.h"

#include "uiactiverunningproc.h"
#include "uiattribpartserv.h"
#include "uidesktopservices.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uifiledlg.h"
#include "uihelpview.h"
#include "uimain.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodapplmgraux.h"
#include "uiodapplmgrattrvis.h"
#include "uiodemsurftreeitem.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodstratlayermodelmgr.h"
#include "uiodtreeitem.h"
#include "uiodviewer2dmgr.h"
#include "uiodviewer2dposdlg.h"
#include "uipickpartserv.h"
#include "uiseispartserv.h"
#include "uistereodlg.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uivisdatapointsetdisplaymgr.h"
#include "uivispartserv.h"
#include "uivolprocpartserv.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"
#include "uizaxistransform.h"

#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "vishorizon2ddisplay.h"
#include "vishorizondisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "visselman.h"

#include "attribdescset.h"
#include "bendpointfinder.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emtracker.h"
#include "externalattrib.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "mouseevent.h"
#include "od_helpids.h"
#include "oddirs.h"
#include "odsession.h"
#include "pickset.h"
#include "posvecdataset.h"
#include "randomlinegeom.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"


uiODApplMgr::uiODApplMgr( uiODMain& a )
    : uiApplMgr(a,*new uiODApplService(&a,*this))
    , attribSetChg(this)
    , getOtherFormatData(this)
    , appl_(a)
    , dispatcher_(*new uiODApplMgrDispatcher(*this,&appl_))
    , attrvishandler_(*new uiODApplMgrAttrVisHandler(*this,&appl_))
    , mousecursorexchange_( *new MouseCursorExchange )
{
    pickserv_ = new uiPickPartServer( applservice_ );
    visserv_ = new uiVisPartServer( applservice_ );
    visserv_->setMouseCursorExchange( &mousecursorexchange_ );
    attrserv_ = new uiAttribPartServer( applservice_ );
    volprocserv_ = new uiVolProcPartServer( applservice_ );

    seisserv_ = new uiSeisPartServer( applservice_ );
    emserv_ = new uiEMPartServer( applservice_ );
    emattrserv_ = new uiEMAttribPartServer( applservice_ );
    wellserv_ = new uiWellPartServer( applservice_ );
    wellattrserv_ = new uiWellAttribPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );

    visdpsdispmgr_ = new uiVisDataPointSetDisplayMgr( *visserv_ );
    visdpsdispmgr_->treeToBeAdded.notify( mCB(this,uiODApplMgr,addVisDPSChild));
    wellattrserv_->setDPSDispMgr( visdpsdispmgr_ );
    attrserv_->setDPSDispMgr( visdpsdispmgr_ );

    if ( survChgReqAttrUpdate() )
    {
	attrserv_->setAttrsNeedUpdt();
	tmpprevsurvinfo_.refresh();
    }
}


uiODApplMgr::~uiODApplMgr()
{
    visdpsdispmgr_->clearDisplays();
    dispatcher_.survChg(true);
    attrvishandler_.survChg(true);

    delete mpeserv_;
    delete pickserv_;
    delete nlaserv_;
    delete attrserv_;
    delete volprocserv_;
    delete seisserv_;
    delete visserv_;

    delete emserv_;
    delete emattrserv_;
    delete wellserv_;
    delete wellattrserv_;
    delete &dispatcher_;
    delete visdpsdispmgr_;

    delete &attrvishandler_;
    delete &mousecursorexchange_;
}


MouseCursorExchange& uiODApplMgr::mouseCursorExchange()
{ return mousecursorexchange_; }


void uiODApplMgr::resetServers()
{
    if ( nlaserv_ ) nlaserv_->reset();

    delete attrserv_;
    attrserv_ = new uiAttribPartServer( applservice_ );
    attrserv_->setDPSDispMgr( visdpsdispmgr_ );

    delete volprocserv_;
    volprocserv_ = new uiVolProcPartServer( applservice_ );

    delete mpeserv_;
    mpeserv_ = new uiMPEPartServer( applservice_ );

    delete emattrserv_;
    emattrserv_ = new uiEMAttribPartServer( applservice_ );

    visserv_->deleteAllObjects();
    emserv_->removeUndo();
}


void uiODApplMgr::setNlaServer( uiNLAPartServer* s )
{
    nlaserv_ = s;
    if ( nlaserv_ )
	nlaserv_->setDPSDispMgr( visdpsdispmgr_ );
}


int uiODApplMgr::selectSurvey( uiParent* p )
{
    const int res = manSurv( p );
    if ( res == 3 )
	setZStretch();
    return res;
}


int uiODApplMgr::editCurrentSurvey( uiParent* p )
{
    if ( !p )
	p = uiMain::instance().topLevel();

    uiSurveyInfoEditor dlg( p, eSI(), false, true );
    return dlg.go();
}


int uiODApplMgr::manSurv( uiParent* p )
{
    uiRetVal uirv;
    return uiSurvey::ensureValidSurveyDir( uirv, p );
}


void uiODApplMgr::exportSurveySetup()
{
    const FilePath outfnm( GetExportToDir(), "surveysetup.txt" );
    uiFileDialog dlg( &appl_, false, outfnm.fullPath() );
    if ( !dlg.go() )
	return;

    const char* fnm = dlg.fileName();
    const FilePath fp( GetDataDir(), SurveyInfo::sKeySetupFileName() );
    if ( !fp.exists() )
    {
	uiString msg( tr("Can not find Survey Setup file at:\n%1") );
	msg.arg( fp.fullPath() );
	uiMSG().error( msg );
	return;
    }

    File::copy( fp.fullPath(), fnm, false );
}


void uiODApplMgr::openSurveyFolder()
{
// Attribs folder is usually the top most folder when sorted alpabetically
    const auto* dirdata = IOObjContext::getStdDirData( IOObjContext::Attr );
    const FilePath fp( GetDataDir(), dirdata ? dirdata->dirnm_
					     : ".survey" );
    uiDesktopServices::showInFolder( fp.fullPath() );
}


void uiODApplMgr::addVisDPSChild( CallBacker* cb )
{
    mCBCapsuleUnpack( EM::ObjectID, emid, cb );
    TypeSet<SceneID> sceneids;
    visserv_->getSceneIds( sceneids );
    sceneMgr().addEMItem( emid, sceneids[0] );
}


void uiODApplMgr::prepSurveyChange()
{
    uiApplMgr::prepSurveyChange();
    bool anythingasked = false;
    if ( !appl_.askStore(anythingasked,tr("Survey change")) )
	IOM().setChangeSurveyBlocked( true );
}


void uiODApplMgr::survToBeChanged()
{
    visdpsdispmgr_->clearDisplays();
    dispatcher_.survChg(true);
    attrvishandler_.survChg(true);

    if ( nlaserv_ )
	nlaserv_->reset();

    deleteAndNullPtr( attrserv_ );
    deleteAndNullPtr( emattrserv_ );
    deleteAndNullPtr( volprocserv_ );
    deleteAndNullPtr( mpeserv_ );
    deleteAndNullPtr( wellserv_ );
    if ( appl_.sceneMgrAvailable() )
	sceneMgr().cleanUp( false );

    uiApplMgr::survToBeChanged();
}


void uiODApplMgr::survChanged()
{
    dispatcher_.survChg(false);
    attrvishandler_.survChg(false);
    bool douse = false;
    MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id.isUdf() )
	sceneMgr().addScene( true );

    attrserv_ = new uiAttribPartServer( applservice_ );
    attrserv_->setDPSDispMgr( visdpsdispmgr_ );
    if ( survChgReqAttrUpdate() )
    {
	attrserv_->setAttrsNeedUpdt();
	tmpprevsurvinfo_.refresh();
    }

    volprocserv_ = new uiVolProcPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );
    wellserv_ = new uiWellPartServer( applservice_ );
    emattrserv_ = new uiEMAttribPartServer( applservice_ );

    uiApplMgr::survChanged();
}


bool uiODApplMgr::survChgReqAttrUpdate()
{
    return !( SI().xyUnit() == tmpprevsurvinfo_.xyunit_ &&
		SI().zUnit( true ) == tmpprevsurvinfo_.zunit_ &&
		mIsEqual( SI().zStep(),tmpprevsurvinfo_.zstep_, 1e-6 ) );
}


void uiODApplMgr::doOperation( ObjType ot, ActType at, int opt )
{
    dispatcher_.doOperation( (int)ot, (int)at, opt );
}


void uiODApplMgr::manPreLoad( ObjType ot )
{
    dispatcher_.manPreLoad( (int)ot );
}


void uiODApplMgr::enableMenusAndToolBars( bool yn )
{
    sceneMgr().disabRightClick( !yn );
    visServer()->disabMenus( !yn );
    visServer()->disabToolBars( !yn );
    if ( appl_.menuMgrAvailable() )
    {
	appl_.menuMgr().dtectTB()->setSensitive( yn );
	appl_.menuMgr().manTB()->setSensitive( yn );
	appl_.menuMgr().enableMenuBar( yn );
    }
}


void uiODApplMgr::enableTree( bool yn )
{
    sceneMgr().disabTrees( !yn );
    visServer()->blockMouseSelection( !yn );
}


void uiODApplMgr::enableSceneManipulation( bool yn )
{
    if ( !yn ) sceneMgr().setToViewMode();
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().enableActButton( yn );
}


void uiODApplMgr::editAttribSet()
{ editAttribSet( SI().has2D() ); }

void uiODApplMgr::editAttribSet( bool is2d )
{ attrserv_->editSet( is2d ); }

void uiODApplMgr::processTime2DepthSeis( CallBacker* )
{ seisserv_->processTime2Depth( false ); }

void uiODApplMgr::processTime2DepthSeis( bool is2d )
{ seisserv_->processTime2Depth( is2d ); }

void uiODApplMgr::processVelConv( CallBacker* )
{ seisserv_->processVelConv(); }

void uiODApplMgr::createMultiCubeDS( CallBacker* )
{ seisserv_->createMultiCubeDataStore(); }

void uiODApplMgr::createMultiAttribVol( CallBacker* )
{ attrserv_->outputVol( MultiID::udf(), false, true ); }

void uiODApplMgr::setStereoOffset()
{
    ObjectSet<ui3DViewer> vwrs;
    sceneMgr().get3DViewers( vwrs );
    uiStereoDlg dlg( &appl_, vwrs );
    dlg.go();
}


void uiODApplMgr::processTime2DepthHor( bool is2d )
{
    emserv_->processTime2Depth( &appl_, is2d ? EM::ObjectType::Hor2D
					: EM::ObjectType::Hor3D );
}


void uiODApplMgr::processTime2DepthFSS( bool is2d )
{
    emserv_->processTime2Depth( &appl_, is2d ? EM::ObjectType::FltSS2D
	: EM::ObjectType::FltSS3D );
}


void uiODApplMgr::fltTimeDepthConvCB( CallBacker* )
{
    emserv_->processTime2Depth( &appl_, EM::ObjectType::Flt3D );
}


void uiODApplMgr::fltsetTimeDepthConvCB( CallBacker* )
{
    emserv_->processTime2Depth( &appl_, EM::ObjectType::FltSet );
}

void uiODApplMgr::addTimeDepthScene( bool is2d )
{
    uiDialog::Setup setup(tr("Velocity model"),
			  tr("Select velocity model to base scene on"),
			  mODHelpKey(mODApplMgraddTimeDepthSceneHelpID));

    uiSingleGroupDlg dlg( &appl_, setup );
    const OD::Pol2D3D poltype = is2d ? OD::Only2D : OD::Only3D;
    auto* uitrans = SI().zIsTime()
	? new uiZAxisTransformSel( &dlg, false, ZDomain::sKeyTime(),
				   ZDomain::sKeyDepth(), true, false, poltype )
	: new uiZAxisTransformSel( &dlg, false, ZDomain::sKeyDepth(),
				   ZDomain::sKeyTime(), true, false, poltype );

    if ( !uitrans->hasTransform() )
    {
	uiMSG().error(tr("No suitable transforms found"));
	return;
    }

    dlg.setGroup( uitrans );
    if ( !dlg.go() )
	return;

    RefMan<ZAxisTransform> ztrans = uitrans->getSelection();
    if ( !ztrans )
	return;

    ZSampling zsampling;
    if ( !uitrans->getTargetSampling(zsampling) )
    {
	pErrMsg( "Cannot get sampling.");
	return;
    }

    const uiString snm = tr( "%1 (using '%2')")
	    .arg( SI().zIsTime() ? sKey::Depth() : sKey::Time() )
	    .arg( ztrans->factoryDisplayName() );

    sceneMgr().tile();
    const SceneID sceneid = sceneMgr().addScene( true, ztrans.ptr(), snm );
    if ( !sceneid.isValid() )
	return;

    const float zscale = ztrans->zScale();
    RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneid );
    TrcKeyZSampling cs = SI().sampling( true );
    cs.zsamp_ = zsampling;
    scene->setTrcKeyZSampling( cs );
    scene->setZScale( zscale );
}


void uiODApplMgr::addHorFlatScene( bool is2d )
{
    RefMan<ZAxisTransform> transform = emserv_->getHorizonZAxisTransform(is2d);
    if ( !transform ) return;

    const MultiID hormid( transform->fromZDomainInfo().getID() );
    PtrMan<IOObj> ioobj = IOM().get( hormid );
    const BufferString hornm = ioobj
		? ioobj->name().buf()
		: transform->factoryDisplayName().getFullString();
    uiString scenenm = tr( "Flattened on '%1'").arg( hornm );
    sceneMgr().tile();
    sceneMgr().addScene( true, transform.ptr(), scenenm);
}


void uiODApplMgr::show2DViewer()
{
    uiODViewer2DPosDlg viewposdlg( appl_ );
    viewposdlg.go();
}


void uiODApplMgr::setWorkingArea()
{
    if ( visserv_->setWorkingArea() )
	sceneMgr().viewAll(0);
}


void uiODApplMgr::selectWells( TypeSet<MultiID>& wellids )
{ wellserv_->selectWells( wellids ); }

bool uiODApplMgr::storePickSets()
{ return pickserv_->storePickSets(); }

bool uiODApplMgr::storePickSet( const Pick::Set& ps )
{ return pickserv_->storePickSet( ps ); }

bool uiODApplMgr::storePickSetAs( const Pick::Set& ps )
{ return pickserv_->storePickSetAs( ps ); }

bool uiODApplMgr::setPickSetDirs( Pick::Set& ps )
{
    const SceneID sceneid = sceneMgr().askSelectScene();
    RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneid );
    const float velocity =
	scene ? scene->getFixedZStretch() * scene->getZScale() : 0;
    return attrserv_->setPickSetDirs( ps, nlaserv_ ? &nlaserv_->getModel() : 0,
				      velocity );
}

bool uiODApplMgr::pickSetsStored() const
{ return pickserv_->pickSetsStored(); }


bool uiODApplMgr::getNewData( const VisID& visid, int attrib )
{
    if ( !visid.isValid() )
	return false;

    const TypeSet<Attrib::SelSpec>* as = visserv_->getSelSpecs( visid, attrib );
    if ( !as || as->isEmpty() )
    {
	uiString msg( tr("Cannot calculate attribute on this object") );
	if ( isRestoringSession() )
	    appl_.restoreMsgs().add( msg );
	else
	    uiMSG().error( msg );

	return false;
    }

    TypeSet<Attrib::SelSpec> myas( *as );
    bool selspecchanged = false;
    for ( int idx=0; idx<myas.size(); idx++ )
    {
	if ( myas[idx].id() != Attrib::DescID::undef() &&
	     myas[idx].id() != Attrib::SelSpec::cOtherAttrib() )
	{
	    attrserv_->updateSelSpec( myas[idx] );
	    if ( (*as)[idx] != myas[idx] )
		selspecchanged = true;
	}

	if ( myas[idx].id().isUnselInvalid() )
	{
	    uiString msg( tr("Cannot find selected attribute: %1").
						    arg(myas[idx].userRef()) );
	    if ( isRestoringSession() )
		appl_.restoreMsgs().add( msg );
	    else
		uiMSG().error( msg );

	    return false;
	}
    }

    if ( selspecchanged )
	visserv_->setSelSpecs( visid, attrib, myas );

    ConstRefMan<VolumeDataPack> voldp =
				visserv_->getVolumeDataPack( visid, attrib );
    bool res = false;
    switch ( visserv_->getAttributeFormat(visid,attrib) )
    {
	case uiVisPartServer::Cube :
	{
	    mDynamicCastGet(const RegularSeisDataPack*,regsdp,voldp.ptr());
	    const TrcKeyZSampling tkzs =
				visserv_->getTrcKeyZSampling( visid, attrib );
	    if ( !tkzs.isDefined() )
		return false;

	    ConstRefMan<RegularSeisDataPack> newdp;
	    if ( myas[0].id().asInt()==Attrib::SelSpec::cOtherAttrib().asInt() )
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		PtrMan<Attrib::ExtAttribCalc> calc =
			Attrib::ExtAttrFact().create( nullptr, myas[0], false );
		if ( !calc )
		{
		    uiString msg(tr("Attribute not in the set,"
				"cannot create: '%1'").arg(myas[0].userRef()));
		    if ( isRestoringSession() )
			appl_.restoreMsgs().add( msg );
		    else
			uiMSG().error( msg );

		    return false;
		}

		uiTaskRunner progm( &appl_ );
		uiTreeItem* treeitem = sceneMgr().findItem( visid, attrib );
		newdp = calc->createAttrib( tkzs, regsdp, &progm );
		if ( !newdp )
		{
		    if ( !calc->errmsg_.isEmpty() )
		    {
			if ( isRestoringSession() )
			    appl_.restoreMsgs().add( calc->errmsg_ );

			if ( treeitem )
			    treeitem->setToolTip( uiODSceneMgr::cColorColumn(),
						  calc->errmsg_ );
		    }
		}
		else
		{
		    const ZDomain::Info& zdomain =
			ZDomain::Info::getFrom( myas[0].zDomainKey(),
						myas[0].zDomainUnit() );
		    if ( &zdomain != &newdp->zDomain() )
			newdp.getNonConstPtr()->setZDomain( zdomain );

		    if ( treeitem )
			treeitem->setToolTip( uiODSceneMgr::cColorColumn(),
					      uiString::empty() );
		}
	    }
	    else
	    {
		attrserv_->setTargetSelSpecs( myas );
		newdp = attrserv_->createOutput( tkzs, regsdp );
	    }

	    const TypeSet<Attrib::SelSpec>& tmpset =
						attrserv_->getTargetSelSpecs();
	    if ( tmpset.size() != myas.size() && !tmpset.isEmpty() )
		visserv_->setSelSpecs( visid, attrib, tmpset );

	    if ( !newdp )
	    {
		// clearing texture and set back original selspec
		const bool isattribenabled =
				visserv_->isAttribEnabled( visid, attrib );
		visserv_->setSelSpecs( visid, attrib, myas );
		visserv_->enableAttrib( visid, attrib, isattribenabled );
		return false;
	    }

	    res = visserv_->setRegularSeisDataPack( visid, attrib,
						    newdp.getNonConstPtr() );
	    break;
	}
	case uiVisPartServer::Traces :
	{
	    const Interval<float> zrg = visserv_->getDataTraceRange( visid );
	    attrserv_->setTargetSelSpecs( myas );
	    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rdmtdisp,
			    visserv_->getObject(visid) );
	    if ( !rdmtdisp )
		break;

	    const RandomLineID rdlid = rdmtdisp->getRandomLineID();
	    ConstRefMan<RandomSeisDataPack> newdp;
	    if ( myas[0].id().asInt()==Attrib::SelSpec::cOtherAttrib().asInt() )
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		PtrMan<Attrib::ExtAttribCalc> calc =
			Attrib::ExtAttrFact().create( nullptr, myas[0], false );
		if ( !calc )
		    break;

		newdp = calc->createRdmTrcAttrib( zrg, rdlid, nullptr );
	    }
	    else
		newdp = attrserv_->createRdmTrcsOutputRM( zrg, rdlid );

	    res = visserv_->setRandomSeisDataPack( visid, attrib,
						   newdp.getNonConstPtr() );
	    break;
	}
	case uiVisPartServer::RandomPos :
	{
	    res = calcRandomPosAttrib( visid, attrib );
	    break;
	}
	case uiVisPartServer::OtherFormat :
	{
	    otherformatvisid_ = visid;
	    otherformatattrib_ = attrib;
	    getOtherFormatData.trigger();
	    otherformatvisid_.setUdf();
	    otherformatattrib_ = -1;
	    res = true;
	    break;
	}
	default :
	{
	    pErrMsg("Invalid format");
	    return false;
	}
    }

    return res;
}


bool uiODApplMgr::getDefaultDescID( Attrib::DescID& descid, bool is2d ) const
{
    const MultiID mid = static_cast<const uiApplMgr&>(*this)
			    .seisServer()->getDefaultDataID( is2d );
    if ( mid.isUdf() )
	return false;

    descid = static_cast<const uiApplMgr&>(*this)
			    .attrServer()->getStoredID( mid, is2d );
    return descid.isValid();
}


void uiODApplMgr::calcShiftAttribute( int attrib, const Attrib::SelSpec& as )
{
    uiTreeItem* parent = sceneMgr().findItem( visserv_->getEventObjId() );
    if ( !parent ) return;

    RefObjectSet<DataPointSet> dpsset;
    emattrserv_->fillHorShiftDPS( dpsset, 0 );

    if ( mIsUdf(attrib) )
    {
	auto* pitm = dCast(uiODEarthModelSurfaceTreeItem*,parent);
	EM::ObjectID objid = pitm ? pitm->emObjectID() : EM::ObjectID::udf();
	uiODAttribTreeItem* itm =
		new uiODEarthModelSurfaceDataTreeItem( objid, nullptr,
			typeid(*parent).name() );
	parent->addChild( itm, false );
	attrib = visserv_->addAttrib( visserv_->getEventObjId() );
	emattrserv_->setAttribIdx( attrib );
    }

    attrserv_->setTargetSelSpec( as );
    attrserv_->createOutput( dpsset, 1 );

    TypeSet<DataPointSet::DataRow> drset;
    BufferStringSet nmset;
    RefMan<DataPointSet> dps = new DataPointSet( drset, nmset, false, true );
    if ( !dps )
	return;

    mDeclareAndTryAlloc(DataColDef*,siddef,DataColDef(emattrserv_->sidDef()));
    if ( !siddef )
	return;

    dps->dataSet().add( siddef );
    if ( !dps->bivSet().setNrVals( dpsset.size()+2 ) )
	return;

    mAllocVarLenArr( float, attribvals, dpsset.size()+2 );
    if ( !mIsVarLenArrOK(attribvals) )
	return;

    attribvals[0] = 0.0; //depth

    BinIDValueSet::SPos bvspos;
    while ( dpsset[0]->bivSet().next(bvspos) )
    {
	const BinID binid = dpsset[0]->bivSet().getBinID( bvspos );
	for ( int idx=0; idx<dpsset.size(); idx++ )
	{
	    const float* vals = dpsset[idx]->bivSet().getVals( bvspos );
	    if ( !idx )
		attribvals[1] = vals[1]; //Sid

	    attribvals[idx+2] = vals[2]; //attrib
	}

	dps->bivSet().add( binid, mVarLenArr(attribvals) );
    }

    dps->dataChanged();
    if ( !setRandomPosData(visServer()->getEventObjId(),attrib,*dps.ptr()) )
	return;

    visserv_->setSelSpec( visserv_->getEventObjId(), attrib, as );
    visserv_->selectTexture( visServer()->getEventObjId(), attrib,
				emattrserv_->textureIdx() );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
}


bool uiODApplMgr::calcRandomPosAttrib( const VisID& visid, int attrib )
{
    const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attrib );
    if ( !as )
    {
	uiMSG().error( tr("Cannot calculate attribute on this object") );
	return false;
    }
    else if ( as->id() == as->cNoAttrib() || as->id() == as->cAttribNotSel() )
	return false;

    Attrib::SelSpec myas( *as );
    const BufferString userref( myas.userRef() );
    if ( myas.id()==Attrib::SelSpec::cOtherAttrib() )
    {
	const MultiID surfmid = visserv_->getMultiID( visid );
	const EM::ObjectID emid = emserv_->getObjectID( surfmid );
	const int auxdatanr = emserv_->loadAuxData( emid, userref.buf() );
	if ( auxdatanr>=0 )
	{
	    RefMan<DataPointSet> dps = new DataPointSet( false, true );
	    TypeSet<float> shifts( 1, 0 );
	    emserv_->getAuxData( emid, auxdatanr, *dps.ptr(), shifts[0] );
	    if ( !setRandomPosData(visid,attrib,*dps.ptr()) )
		return false;

	    mDynamicCastGet(visSurvey::HorizonDisplay*,vishor,
			    visserv_->getObject(visid) )
	    vishor->setAttribShift( attrib, shifts );

	    auto* userrefs = new BufferStringSet;
	    userrefs->add( "Section ID" );
	    userrefs->add( userref.buf() );
	    vishor->setUserRefs( attrib, userrefs );
	}

	return auxdatanr>=0;
    }

    visBase::DataObject* dataobj = visserv_->getObject( visid );
    mDynamicCastGet(visSurvey::SurveyObject*,survobj,dataobj);
    if ( !survobj || survobj->getAttributeFormat(attrib) !=
			visSurvey::SurveyObject::RandomPos )
	return false;

    RefMan<DataPointSet> dps = new DataPointSet( false, true );
    if ( !visserv_->getRandomPos(visid,*dps.ptr()) || dps->isEmpty() )
	return false;

    const DataColDef dcol( userref.buf() );
    dps->dataSet().add( new DataColDef(dcol) );
    const DataPointSet::ColID colid = dps->indexOf( dcol.name_.buf() );
    attrserv_->setTargetSelSpec( myas );
    if ( !attrserv_->createOutput(*dps.ptr(),colid) )
	return false;

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,survobj)
    if ( hd )
    {
	if ( !setRandomPosData(visid,attrib,*dps.ptr()) )
	    return false;

	TypeSet<float> shifts( 1,(float)visserv_->getTranslation(visid).z_);
	hd->setAttribShift( attrib, shifts );
    }
    else
    {
	if ( !setRandomPosData(visid,attrib,*dps.ptr()) )
	    return false;

	if ( visserv_->getSelAttribNr() == attrib )
	{
	    survobj->useTexture( true, true );
	    // tree only, not at restore session
	}
    }

    return true;
}


bool uiODApplMgr::evaluateAttribute( const VisID& visid, int attrib )
{
    const uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, attrib );
    if ( format == uiVisPartServer::Cube )
    {
	const TrcKeyZSampling tkzs = visserv_->getTrcKeyZSampling( visid );
	RefMan<RegularSeisDataPack> regsdp =
				attrserv_->createOutputRM( tkzs, nullptr );
	visserv_->setRegularSeisDataPack( visid, attrib, regsdp.ptr() );
    }
    else if ( format==uiVisPartServer::Traces )
    {
	const Interval<float> zrg = visserv_->getDataTraceRange( visid );
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rdmtdisp,
			visserv_->getObject(visid) );
	if ( !rdmtdisp )
	    return false;

	RefMan<RandomSeisDataPack> randdp =
	    attrserv_->createRdmTrcsOutputRM( zrg, rdmtdisp->getRandomLineID());
	visserv_->setRandomSeisDataPack( visid, attrib, randdp.ptr() );
    }
    else if ( format==uiVisPartServer::RandomPos )
    {
	RefMan<DataPointSet> dps = new DataPointSet( false, true );
	return visserv_->getRandomPos( visid, *dps.ptr() ) && !dps->isEmpty() &&
	       attrserv_->createOutput( *dps.ptr(), dps->nrCols() ) &&
	       setRandomPosData( visid, attrib, *dps.ptr() );
    }
    else
    {
	uiMSG().error( tr("Cannot evaluate attributes on this object") );
	return false;
    }

    return true;
}


bool uiODApplMgr::evaluate2DAttribute( const VisID& visid, int attrib )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(visid))
    if ( !s2d )
	return false;

    const TrcKeyZSampling tkzs = s2d->getTrcKeyZSampling( false );
    RefMan<RegularSeisDataPack> regsdp =
				attrserv_->createOutputRM( tkzs, nullptr );
    return visserv_->setRegularSeisDataPack( visid, attrib, regsdp.ptr() );
}


bool uiODApplMgr::handleEvent( const uiApplPartServer* aps, int evid )
{
    if ( !aps )
	return true;

    if ( aps == pickserv_ )
	return handlePickServEv(evid);
    else if ( aps == visserv_ )
	return handleVisServEv(evid);
    else if ( aps == nlaserv_ )
	return handleNLAServEv(evid);
    else if ( aps == attrserv_ )
	return handleAttribServEv(evid);
    else if ( aps == volprocserv_ )
	return handleVolProcServEv(evid);
    else if ( aps == emserv_ )
	return handleEMServEv(evid);
    else if ( aps == emattrserv_ )
	return handleEMAttribServEv(evid);
    else if ( aps == wellserv_ )
	return handleWellServEv(evid);
    else if ( aps == wellattrserv_ )
	return handleWellAttribServEv(evid);
    else if ( aps == mpeserv_ )
	return handleMPEServEv(evid);

    return false;
}


void* uiODApplMgr::deliverObject( const uiApplPartServer* aps, int id )
{
    if ( aps == attrserv_ )
    {
	bool isnlamod2d = id == uiAttribPartServer::objNLAModel2D();
	bool isnlamod3d = id == uiAttribPartServer::objNLAModel3D();
	if ( isnlamod2d || isnlamod3d  )
	{
	    if ( nlaserv_ )
		nlaserv_->set2DEvent( isnlamod2d );

	    return nlaserv_ ? (void*)(&nlaserv_->getModel()) : nullptr;
	}
    }
    else
    {
	pErrMsg("deliverObject for unsupported part server");
    }

    return nullptr;
}


bool uiODApplMgr::handleMPEServEv( int evid )
{
    if ( evid == uiMPEPartServer::evAddTreeObject() )
    {
	const EM::ObjectID emid = mpeserv_->activeTrackerEMID();
	const SceneID sceneid = mpeserv_->getCurSceneID();
	const VisID sdid = sceneMgr().addEMItem( emid, sceneid );
	if ( !sdid.isValid() )
	    return false;

	mDynamicCastGet(visSurvey::EMObjectDisplay*,visemdisplay,
			visserv_->getObject(sdid));
	if ( !visemdisplay || !visemdisplay->activateTracker() )
	    return false;

	ConstRefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );
	if ( !emobj )
	    return false;

	const StringView typestr = emobj->getTypeStr();
	if ( typestr == EM::Horizon3D::typeStr() )
	    viewer2DMgr().addNewTrackingHorizon3D( emid, sceneid );
	else if ( typestr == EM::Horizon2D::typeStr() )
	    viewer2DMgr().addNewTrackingHorizon2D( emid, sceneid );

	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evRemoveTreeObject() )
    {
	ConstRefMan<MPE::EMTracker> activetracker =
						mpeserv_->getActiveTracker();
	if ( !activetracker )
	    return false;

	const EM::ObjectID emid = activetracker->objectID();
	TypeSet<SceneID> sceneids;
	visserv_->getSceneIds( sceneids );

	TypeSet<VisID> hordisplayids;
	visserv_->findObject( typeid(visSurvey::HorizonDisplay),
			      hordisplayids );

	TypeSet<VisID> hor2ddisplayids;
	visserv_->findObject( typeid(visSurvey::Horizon2DDisplay),
			      hor2ddisplayids );

	hordisplayids.append( hor2ddisplayids );
	viewer2DMgr().removeHorizon3D( emid );
	viewer2DMgr().removeHorizon2D( emid );

	for ( int idx=0; idx<hordisplayids.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
			    visserv_->getObject(hordisplayids[idx]));
	    if ( emod && emod->getObjectID()==emid )
	    {
		for ( int idy=0; idy<sceneids.size(); idy++ )
		    visserv_->removeObject( hordisplayids[idx], sceneids[idy] );
		sceneMgr().removeTreeItem(hordisplayids[idx] );
	    }
	}

	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid==uiMPEPartServer::evHorizonTracking() )
    {
	sceneMgr().updateItemToolbar( visserv_->getSelObjectId() );
    }
    else if ( evid == uiMPEPartServer::evStartSeedPick() )
    {
	//Turn off everything
	visserv_->turnSeedPickingOn( true );
	sceneMgr().setToViewMode( false );
    }
    else if ( evid==uiMPEPartServer::evEndSeedPick() )
    {
	visserv_->turnSeedPickingOn( false );
    }
    else if ( evid==uiMPEPartServer::evSetupClosed() )
    {
	visserv_->reportTrackingSetupActive( false );
    }
    else if ( evid==uiMPEPartServer::evSetupLaunched() )
    {
	visserv_->reportTrackingSetupActive( true );
    }
    else if ( evid==uiMPEPartServer::evGetAttribData() )
    {
    }
    else if ( evid==uiMPEPartServer::evInitFromSession() )
	visserv_->initMPEStuff();
    else if ( evid==uiMPEPartServer::evUpdateTrees() )
	sceneMgr().updateTrees();
    else if ( evid==uiMPEPartServer::evStoreEMObject() )
	storeEMObject();
    else if ( evid==uiMPEPartServer::evSelectAttribForTracking() )
    {
	if ( visserv_->selectAttribForTracking() )
	    mpeserv_->attribSelectedForTracking();
    }
    else
    {
	pErrMsg("Unknown event from mpeserv");
    }

    return true;
}


bool uiODApplMgr::handleWellServEv( int evid )
{
    if ( evid == uiWellPartServer::evPreviewRdmLine() )
    {
	TypeSet<Coord> coords;
	wellserv_->getRdmLineCoordinates( coords );
	setupRdmLinePreview( coords );
    }
    else if ( evid == uiWellPartServer::evCleanPreview() )
    {
	cleanPreview();
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    else if ( evid == uiWellPartServer::evDisplayWell() )
    {
	const SceneID sceneid = sceneMgr().askSelectScene();
	if ( !sceneid.isValid() )
	    return false;

	const TypeSet<MultiID>& wellids = wellserv_->createdWellIDs();
	for ( int idx=0; idx<wellids.size(); idx++ )
	    sceneMgr().addWellItem( wellids.get(idx), sceneid );
    }

    return true;
}


bool uiODApplMgr::handleWellAttribServEv( int evid )
{
    if ( evid == uiWellAttribPartServer::evPreview2DFromWells() )
    {
	TypeSet<Coord> coords;
	if ( !wellattrserv_->getPrev2DFromWellCoords(coords) )
	    return false;

	setupRdmLinePreview( coords );
    }
    else if ( evid == uiWellAttribPartServer::evShow2DFromWells() )
    {
	const Pos::GeomID wellto2dgeomid = wellattrserv_->new2DFromWellGeomID();
	if ( !wellto2dgeomid.is2D() )
	    return false;

	sceneMgr().add2DLineItem( wellto2dgeomid );
	sceneMgr().updateTrees();
    }
    else if ( evid == uiWellAttribPartServer::evCleanPreview() )
    {
	cleanPreview();
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    return true;
}


bool uiODApplMgr::handleEMServEv( int evid )
{
    if ( evid == uiEMPartServer::evDisplayHorizon() )
    {
	const SceneID sceneid = sceneMgr().askSelectScene();
	if ( !sceneid.isValid() )
	    return false;

	const EM::ObjectID emid = emserv_->selEMID();
	sceneMgr().addEMItem( emid, sceneid );
	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiEMPartServer::evRemoveTreeObject() )
    {
	const EM::ObjectID emid = emserv_->selEMID();

	TypeSet<SceneID> sceneids;
	visserv_->getSceneIds( sceneids );

	TypeSet<VisID> emdisplayids;

	TypeSet<VisID> hordisplayids;
	visserv_->findObject( typeid(visSurvey::HorizonDisplay),
			      hordisplayids );
	emdisplayids.append( hordisplayids );

	TypeSet<VisID> hor2ddisplayids;
	visserv_->findObject( typeid(visSurvey::Horizon2DDisplay),
			      hor2ddisplayids );
	emdisplayids.append( hor2ddisplayids );

	TypeSet<VisID> faultdisplayids;
	visserv_->findObject( typeid(visSurvey::FaultDisplay),
			      faultdisplayids );
	emdisplayids.append( faultdisplayids );

	TypeSet<VisID> faultstickdisplay;
	visserv_->findObject( typeid(visSurvey::FaultStickSetDisplay),
			      faultstickdisplay );
	emdisplayids.append( faultstickdisplay );

	for ( int idx=0; idx<emdisplayids.size(); idx++ )
	{
	    bool remove = false;
	    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
		    visserv_->getObject(emdisplayids[idx]));
	    if ( emod && emod->getObjectID()==emid )
		remove = true;

	    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
		    visserv_->getObject(emdisplayids[idx]));
	    if ( fd && fd->getEMObjectID()==emid )
		remove = true;

	    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fsd,
		    visserv_->getObject(emdisplayids[idx]));
	    if ( fsd && fsd->getEMObjectID()==emid )
		remove = true;

	    if ( !remove ) continue;

	    for ( int idy=0; idy<sceneids.size(); idy++ )
		visserv_->removeObject( emdisplayids[idx], sceneids[idy] );
	    sceneMgr().removeTreeItem(emdisplayids[idx] );
	}


	sceneMgr().updateTrees();
	return true;
    }
    else
    {
	pErrMsg("Unknown event from emserv");
    }

    return true;
}


bool uiODApplMgr::handleEMAttribServEv( int evid )
{
    const VisID visid = visserv_->getEventObjId();
    const int attribidx = emattrserv_->attribIdx();
    const VisID shiftvisid = emattrserv_->getShiftedObjectVisID();

    if ( evid == uiEMAttribPartServer::evCalcShiftAttribute() )
    {
	const Attrib::SelSpec as( emattrserv_->getAttribBaseNm(),
				  emattrserv_->attribID() );
	calcShiftAttribute( attribidx, as );
    }
    else if ( evid == uiEMAttribPartServer::evHorizonShift() )
    {
	const int textureidx = emattrserv_->textureIdx();
	visserv_->setTranslation( shiftvisid,
				    Coord3(0,0,emattrserv_->getShift()) );
	if ( !mIsUdf(attribidx) )
	    visserv_->selectTexture( shiftvisid, attribidx, textureidx );

	uiTreeItem* parent = sceneMgr().findItem( shiftvisid );
	if ( parent )
	    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( evid == uiEMAttribPartServer::evStoreShiftHorizons() )
    {
	const uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, -1 );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	RefMan<DataPointSet> data = new DataPointSet( false, true );
	visserv_->getRandomPosCache( visid, attribidx, *data );
	if ( data->isEmpty() )
	    return false;

	const MultiID mid = visserv_->getMultiID( visid );
	const EM::ObjectID emid = emserv_->getObjectID( mid );
	const int nrvals = data->bivSet().nrVals()-2;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    const float shift = emattrserv_->shiftRange().atIndex(idx);
	    float usrshift = shift * SI().zDomain().userFactor();
	    if ( mIsZero(usrshift,1e-3) ) usrshift = 0.f;
	    BufferString shiftstr;
	    shiftstr.set( usrshift ).embed( '[', ']' );
	    BufferString nm( emattrserv_->getAttribBaseNm(), " ", shiftstr );
	    emserv_->setAuxData( emid, *data, nm, idx+2, shift );
	    BufferString dummy;
	    if ( !emserv_->storeAuxData( emid, dummy, false ) )
		return false;
	}
    }
    else if ( evid == uiEMAttribPartServer::evShiftDlgOpened() )
    {
	enableMenusAndToolBars( false );
    }
    else if ( evid==uiEMAttribPartServer::evShiftDlgClosedCancel() ||
	      evid==uiEMAttribPartServer::evShiftDlgClosedOK() )
    {
	enableMenusAndToolBars( true );

	const bool isok = evid==uiEMAttribPartServer::evShiftDlgClosedOK();
	const BoolTypeSet& enableattrib = emattrserv_->initialAttribStatus();
	const int textureidx = emattrserv_->textureIdx();

	if ( !isok || mIsUdf(textureidx) )
	{
	    uiTreeItem* parent = sceneMgr().findItem(visid);
	    if ( parent && !mIsUdf(emattrserv_->attribIdx()) )
	    {
		uiTreeItem* itm = parent->lastChild();
		while ( true )
		{
		    uiTreeItem* nxt = itm ? itm->siblingAbove() : nullptr;
		    if ( !nxt )
			break;

		    itm = nxt;
		}

		parent->removeChild( itm );
		visserv_->removeAttrib( shiftvisid, emattrserv_->attribIdx() );
	    }
	}

	if ( !isok )
	{
	    visserv_->setTranslation( shiftvisid,
		    Coord3(0,0,emattrserv_->initialShift() ) );

	    for ( int idx=0; idx<enableattrib.size(); idx++ )
		visserv_->enableAttrib( shiftvisid, idx, enableattrib[idx] );

	    uiTreeItem* parent = sceneMgr().findItem( shiftvisid );
	    if ( parent )
		parent->updateColumnText( uiODSceneMgr::cNameColumn() );
	}
	else
	{
	    for ( int idx=0; idx<visserv_->getNrAttribs(visid); idx++ )
	    {
		if ( idx==attribidx )
		{
		    RefMan<DataPointSet> dps = new DataPointSet( false, true );
		    if ( !visserv_->getRandomPosCache(shiftvisid,attribidx,
						      *dps.ptr()) ||
			 dps->isEmpty() )
			continue;

		    const int sididx = dps->dataSet().findColDef(
			    emattrserv_->sidDef(), PosVecDataSet::NameExact );

		    int texturenr = emattrserv_->textureIdx() + 1;
		    if ( sididx<=texturenr )
			texturenr++;

		    const int nrvals = dps->bivSet().nrVals();
		    for ( int idy=nrvals-1; idy>0; idy-- )
		    {
			if ( idy!=texturenr && idy!=sididx )
			    dps->bivSet().removeVal( idy );
		    }

		    if ( !setRandomPosData(shiftvisid,attribidx,*dps.ptr()) )
			return false;
		}
	    }
	}
    }
    else if ( evid == uiEMAttribPartServer::evDisplayEMObject() )
    {
	const SceneID sceneid = sceneMgr().askSelectScene();
	if ( !sceneid.isValid() )
	    return false;

	const TypeSet<EM::ObjectID>& emobjids = emattrserv_->getEMObjIDs();
	for ( int idx=0; idx<emobjids.size(); idx++ )
	    sceneMgr().addEMItem( emobjids[idx], sceneid );

	sceneMgr().updateTrees();
    }
    else
    {
	pErrMsg("Unknown event from emattrserv");
    }

    return true;
}


bool uiODApplMgr::handlePickServEv( int evid )
{
    if ( evid == uiPickPartServer::evGetHorInfo3D() )
    {
	emserv_->getAllSurfaceInfo( pickserv_->horInfos(), false );
    }
    else if ( evid == uiPickPartServer::evGetHorInfo2D() )
    {
	emserv_->getAllSurfaceInfo( pickserv_->horInfos(), true );
    }
    else if ( evid == uiPickPartServer::evGetHorDef3D() )
    {
	TypeSet<EM::ObjectID> horids;
	const ObjectSet<MultiID>& storids = pickserv_->selHorIDs();
	for ( int idx=0; idx<storids.size(); idx++ )
	{
	    const MultiID horid = *storids[idx];
	    const EM::ObjectID id = emserv_->getObjectID(horid);
	    if ( !id.isValid() || !emserv_->isFullyLoaded(id) )
		emserv_->loadSurface( horid );

	    horids += emserv_->getObjectID(horid);
	}

	emserv_->getSurfaceDef3D( horids, pickserv_->genDef(),
			       pickserv_->selTrcKeySampling() );
    }
    else if ( evid == uiPickPartServer::evGetHorDef2D() )
	emserv_->getSurfaceDef2D( pickserv_->selHorIDs(),
				  pickserv_->selectLines(),
				  pickserv_->getPos2D(),
				  pickserv_->getTrcPos2D(),
				  pickserv_->getHor2DZRgs() );
    else if ( evid == uiPickPartServer::evFillPickSet() )
	emserv_->fillPickSet( *pickserv_->pickSet(), pickserv_->horID() );
    else if ( evid == uiPickPartServer::evDisplayPickSet() )
    {
	const MultiID key = pickserv_->pickSetID();
	TypeSet<MultiID> allkeys;
	sceneMgr().getLoadedPickSetIDs( allkeys, false );
	if ( !allkeys.isPresent(key) )
	    sceneMgr().addPickSetItem( pickserv_->pickSetID() );
    }
    else
    {
	pErrMsg("Unknown event from pickserv");
    }

    return true;
}


bool uiODApplMgr::handleVisServEv( int evid )
{
    const VisID visid = visserv_->getEventObjId();
    visserv_->unlockEvent();

    if ( evid == uiVisPartServer::evUpdateTree() )
	sceneMgr().updateTrees();
    else if ( evid == uiVisPartServer::evDeSelection()
	   || evid == uiVisPartServer::evSelection() )
	sceneMgr().updateSelectedTreeItem();
    else if ( evid == uiVisPartServer::evGetNewData() )
	return getNewData( visid, visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evInteraction() )
	sceneMgr().setItemInfo( visid );
    else if ( evid==uiVisPartServer::evPickingStatusChange() ||
	      evid == uiVisPartServer::evMouseMove() )
	sceneMgr().updateStatusBar();
    else if ( evid == uiVisPartServer::evViewModeChange() )
	sceneMgr().setToWorkMode( visserv_->getWorkMode() );
    else if ( evid == uiVisPartServer::evSelectAttrib() )
	return selectAttrib( visid, visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evViewAll() )
	sceneMgr().viewAll(0);
    else if ( evid == uiVisPartServer::evToHomePos() )
	sceneMgr().toHomePos(0);
    else if ( evid == uiVisPartServer::evShowMPEParentPath() )
    {
	mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
			visserv_->getObject(visid))
	if ( !hd || !hd->getScene() )
	    return false;

	const TrcKeyValue tkv = hd->getScene()->getMousePos();
	addMPEParentPath( visid, tkv.tk_ );
    }
    else if ( evid == uiVisPartServer::evShowMPESetupDlg() )
    {
	const VisID selobjvisid = visserv_->getSelObjectId();
	mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
				visserv_->getObject(selobjvisid))
	const EM::ObjectID emid =
		emod ? emod->getObjectID() : EM::ObjectID::udf();
	mpeserv_->showSetupDlg( emid );
    }
    else if ( evid == uiVisPartServer::evShowSetupGroupOnTop() )
    {
	mDynamicCastGet( visSurvey::EMObjectDisplay*, emod,
			 visserv_->getObject(visserv_->getSelObjectId()) );
	if ( !emod )
	    return false;

	return mpeserv_->showSetupGroupOnTop( emod->getObjectID(),
					      visserv_->getTopSetupGroupName());
    }
    else if ( evid == uiVisPartServer::evDisableSelTracker() )
    {
	const VisID selobjvisid = visserv_->getSelObjectId();
	mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
			visserv_->getObject(selobjvisid))
	const EM::ObjectID emid =
			emod ? emod->getObjectID() : EM::ObjectID::udf();
	RefMan<MPE::EMTracker> tracker =  mpeserv_->getTrackerByID( emid );
	if ( tracker )
	    tracker->enable( false );
    }
    else if ( evid == uiVisPartServer::evColorTableChange() )
	updateColorTable( visserv_->getEventObjId(),
			  visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evStoreEMObject() )
	storeEMObject();
    else if ( evid == uiVisPartServer::evKeyboardEvent() )
    {
    }
    else if ( evid == uiVisPartServer::evMouseEvent() )
    {
    }
    else
    {
	pErrMsg("Unknown event from visserv");
    }

    return true;
}


void uiODApplMgr::addMPEParentPath( const VisID& visid, const TrcKey& tk )
{
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv_->getObject(visid))
    if ( !hd ) return;

    mDynamicCastGet(EM::Horizon3D*,hor3d,
		    EM::EMM().getObject(hd->getObjectID()))
    if ( !hor3d )
	return;

    TypeSet<TrcKey> trcs; hor3d->getParents( tk, trcs );
    if ( trcs.isEmpty() ) return;

    BendPointFinderTrcKey bpf( trcs, 10 );
    if ( !bpf.execute() ) return;

    const TypeSet<int>& bends = bpf.bendPoints();
    RefMan<Geometry::RandomLine> rl = new Geometry::RandomLine;
    BufferString rlnm = "Parents path";
    rlnm.add( " [" ).add( tk.lineNr() ).add( "," )
	.add( tk.trcNr() ).add( "]" );
    rl->setName( rlnm );
    Geometry::RLM().add( rl.ptr() );
    for ( int idx=0; idx<bends.size(); idx++ )
	rl->addNode( trcs[bends[idx]].position() );

    const VisID rlvisid =
	sceneMgr().addRandomLineItem( rl->ID(), hd->getSceneID() );
    viewer2DMgr().displayIn2DViewer( rlvisid, 0, FlatView::Viewer::VD );
    visserv_->setSelObjectId( visid );
}


bool uiODApplMgr::handleNLAServEv( int evid )
{
    if ( evid == uiNLAPartServer::evPrepareWrite() )
    {
	// Before NLA model can be written, the AttribSet's IOPar must be
	// made available as it almost certainly needs to be stored there.
	const Attrib::DescSet* ads =
	    attrserv_->curDescSet(nlaserv_->is2DEvent());
	if ( !ads ) return false;
	IOPar& iopar = nlaserv_->modelPars();
	iopar.setEmpty();
	BufferStringSet inputs = nlaserv_->modelInputs();
	const Attrib::DescSet* cleanads = ads->optimizeClone( inputs );
	(cleanads ? cleanads : ads)->fillPar( iopar );
	delete cleanads;
    }
    else if ( evid == uiNLAPartServer::evConfirmWrite() )
    {
	MultiID nlaid = nlaserv_->modelId();
	attrserv_->updateNLAInput( nlaid, nlaserv_->is2DEvent() );
	//Multiple NN volume output is not supported, hence no flag
    }
    else if ( evid == uiNLAPartServer::evPrepareRead() )
    {
	bool saved = attrserv_->setSaved(nlaserv_->is2DEvent());
	uiString msg = tr("Current attribute set is not saved.\nSave now?");
	if ( !saved && uiMSG().askSave( msg ) )
	    attrserv_->saveSet(nlaserv_->is2DEvent());
    }
    else if ( evid == uiNLAPartServer::evReadFinished() )
    {
	// New NLA Model available: replace the attribute set!
	// Create new attrib set from NLA model's IOPar

	attrserv_->replaceSet( nlaserv_->modelPars(), nlaserv_->is2DEvent() );
	wellattrserv_->setNLAModel( &nlaserv_->getModel() );
	emattrserv_->setNLAModel( &nlaserv_->getModel(), nlaserv_->modelId() );
    }
    else if ( evid == uiNLAPartServer::evGetInputNames() )
    {
	// Construct the choices for input nodes.
	// Should be:
	// * All attributes (stored ones filtered out)
	// * All cubes - between []
	attrserv_->getPossibleOutputs( nlaserv_->is2DEvent(),
				      nlaserv_->inputNames() );
    }
    else if ( evid == uiNLAPartServer::evGetStoredInput() )
    {
	TypeSet<MultiID> keys;
	nlaserv_->getNeededStoredInputs( keys );
	for ( const auto& key : keys )
	    attrserv_->addToDescSet( key, nlaserv_->is2DEvent() );
    }
    else if ( evid == uiNLAPartServer::evGetData() )
    {
	// OK, the input and output nodes are known.
	// Query the server and make sure the relevant data is extracted
	// Put data in the training and test posvec data sets

	if ( !attrserv_->curDescSet(nlaserv_->is2DEvent()) )
	    { pErrMsg("Huh"); return false; }
	RefObjectSet<DataPointSet> dpss;
	const bool dataextraction = nlaserv_->willDoExtraction();
	if ( !dataextraction )
	    dpss += new DataPointSet( nlaserv_->is2DEvent(), false );
	else
	{
	    nlaserv_->getDataPointSets( dpss );
	    if ( dpss.isEmpty() )
	    {
		uiMSG().error(tr("No matching well data found"));
		return false;
	    }

	    bool allempty = true;
	    for ( int idx=0; idx<dpss.size(); idx++ )
	    {
		if ( !dpss[idx]->isEmpty() )
		{
		    allempty = false;
		    break;
		}
	    }

	    if ( allempty )
	    {
		uiMSG().error(tr("No valid data locations found"));
		return false;
	    }

	    if ( !attrserv_->extractData(dpss) )
		return true;

	    IOPar& iop = nlaserv_->storePars();
	    attrserv_->curDescSet(nlaserv_->is2DEvent())->fillPar( iop );
	    if ( iop.name().isEmpty() )
		iop.setName( "Attributes" );
	}

	const uiString res = nlaserv_->prepareInputData( dpss );
	if ( res != uiNLAPartServer::sKeyUsrCancel() )
	    uiMSG().warning( res );

	if ( !dataextraction ) // i.e. if we have just read a DataPointSet
	    attrserv_->replaceSet( dpss[0]->dataSet().pars(), dpss[0]->is2D() );
    }
    else if ( evid == uiNLAPartServer::evSaveMisclass() )
    {
	ConstRefMan<DataPointSet> dps = nlaserv_->dps();
	RefMan<DataPointSet> mcpicks = new DataPointSet( dps->is2D() );
	for ( int irow=0; irow<dps->size(); irow++ )
	{
	    if ( dps->group(irow) == 3 )
		mcpicks->addRow( dps->dataRow(irow) );
	}

	mcpicks->dataChanged();
	pickserv_->setMisclassSet( *mcpicks );
    }
    else if ( evid == uiNLAPartServer::evCreateAttrSet() )
    {
	Attrib::DescSet attrset(nlaserv_->is2DEvent());
	if ( !attrserv_->createAttributeSet(nlaserv_->modelInputs(),&attrset) )
	    return false;
	attrset.fillPar( nlaserv_->modelPars() );
	attrserv_->replaceSet( nlaserv_->modelPars(), nlaserv_->is2DEvent() );
    }
    else if ( evid == uiNLAPartServer::evCr2DRandomSet() )
    {
	pickserv_->createRandom2DSet();
    }
    else
    {
	pErrMsg("Unknown event from nlaserv");
    }

    return true;
}


bool uiODApplMgr::handleAttribServEv( int evid )
{
    const VisID visid = visserv_->getEventObjId();
    const int attrib = visserv_->getSelAttribNr();
    if ( evid==uiAttribPartServer::evDirectShowAttr() )
    {
	Attrib::SelSpec as;
	attrserv_->getDirectShowAttrSpec( as );
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	{
	    uiMSG().error( tr("Please select an attribute"
			      " element in the tree") );
	    return false;
	}

	visserv_->setSelSpec( visid, attrib, as );
	visserv_->setColTabMapperSetup( visid, attrib, ColTab::MapperSetup() );
	getNewData( visid, attrib );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evNewAttrSet() )
    {
	attribSetChg.trigger();
	const Attrib::DescSet* ads =
		attrserv_->curDescSet( attrserv_->is2DEvent() );
	mpeserv_->setCurrentAttribDescSet( ads );
	emattrserv_->setDescSet( ads );
    }
    else if ( evid==uiAttribPartServer::evAttrSetDlgClosed() )
    {
	enableMenusAndToolBars( true );
	enableSceneManipulation( true );
    }
    else if ( evid==uiAttribPartServer::evEvalAttrInit() )
    {
	const uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, attrib );
	const bool alloweval = !( format==uiVisPartServer::None );
	const bool allowstorage = format==uiVisPartServer::RandomPos;
	attrserv_->setEvaluateInfo( alloweval, allowstorage );
    }
    else if ( evid==uiAttribPartServer::evEvalCalcAttr() )
    {
	Attrib::SelSpec as( "Evaluation", Attrib::SelSpec::cOtherAttrib() );
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	{
	    uiMSG().error( tr("Please select an attribute"
			      " element in the tree") );
	    return false;
	}
	if ( !calcMultipleAttribs( as ) )
	{
	    uiMSG().error( tr("Could not evaluate this attribute") );
	    return false;
	}

	const TypeSet<Attrib::SelSpec>& tmpset = attrserv_->getTargetSelSpecs();
	const ColTab::MapperSetup* ms =
	    visserv_->getColTabMapperSetup( visid, attrib, tmpset.size()/2 );

	attrserv_->setEvalBackupColTabMapper( ms );

	if ( ms )
	{
	    ColTab::MapperSetup myms = *ms;
	    myms.type_ = ColTab::MapperSetup::Fixed;
	    visserv_->setColTabMapperSetup( visid, attrib, myms );
	}
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalShowSlice() )
    {
	const int sliceidx = attrserv_->getSliceIdx();
	const int attrnr =
	    visserv_->getSelAttribNr()==-1 ? 0 : visserv_->getSelAttribNr();
	visserv_->selectTexture( visid, attrnr, sliceidx );

	updateColorTable( visid, attrnr );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalStoreSlices() )
    {
	const int attrnr =
	    visserv_->getSelAttribNr()==-1 ? 0 : visserv_->getSelAttribNr();
	const uiVisPartServer::AttribFormat format =
				visserv_->getAttributeFormat( visid, attrib );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	RefMan<DataPointSet> data = new DataPointSet( false, true );
	visserv_->getRandomPosCache( visid, attrnr, *data );
	if ( data->isEmpty() )
	    return false;

	const MultiID mid = visserv_->getMultiID( visid );
	const EM::ObjectID emid = emserv_->getObjectID( mid );
        const float shift = (float) visserv_->getTranslation(visid).z_;
	const TypeSet<Attrib::SelSpec>& specs = attrserv_->getTargetSelSpecs();
	const int nrvals = data->bivSet().nrVals()-2;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    emserv_->setAuxData( emid, *data, specs[idx].userRef(),
				 idx+2, shift );
	    BufferString dummy;
	    emserv_->storeAuxData( emid, dummy, false );
	}
    }
    else if ( evid==uiAttribPartServer::evEvalRestore() )
    {
	if ( attrserv_->getEvalBackupColTabMapper() )
	{
	    visserv_->setColTabMapperSetup( visid, attrib,
		    *attrserv_->getEvalBackupColTabMapper() );
	}

	Attrib::SelSpec* as = const_cast<Attrib::SelSpec*>(
		visserv_->getSelSpec(visid,attrib) );
	const TypeSet<Attrib::SelSpec>& tmpset = attrserv_->getTargetSelSpecs();
	const int sliceidx = attrserv_->getSliceIdx();
	if ( as && tmpset.validIdx(sliceidx) )
	{
	    // userref stored in objectref during evaluation process
	    BufferString usrref = as->objectRef();
	    *as = tmpset[sliceidx];
	    as->setUserRef( usrref );
	}

	sceneMgr().updateTrees();
    }
    else
    {
	pErrMsg("Unknown event from attrserv");
    }

    return true;
}


bool uiODApplMgr::handleVolProcServEv( int /*evid*/ )
{
    return true;
}


bool uiODApplMgr::calcMultipleAttribs( Attrib::SelSpec& as )
{
    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    const VisID visid = visserv_->getEventObjId();
    const int attrib = visserv_->getSelAttribNr();
    const TypeSet<Attrib::SelSpec>& tmpset = attrserv_->getTargetSelSpecs();
    BufferString savedusrref = tmpset.size() ? tmpset[0].objectRef() : "";
    as.setObjectRef( savedusrref );
    as.set2DFlag( attrserv_->is2DEvent() );
    as.setUserRef( tmpset[0].userRef() );
    if ( tmpset.isEmpty() )
	visserv_->setSelSpec( visid, attrib, as );
    else
	visserv_->setSelSpecs( visid, attrib, tmpset );

    BufferStringSet* refs = new BufferStringSet();
    for ( int idx=0; idx<tmpset.size(); idx++ )
	refs->add( tmpset[idx].userRef() );
    visserv_->setUserRefs( visid, attrib, refs );
    visserv_->setColTabMapperSetup( visid, attrib, ColTab::MapperSetup() );
    return as.is2D() ? evaluate2DAttribute(visid,attrib)
		     : evaluateAttribute(visid,attrib);
}


void uiODApplMgr::setupRdmLinePreview(const TypeSet<Coord>& coords)
{
    if ( wellserv_->getPreviewIds().size()>0 )
	cleanPreview();

    TypeSet<VisID> plids;
    TypeSet<SceneID> sceneids;
    visserv_->getSceneIds( sceneids );

    for ( const auto& sceneid : sceneids )
    {
	RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneid );
	auto* pl = new visSurvey::PolyLineDisplay;
	pl->fillPolyLine( coords );
	pl->setColor( scene->getAnnotColor() );
	visserv_->addObject( pl, sceneid, true );
	plids.addIfNew( pl->id() );
    }

    wellserv_->setPreviewIds( plids );
}


void uiODApplMgr::cleanPreview()
{
    TypeSet<VisID>& previds = wellserv_->getPreviewIds();
    if ( previds.isEmpty() )
	return;

    TypeSet<SceneID> sceneids;
    visserv_->getSceneIds( sceneids );
    for ( const auto& sceneid : sceneids )
	for ( const auto& plid : previds )
	    visserv_->removeObject( plid, sceneid );

    previds.erase();
}


void uiODApplMgr::storeEMObject()
{
    const TypeSet<VisID>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv_->isLocked(selectedids[0]) )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
				surface, visserv_->getObject(selectedids[0]) );
    if ( !surface ) return;

    const EM::ObjectID emid = surface->getObjectID();
    MultiID mid = emserv_->getStorageID( emid );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    const bool saveas = mid.isUdf() || !ioobj;
    emserv_->storeObject( emid, saveas );

    TypeSet<VisID> ids;
    mid = emserv_->getStorageID( emid );
    visserv_->findObject( mid, ids );

    for ( int idx=0; idx<ids.size(); idx++ )
	visserv_->setUiObjectName( ids[idx], emserv_->getUiName(emid) );

    mpeserv_->saveSetup( mid );
    sceneMgr().updateTrees();
}


void uiODApplMgr::manSurvCB( CallBacker* )
{
    const int retval = selectSurvey(0);
    if ( retval == 4 )
	appl_.exit( false, false );
}


void uiODApplMgr::editSurvCB( CallBacker* )
{
    editCurrentSurvey( nullptr );
}


void uiODApplMgr::tieWellToSeismic( CallBacker* )
{ wellattrserv_->createD2TModel(MultiID()); }
void uiODApplMgr::doWellLogTools( CallBacker* )
{ wellserv_->doLogTools(); }
void uiODApplMgr::launchRockPhysics( CallBacker* )
{ wellserv_->launchRockPhysics(); }
void uiODApplMgr::launch2DViewer( CallBacker* )
{ show2DViewer(); }
void uiODApplMgr::doLayerModeling( CallBacker* )
{ uiStratLayerModelManager::doBasicLayerModel(); }

void uiODApplMgr::doVolProcCB( CallBacker* )
{ volprocserv_->doVolProc( 0, 0, false ); }
void uiODApplMgr::doVolProc2DCB( CallBacker* )
{ volprocserv_->doVolProc( 0, 0, true ); }
void uiODApplMgr::doVolProc( const MultiID& mid, bool is2d )
{ volprocserv_->doVolProc( &mid, 0, is2d ); }
void uiODApplMgr::createVolProcOutput( bool is2d )
{ volprocserv_->createVolProcOutput( 0, is2d ); }

bool uiODApplMgr::editNLA( bool is2d )
{ return attrvishandler_.editNLA( is2d ); }
bool uiODApplMgr::uvqNLA( bool is2d )
{ return attrvishandler_.uvqNLA( is2d ); }
void uiODApplMgr::createHorOutput( int tp, bool is2d )
{ attrvishandler_.createHorOutput( tp, is2d ); }
void uiODApplMgr::createVol( bool is2d, bool multiattrib )
{ attrvishandler_.createVol( is2d, multiattrib ); }

void uiODApplMgr::doStratAmp(CallBacker*)
{ emattrserv_->computeStratAmp(); }

void uiODApplMgr::doIsochron(CallBacker*)
{ emserv_->computeIsochron(); }

void uiODApplMgr::seisOut2DCB(CallBacker*)
{ createVol(true,false); }
void uiODApplMgr::seisOut3DCB(CallBacker*)
{ createVol(false,false); }

void uiODApplMgr::doWellXPlot( CallBacker* )
{ attrvishandler_.doXPlot(); }
void uiODApplMgr::doAttribXPlot( CallBacker* )
{ attrvishandler_.crossPlot(); }
void uiODApplMgr::openCrossPlot( CallBacker* )
{ dispatcher_.openXPlot(); }
void uiODApplMgr::setZStretch()
{ attrvishandler_.setZStretch(); }
bool uiODApplMgr::selectAttrib( const VisID& id, int attrib )
{ return attrvishandler_.selectAttrib( id, attrib ); }
void uiODApplMgr::setHistogram( const VisID& visid, int attrib )
{ attrvishandler_.setHistogram(visid,attrib); }
void uiODApplMgr::colMapperChg( CallBacker* )
{ attrvishandler_.colMapperChg(); }
bool uiODApplMgr::setRandomPosData( const VisID& visid, int attrib,
				    const DataPointSet& data )
{ return attrvishandler_.setRandomPosData( visid, attrib, data ); }
void uiODApplMgr::pageUpDownPressed( bool pageup )
{ attrvishandler_.pageUpDownPressed(pageup); sceneMgr().updateTrees(); }
void uiODApplMgr::updateColorTable( const VisID& visid, int attrib )
{ attrvishandler_.updateColorTable( visid, attrib ); }
void uiODApplMgr::colSeqChg( CallBacker* )
{ attrvishandler_.colSeqChg(); sceneMgr().updateSelectedTreeItem(); }
NotifierAccess* uiODApplMgr::colorTableSeqChange()
{ return attrvishandler_.colorTableSeqChange(); }
void uiODApplMgr::useDefColTab( const VisID& visid, int attrib )
{ attrvishandler_.useDefColTab(visid,attrib); }
void uiODApplMgr::saveDefColTab( const VisID& visid, int attrib )
{ attrvishandler_.saveDefColTab(visid,attrib); }

void uiODApplMgr::processPreStack( bool is2d )
{ dispatcher_.processPreStack( is2d ); }
void uiODApplMgr::genAngleMuteFunction( CallBacker* )
{ dispatcher_.genAngleMuteFunction(); }
void uiODApplMgr::createCubeFromWells( CallBacker* )
{ dispatcher_.createCubeFromWells(); }
void uiODApplMgr::bayesClass2D( CallBacker* )
{ dispatcher_.bayesClass(true); }
void uiODApplMgr::bayesClass3D( CallBacker* )
{ dispatcher_.bayesClass(false); }
void uiODApplMgr::startBatchJob()
{ dispatcher_.startBatchJob(); }
void uiODApplMgr::setupBatchHosts()
{ dispatcher_.setupBatchHosts(); }
void uiODApplMgr::batchProgs()
{ dispatcher_.batchProgs(); }
void uiODApplMgr::pluginMan()
{ dispatcher_.pluginMan(); }
void uiODApplMgr::posConversion()
{ dispatcher_.posConversion(); }
void uiODApplMgr::manageShortcuts()
{ dispatcher_.manageShortcuts(); }
void uiODApplMgr::startInstMgr()
{ dispatcher_.startInstMgr(); }
void uiODApplMgr::setAutoUpdatePol()
{ dispatcher_.setAutoUpdatePol(); }
void uiODApplMgr::process2D3D( int opt )
{ dispatcher_.process2D3D( opt ); }

void uiODApplMgr::MiscSurvInfo::refresh()
{
    xyunit_ = SI().xyUnit();
    zunit_ = SI().zUnit( true );
    zstep_ = SI().zStep();
}


bool uiODApplMgr::isRestoringSession() const
{
    return appl_.isRestoringSession();
}


void uiODApplMgr::showReleaseNotes( bool isonline )
{
    const HelpKey key( ReleaseNotesProvider::sKeyFactoryName(),
		       isonline ? ReleaseNotesProvider::sKeyFactoryName()
				: nullptr );
    HelpProvider::provideHelp( key );
}
