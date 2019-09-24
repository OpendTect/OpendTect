/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2001
________________________________________________________________________

-*/

#include "uiattribpartserv.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "attribdataholder.h"
#include "attribsetcreator.h"
#include "attribstorprovider.h"

#include "arraynd.h"
#include "arrayndslice.h"
#include "arrayndwrapper.h"
#include "coltabmapper.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "executor.h"
#include "dbman.h"
#include "nlamodel.h"
#include "randomlinegeom.h"
#include "rangeposprovider.h"
#include "seisbuf.h"
#include "seisdatapack.h"
#include "seispreload.h"
#include "seistrc.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uiattrdesced.h"
#include "uiattrdescseted.h"
#include "uiattrgetfile.h"
#include "uiattribcrossplot.h"
#include "uiattrsel.h"
#include "uiattrsetman.h"
#include "uiattrvolout.h"
#include "uiattr2dsel.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicrossattrevaluatedlg.h"
#include "uievaluatedlg.h"
#include "uigeninputdlg.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uimultcomputils.h"
#include "uimultoutsel.h"
#include "uirgbattrseldlg.h"
#include "uiseisioobjinfo.h"
#include "uisetpickdirs.h"
#include "uiseispartserv.h"
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


uiAttribPartServer::uiAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , dirshwattrdesc_(0)
    , attrsetdlg_(0)
    , is2devsent_(false)
    , attrsetclosetim_("Attrset dialog close")
    , multcomp3d_(uiStrings::s3D())
    , multcomp2d_(uiStrings::s2D())
    , dpsdispmgr_( 0 )
    , attrsneedupdt_(true)
    , manattribset2ddlg_(0)
    , manattribset3ddlg_(0)
    , impattrsetdlg_(0)
    , volattrdlg_(0)
    , multiattrdlg_(0)
    , dataattrdlg_(0)
    , evalmapperbackup_ (0)
    , targetspecs_(*new Attrib::SelSpecList)
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

    mAttachCB( DBM().surveyChanged, uiAttribPartServer::survChangedCB );
}


uiAttribPartServer::~uiAttribPartServer()
{
    detachAllNotifiers();

    delete attrsetdlg_;

    deepErase( attrxplotset_ );
    delete manattribset2ddlg_;
    delete manattribset3ddlg_;
    delete impattrsetdlg_;
    delete volattrdlg_;
    delete multiattrdlg_;
    delete dataattrdlg_;

    delete &targetspecs_;
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
	menutext = issteering ? uiStrings::sSteeringCube(mPlural) :
				tr("Stored Cube");

    return endmenu ? m3Dots(menutext) : menutext;
}


bool uiAttribPartServer::replaceSet( const IOPar& iopar, bool is2d )
{
    DescSet updset( is2d );
    uiRetVal uirv = updset.usePar( iopar );
    if ( !uirv.isOK() )
	{ uimsg().error( uirv ); return false; }

    DescSet& attrset = curDescSet4Edit( is2d );
    attrset = updset;
    attrset.setStoreID( DBKey() );

    set2DEvent( is2d );
    sendEvent( evNewAttrSet() );
    return true;
}


bool uiAttribPartServer::addToDescSet( const char* key, bool is2d )
{
    DescSet& attrset = curDescSet4Edit( is2d );
    const DBKey dbky = DBKey( key );
    if ( !dbky.isValid() )
	return false;

    const DescID descid = attrset.ensureStoredPresent( dbky );
    return descid.isValid();
}


Attrib::DescSet& uiAttribPartServer::curDescSet4Edit( bool is2d ) const
{
    return DescSet::global4Edit( is2d );
}


const Attrib::DescSet& uiAttribPartServer::curDescSet( bool is2d ) const
{
    return DescSet::global( is2d );
}


void uiAttribPartServer::getDirectShowAttrSpec( SelSpec& as ) const
{
   if ( !dirshwattrdesc_ )
       as.set( 0, SelSpec::cNoAttribID(), false, 0 );
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
	delete manattribset3ddlg_;
	manattribset3ddlg_ = new uiAttrSetMan( parent(), false );
	manattribset3ddlg_->show();
    }
}


bool uiAttribPartServer::editSet( bool is2d )
{
    DescSet& attrset = DescSet::global4Edit( is2d );

    delete attrsetdlg_;
    attrsetdlg_ = new uiAttribDescSetEd( parent(), attrset );
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
	attrsetdlg_->updateAllDescsDefaults();
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
	is2d = attrsetdlg_->is2D();
    uiAttribCrossPlot* uiattrxplot = new uiAttribCrossPlot( 0,
							    curDescSet(is2d));
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
	    attrxplotset_.removeSingle( attrxplotset_.indexOf(crossplot) );
	xplot->windowClosed.remove(
		mCB(this,uiAttribPartServer,xplotClosedCB) );
	xplot->setDeleteOnClose( true );
    }
}


void uiAttribPartServer::attrsetDlgApply( CallBacker* )
{
    set2DEvent( attrsetdlg_->is2D() );
    sendEvent( evNewAttrSet() );
}


void uiAttribPartServer::attrsetDlgClosed( CallBacker* )
{
    attrsetclosetim_.start( 10, true );
}


void uiAttribPartServer::attrsetDlgCloseTimTick( CallBacker* )
{
    if ( attrsetdlg_ && attrsetdlg_->uiResult() )
    {
	set2DEvent( attrsetdlg_->is2D() );
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
				       const ZDomain::Info* zdominfo,
				       Pos::GeomID geomid,
				       const uiString& seltxt )
{
    const bool is2d = geomid.is2D();
    const DescSet& attrset = curDescSet( is2d );
    uiAttrSelData attrdata( attrset );
    attrdata.attribid_ = selspec.isNLA() ? SelSpec::cNoAttribID()
					 : selspec.id();
    attrdata.setOutputNr( selspec.isNLA() ? selspec.id().getI() : -1 );
    attrdata.nlamodel_ = getNLAModel(is2d);
    attrdata.zdomaininfo_ = zdominfo;

    if ( is2d )
    {
	GeomIDSet geomids; geomids += geomid;
	uiAttr2DSelDlg dlg( parent(), &attrset, geomids, attrdata.nlamodel_ );
	if ( !dlg.go() )
	    return false;

	if ( dlg.getSelType()==0 || dlg.getSelType()==1 )
	{
	    SeisIOObjInfo info( dlg.getStoredAttrName(), Seis::Line );
	    attrdata.attribid_ = attrset.getStoredID(
		    info.ioObj() ? info.ioObj()->key() : DBKey::getInvalid(),
		    dlg.getComponent(), true );
	}
	else
	{
	    attrdata.attribid_ = dlg.getSelDescID();
	    attrdata.setOutputNr( dlg.getOutputNr() );
	}

    }
    else
    {
	uiAttrSelDlg::Setup setup( seltxt );
	setup.showsteeringdata(true);
	uiAttrSelDlg dlg( parent(), attrdata, setup );
	if ( !dlg.go() )
	    return false;

	attrdata.attribid_ = dlg.attribID();
	attrdata.setOutputNr( dlg.outputNr() );
	attrdata.setAttrSet( dlg.attrSet() );
    }

    const bool isnla = attrdata.isNLA();
    const Desc* desc = attrdata.attrSet().getDesc( attrdata.attribid_ );
    const bool isstored = desc && desc->isStored();
    BufferString objref;
    if ( isnla )
	objref = nlaname_;
    else if ( !isstored )
	objref = attrset.name();

    selspec.set( 0, isnla ? DescID(attrdata.outputNr())
			  : attrdata.attribid_, isnla, objref );
    if ( isnla && attrdata.nlamodel_ )
	selspec.setRefFromID( *attrdata.nlamodel_ );
    else if ( !isnla )
	selspec.setRefFromID( attrdata.attrSet() );
    //selspec.setZDomainKey( dlg.zDomainKey() );

    return true;
}


bool uiAttribPartServer::selectRGBAttribs( SelSpecList& rgbaspecs,
					   const ZDomain::Info* zinf,
					   Pos::GeomID geomid )
{
    const bool is2d = geomid.is2D();
    uiRGBAttrSelDlg dlg( parent(), curDescSet(is2d) );
    dlg.setSelSpec( rgbaspecs );
    if ( !dlg.go() )
	return false;

    dlg.fillSelSpec( rgbaspecs );
    return true;
}


void uiAttribPartServer::directShowAttr( CallBacker* cb )
{
    mDynamicCastGet( uiAttribDescSetEd*, ed, cb );
    if ( !ed )
	{ pErrMsg("cb is not uiAttribDescSetEd*"); return; }

    dirshwattrdesc_ = ed->curDesc();
    sendEvent( evDirectShowAttr() );
}


void uiAttribPartServer::updateSelSpec( SelSpec& ss ) const
{
    bool is2d = ss.is2D();
    if ( ss.isNLA() )
    {
	const NLAModel* nlamod = getNLAModel( is2d );
	if ( !nlamod )
	    ss.set( ss.userRef(), SelSpec::cNoAttribID(), true, 0 );
	else
	{
	    ss.setIDFromRef( *nlamod );
	    ss.setObjectRef( nlaname_ );
	}
    }
    else if ( !is2d )
    {
	ss.setIDFromRef( curDescSet(false) );
	ss.setObjectRef( curDescSet(false).name() );
    }
}


void uiAttribPartServer::getPossibleOutputs( bool is2d,
					     BufferStringSet& nms ) const
{
    nms.erase();
    Attrib::SelInfo attrinf( &curDescSet(is2d), DescID(), 0, is2d );
    nms.append( attrinf.attrnms_ );
    attrinf.ioobjids_.addTo( nms );
}


bool uiAttribPartServer::setSaved( bool is2d ) const
{
    if ( !DescSet::globalUsed(is2d) )
	return true;

    return !curDescSet(is2d).isChanged();
}


const Attrib::DescSet* uiAttribPartServer::getUserPrefDescSet( uiParent* p )
						    // static
{
    const DescSet& ds2d = DescSet::global( true );
    if ( !SI().has3D() )
	return &ds2d;
    const DescSet& ds3d = DescSet::global( false );
    if ( !SI().has2D() )
	return &ds3d;

    const int nr3d = ds3d.nrDescs( false, true );
    const int nr2d = ds2d.nrDescs( false, true );
    if ( (nr3d>0) != (nr2d>0) )
	return nr2d > 0 ? &ds2d : &ds3d;

    const int res =
	gUiMsg(p).ask2D3D( tr("Which attributes do you want to use?"), true );
    if ( res == -1 )
	return 0;
    return res == 1 ? &ds2d : &ds3d;
}


void uiAttribPartServer::saveSet( bool is2d )
{
    const DescSet& attrset = curDescSet( is2d );
    PtrMan<CtxtIOObj> ctio = attrset.getCtxtIOObj( false );
    uiIOObjSelDlg dlg( parent(), *ctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	uiRetVal uirv = attrset.store( dlg.ioObj()->key() );
	if ( !uirv.isOK() )
	    uimsg().error( uirv );
    }
}


#define mAttrProcDlg( dlgobj ) \
    { \
	if ( !dlgobj ) \
	    dlgobj = new uiAttrVolOut( parent(), attrset, multiattrib, \
					   getNLAModel(is2d), nlaid ); \
	else \
	    dlgobj->updateAttributes(attrset,getNLAModel(is2d),nlaid ); \
	dlgobj->show(); \
    }

void uiAttribPartServer::outputVol( const DBKey& nlaid, bool is2d,
				    bool multiattrib )
{
    const DescSet& attrset = curDescSet( is2d );

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
    return targetspecs_.size() <= nr ? DescID() : targetspecs_[nr].id();
}


Attrib::EngineMan* uiAttribPartServer::createEngMan(
		const TrcKeyZSampling* tkzs, Pos::GeomID geomid )
{
    if ( targetspecs_.isEmpty() ||
	 targetspecs_[0].id() == SelSpec::cNoAttribID() )
	{ pErrMsg("Nothing to do"); return 0; }

    const bool istargetstored = targetspecs_[0].isStored( 0 );
    const bool is2d = targetspecs_[0].is2D();
    if ( is2d && tkzs && mIsUdfGeomID(geomid) )
	geomid = tkzs->hsamp_.getGeomID();

    const DescSet& attrset = curDescSet( is2d );

    if ( !istargetstored )
    {
	DescID attribid = targetspecs_[0].id();
	const Desc* seldesc = attrset.getDesc( attribid );
	if ( seldesc )
	{
	    DescID multoiid = seldesc->getMultiOutputInputID();
	    if ( multoiid.isValid() )
	    {
		uiAttrSelData attrdata( attrset );
		Attrib::SelInfo attrinf( &attrset, DescID(),
				 attrdata.nlamodel_, is2d, false, false );
		if ( !uiMultOutSel::handleMultiCompChain( attribid, multoiid,
			    is2d, attrinf, attrset, parent(), targetspecs_ ))
		    return 0;
	    }
	}
    }

    Attrib::EngineMan* aem = new Attrib::EngineMan;
    aem->setAttribSet( &curDescSet(is2d) );
    aem->setNLAModel( getNLAModel(is2d) );
    aem->setAttribSpecs( targetspecs_ );
    if ( tkzs )
	aem->setSubSel( Survey::FullSubSel(*tkzs) );
    else if ( geomid.isValid() )
	aem->setGeomID( geomid );

    return aem;
}


DataPack::ID uiAttribPartServer::createOutput( const TrcKeyZSampling& tkzs,
					       DataPack::ID cacheid )
{
    if ( tkzs.hsamp_.is2D() )
    {
	uiTaskRunner taskrunner( parent() );
	return create2DOutput( tkzs, taskrunner );
    }

    auto& dpm = DPM(DataPackMgr::SeisID());
    auto cache = dpm.get<RegularSeisDataPack>( cacheid ).ptr();
    auto newpack = createOutput( tkzs, cache );
    if ( !newpack )
	return DataPack::cNoID();

    newpack.setNoDelete( true );
    dpm.add( newpack );

    return newpack->id();
}


const Attrib::Desc* uiAttribPartServer::getTargetDesc() const
{
    const bool havetargetspecs = !targetspecs_.isEmpty();
    const bool is2d = havetargetspecs && targetspecs_[0].is2D();
    const Attrib::DescSet& attrset = curDescSet( is2d );
    return attrset.getDesc( targetspecs_[0].id() );
}


#define mErrRet(s) { uimsg().error(s); return 0; }


RefMan<RegularSeisDataPack> uiAttribPartServer::createOutput(
				const TrcKeyZSampling& tkzs,
				const RegularSeisDataPack* cache )
{
    PtrMan<Attrib::EngineMan> aem = createEngMan( &tkzs );
    if ( !aem )
	mErrRet( mINTERNAL("Cannot make AEM") )

    bool atsamplepos = true;

    const Desc* targetdesc = getTargetDesc();
    RefMan<RegularSeisDataPack> preloadeddatapack = 0;

    if ( targetdesc )
    {
	if ( targetdesc->isStored() )
	{
	    const DBKey dbky( targetdesc->getStoredID() );
	    preloadeddatapack =
			Seis::PLDM().get<RegularSeisDataPack>( dbky );
	}

	BufferString defstr;
	targetdesc->getDefStr( defstr );
	if ( defstr != targetspecs_[0].defString() )
	    cache = 0;

	const bool isz = tkzs.isFlat()&&tkzs.defaultDir() == OD::ZSlice;
	if ( !preloadeddatapack && isz )
	{
	    uiRetVal uirv;
	    Desc* nonconsttargetdesc = const_cast<Desc*>( targetdesc );
	    RefMan<Attrib::Provider> tmpprov =
			Attrib::Provider::create( *nonconsttargetdesc, uirv );
	    if ( !tmpprov )
		mErrRet( uirv )

	    tmpprov->computeRefZStep();
	    tmpprov->computeRefZ0();
	    const float floatres = (tkzs.zsamp_.start - tmpprov->refZ0()) /
				    tmpprov->refZStep();
	    const int intres = mNINT32( floatres );
	    if ( Math::Abs(floatres-intres) > 1e-2 )
		atsamplepos = false;
	}
    }

    bool success = true;
    PtrMan<Attrib::Processor> processor = 0;
    RefMan<RegularSeisDataPack> output = 0;
    if ( !preloadeddatapack && !atsamplepos )//note: 1 attrib computed at a time
    {
	if ( !targetdesc )
	    return 0;
	Pos::RangeProvider3D rgprov3d;
	rgprov3d.setSampling( tkzs );
	DataColDef* dtcd = new DataColDef( targetdesc->userRef() );
	ManagedObjectSet<DataColDef> dtcoldefset;
	dtcoldefset += dtcd;
	uiTaskRunnerProvider trprov( parent() );
	RefMan<DataPointSet> posvals = new DataPointSet( rgprov3d.is2D() );
	if ( !posvals->extractPositions(rgprov3d,dtcoldefset,trprov) )
	    return 0;

	const int firstcolidx = 0;

	uiRetVal uirv;
	processor = aem->getTableOutExecutor( *posvals, uirv, firstcolidx );
	if ( !processor )
	    { uimsg().error(uirv); return 0; }

	if ( !trprov.execute(*processor) )
	    return 0;

	TypeSet<float> vals;
	posvals->bivSet().getColumn( posvals->nrFixedCols()+firstcolidx, vals,
				    true );
	if ( !vals.isEmpty() )
	{
	    ArrayValueSeries<float, float> avs( vals.arr(), false, vals.size());
	    output = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(false,false) );
	    output->setSampling( tkzs );
	    if ( !output->addComponent(targetspecs_[0].userRef(),true) ||
		    !output->data(0).valueSeries() )
	    {
		output = 0;
	    }
	    else
	    {
		ValueSeries<float>* arr3dvs = output->data(0).valueSeries();
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

	uiRetVal uirv;
	processor = aem->createDataPackOutput( uirv, cache );
	if ( !processor )
	    { uimsg().error(uirv); return 0; }

	processor->showDataAvailabilityErrors( !aem->hasCache() );

	bool showinlprogress = true;
	bool showcrlprogress = true;
	Settings::common().getYN( SettingsAccess::sKeyShowInlProgress(),
				  showinlprogress );
	Settings::common().getYN( SettingsAccess::sKeyShowCrlProgress(),
				  showcrlprogress );

	const bool isstored =
	    targetdesc && targetdesc->isStored() && !targetspecs_[0].isNLA();
	const bool isinl =
		    tkzs.isFlat() && tkzs.defaultDir() == OD::InlineSlice;
	const bool iscrl =
		    tkzs.isFlat() && tkzs.defaultDir() == OD::CrosslineSlice;
	const bool hideprogress = isstored &&
	    ( (isinl&&!showinlprogress) || (iscrl&&!showcrlprogress) );

	if ( aem->getNrOutputsToBeProcessed(*processor) != 0 )
	{
	    if ( !hideprogress )
		success = uiTaskRunner(parent()).execute( *processor );
	    else
	    {
		uiUserShowWait usw( parent(), processor->message() );
		if ( !processor->execute() )
		    { uimsg().error( processor->message() ); return 0; }
	    }
	}

	output = const_cast<RegularSeisDataPack*>(
			aem->getDataPackOutput(*processor).ptr() );
    }

    if ( output && !success )
    {
	if ( !uimsg().askGoOn(tr("Attribute loading/calculation aborted.\n"
	    "Do you want to use the partially loaded/computed data?"), true ) )
	{
	    output = 0;
	}
    }

    return output;
}


bool uiAttribPartServer::createOutput( DataPointSet& posvals, int firstcol )
{
    if ( targetspecs_.isEmpty() )
	return false;

    const Desc* targetdesc = getTargetDesc();
    if ( targetdesc && targetdesc->isStored() )
    {
	const DBKey dbky( targetdesc->getStoredID() );
	auto sdp = Seis::PLDM().get<RegularSeisDataPack>(dbky);
	if ( sdp )
	{
	    const TrcKeyZSampling seistkzs( sdp->subSel() );
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
		uiLabel* lbl = new uiLabel( &dlg, msg );
		uiButtonGroup* grp =
		    new uiButtonGroup( &dlg, "Options", OD::Vertical );
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
		uiTaskRunner uitr( parent() );
		const int comp = targetdesc->selectedOutput();
		DPSFromVolumeFiller filler( posvals, firstcol, *sdp, comp );
		const TrcKeyZSampling sdptkzs( sdp->subSel() );
		filler.setSampling( &sdptkzs );
		return TaskRunner::execute( &uitr, filler );
	    }
	}
    }

    PtrMan<Attrib::EngineMan> aem = createEngMan();
    if ( !aem )
	return false;

    uiRetVal uirv;
    PtrMan<Attrib::Processor> processor =
			aem->getTableOutExecutor( posvals, uirv, firstcol );
    if ( !processor )
	{ uimsg().error(uirv); return false; }
    else if ( !uiTaskRunner(parent()).execute(*processor) )
	return false;

    posvals.setName( targetspecs_[0].userRef() );
    return true;
}


bool uiAttribPartServer::createOutput( ObjectSet<DataPointSet>& dpss,
				       int firstcol )
{
    ExecutorGroup execgrp( "Calculating Attribute", true );
    uiRetVal uirv;

    ObjectSet<Attrib::EngineMan> aems;
    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	Attrib::EngineMan* aem = createEngMan();
	if ( !aem ) continue;

	execgrp.add( aem->getTableOutExecutor(*dpss[idx],uirv,firstcol) );
	aems += aem;
    }

    bool res = true;
    uiTaskRunner taskrunner( parent() );
    res = uiTaskRunner(parent()).execute( execgrp );

    deepErase( aems );
    return res;
}


DataPack::ID uiAttribPartServer::createRdmTrcsOutput(
	const Interval<float>& zrg, int rdlid, const TypeSet<BinID>* subpath )
{
    RefMan<Geometry::RandomLine> rdmline = Geometry::RLM().get( rdlid );
    if ( !rdmline || targetspecs_.isEmpty() )
	return DataPack::cNoID();

    const SelSpec firsttargetspec( targetspecs_.first() );
    const DescSet& attrset = curDescSet( false );
    const Desc* targetdesc = attrset.getDesc(firsttargetspec.id());

    if ( targetdesc )
    {
	const DBKey dbky( targetdesc->getStoredID() );
	auto sdp = Seis::PLDM().get<RegularSeisDataPack>( dbky );

	if ( sdp )
	{
	    BufferStringSet componentnames;
	    for ( int idx=0; idx<targetspecs_.size(); idx++ )
		componentnames.add( targetspecs_[idx].userRef() );

	    return RandomSeisDataPack::createDataPackFrom(
			*sdp, rdlid, zrg, &componentnames, subpath );
	}
    }

    TypeSet<BinID> knots, path;
    rdmline->getNodePositions( knots );
    rdmline->getPathBids( knots, path );
    snapToValidRandomTraces( path, targetdesc );
    BinnedValueSet bidset( 2, false );
    if ( subpath )
    {
	const int subpathstopidx = path.indexOf( subpath->last() );
	if ( subpathstopidx < path.size()-1 )
	    path.removeRange( subpathstopidx+1, path.size()-1 );
	const int subpathstartidx = path.indexOf( subpath->first() );
	if ( subpathstartidx > 0 )
	    path.removeRange( 0, subpathstartidx-1 );
    }

    for ( int idx = 0; idx<path.size(); idx++ )
	bidset.add( path[idx], zrg.start, zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput(bidset,output,knots,path) || output.isEmpty() )
	return DataPack::cNoID();

    RandomSeisDataPack* newpack = new RandomSeisDataPack(
				VolumeDataPack::categoryStr(true,false) );
    newpack->setRandomLineID( rdlid, subpath );
    newpack->setZRange( output.get(0)->zRange() );
    for ( int idx=0; idx<output.get(0)->nrComponents(); idx++ )
    {
	if ( !newpack->addComponent(targetspecs_[idx].userRef(),true) )
	    continue;

	const TrcKeyPath& tkpath = newpack->path();
	const int pathsz = tkpath.size();
	const int nrz = newpack->data(idx).getSize(2);

	for ( int idy=0; idy<pathsz; idy++ )
	{
	    const int trcidx = output.find( tkpath[idy].binID() );
	    const SeisTrc* trc = trcidx<0 ? 0 : output.get( trcidx );
	    for ( int idz=0; idz<nrz; idz++ )
	    {
		const float val = trc ? trc->get(idz,idx) : mUdf(float);
		newpack->data(idx).set( 0, idy, idz, val );
	    }
	}
    }

    newpack->setZDomain(
	    ZDomain::Info(ZDomain::Def::get(firsttargetspec.zDomainKey())));
    newpack->setName( firsttargetspec.userRef() );
    DPM(DataPackMgr::SeisID()).add( newpack );
    return newpack->id();
}


void uiAttribPartServer::snapToValidRandomTraces( TypeSet<BinID>& path,
						  const Desc* targetdesc )
{
    if ( !targetdesc )
	return;

    uiRetVal uirv;
    Desc* nonconsttargetdesc = const_cast<Desc*>( targetdesc );
    RefMan<Attrib::Provider> tmpprov
	= Attrib::Provider::create( *nonconsttargetdesc, uirv );

    Survey::FullSubSel desiredss;
    if ( !tmpprov || !tmpprov->calcPossibleSubSel(-1,desiredss) )
	return;

    TrcKeyZSampling tkzs( tmpprov->possibleSubSel() );
    if ( tkzs.hsamp_.step_.lineNr()==1 && tkzs.hsamp_.step_.trcNr()==1 )
	return;

    for ( int idx=0; idx<path.size(); idx++ )
    {
	if ( tkzs.hsamp_.lineRange().includes(path[idx].lineNr(),true) &&
	     tkzs.hsamp_.trcRange().includes(path[idx].trcNr(),true) )
	{
	    const int shiftedtogetnearestinl = path[idx].lineNr() +
					       tkzs.hsamp_.step_.lineNr()/2;
	    const int inlidx = tkzs.hsamp_.lineIdx( shiftedtogetnearestinl );
	    const int shiftedtogetnearestcrl = path[idx].trcNr() +
					       tkzs.hsamp_.step_.trcNr()/2;
	    const int crlidx = tkzs.hsamp_.trcIdx( shiftedtogetnearestcrl );
	    path[idx] = tkzs.hsamp_.atIndex( inlidx, crlidx );
	}
    }
}


bool uiAttribPartServer::createOutput( const BinnedValueSet& bidset,
				       SeisTrcBuf& output,
				       const TypeSet<BinID>& trueknotspos,
				       const TypeSet<BinID>& snappedpos )
{
    PtrMan<Attrib::EngineMan> aem = createEngMan();
    if ( !aem )
	return 0;

    uiRetVal uirv;
    PtrMan<Attrib::Processor> processor =
	aem->createTrcSelOutput( uirv, bidset, output, mUdf(float), 0,
				 &trueknotspos, &snappedpos );
    if ( !processor )
	{ uimsg().error(uirv); return false; }

    bool showprogress = true;
    Settings::common().getYN( SettingsAccess::sKeyShowRdlProgress(),
			      showprogress );

    const bool isstored = !targetspecs_.isEmpty()
		       && targetspecs_.first().isStored(0);
    if ( !isstored || showprogress )
    {
	uiTaskRunner uitr( parent() );
	return uitr.execute( *processor );
    }

    uiUserShowWait usw( parent(), processor->message() );
    if ( !processor->execute() )
	{ uimsg().error( processor->message() ); return false; }

    return true;
}


class RegularSeisDataPackCreatorFor2D : public ParallelTask
{
public:
RegularSeisDataPackCreatorFor2D( const Attrib::Data2DHolder& input,
				 const ZDomain::Def& zdef,
				 const BufferStringSet* compnames,
				 DataPack::ID& outputid )
    : input_(input)
    , sampling_(input.getTrcKeyZSampling())
    , zdef_(zdef)
    , refnrs_(sampling_.hsamp_.nrTrcs(),mUdf(float))
    , outputid_(outputid)
    , outputdp_(0)
{
    if ( compnames )
	compnames_ = *compnames;
}

od_int64 nrIterations() const		{ return input_.trcinfoset_.size(); }

bool doPrepare( int nrthreads )
{
    if ( input_.trcinfoset_.isEmpty() || sampling_.hsamp_.is3D() )
	return false;

    outputdp_ = new RegularSeisDataPack(VolumeDataPack::categoryStr(true,true));
    outputdp_->setSampling( sampling_ );
    for ( int idx=0; idx<input_.dataset_[0]->validSeriesIdx().size(); idx++ )
    {
	const char* compname = compnames_.validIdx(idx) ?
		compnames_[idx]->str() : OD::EmptyString();
	if ( !outputdp_->addComponent(compname,true) )
	    continue;
    }

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int threadid )
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
	for ( int tidx=mCast(int,start); tidx<=mCast(int,stop); tidx++ )
	{
	    const int trcidx =
			outputdp_->getGlobalIdx( trcinfoset[tidx]->trcKey() );
	    if ( trcidx < 0 )
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
		refnrs_[trcidx] = trcinfoset[tidx]->refnr_;
	}
    }

    return true;
}

bool doFinish( bool success )
{
    outputdp_->setRefNrs( refnrs_ );
    outputdp_->setZDomain( ZDomain::Info(zdef_) );
    if ( !compnames_.isEmpty() )
	outputdp_->setName( compnames_[0]->buf() );
    DPM(DataPackMgr::SeisID()).add( outputdp_ );
    outputid_ = outputdp_->id();
    return true;
}

protected:

    const Attrib::Data2DHolder&		input_;
    const TrcKeyZSampling		sampling_;
    const ZDomain::Def&			zdef_;
    BufferStringSet			compnames_;
    RegularSeisDataPack*		outputdp_;
    DataPack::ID&			outputid_;
    TypeSet<float>			refnrs_;
};

DataPack::ID uiAttribPartServer::create2DOutput( const TrcKeyZSampling& tkzs,
						 TaskRunner& taskrunner )
{
    if ( targetspecs_.isEmpty() )
	return DataPack::cNoID();

    const DescSet& curds = curDescSet( true );
    const Desc* targetdesc = curds.getDesc( targetID(true) );
    const auto geomid = tkzs.hsamp_.getGeomID();
    if ( targetdesc )
    {
	const DBKey dbky( targetdesc->getStoredID() );
	auto sdp = Seis::PLDM().get<RegularSeisDataPack>(dbky,geomid);

	if ( sdp )
	    return sdp->id();
    }

    PtrMan<Attrib::EngineMan> aem = createEngMan( &tkzs, geomid );
    if ( !aem )
	return DataPack::cNoID();

    uiRetVal uirv;
    RefMan<Attrib::Data2DHolder> data2d = new Attrib::Data2DHolder;
    PtrMan<Attrib::Processor> processor
	    = aem->createScreenOutput2D( uirv, *data2d );
    if ( !processor )
	{ uimsg().error(uirv); return DataPack::cNoID(); }

    if ( !taskrunner.execute(*processor) )
	return DataPack::cNoID();

    BufferStringSet userrefs;
    for ( int idx=0; idx<targetspecs_.size(); idx++ )
	userrefs.add( targetspecs_[idx].userRef() );

    return createDataPackFor2D( *data2d,
	    ZDomain::Def::get(targetspecs_.first().zDomainKey()), &userrefs );
}


DataPack::ID uiAttribPartServer::createDataPackFor2D(
					const Attrib::Data2DHolder& input,
					const ZDomain::Def& zdef,
					const BufferStringSet* compnms )
{
    DataPack::ID id;
    RegularSeisDataPackCreatorFor2D datapackcreator( input, zdef, compnms, id );
    datapackcreator.execute();
    return id;
}


bool uiAttribPartServer::extractData( ObjectSet<DataPointSet>& dpss )
{
    if ( dpss.isEmpty() )
	{ pErrMsg("No inp data"); return 0; }
    const DescSet& ads = curDescSet( dpss[0]->is2D() );

    Attrib::EngineMan aem;
    uiTaskRunner taskrunner( parent() );
    bool somesuccess = false;
    bool somefail = false;

    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	uiRetVal uirv;
	DataPointSet& dps = *dpss[idx];
	Executor* tabextr = aem.getTableExtractor( dps, ads, uirv );
	if ( !tabextr )
	    { uimsg().error(uirv); return 0; }

	if ( taskrunner.execute(*tabextr) )
	    somesuccess = true;
	else
	    somefail = true;

	delete tabextr;
    }

    if ( somefail )
    {
	return somesuccess &&
	     uimsg().askGoOn(
	      tr("Some data extraction failed.\n\nDo you want to continue and "
		  "use the (partially) extracted data?"), true);
    }

    return true;
}


Attrib::DescID uiAttribPartServer::getDefaultAttribID( bool is2d ) const
{
    return curDescSet( is2d ).isEmpty() ? Attrib::DescID()
	 : curDescSet( is2d ).getDefaultTargetID();
}


Attrib::DescID uiAttribPartServer::getStoredID( const DBKey& dbkey,
						bool is2d, int selout ) const
{
    return curDescSet( is2d ).getStoredID( dbkey, selout );
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
    uimsg().error( mTODONotImplPhrase() );
}


bool uiAttribPartServer::setPickSetDirs( Pick::Set& ps, const NLAModel* nlamod,
					 float velocity )
{
    //TODO: force 3D to avoid crash for 2D, need workaround for 2D later

    const DescSet& ds = curDescSet( false );
    uiSetPickDirs dlg( parent(), ps, &ds, nlamod, velocity );
    return dlg.go();
}


static void insertItems( MenuItem& mnu, const BufferStringSet& nms,
	const BufferStringSet* ids, const char* cursel,
	int start, int stop, bool correcttype )
{
    StringPair sp( cursel );
    const BufferString curselnm = sp.first();

    mnu.removeItems();
    mnu.enabled = !nms.isEmpty();
    bool checkparent = false;
    for ( int idx=start; idx<stop; idx++ )
    {
	const BufferString& nm = nms.get( idx );
	MenuItem* itm = new MenuItem( toUiString(nm) );
	itm->checkable = true;
	const DBKey dbky = ids ? DBKey( ids->get(idx) ) : DBKey::getInvalid();
	if ( ids && Seis::PLDM().isPresent(dbky) )
	    itm->iconfnm = "preloaded";
	const bool docheck = correcttype && nm == curselnm;
	if ( docheck ) checkparent = true;
	mAddMenuItem( &mnu, itm, true, docheck );
    }

    if ( checkparent ) mnu.checked = true;
}


MenuItem* uiAttribPartServer::storedAttribMenuItem( const SelSpec& ass,
						    bool is2d, bool issteer )
{
    MenuItem* storedmnuitem = is2d ? issteer ? &steering2dmnuitem_
					     : &stored2dmnuitem_
				   : issteer ? &steering3dmnuitem_
					     : &stored3dmnuitem_;
    fillInStoredAttribMenuItem( storedmnuitem, is2d, issteer, ass, false );

    return storedmnuitem;
}


void uiAttribPartServer::fillInStoredAttribMenuItem(
					MenuItem* menu, bool is2d, bool issteer,
					const SelSpec& ass, bool multcomp,
					bool needext )
{
    const DescSet& ds = curDescSet( is2d );
    const Desc* desc = ds.getDesc( ass.id() );
    Attrib::SelInfo attrinf( &ds, DescID(), 0, is2d, issteer, multcomp );
    const bool isstored = desc ? desc->isStored() : false;
    const DBKeySet& dbkys = issteer ? attrinf.steerids_ : attrinf.ioobjids_;

    MenuItem* mnu = menu;
    if ( multcomp && needext )
    {
	MenuItem* submnu = is2d ? &multcomp2d_ : &multcomp3d_;
	mAddManagedMenuItem( menu, submnu, true, submnu->checked );
	mnu = submnu;
    }

    int nritems = dbkys.size();
    if ( nritems <= cMaxMenuSize )
    {
	const int start = 0; const int stop = nritems;
	BufferStringSet idnms;
	(issteer ? attrinf.steerids_ : attrinf.ioobjids_).addTo( idnms );
	const BufferStringSet& nms = issteer ? attrinf.steernms_
					     : attrinf.ioobjnms_;
	insertItems( *mnu, nms, &idnms, ass.userRef(), start, stop, isstored );
    }

    menu->text = getMenuText( is2d, issteer, nritems>cMaxMenuSize );
}



void uiAttribPartServer::insertNumerousItems( const BufferStringSet& bfset,
					      const SelSpec& ass,
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

	Attrib::SelInfo attrinf( &curDescSet(false), DescID(), 0, false );
	BufferStringSet idnms;
	(issteer ? attrinf.steerids_ : attrinf.ioobjids_).addTo( idnms );
	const BufferStringSet& nms = issteer ? attrinf.steernms_
					     : attrinf.ioobjnms_;
	insertItems( *submnu, nms, &idnms, ass.userRef(),
		start, stop, correcttype );

	MenuItem* storedmnuitem = is2d ? issteer ? &steering2dmnuitem_
						 : &stored2dmnuitem_
				       : issteer ? &steering3dmnuitem_
						 : &stored3dmnuitem_;
	mAddManagedMenuItem( storedmnuitem, submnu, true,submnu->checked);
    }
}


MenuItem* uiAttribPartServer::calcAttribMenuItem( const SelSpec& ass,
						  bool is2d, bool useext )
{
    Attrib::SelInfo attrinf( &curDescSet(is2d), DescID(), 0, is2d );
    const bool isattrib = attrinf.attrids_.isPresent( ass.id() );

    const int start = 0; const int stop = attrinf.attrnms_.size();
    MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    uiString txt = useext ? ( is2d ? tr("Attributes 2D")
				   : tr("Attributes 3D") )
			  : uiStrings::sAttribute(mPlural);
    calcmnuitem->text = txt;
    insertItems( *calcmnuitem, attrinf.attrnms_, 0, ass.userRef(),
		 start, stop, isattrib );

    calcmnuitem->enabled = calcmnuitem->nrItems();
    return calcmnuitem;
}


MenuItem* uiAttribPartServer::nlaAttribMenuItem( const SelSpec& ass, bool is2d,
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
	Attrib::SelInfo attrinf( &curDescSet(is2d), DescID(), nlamodel, is2d );
	const bool isnla = ass.isNLA();
	const int start = 0; const int stop = attrinf.nlaoutnms_.size();
	insertItems( *nlamnuitem, attrinf.nlaoutnms_, 0, ass.userRef(),
		     start, stop, isnla );
    }

    nlamnuitem->enabled = nlamnuitem->nrItems();
    return nlamnuitem;
}


MenuItem* uiAttribPartServer::zDomainAttribMenuItem( const SelSpec& ass,
						     const ZDomain::Info& zdinf,
						     bool is2d, bool useext)
{
    MenuItem* zdomainmnuitem = is2d ? &zdomain2dmnuitem_
				    : &zdomain3dmnuitem_;
    uiString itmtxt = toUiString("%1 %2").arg(toUiString(zdinf.key()))
			    .arg(useext ? (!is2d ? uiStrings::sCube(mPlural)
			    : uiStrings::s2DLine()) : uiStrings::sData());
    zdomainmnuitem->text = itmtxt;
    zdomainmnuitem->removeItems();
    zdomainmnuitem->checkable = true;
    zdomainmnuitem->checked = false;

    BufferStringSet ioobjnms;
    Attrib::SelInfo zdomselinfo( zdinf, is2d );
    for ( int idx=0; idx<zdomselinfo.ioobjnms_.size(); idx++ )
    {
	const BufferString& nm = zdomselinfo.ioobjnms_.get( idx );
	MenuItem* itm = new MenuItem( toUiString(nm) );
	const bool docheck = nm == ass.userRef();
	mAddManagedMenuItem( zdomainmnuitem, itm, true, docheck );
	if ( docheck )
	    zdomainmnuitem->checked = true;
    }

    zdomainmnuitem->enabled = zdomainmnuitem->nrItems() > 0;
    return zdomainmnuitem;
}


void uiAttribPartServer::filter2DMenuItems(
	MenuItem& subitem, const SelSpec& ass, Pos::GeomID geomid,
	bool isstored, int steerpol )
{
    if ( mIsUdfGeomID(geomid) )
	return;

    BufferStringSet childitemnms;
    for ( int idx=0; idx<subitem.nrItems(); idx++ )
	childitemnms.add( toString(subitem.getItem(idx)->text) );

    subitem.removeItems();
    BufferString linenm( geomid.name() );
    BufferStringSet attribnms;
    uiSeisPartServer::get2DStoredAttribs( linenm, attribnms, steerpol );
    for ( int idx=0; idx<childitemnms.size(); idx++ )
    {
	FixedString childnm( childitemnms.get(idx).buf() );
	if ( isstored )
	{
	    if ( attribnms.isPresent(childnm) )
	    {
		MenuItem* item = new MenuItem( toUiString(childnm) );
		const bool docheck = childnm==ass.userRef();
		mAddMenuItem(&subitem,item,true,docheck);
	    }
	}
	else
	{
	    const DescSet& ds = curDescSet( true );
	    int descidx = ds.indexOf( childnm );
	    if ( descidx<0 )
		continue;
	    const Desc* desc = ds.desc( descidx );
	    if ( !desc )
		continue;

	    DBKey dbky( desc->getStoredID(true) );
	    PtrMan<IOObj> seisobj = dbky.getIOObj();
	    if ( !seisobj || attribnms.isPresent(seisobj->name()) )
	    {
		MenuItem* item = new MenuItem( toUiString(childnm) );
		const bool docheck = childnm==ass.userRef();
		mAddMenuItem(&subitem,item,true,docheck);
	    }
	}
    }
}


bool uiAttribPartServer::handleAttribSubMenu( int mnuid, SelSpec& ass,
					      bool& dousemulticomp )
{
    if ( stored3dmnuitem_.id == mnuid )
	return selectAttrib( ass, 0, mUdfGeomID,
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
    uiAttrSelData attrdata( curDescSet(is2d) );
    attrdata.nlamodel_ = getNLAModel(is2d);
    Attrib::SelInfo attrinf( &attrdata.attrSet(), DescID(), attrdata.nlamodel_,
		     is2d, issteering, issteering );
    const MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    const MenuItem* nlamnuitem = is2d ? &nla2dmnuitem_ : &nla3dmnuitem_;
    const MenuItem* zdomainmnuitem = is2d ? &zdomain2dmnuitem_
					      : &zdomain3dmnuitem_;

    DescID attribid = SelSpec::cAttribNotSelID();
    int outputnr = -1;
    bool isnla = false;
    bool isstored = false;
    DBKey dbkey;

    if ( stored3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = stored3dmnuitem_.findItem(mnuid);
	const int idx = attrinf.ioobjnms_.indexOf( toString(item->text) );
	dbkey = attrinf.ioobjids_.get(idx);
	attribid = curDescSet(false).getStoredID( dbkey );
	isstored = true;
    }
    else if ( steering3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = steering3dmnuitem_.findItem( mnuid );
	const int idx = attrinf.steernms_.indexOf( toString(item->text) );
	dbkey = attrinf.steerids_.get( idx );
	attribid = curDescSet(false).getStoredID( dbkey );
	isstored = true;
    }
    else if ( stored2dmnuitem_.findItem(mnuid) ||
	      steering2dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = stored2dmnuitem_.findItem(mnuid);
	if ( !item )
	    item = steering2dmnuitem_.findItem(mnuid);
	if ( !item )
	    return false;

	const BufferString itmnm = toString( item->text );
	const int idx = issteering ? attrinf.steernms_.indexOf( itmnm )
				   : attrinf.ioobjnms_.indexOf( itmnm );
	dbkey = issteering ? attrinf.steerids_.get(idx)
			     : attrinf.ioobjids_.get(idx);
	const int selout = issteering ? 1 : -1;
	attribid = curDescSet(true).getStoredID( dbkey, selout );
	isstored = true;
    }
    else if ( calcmnuitem->findItem(mnuid) )
    {
	const MenuItem* item = calcmnuitem->findItem(mnuid);
	int idx = attrinf.attrnms_.indexOf( toString(item->text) );
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
	PtrMan<IOObj> ioobj = DBM().getByName( IOObjContext::Seis,
						toString(item->text) );
	if ( ioobj )
	{
	    dbkey = ioobj->key();
	    attribid = curDescSet(false).getStoredID( dbkey );
	    isstored = true;
	}
    }
    else
	return false;

    const bool nocompsel = is2d && issteering;
    if ( isstored && !nocompsel )
    {
	const SeisIOObjInfo ioobjinf( dbkey ); BufferStringSet compnms;
	ioobjinf.getComponentNames( compnms );
	if ( compnms.size()>1 )
	{
	    TypeSet<int> selcomps;
	    if ( !handleMultiComp( dbkey, is2d, issteering, compnms,
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
	objref = curDescSet( is2d ).name();

    DescID did = isnla ? DescID(outputnr) : attribid;
    ass.set( 0, did, isnla, objref );

    BufferString bfs;
    if ( attribid != SelSpec::cAttribNotSelID() )
    {
	const Desc* desc = curDescSet(is2d).getDesc( attribid );
	if ( desc  )
	{
	    desc->getDefStr( bfs );
	    ass.setDefString( bfs.buf() );
	}
    }

    if ( isnla )
	ass.setRefFromID( *attrdata.nlamodel_ );
    else
	ass.setRefFromID( curDescSet(is2d) );

    ass.set2D( is2d );
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
	attbnm = toString( item->text );
    }
    else if ( calc2dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = calc2dmnuitem_.findItem( mnuid );
	attbnm = toString( item->text );
    }
}


#define mFakeCompName( searchfor, replaceby ) \
{ \
    StringPair strpair( desc->userRef() ); \
    if ( strpair.second() == searchfor ) \
	strpair.second() = replaceby;\
    desc->setUserRef( strpair.getCompString() ); \
}

bool uiAttribPartServer::handleMultiComp( const DBKey& dbkey, bool is2d,
					  bool issteering,
					  BufferStringSet& compnms,
					  DescID& attribid,
					  TypeSet<int>& selectedcomps )
{
    //Trick for old steering cubes: fake good component names
    if ( !is2d && issteering && compnms.isPresent("Component 1") )
    {
	compnms.erase();
	compnms.add( toString(uiStrings::sInlineDip()) );
	compnms.add( toString(uiStrings::sCrosslineDip()) );
    }

    uiMultCompDlg compdlg( parent(), compnms );
    if ( compdlg.go() )
    {
	selectedcomps.erase();
	compdlg.getCompNrs( selectedcomps );
	if ( !selectedcomps.size() ) return false;

	DescSet& ads = curDescSet4Edit( is2d );
	if ( selectedcomps.size() == 1 )
	{
	    attribid = ads.getStoredID( dbkey, selectedcomps[0] );
	    //Trick for old steering cubes: fake good component names
	    if ( !is2d && issteering )
	    {
		Desc* desc = ads.getDesc( attribid );
		if ( !desc )
		    return false;
		mFakeCompName( "Component 1",
			       toString(uiStrings::sInlineDip()) );
		mFakeCompName( "Component 2",
				toString(uiStrings::sCrosslineDip()) );
	    }

	    return true;
	}
	prepMultCompSpecs( selectedcomps, dbkey, is2d, issteering );
    }
    else
	return false;

    return true;
}


bool uiAttribPartServer::prepMultCompSpecs( TypeSet<int> selectedcomps,
					    const DBKey& dbkey, bool is2d,
					    bool issteering )
{
    targetspecs_.erase();
    DescSet& ads = curDescSet4Edit( is2d );
    for ( int idx=0; idx<selectedcomps.size(); idx++ )
    {
	DescID did = ads.getStoredID( dbkey, selectedcomps[idx], true );
	SelSpec ass( 0, did );
	BufferString bfs;
	Desc* desc = ads.getDesc( did );
	if ( !desc )
	    return false;

	desc->getDefStr( bfs );
	ass.setDefString( bfs.buf() );
	//Trick for old steering cubes: fake good component names
	if ( !is2d && issteering )
	{
	    mFakeCompName( "Component 1",
			    toString(uiStrings::sInlineDip()) );
	    mFakeCompName( "Component 2",
			    toString(uiStrings::sCrosslineDip()) );
	}

	//Trick for Prestack offsets displayed on the fly
	if ( desc->isStored() && desc->userRef()[0] == '{' )
	{
	    StringPair strpair( desc->userRef() );
            BufferString newnm = "offset index "; newnm += selectedcomps[idx];
	    strpair.second() = newnm;
	    desc->setUserRef( strpair.getCompString() );
	}

	ass.setRefFromID( ads );
	ass.set2D( is2d );
	targetspecs_ += ass;
    }
    set2DEvent( is2d );
    return true;
}


IOObj* uiAttribPartServer::getIOObj( const SelSpec& ass ) const
{
    if ( ass.isNLA() )
	return 0;

    const DescSet& attrset = curDescSet( ass.is2D() );
    const Desc* desc = attrset.getDesc( ass.id() );
    if ( !desc )
	return 0;

    DBKey storedid = desc->getStoredID();
    if ( !desc->isStored() || storedid.isInvalid() )
	return 0;

    return storedid.getIOObj();
}


#undef mErrRet
#define mErrRet(msg) { uimsg().error(msg); return; }

void uiAttribPartServer::processEvalDlg( bool iscrossevaluate )
{
    if ( !attrsetdlg_ ) return;
    const Desc* curdesc = attrsetdlg_->curDesc();
    if ( !curdesc )
        mErrRet( tr("Please add this attribute first") );

    uiAttrDescEd& ade = attrsetdlg_->curDescEd();
    sendEvent( evEvalAttrInit() );

    if ( !iscrossevaluate )
    {
	uiEvaluateDlg* evaldlg =
	    new uiEvaluateDlg( attrsetdlg_, ade, allowevalstor_ );

	if ( !evaldlg->evaluationPossible() )
	    mErrRet( tr("This attribute has no parameters to evaluate") );

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
	    const TypeSet<DescID>& cids = crossevaldlg->evaluateChildIds();
	    BufferString ds = crossevaldlg->acceptedDefStr();
	    DescSet& ads = attrsetdlg_->getSet();
	    for ( int idx=0; idx<cids.size(); idx++ )
	    {
		Desc* ad = ads.getDesc( cids[idx] );
		if ( ad )
		    ad->parseDefStr( ds );
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
    DescSet* ads = evaldlg ? evaldlg->getEvalSet() : crossevaldlg->getEvalSet();
    if ( evaldlg )
	evaldlg->getEvalSpecs( targetspecs_ );
    else
	crossevaldlg->getEvalSpecs( targetspecs_ );

    set2DEvent( is2d );
    DescSet::pushGlobal( is2d, ads );
    sendEvent( evEvalCalcAttr() );
    delete DescSet::popGlobal( is2d );
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


void uiAttribPartServer::fillPar( IOPar& iopar, bool is2d ) const
{
    curDescSet(is2d).fillPar( iopar );
}


void uiAttribPartServer::usePar( const IOPar& iopar, bool is2d )
{
    const uiRetVal uirv = curDescSet4Edit(is2d).usePar( iopar );
    if ( !uirv.isOK() )
	uimsg().error( uirv );
    else
    {
	set2DEvent( is2d );
	sendEvent( evNewAttrSet() );
    }
}


void uiAttribPartServer::setEvalBackupColTabMapper( const ColTab::Mapper* mp )
{
    if ( evalmapperbackup_ == mp )
	return;

    if ( evalmapperbackup_ )
    {
	evalmapperbackup_->unRef();
	evalmapperbackup_ = 0;
    }

    if ( mp )
    {
	evalmapperbackup_ = mp;
	evalmapperbackup_->ref();
    }
}


const ColTab::Mapper* uiAttribPartServer::getEvalBackupColTabMapper() const
{
    return evalmapperbackup_;
}


void uiAttribPartServer::survChangedCB( CallBacker* )
{
    deleteAndZeroPtr( manattribset2ddlg_ );
    deleteAndZeroPtr( manattribset3ddlg_ );
    deleteAndZeroPtr( attrsetdlg_ );
    deleteAndZeroPtr( impattrsetdlg_ );
    deleteAndZeroPtr( volattrdlg_);
    deleteAndZeroPtr( multiattrdlg_);
    deleteAndZeroPtr( dataattrdlg_);
}


void uiAttribPartServer::setSelAttr( const char* attrnm, bool isnewset )
{
    if ( attrsetdlg_ )
	attrsetdlg_->setSelAttr( attrnm, isnewset );
}


void uiAttribPartServer::loadDefaultAttrSet( const char* attrsetnm )
{
    if ( attrsetdlg_ )
	attrsetdlg_->loadDefaultAttrSet( attrsetnm );
}
