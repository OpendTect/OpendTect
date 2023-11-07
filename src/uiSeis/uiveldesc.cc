/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "od_helpids.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uimsg.h"
#include "uistaticsdesc.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiunitsel.h"
#include "uizrangeinput.h"


// uiVelocityDesc::Setup

uiVelocityDesc::Setup::Setup( const IOObj* ioobj, bool is2d, bool onlyvelocity )
    : is2d_(is2d)
    , onlyvelocity_(onlyvelocity)
{
    if ( ioobj )
	desc_.usePar( ioobj->pars() );
}


uiVelocityDesc::Setup::~Setup()
{
}


// uiVelocityDesc

uiVelocityDesc::uiVelocityDesc( uiParent* p, const uiVelocityDesc::Setup& vsu )
    : uiGroup( p, "Velocity type selector" )
    , vsu_(vsu)
    , veltypedef_(Vel::TypeDef())
    , unitChanged(this)
{
    const EnumDefImpl<Vel::Type>& typdef = Vel::TypeDef();
    if ( vsu.onlyvelocity_ )
    {
	veltypedef_.remove( typdef.getKey(Vel::Delta) );
	veltypedef_.remove( typdef.getKey(Vel::Epsilon) );
	veltypedef_.remove( typdef.getKey(Vel::Eta) );
    }

    typefld_ = new uiGenInput( this, tr("Velocity type"),
			       StringListInpSpec(veltypedef_) );
    mAttachCB( typefld_->valueChanged, uiVelocityDesc::typeChgCB );

    unitchkfld_ = new uiCheckBox( this, uiString::empty(),
			mCB(this,uiVelocityDesc,unitCheckCB) );
    unitchkfld_->setMaximumWidth( 16 );
    unitchkfld_->attach( rightOf, typefld_ );

    uiUnitSel::Setup uiusu( Mnemonic::Vel, tr("in"),
			    &Mnemonic::defVEL() );
    uiusu.mode( uiUnitSel::Setup::SymbolsOnly );
    unitfld_ = new uiUnitSel( this, uiusu );
    unitfld_->attach( rightOf, unitchkfld_ );

    auto* vigrp = new uiGroup( this, "Vel info grp" );
    hasstaticsfld_ = new uiGenInput( vigrp, tr("Has statics"),
				     BoolInpSpec(true) );
    mAttachCB( hasstaticsfld_->valueChanged, uiVelocityDesc::hasStaticsChgCB );

    staticsfld_ = new uiStaticsDesc( vigrp );
    staticsfld_->attach( alignedBelow, hasstaticsfld_ );
    vigrp->setHAlignObj( hasstaticsfld_ );
    vigrp->attach( alignedBelow, typefld_ );

    setHAlignObj( typefld_ );
    mAttachCB( postFinalize(), uiVelocityDesc::initGrpCB );
}


uiVelocityDesc::~uiVelocityDesc()
{
    detachAllNotifiers();
}


void uiVelocityDesc::initGrpCB( CallBacker* )
{
    set( vsu_.desc_ );
}


Vel::Type uiVelocityDesc::getType() const
{
    const int curtypeidx = typefld_->getIntValue();
    return veltypedef_.getEnumForIndex( curtypeidx );
}


void uiVelocityDesc::setType( Vel::Type type )
{
    const BufferString typestr( Vel::TypeDef().getKey(type) );
    if ( !veltypedef_.keys().isPresent(typestr) )
	return;

    const int typeidx = veltypedef_.indexOf( typestr.str() );
    typefld_->setValue( typeidx );
}


NotifierAccess& uiVelocityDesc::typeChangeNotifier()
{
    return typefld_->valueChanged;
}


void uiVelocityDesc::typeChgCB( CallBacker* )
{
    const Vel::Type type = getType();
    const bool dispunit = VelocityDesc::isUdf( type ) ||
			  VelocityDesc::isVelocity( type );
    unitchkfld_->display( dispunit );
    if ( dispunit )
    {
	NotifyStopper ns( unitchkfld_->activated );
	unitchkfld_->setChecked( wasguessed_ );
	uiVelocityDesc::unitCheckCB( nullptr );
    }

    unitfld_->display( dispunit );
    const bool isvrms = type==Vel::RMS;
    hasstaticsfld_->display( isvrms );
    hasStaticsChgCB( nullptr );
}


void uiVelocityDesc::unitCheckCB( CallBacker* )
{
    const bool cansetunit = unitchkfld_->isChecked();
    unitfld_->setSensitive( cansetunit );
}


void uiVelocityDesc::hasStaticsChgCB( CallBacker* )
{
    const bool displaystatics = hasstaticsfld_->isDisplayed() &&
				hasstaticsfld_->getBoolValue();
    staticsfld_->display( displaystatics );
}


void uiVelocityDesc::set( const VelocityDesc& desc )
{
    NotifyStopper ns( typefld_->valueChanged, this );
    NotifyStopper nschk( unitchkfld_->activated );
    setType( desc.type_ );
    if ( desc.isVelocity() || desc.isUdf() )
    {
	const bool hasunit = desc.hasVelocityUnit();
	unitchkfld_->setChecked( !hasunit );
	unitfld_->setUnit( desc.getUnit() );
	wasguessed_ = !hasunit;
    }

    hasstaticsfld_->setValue( !desc.statics_.horizon_.isUdf() );
    staticsfld_->set( desc.statics_ );
    typeChgCB( nullptr );
}


bool uiVelocityDesc::isVelocity() const
{
    const Vel::Type type = getType();
    return VelocityDesc::isVelocity( type );
}


bool uiVelocityDesc::get( VelocityDesc& res, bool disperr ) const
{
    res.type_ = getType();
    if ( res.isUdf() )
    {
	if ( disperr )
	    uiMSG().error( tr("Please specify the velocity type") );
	return false;
    }

    if ( !res.isRMS() || !hasstaticsfld_->getBoolValue() )
    {
	res.statics_.horizon_.setUdf();
	res.statics_.vel_ = mUdf(float);
	res.statics_.velattrib_.setEmpty();
    }
    else
    {
	if ( !staticsfld_->get(res.statics_,disperr) )
	    return false;
    }

    if ( unitfld_->isDisplayed() )
    {
	const UnitOfMeasure* veluom = UoMR().get( unitfld_->getUnitName() );
	res.setUnit( veluom );
    }

    return true;
}


bool uiVelocityDesc::updateAndCommit( IOObj& ioobj, bool disperr )
{
    VelocityDesc desc;
    if ( !get(desc,disperr) )
	return false;

    uiString errmsg = tr("Cannot write velocity information");
    if ( desc.isUdf() )
    {
	VelocityDesc::removePars( ioobj.pars() );
	if ( !IOM().commitChanges(ioobj) )
	{
	    if ( disperr )
		uiMSG().error( errmsg );

	    return false;
	}
    }
    else
    {
	desc.fillPar( ioobj.pars() );
	if ( !IOM().commitChanges(ioobj) )
	{
	    if ( disperr )
		uiMSG().error( errmsg );

	    return false;
	}
    }

    return true;
}


// uiVelocityDescDlg

uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p, const IOObj* sel,
				      const uiVelocityDesc::Setup& vsu )
    : uiDialog( p, uiDialog::Setup(tr("Specify velocity information"),
				    mNoDlgTitle, mODHelpKey(mVelocityDescDlg)))
    , toprange_(Interval<float>::udf())
    , bottomrange_(Interval<float>::udf())
{
    const Seis::GeomType gt = vsu.is2d_ ? Seis::Line : Seis::Vol;
    uiSeisSel::Setup ssu( gt ); ssu.seltxt( tr("Velocity cube") );
    volselfld_ = new uiSeisSel( this, uiSeisSel::ioContext(gt,true),
				ssu );
    if ( sel )
	volselfld_->setInput( *sel );

    mAttachCB( volselfld_->selectionDone, uiVelocityDescDlg::volSelChange );

    veldescfld_ = new uiVelocityDesc( this, vsu );
    veldescfld_->attach( alignedBelow, volselfld_ );
    oldveldesc_ = vsu.desc_;

    mAttachCB( postFinalize(), uiVelocityDescDlg::initDlgCB );
}


uiVelocityDescDlg::~uiVelocityDescDlg()
{
    detachAllNotifiers();
}


const UnitOfMeasure* uiVelocityDescDlg::velUnit() const
{
    VelocityDesc desc;
    return veldescfld_->get( desc, false ) ? desc.getUnit() : nullptr;
}


void uiVelocityDescDlg::initDlgCB( CallBacker* )
{
    volSelChange( nullptr );
}


void uiVelocityDescDlg::volSelChange( CallBacker* cb )
{
    const IOObj* ioobj = volselfld_->ioobj( true );
    if ( ioobj )
    {
	if ( cb == volselfld_ && !oldveldesc_.usePar(ioobj->pars()) )
	    oldveldesc_.type_ = Vel::Unknown;

	const IOPar& par = ioobj->pars();
	if ( !VelocityStretcher::getRange(par,oldveldesc_,true,toprange_) )
	    toprange_.setUdf();

	if ( !VelocityStretcher::getRange(par,oldveldesc_,false,bottomrange_) )
	    bottomrange_.setUdf();
    }

    if ( cb == volselfld_ )
	veldescfld_->set( oldveldesc_ );
}


bool uiVelocityDescDlg::doScanVels( const IOObj& ioobj,
				    const VelocityDesc& desc, bool writergs,
				    Interval<float>& topvelrg,
				    Interval<float>& botvelrg,
				    TaskRunner* taskrunner )
{
    VelocityModelScanner scanner( ioobj, desc );
    if ( !TaskRunner::execute(taskrunner,scanner) )
	return false;

    topvelrg = scanner.getTopVAvg();
    botvelrg = scanner.getBotVAvg();
    if ( writergs )
    {
	VelocityStretcher::setRange( topvelrg, desc, true, ioobj.pars() );
	VelocityStretcher::setRange( botvelrg, desc, false, ioobj.pars() );
	if ( !IOM().commitChanges(ioobj) )
	    return false;
    }

    return true;
}


bool uiVelocityDescDlg::scanAvgVel( const IOObj& ioobj,
				    const VelocityDesc& desc )
{
    uiTaskRunner taskrunner( this );
    return doScanVels( ioobj, desc, false, toprange_, bottomrange_,
		       &taskrunner );
}


bool uiVelocityDescDlg::acceptOK( CallBacker* )
{
    const IOObj* ioobj = volselfld_->ioobj( true );
    if ( !ioobj )
    {
	uiMSG().error(tr("Please select a valid velocity cube."));
	return false;
    }

    VelocityDesc desc;
    if ( !veldescfld_->get(desc,true) )
	return false;

    if ( oldveldesc_==desc )
	return true;

    if ( desc.isVelocity() && !scanAvgVel(*ioobj,desc) )
	return false;

    VelocityStretcher::setRange( toprange_, desc, true, ioobj->pars() );
    VelocityStretcher::setRange( bottomrange_, desc, false, ioobj->pars() );

    return veldescfld_->updateAndCommit( *cCast(IOObj*,ioobj), true );
}


MultiID uiVelocityDescDlg::getSelection() const
{
    MultiID ret;
    PtrMan<IOObj> ioobj = volselfld_->getIOObj( true );
    if ( ioobj )
	ret = ioobj->key();

    return ret;
}


bool uiVelocityDescDlg::isVelocity() const
{
    return veldescfld_->isVelocity();
}


// uiVelSel

uiVelSel::uiVelSel( uiParent* p, const uiString& lbltxt,
		    bool is2d, bool enabotherdomain )
    : uiVelSel(p,ioContext(is2d),
	    (uiSeisSel::Setup&)uiSeisSel::Setup(is2d ? Seis::Line : Seis::Vol)
					.enabotherdomain(enabotherdomain)
					.seltxt(lbltxt))
{
}


uiVelSel::uiVelSel( uiParent* p, const IOObjContext& ctxt,
		    const uiSeisSel::Setup& setup )
    : uiSeisSel(p,ctxt,setup)
    , trg_(Interval<float>::udf())
    , brg_(Interval<float>::udf())
    , velChanged(this)
{
    seissetup_.allowsetsurvdefault( true );
    editcubebutt_ = new uiPushButton( this, uiString::empty(),
			    mCB(this,uiVelSel,editCB), false );
    editcubebutt_->attach( rightOf, endObj(false) );
    mAttachCB( selectionDone, uiVelSel::selectionDoneCB );

    mAttachCB( postFinalize(), uiVelSel::initGrpCB );
}


uiVelSel::~uiVelSel()
{
    detachAllNotifiers();
}


void uiVelSel::initGrpCB( CallBacker* )
{
    selectionDoneCB( nullptr );
    if ( trg_.isUdf() || brg_.isUdf() )
    {
	const UnitOfMeasure* veluom = velUnit();
	const Interval<float> defvelrg =
				VelocityStretcher::getDefaultVAvg( veluom );
	if ( trg_.isUdf() )
	    trg_.set( defvelrg.start, defvelrg.start );
	if ( brg_.isUdf() )
	    brg_.set( defvelrg.stop, defvelrg.stop );

	velChanged.trigger();
    }
}


const IOObjContext& uiVelSel::ioContext( bool is2d )
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, velctxt, = nullptr );
    mDefineStaticLocalObject( PtrMan<IOObjContext>, linectxt, = nullptr );
    convertLegacyTypes();
    if ( is2d && !linectxt )
    {
	auto* newctxt =
		new IOObjContext( uiSeisSel::ioContext(Seis::Line,true) );
	newctxt->toselect_.require_.set( sKey::Type(), sKey::Velocity() );
	linectxt.setIfNull( newctxt, true );
    }
    else if ( !is2d && !velctxt )
    {
	auto* newctxt =
		new IOObjContext( uiSeisSel::ioContext(Seis::Vol,true) );
	newctxt->toselect_.require_.set( sKey::Type(), sKey::Velocity() );
	velctxt.setIfNull( newctxt, true );
    }

    return is2d ? *linectxt.ptr() : *velctxt.ptr();
}


BufferString uiVelSel::getDefaultKey( Seis::GeomType gt ) const
{
    const BufferString seiskey = uiSeisSel::getDefaultKey( gt );
    return IOPar::compKey( seiskey.str(), sKey::Velocity() );
}


void uiVelSel::setInput( const IOObj& ioobj )
{
    uiSeisSel::setInput( ioobj );
    updateEditButton();
}


void uiVelSel::setInput( const MultiID& mid )
{
    uiSeisSel::setInput( mid );
    updateEditButton();
}


void uiVelSel::setVelocityOnly( bool yn )
{
    onlyvelocity_ = yn;
}


uiRetVal uiVelSel::get( VelocityDesc& desc,
			const ZDomain::Info** zdomain ) const
{
    const IOObj* obj = ioobj();
    if ( !obj )
	return tr("Please select a valid velocity model");

    if ( zdomain )
    {
	const ZDomain::Info* zinfo = ZDomain::get( obj->pars() );
	*zdomain = zinfo ? zinfo : &SI().zDomainInfo();
    }

    if ( !desc.usePar(obj->pars()) )
	return tr("Cannot read velocity information for selected model");

    return uiRetVal::OK();
}


uiRetVal uiVelSel::isOK() const
{
    VelocityDesc desc;
    uiRetVal uirv = get( desc );
    if ( !uirv.isOK() )
	return uirv;

    if ( desc.isUdf() )
	uirv.add( tr("The velocity model type is undefined.") );
    else if ( onlyvelocity_ && !desc.isVelocity() )
    {
	uirv.add( tr("The velocity type for this velocity model is not "
		     "allowed for this workflow") );
    }

    if ( !uirv.isOK() )
	uirv.add( tr("Please edit the velocity model information") );

    return uirv;
}


const UnitOfMeasure* uiVelSel::velUnit() const
{
    VelocityDesc desc;
    const uiRetVal uirv = get( desc );
    return uirv.isOK() ? desc.getUnit() : nullptr;
}


void uiVelSel::selectionDoneCB( CallBacker* )
{
    PtrMan<IOObj> ioobj = getIOObj( true );
    if ( ioobj )
    {
	VelocityDesc desc;
	if ( desc.usePar(ioobj->pars()) && desc.isVelocity() )
	{
	    Interval<float> trg = Interval<float>::udf();
	    Interval<float> brg = Interval<float>::udf();
	    VelocityStretcher::getRange( ioobj->pars(), desc, true, trg );
	    VelocityStretcher::getRange( ioobj->pars(), desc, false, brg );
	    if ( trg.isUdf() || brg.isUdf() )
	    {
		uiTaskRunner taskrunner( this );
		if ( uiVelocityDescDlg::doScanVels(*ioobj,desc,true,trg,brg,
						   &taskrunner) )
		{
		    trg_ = trg;
		    brg_ = brg;
		}
	    }
	    else
	    {
		trg_ = trg;
		brg_ = brg;
	    }

	    velChanged.trigger();
	}
    }

    updateEditButton();
}


void uiVelSel::editCB( CallBacker* )
{
    const uiVelocityDesc::Setup vsu( workctio_.ioobj_, is2D(), onlyvelocity_ );
    uiVelocityDescDlg dlg( this, workctio_.ioobj_, vsu );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    const MultiID selid = dlg.getSelection();
    if ( selid.isUdf() )
	return;

    setInput( selid );
    if ( dlg.isVelocity() )
    {
	trg_ = dlg.getVelocityTopRange();
	brg_ = dlg.getVelocityBottomRange();
	velChanged.trigger();
    }

    updateEditButton();
}


void uiVelSel::updateEditButton()
{
    editcubebutt_->setText( ioobj(true) ? m3Dots(uiStrings::sEdit())
					: m3Dots(uiStrings::sCreate()) );
}


void uiVelSel::convertLegacyTypes()
{
    if ( !IOM().isOK() )
	return;

    PtrMan<IODir> seisdir = IOM().getDir( IOObjContext::Seis );
    if ( !seisdir )
	return;

    static bool wasconverted = false;
    if ( wasconverted )
	return;

    const ObjectSet<IOObj>& seisobjs = seisdir->getObjs();
    for ( const auto* seisobj : seisobjs )
    {
	if ( !seisobj )
	    continue;

	const IOPar curiop = seisobj->pars();
	VelocityDesc desc;
	if ( !desc.usePar(curiop) || !desc.isVelocity() )
	    continue;

	IOPar newiop( curiop );
	desc.fillPar( newiop );
	if ( newiop == curiop )
	    continue;

	mNonConst(seisobj)->pars() = newiop;
	seisdir->commitChanges( seisobj );
    }

    wasconverted = true;
}


// uiVelModelZAxisTransform

uiVelModelZAxisTransform::uiVelModelZAxisTransform( uiParent* p, bool t2d )
    : uiTime2DepthZTransformBase(p,t2d)
{
    velsel_ = new uiVelSel( this, VelocityDesc::getVelVolumeLabel(),
			    is2D(), true );
    setHAlignObj( velsel_ );
}


uiVelModelZAxisTransform::~uiVelModelZAxisTransform()
{
    detachAllNotifiers();
}


ZAxisTransform* uiVelModelZAxisTransform::getSelection()
{
    return transform_;
}


void uiVelModelZAxisTransform::doInitGrp()
{
    setZRangeCB( nullptr );
    mAttachCB( velsel_->velChanged, uiVelModelZAxisTransform::setZRangeCB );
}


void uiVelModelZAxisTransform::setZRangeCB( CallBacker* )
{
    if ( !rangefld_ )
	return;

    const ZDomain::Info& twtdef = ZDomain::TWT();
    const ZDomain::Info& ddef = SI().depthsInFeet() ? ZDomain::DepthFeet()
						    : ZDomain::DepthMeter();
    const ZDomain::Info& from = isTimeToDepth() ? twtdef : ddef;
    const ZDomain::Info& to = isTimeToDepth() ? ddef : twtdef;
    const ZSampling zsamp = SI().zRange( true );
    const Interval<float> topvelrg = velsel_->getVelocityTopRange();
    const Interval<float> botvelrg = velsel_->getVelocityBottomRange();
    const UnitOfMeasure* veluom = velsel_->velUnit();
    const ZSampling zrg = VelocityStretcher::getWorkZSampling( zsamp, from, to,
						 topvelrg, botvelrg, veluom );
    rangefld_->setZRange( zrg );
}


const char* uiVelModelZAxisTransform::selName() const
{ return selname_.buf(); }


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiVelModelZAxisTransform::acceptOK()
{
    transform_ = nullptr;
    uiRetVal uirv = velsel_->isOK();
    if ( !uirv.isOK() )
	mErrRet(uirv)

    const IOObj* ioobj = velsel_->ioobj( true );
    if ( !ioobj )
    {
	pErrMsg("Should not be reached");
	return false;
    }

    const MultiID mid = ioobj->key();
    if ( mid.isUdf() )
    {
	pErrMsg("Should not be reached");
	return false;
    }

    const BufferString selname = ioobj->name();

    VelocityDesc desc;
    const ZDomain::Info* zinfo = nullptr;
    BufferString zdomain;
    uirv = velsel_->get( desc, &zinfo );
    if ( !uirv.isOK() )
	mErrRet(uirv)

    uiRetVal res;
    if ( !VelocityDesc::isUsable(desc.type_,zinfo->def_,res) )
    {
	uirv = tr("Cannot setup a time-depth transformation with the "
		  "selected velocity model:");
	uirv.add( res );
	mErrRet(uirv)
    }

    if ( isTimeToDepth() )
	transform_ = new Time2DepthStretcher( mid );
    else
	transform_ = new Depth2TimeStretcher( mid );

    if ( !transform_->isOK() )
    {
	uiRetVal msgs( tr("Internal: Could not initialize transform") );
	if ( !transform_->errMsg().isEmpty() )
	    msgs.add( transform_->errMsg() );
	uiMSG().errorWithDetails( msgs );
	return false;
    }

    selname_ = selname;
    selkey_ = mid;

    return true;
}


StringView uiVelModelZAxisTransform::getZDomain() const
{
    return isTimeToDepth() ? ZDomain::sKeyDepth() : ZDomain::sKeyTime();
}


void uiTime2Depth::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance,
		Time2DepthStretcher::sFactoryKeyword(), tr("Velocity volume") );
}


uiZAxisTransform* uiTime2Depth::createInstance( uiParent* p,
					const char* fromdomain,
					const char* todomain )
{
    if ( !fromdomain || !todomain )
	return nullptr;

    if ( StringView(fromdomain) != ZDomain::sKeyTime() ||
	 StringView(todomain) != ZDomain::sKeyDepth() )
	return nullptr;

    return new uiTime2Depth( p );
}


uiTime2Depth::uiTime2Depth( uiParent* p )
    : uiVelModelZAxisTransform(p,true)
{}


uiTime2Depth::~uiTime2Depth()
{}


void uiDepth2Time::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance,
		Depth2TimeStretcher::sFactoryKeyword(), tr("Velocity Model") );
}


uiZAxisTransform* uiDepth2Time::createInstance( uiParent* p,
			const char* fromdomain, const char* todomain )
{
    if ( !fromdomain || !todomain )
	return nullptr;

    if ( StringView(fromdomain) != ZDomain::sKeyDepth() ||
	 StringView(todomain) != ZDomain::sKeyTime() )
	return nullptr;

    return new uiDepth2Time( p );
}


uiDepth2Time::uiDepth2Time( uiParent* p )
    : uiVelModelZAxisTransform( p, false )
{}


uiDepth2Time::~uiDepth2Time()
{}
