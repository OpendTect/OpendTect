/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattribpartserv.h"

#include "attribdataholder.h"
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
#include "attribsetcreator.h"
#include "attribstorprovider.h"

#include "arraynd.h"
#include "arrayndslice.h"
#include "arrayndwrapper.h"
#include "coltabmapper.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "executor.h"
#include "ioman.h"
#include "nlamodel.h"
#include "datapointset.h"
#include "rangeposprovider.h"
#include "randomlinegeom.h"
#include "seisbuf.h"
#include "seisdatapack.h"
#include "seistrc.h"
#include "seispreload.h"
#include "survinfo.h"
#include "settingsaccess.h"
#include "zdomain.h"

#include "uiattrdesced.h"
#include "uiattrdescseted.h"
#include "uiattrgetfile.h"
#include "uiattrsel.h"
#include "uiattrsetman.h"
#include "uiattr2dsel.h"
#include "uiattrvolout.h"
#include "uiattribcrossplot.h"
#include "uibutton.h"
#include "uibuttongroup.h"
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
#include "uicrossattrevaluatedlg.h"

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

static const int cMaxNrClasses = 100;
static const int cMaxMenuSize = 150;

const char* uiAttribPartServer::sKeyUserSettingAttrErrMsg()
{ return "dTect.Display attribute positioning error messages"; }


uiAttribPartServer::uiAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , dirshwattrdesc_(0)
    , attrsetdlg_(0)
    , is2devsent_(false)
    , attrsetclosetim_("Attrset dialog close")
    , multcomp3d_(uiStrings::s3D())
    , multcomp2d_(uiStrings::s2D())
    , dpsdispmgr_( 0 )
    , evalmapperbackup_( 0 )
    , attrsneedupdt_(true)
    , manattribset2ddlg_(0)
    , manattribsetdlg_(0)
    , needSaveNLA(this)
    , impattrsetdlg_(0)
    , volattrdlg_(0)
    , multiattrdlg_(0)
    , dataattrdlg_(0)
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

    delete attrsetdlg_;

    deepErase( attrxplotset_ );
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
	menutext = issteering ? mJoinUiStrs(sSteering(),sCube(mPlural)) :
				mJoinUiStrs(sStored(),sCube(mPlural));

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
    adsman->attrsetid_ = "";
    if ( attrsetdlg_ && attrsetdlg_->is2D()==is2d )
	attrsetdlg_->setDescSetMan( adsman );
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
					 const_cast<DescSetMan*>(adsman), 0 );
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
    attrsetclosetim_.start( 10, true );
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
    attrsetdlg_ = 0;
    sendEvent( evAttrSetDlgClosed() );
}


const NLAModel* uiAttribPartServer::getNLAModel( bool is2d ) const
{
    const NLAModel* nlamod
	= (NLAModel*)getObject( is2d ? objNLAModel2D() : objNLAModel3D() );
    return isEmpty( nlamod ) ? 0 : nlamod;
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
					info.ioObj() ? info.ioObj()->key() : 0,
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

	    selspec.set( 0, Attrib::SelSpec::cOtherAttrib(), false, 0 );
	    return prepMultCompSpecs( selectedcomps, dbky, false, true );
	}
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
	objref = ioobj ? ioobj->name().buf() : "";
	attrdata.attribid_.setStored( false );
    }

    selspec.set( 0, isnla ? DescID(attrdata.outputnr_,isstored)
			  : attrdata.attribid_, isnla, objref );
    if ( isnla && attrdata.nlamodel_ )
	selspec.setRefFromID( *attrdata.nlamodel_ );
    else if ( !isnla )
	selspec.setRefFromID( attrdata.attrSet() );
    //selspec.setZDomainKey( dlg.zDomainKey() );

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
    nms.append( attrinf.attrnms_ );
    nms.append( attrinf.ioobjids_ );
}


bool uiAttribPartServer::setSaved( bool is2d ) const
{
    return DSHolder().getDescSetMan( is2d )->isSaved();
}


int uiAttribPartServer::use3DMode() const
{
    const DescSet* ads = getUserPrefDescSet();
    if ( !ads ) return -1;
    return DSHolder().getDescSetMan(true)
		&& ads==DSHolder().getDescSet(true,false) ? 0 : 1;
}


const Attrib::DescSet* uiAttribPartServer::getUserPrefDescSet() const
{
    const DescSet* ds3d = DSHolder().getDescSet( false, false );
    const DescSet* ds2d = DSHolder().getDescSet( true, false );
    if ( !ds3d && !ds2d ) return 0;
    if ( !(ds3d && ds2d) ) return ds3d ? ds3d : ds2d;
    if ( !SI().has3D() ) return ds2d;
    if ( !SI().has2D() ) return ds3d;

    const int nr3d = ds3d->nrDescs( false, true );
    const int nr2d = ds2d->nrDescs( false, true );
    if ( (nr3d>0) != (nr2d>0) ) return nr2d > 0 ? ds2d : ds3d;

    const int res =
	uiMSG().ask2D3D( tr("Which attributes do you want to use?"), true );
    if ( res == -1 ) return 0;
    return res == 1 ? ds2d : ds3d;
}


void uiAttribPartServer::saveSet( bool is2d )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(AttribDescSet);
    ctio->ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( parent(), *ctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	ctio->ioobj_ = 0;
	ctio->setObj( dlg.ioObj()->clone() );
	uiString bs;
	if ( !ctio->ioobj_ )
	    uiMSG().error(tr("Cannot find attribute set in data base"));
	else if (
	    !AttribDescSetTranslator::store(*DSHolder().getDescSet(is2d,false),
					      ctio->ioobj_,bs) )
	    uiMSG().error(bs);
    }
    ctio->setObj( 0 );
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
	{ pErrMsg("Nothing to do"); return 0; }

    const bool istargetstored = targetspecs_[0].isStored();
    const bool is2d = targetspecs_[0].is2D();
    DescSet* curdescset = eDSHolder().getDescSet(is2d,istargetstored);
    if ( !curdescset )
	{ pErrMsg("No attr set"); return 0; }

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
		    return 0;
	    }
	}
    }

    EngineMan* aem = new EngineMan;
    aem->setAttribSet( curdescset );
    aem->setNLAModel( getNLAModel(is2d) );
    aem->setAttribSpecs( targetspecs_ );
    if ( tkzs )
	aem->setTrcKeyZSampling( *tkzs );
    aem->setGeomID( geomid );

    return aem;
}


DataPack::ID uiAttribPartServer::createOutput( const TrcKeyZSampling& tkzs,
					       DataPack::ID cacheid )
{
    if ( tkzs.hsamp_.survid_ == Survey::GM().get2DSurvID() )
    {
	uiTaskRunner taskrunner( parent() );
	const Pos::GeomID& geomid = tkzs.hsamp_.trcKeyAt(0).geomID();
	return create2DOutput( tkzs, geomid, taskrunner );
    }

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    ConstDataPackRef<RegularSeisDataPack> cache = dpm.obtain( cacheid );
    const RegularSeisDataPack* newpack = createOutput( tkzs, cache.ptr() );
    if ( !newpack ) return DataPack::cNoID();

    dpm.add( const_cast<RegularSeisDataPack*>(newpack) );
    return newpack->id();
}


static const Desc* getTargetDesc( const TypeSet<Attrib::SelSpec>& targetspecs )
{
    if ( targetspecs.isEmpty() )
	return 0;

    const bool isstortarget = targetspecs[0].isStored();
    const bool is2d = targetspecs[0].is2D();
    const DescSet* attrds = DSHolder().getDescSet( is2d, isstortarget );
    const Desc* targetdesc = !attrds || attrds->isEmpty() ? 0
				: attrds->getDesc( targetspecs[0].id() );
    return targetdesc;
}


const RegularSeisDataPack* uiAttribPartServer::createOutput(
				const TrcKeyZSampling& tkzs,
				const RegularSeisDataPack* cache )
{
    PtrMan<EngineMan> aem = createEngMan( &tkzs, 0 );
    if ( !aem ) return 0;

    bool atsamplepos = true;

    const Desc* targetdesc = getTargetDesc( targetspecs_ );
    RegularSeisDataPack* preloadeddatapack = nullptr;
    if ( targetdesc )
    {
	if ( targetdesc->isStored() && !targetspecs_[0].isNLA() )
	{
	    const MultiID mid( targetdesc->getStoredID() );
	    mDynamicCast( RegularSeisDataPack*, preloadeddatapack,
						Seis::PLDM().get(mid) );
	}

	BufferString defstr;
	targetdesc->getDefStr( defstr );
	if ( defstr != targetspecs_[0].defString() )
	    cache = 0;

	const bool isz = tkzs.isFlat()&&tkzs.defaultDir() == TrcKeyZSampling::Z;
	if ( !preloadeddatapack && isz )
	{
	    uiString errmsg;
	    Desc* nonconsttargetdesc = const_cast<Desc*>( targetdesc );
	    RefMan<Provider> tmpprov =
			Provider::create( *nonconsttargetdesc, errmsg );
	    if ( !tmpprov ) return 0;

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
    PtrMan<Processor> process = 0;
    RegularSeisDataPack* output = 0;
    if ( !preloadeddatapack && !atsamplepos )//note: 1 attrib computed at a time
    {
	if ( !targetdesc ) return 0;
	Pos::RangeProvider3D rgprov3d;
	rgprov3d.setSampling( tkzs );
	DataColDef* dtcd = new DataColDef( targetdesc->userRef() );
	ManagedObjectSet<DataColDef> dtcoldefset;
	dtcoldefset += dtcd;
	uiTaskRunner taskrunner( parent() );
	DataPointSet posvals( rgprov3d.is2D() );
	if ( !posvals.extractPositions(rgprov3d,dtcoldefset,0,&taskrunner) )
	    return 0;

	const int firstcolidx = 0;

	uiString errmsg;
	process = aem->getTableOutExecutor( posvals, errmsg, firstcolidx );
	if ( !process )
	    { uiMSG().error(errmsg); return 0; }

	if ( !TaskRunner::execute( &taskrunner, *process ) )
	    return 0;

	TypeSet<float> vals;
	posvals.bivSet().getColumn( posvals.nrFixedCols()+firstcolidx, vals,
				    true );
	if ( !vals.isEmpty() )
	{
	    ArrayValueSeries<float, float> avs( vals.arr(), false, vals.size());
	    output = new RegularSeisDataPack(
				SeisDataPack::categoryStr(false,false) );
	    output->setSampling( tkzs );
	    if ( !output->addComponent(targetspecs_[0].userRef()) ||
		    !output->data(0).getStorage() )
	    {
		delete output;
		output = 0;
	    }
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

	TrcKeyZSampling posstkzs( tkzs );
	if ( !targetdesc || !targetdesc->descSet() )
	    return nullptr;

	PtrMan<DescSet> targetdescset =
		       targetdesc->descSet()->optimizeClone( targetdesc->id() );
	const bool haspossvol = aem->getPossibleVolume( *targetdescset,
					posstkzs, nullptr, targetdesc->id() );
	if ( !haspossvol )
	    return nullptr;

	uiString errmsg;
	process = aem->createDataPackOutput( errmsg, cache );
	if ( !process )
	    { uiMSG().error(errmsg); return 0; }

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
	    targetdesc && targetdesc->isStored() && !targetspecs_[0].isNLA();
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
		success = TaskRunner::execute( &taskrunner, *process );
	    }
	    else
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		if ( !process->execute() )
		{
		    const uiString msg( process->uiMessage() );
		    if ( !msg.isEmpty() )
			uiMSG().error( msg );
		    return 0;
		}
	    }
	}

	output = const_cast<RegularSeisDataPack*>(
			aem->getDataPackOutput(*process) );
    }

    if ( output && !success )
    {
	if ( !uiMSG().askGoOn(tr("Attribute loading/calculation aborted.\n"
	    "Do you want to use the partially loaded/computed data?"), true ) )
	{
	    delete output;
	    output = 0;
	}
    }

    return output;
}


bool uiAttribPartServer::createOutput( DataPointSet& posvals, int firstcol )
{
    const Desc* targetdesc = getTargetDesc( targetspecs_ );
    if ( targetdesc && targetdesc->isStored() )
    {
	const MultiID mid( targetdesc->getStoredID() );
	mDynamicCastGet(RegularSeisDataPack*,sdp,Seis::PLDM().get(mid))
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
		filler.setSampling( &sdp->sampling() );
		return TaskRunner::execute( &uitr, filler );
	    }
	}
    }

    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return false;

    uiString errmsg;
    PtrMan<Processor> process =
			aem->getTableOutExecutor( posvals, errmsg, firstcol );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiTaskRunner taskrunner( parent() );
    if ( !TaskRunner::execute( &taskrunner, *process ) ) return false;

    posvals.setName( targetspecs_[0].userRef() );
    return true;
}


bool uiAttribPartServer::createOutput( ObjectSet<DataPointSet>& dpss,
				       int firstcol )
{
    ExecutorGroup execgrp( "Calculating Attribute", true );
    uiString errmsg;

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
    res = TaskRunner::execute( &taskrunner, execgrp );

    deepErase( aems );
    return res;
}


DataPack::ID uiAttribPartServer::createRdmTrcsOutput(
	const Interval<float>& zrg, int rdlid )
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
	const MultiID mid( targetdesc->getStoredID() );
	mDynamicCastGet( RegularSeisDataPack*,sdp,Seis::PLDM().get(mid) );
	if ( sdp )
	{
	    BufferStringSet componentnames;
	    for ( int idx=0; idx<targetspecs_.size(); idx++ )
		componentnames.add( targetspecs_[idx].userRef() );

	    return RandomSeisDataPack::createDataPackFrom( *sdp, rdlid, zrg,
							   &componentnames );
	}
    }

    TypeSet<BinID> knots, path;
    rdmline->allNodePositions( knots );
    rdmline->getPathBids( knots, path );

    if ( path.isEmpty() )
	return DataPack::cNoID();

    snapToValidRandomTraces( path, targetdesc );

    TrcKeyPath trckeys;
    for ( int idx=0; idx<path.size(); idx++ )
	trckeys += Survey::GM().traceKey( Survey::GM().default3DSurvID(),
					  path[idx].inl(), path[idx].crl() );

    BinIDValueSet bidset( 2, false );
    for ( int idx = 0; idx<path.size(); idx++ )
	bidset.add( path[idx],zrg.start,zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput(bidset,output,&knots,&path) || output.isEmpty() )
	return DataPack::cNoID();

    RandomSeisDataPack* newpack = new RandomSeisDataPack(
				SeisDataPack::categoryStr(true,false) );
    newpack->setRandomLineID( rdlid );
    newpack->setPath( trckeys );
    newpack->setZRange( output.get(0)->zRange() );
    for ( int idx=0; idx<output.get(0)->nrComponents(); idx++ )
    {
	if ( !newpack->addComponent(targetspecs_[idx].userRef()) )
	    continue;

	for ( int idy=0; idy<newpack->data(idx).info().getSize(1); idy++ )
	{
	    const int trcidx = output.find( path[idy] );
	    const SeisTrc* trc = trcidx<0 ? 0 : output.get( trcidx );
	    if ( !trc ) continue;
	    for ( int idz=0; idz<newpack->data(idx).info().getSize(2);idz++)
		newpack->data(idx).set( 0, idy, idz, trc->get(idz,idx) );
	}
    }

    newpack->setZDomain(
	    ZDomain::Info(ZDomain::Def::get(targetspecs_[0].zDomainKey())));
    newpack->setName( targetspecs_[0].userRef() );
    DPM(DataPackMgr::SeisID()).add( newpack );
    return newpack->id();
}


void uiAttribPartServer::snapToValidRandomTraces( TypeSet<BinID>& path,
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


DataPack::ID uiAttribPartServer::createRdmTrcsOutput(
				const Interval<float>& zrg,
				TypeSet<BinID>* path,
				TypeSet<BinID>* trueknotspos )
{
    const bool isstortarget = targetspecs_.size() && targetspecs_[0].isStored();
    const DescSet* attrds = DSHolder().getDescSet(false,isstortarget);
    const Desc* targetdesc = !attrds || attrds->isEmpty() ? 0
	: attrds->getDesc(targetspecs_[0].id());

    TrcKeyPath trckeys;
    for ( int idx=0; idx<path->size(); idx++ )
	trckeys += Survey::GM().traceKey( Survey::GM().default3DSurvID(),
				       (*path)[idx].inl(), (*path)[idx].crl() );

    const MultiID mid( targetdesc->getStoredID() );
    mDynamicCastGet( RegularSeisDataPack*,sdp,Seis::PLDM().get(mid) );
    if ( sdp )
    {
	BufferStringSet componentnames;
	for ( int idx=0; idx<targetspecs_.size(); idx++ )
	    componentnames.add( targetspecs_[idx].userRef() );

	return RandomSeisDataPack::createDataPackFrom( *sdp, trckeys, zrg,
						       &componentnames );
    }

    BinIDValueSet bidset( 2, false );
    for ( int idx = 0; idx<path->size(); idx++ )
	bidset.add( ( *path )[idx],zrg.start,zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput(bidset,output,trueknotspos,path) )
	return DataPack::cNoID();

    RandomSeisDataPack* newpack = new RandomSeisDataPack(
				SeisDataPack::categoryStr(true,false) );
    newpack->setPath( trckeys );
    newpack->setZRange( output.get(0)->zRange() );
    for ( int idx=0; idx<output.get(0)->nrComponents(); idx++ )
    {
	if ( !newpack->addComponent(targetspecs_[idx].userRef()) )
	    continue;

	for ( int idy=0; idy<newpack->data(idx).info().getSize(1); idy++ )
	{
	    const int trcidx = output.find( (*path)[idy] );
	    const SeisTrc* trc = trcidx<0 ? 0 : output.get( trcidx );
	    if ( !trc ) continue;
	    for ( int idz=0; idz<newpack->data(idx).info().getSize(2);idz++)
		newpack->data(idx).set( 0, idy, idz, trc->get(idz,idx) );
	}
    }

    newpack->setZDomain(
	    ZDomain::Info(ZDomain::Def::get(targetspecs_[0].zDomainKey())));
    newpack->setName( targetspecs_[0].userRef() );
    DPM(DataPackMgr::SeisID()).add( newpack );
    return newpack->id();
}


bool uiAttribPartServer::createOutput( const BinIDValueSet& bidset,
				       SeisTrcBuf& output,
				       TypeSet<BinID>* trueknotspos,
				       TypeSet<BinID>* snappedpos )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return 0;

    uiString errmsg;
    PtrMan<Processor> process = aem->createTrcSelOutput( errmsg, bidset,
							 output, mUdf(float), 0,
							 trueknotspos,
							 snappedpos );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

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
				 const BufferStringSet& compnames,
				 DataPack::ID& outputid )
    : input_(input)
    , sampling_(input.getTrcKeyZSampling())
    , zdef_(zdef)
    , compnames_(compnames)
    , refnrs_(sampling_.hsamp_.nrTrcs(),mUdf(float))
    , outputid_(outputid)
    , outputdp_(0)
{
    sampling_.hsamp_.setLineRange( StepInterval<int>(geomid,geomid,1) );
}

od_int64 nrIterations() const		{ return input_.trcinfoset_.size(); }

bool doPrepare( int nrthreads )
{
    if ( input_.trcinfoset_.isEmpty() ||
	    sampling_.hsamp_.survid_!=Survey::GM().get2DSurvID() )
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
	    const int trcidx = sampling_.hsamp_.trcIdx( trcinfoset[tidx]->nr );
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

    const Data2DHolder&			input_;
    TrcKeyZSampling			sampling_;
    const ZDomain::Def&			zdef_;
    const BufferStringSet&		compnames_;
    RegularSeisDataPack*		outputdp_;
    DataPack::ID&			outputid_;
    TypeSet<float>			refnrs_;
};

DataPack::ID uiAttribPartServer::create2DOutput( const TrcKeyZSampling& tkzs,
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
	    const MultiID mid( targetdesc->getStoredID() );
	    mDynamicCastGet(const RegularSeisDataPack*,regsdp,
			    Seis::PLDM().get(mid,geomid) );
	    if ( regsdp ) return regsdp->id();
	}
    }

    PtrMan<EngineMan> aem = createEngMan( &tkzs, geomid );
    if ( !aem ) return DataPack::cNoID();

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

    return createDataPackFor2D( *data2d, tkzs,
	    ZDomain::Def::get(targetspecs_[0].zDomainKey()), userrefs );
}


DataPack::ID uiAttribPartServer::createDataPackFor2D(
					const Attrib::Data2DHolder& input,
					const TrcKeyZSampling& outputsampling,
					const ZDomain::Def& zdef,
					const BufferStringSet& compnames )
{
    DataPack::ID outputid = DataPack::cNoID();
    RegularSeisDataPackCreatorFor2D datapackcreator(
		input, outputsampling.hsamp_.getGeomID(), zdef,
		compnames, outputid );
    datapackcreator.execute();
    return outputid;
}


bool uiAttribPartServer::extractData( ObjectSet<DataPointSet>& dpss )
{
    if ( dpss.isEmpty() ) { pErrMsg("No inp data"); return 0; }
    const DescSet* ads = DSHolder().getDescSet( dpss[0]->is2D(), false );
    if ( !ads ) { pErrMsg("No attr set"); return 0; }

    EngineMan aem;
    uiTaskRunner taskrunner( parent() );
    bool somesuccess = false;
    bool somefail = false;

    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	uiString err;
	DataPointSet& dps = *dpss[idx];
	Executor* tabextr = aem.getTableExtractor( dps, *ads, err );
	if ( !tabextr ) { pErrMsg(err.getFullString()); return 0; }

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
    return ds ? ds->getStoredID( multiid, selout ) : DescID::undef();
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
	const BufferStringSet* ids, const char* cursel,
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
	MenuItem* itm = new MenuItem( toUiString(nm) );
	itm->checkable = true;
	if ( ids && Seis::PLDM().isPresent(MultiID(ids->get(idx))) )
	    itm->iconfnm = "preloaded";
	const bool docheck = correcttype && nm == selnm;
	if ( docheck ) checkparent = true;
	mAddMenuItem( &mnu, itm, true, docheck );
    }

    if ( checkparent ) mnu.checked = true;
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
    const Attrib::Desc* desc = 0;
    if ( ds && ds->getDesc(as.id()) )
	desc = ds->getDesc( as.id() );
    else if ( nonstoredds && nonstoredds->getDesc(as.id()) )
	desc = nonstoredds->getDesc( as.id() );
    const SelInfo attrinf( ds, 0, is2d, DescID::undef(),
			   issteer, issteer, multcomp );

    const bool isstored = desc ? desc->isStored() : false;
    const BufferStringSet bfset =
		issteer ? attrinf.steerids_ : attrinf.ioobjids_;

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
	const bool correcttype = desc ? isstored : true;
	const int start = 0; const int stop = nritems;
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

	    SelInfo attrinf( DSHolder().getDescSet(false,true), 0, false,
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
    SelInfo attrinf( DSHolder().getDescSet(is2d,false), 0, is2d );
    const bool isattrib = attrinf.attrids_.isPresent( as.id() );

    const int start = 0; const int stop = attrinf.attrnms_.size();
    MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    uiString txt = useext ? ( is2d ? tr("Attributes 2D")
				   : tr("Attributes 3D") )
			  : uiStrings::sAttribute(mPlural);
    calcmnuitem->text = txt;
    insertItems( *calcmnuitem, attrinf.attrnms_, 0, as.userRef(),
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
	insertItems( *nlamnuitem, attrinf.nlaoutnms_, 0, as.userRef(),
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
	MenuItem& subitem, const Attrib::SelSpec& as, int geomid,
	bool isstored, int steerpol )
{
    if ( geomid == Survey::GM().cUndefGeomID() )
	return;

    BufferStringSet childitemnms;
    for ( int idx=0; idx<subitem.nrItems(); idx++ )
	childitemnms.add( subitem.getItem(idx)->text.getFullString() );

    subitem.removeItems();
    FixedString linenm( Survey::GM().getName(geomid) );
    BufferStringSet attribnms;
    uiSeisPartServer::get2DStoredAttribs( linenm, attribnms, steerpol );
    for ( int idx=0; idx<childitemnms.size(); idx++ )
    {
	FixedString childnm( childitemnms.get(idx).buf() );
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

	    MultiID mid( desc->getStoredID(true) );
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
	return selectAttrib( as, 0, Survey::GM().cUndefGeomID(),
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
	multiid = attrinf.ioobjids_.get(idx);
	attribid = eDSHolder().getDescSet(false,true)->getStoredID( multiid );
	isstored = true;
    }
    else if ( steering3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = steering3dmnuitem_.findItem( mnuid );
	const int idx = attrinf.steernms_.indexOf( item->text.getFullString() );
	multiid = attrinf.steerids_.get( idx );
	attribid = eDSHolder().getDescSet(false,true)->getStoredID( multiid );
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
	attribid =
	    eDSHolder().getDescSet(true,true)->getStoredID( multiid, selout );
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
	IOM().to(MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id_));
	PtrMan<IOObj> ioobj = IOM().getLocal( item->text.getFullString(), 0 );
	if ( ioobj )
	{
	    multiid = ioobj->key();
	    attribid = eDSHolder().getDescSet(false,true)->getStoredID(multiid);
	    isstored = true;
	}
    }
    else
	return false;

    const bool nocompsel = is2d && issteering;
    if ( isstored && !nocompsel )
    {
	BufferStringSet complist;
	SeisIOObjInfo::getCompNames( multiid.buf(), complist );
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
    as.set( 0, did, isnla, objref );

    BufferString bfs;
    if ( attribid.asInt() != SelSpec::cAttribNotSel().asInt() )
    {
	const DescSet* attrset = DSHolder().getDescSet(is2d, isstored);
	const Desc* desc = attrset ? attrset->getDesc( attribid ) : 0;
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
	complist.add( BufferString(uiStrings::sInlineDip().getFullString() ) );
	complist.add( BufferString(uiStrings::sCrosslineDip().getFullString()));
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
			->getStoredID( multiid, selectedcomps[0], false );
	    //Trick for old steering cubes: fake good component names
	    if ( !is2d && issteering )
	    {
		Desc* desc = ads->getDesc(attribid);
		if ( !desc ) return false;
		mFakeCompName( "Component 1",
		    BufferString( uiStrings::sInlineDip().getFullString() ) );
		mFakeCompName( "Component 2",
		    BufferString( uiStrings::sCrosslineDip().getFullString() ));
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
	SelSpec as( 0, did );
	BufferString bfs;
	Desc* desc = ads->getDesc(did);
	if ( !desc ) return false;

	desc->getDefStr(bfs);
	as.setDefString(bfs.buf());
	//Trick for old steering cubes: fake good component names
	if ( !is2d && issteering )
	{
	    mFakeCompName( "Component 1",
		    BufferString( uiStrings::sInlineDip().getFullString() ) );
	    mFakeCompName( "Component 2",
		    BufferString( uiStrings::sCrosslineDip().getFullString() ));
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
    if ( as.isNLA() ) return 0;

    const DescSet* attrset = DSHolder().getDescSet( as.is2D(), true );
    const Desc* desc = attrset ? attrset->getDesc( as.id() ) : 0;
    if ( !desc )
    {
	attrset = DSHolder().getDescSet( as.is2D(), false );
	desc = attrset ? attrset->getDesc( as.id() ) : 0;
	if ( !desc )
	    return 0;
    }

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
	mErrRet( tr("Please add this attribute first") );

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
	uiStringSet errmsgs;
	const int odversion = iopar.odVersion();
	if ( isstored && odversion<411 )	//backward compatibility v<4.1.1
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
	    ads->usePar( iopar, &errmsgs );

	if ( !errmsgs.isEmpty() )
	{
	    uiString basemsg = tr("Error during restore of %1 Attribute Set")
			     .arg(is2d ? uiStrings::s2D()
				       : uiStrings::s3D());
	uiMSG().errorWithDetails( errmsgs, basemsg );
	}

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


void uiAttribPartServer::survChangedCB( CallBacker* )
{
    deleteAndZeroPtr( manattribset2ddlg_ );
    deleteAndZeroPtr( manattribsetdlg_ );
    deleteAndZeroPtr( attrsetdlg_ );
    deleteAndZeroPtr( impattrsetdlg_ );
    deleteAndZeroPtr( volattrdlg_);
    deleteAndZeroPtr( multiattrdlg_);
    deleteAndZeroPtr( dataattrdlg_);
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
