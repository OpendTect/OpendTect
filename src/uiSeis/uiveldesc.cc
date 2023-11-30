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
#include "separstr.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "veldescimpl.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uimsg.h"
#include "uistaticsdesc.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiunitsel.h"
#include "uizrangeinput.h"

#include "hiddenparam.h"


// uiVelocityDesc::Setup

static HiddenParam<uiVelocityDesc::Setup,char> uiveldescsuvelmgr_(0);

uiVelocityDesc::Setup::Setup( const IOObj* ioobj, bool is2d, bool onlyvelocity )
    : is2d_(is2d)
{
    uiveldescsuvelmgr_.setParam( this, onlyvelocity );
    if ( ioobj )
	desc_.usePar( ioobj->pars() );
}


bool uiVelocityDesc::Setup::getOnlyVelocity() const
{
    if ( !uiveldescsuvelmgr_.hasParam(this) )
	uiveldescsuvelmgr_.setParam( &mSelf(), false );

    return uiveldescsuvelmgr_.getParam( this );
}


uiVelocityDesc::Setup& uiVelocityDesc::Setup::onlyvelocity( bool yn )
{
    uiveldescsuvelmgr_.setParam( this, yn );
    return *this;
}


void uiVelocityDesc::Setup::removeParam()
{
    if ( uiveldescsuvelmgr_.hasParam(this) )
	uiveldescsuvelmgr_.removeParam( this );
}


class uiVelocityDescHP
{
public:

uiVelocityDescHP( const uiVelocityDesc::Setup& su )
    : vsu_(su)
    , veltypedef_(OD::VelocityTypeDef())
{
}


    uiVelocityDesc::Setup	vsu_;
    EnumDefImpl<OD::VelocityType> veltypedef_;
    uiCheckBox*			unitchkfld_;
    uiUnitSel*			unitfld_;
    bool			wasguessed_ = false;
};


static HiddenParam<uiVelocityDesc,uiVelocityDescHP*>
						uiveldescparsmgr_(nullptr);

uiVelocityDesc::uiVelocityDesc( uiParent* p, const uiVelocityDesc::Setup& vsu )
    : uiGroup( p, "Velocity type selector" )
{
    auto* extrapars = new uiVelocityDescHP( vsu );
    uiveldescparsmgr_.setParam( this, extrapars );
    EnumDefImpl<OD::VelocityType>& veltypedef_ = extrapars->veltypedef_;

    const EnumDefImpl<OD::VelocityType>& typdef = OD::VelocityTypeDef();
    if ( vsu.getOnlyVelocity() )
    {
	veltypedef_.remove( typdef.getKey(OD::VelocityType::Delta) );
	veltypedef_.remove( typdef.getKey(OD::VelocityType::Epsilon) );
	veltypedef_.remove( typdef.getKey(OD::VelocityType::Eta) );
    }

    typefld_ = new uiGenInput( this, tr("Velocity type"),
			       StringListInpSpec(veltypedef_) );
    mAttachCB( typefld_->valueChanged, uiVelocityDesc::typeChgCB );

    extrapars->unitchkfld_ = new uiCheckBox( this, uiString::empty(),
			mCB(this,uiVelocityDesc,unitCheckCB) );
    extrapars->unitchkfld_->setMaximumWidth( 16 );
    extrapars->unitchkfld_->attach( rightOf, typefld_ );

    uiUnitSel::Setup uiusu( Mnemonic::Vel, tr("in"),
			    &Mnemonic::defVEL() );
    uiusu.mode( uiUnitSel::Setup::SymbolsOnly );
    extrapars->unitfld_ = new uiUnitSel( this, uiusu );
    extrapars->unitfld_->setUnit( UnitOfMeasure::surveyDefVelUnit() );
    extrapars->unitfld_->attach( rightOf, extrapars->unitchkfld_ );

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


mStartAllowDeprecatedSection

uiVelocityDesc::uiVelocityDesc( uiParent* p, const uiVelocityDesc::Setup* vsu )
    : uiVelocityDesc(p,vsu ? *vsu : uiVelocityDesc::Setup())
{
}

mStopAllowDeprecatedSection


uiVelocityDesc::~uiVelocityDesc()
{
    detachAllNotifiers();
    uiveldescparsmgr_.removeAndDeleteParam( this );
}


uiVelocityDesc::Setup& uiVelocityDesc::vsu_()
{
    return uiveldescparsmgr_.getParam( this )->vsu_;
}


const EnumDefImpl<OD::VelocityType>& uiVelocityDesc::veltypedef_() const
{
    return uiveldescparsmgr_.getParam( this )->veltypedef_;
}


uiCheckBox* uiVelocityDesc::unitchkfld_()
{
    return uiveldescparsmgr_.getParam( this )->unitchkfld_;
}


const uiUnitSel* uiVelocityDesc::unitfld_() const
{
    return uiveldescparsmgr_.getParam( this )->unitfld_;
}


uiUnitSel* uiVelocityDesc::unitfld_()
{
    return uiveldescparsmgr_.getParam( this )->unitfld_;
}


void uiVelocityDesc::initGrpCB( CallBacker* )
{
    set( vsu_().desc_ );
}


void uiVelocityDesc::typeChgCB( CallBacker* )
{
    const OD::VelocityType type = getType();
    const bool dispunit = VelocityDesc::isUdf( type ) ||
			  VelocityDesc::isVelocity( type );
    auto& extrapars = *uiveldescparsmgr_.getParam( this );
    extrapars.unitchkfld_->display( dispunit );
    if ( dispunit )
    {
	NotifyStopper ns( extrapars.unitchkfld_->activated );
	extrapars.unitchkfld_->setChecked(
			uiveldescparsmgr_.getParam( this )->wasguessed_ );
	uiVelocityDesc::unitCheckCB( nullptr );
    }

    extrapars.unitfld_->display( dispunit );
    const bool isvrms = type==OD::VelocityType::RMS;
    hasstaticsfld_->display( isvrms );
    hasStaticsChgCB( nullptr );
}


void uiVelocityDesc::unitCheckCB( CallBacker* )
{
    auto& extrapars = *uiveldescparsmgr_.getParam( this );
    const bool cansetunit = extrapars.unitchkfld_->isChecked();
    extrapars.unitfld_->setSensitive( cansetunit );
}


void uiVelocityDesc::hasStaticsChgCB( CallBacker* )
{
    const bool displaystatics = hasstaticsfld_->isDisplayed() &&
				hasstaticsfld_->getBoolValue();
    staticsfld_->display( displaystatics );
}


void uiVelocityDesc::updateFlds( CallBacker* )
{
}


OD::VelocityType uiVelocityDesc::getType() const
{
    const int curtypeidx = typefld_->getIntValue();
    return veltypedef_().getEnumForIndex( curtypeidx );
}


void uiVelocityDesc::setType( OD::VelocityType type )
{
    const BufferString typestr( OD::VelocityTypeDef().getKey(type) );
    if ( !veltypedef_().keys().isPresent(typestr) )
	return;

    const int typeidx = veltypedef_().indexOf( typestr.str() );
    typefld_->setValue( typeidx );
}


NotifierAccess& uiVelocityDesc::typeChangeNotifier()
{ return typefld_->valuechanged; }


void uiVelocityDesc::set( const VelocityDesc& desc )
{
    auto& extrapars = *uiveldescparsmgr_.getParam( this );
    NotifyStopper ns( typefld_->valueChanged, this );
    NotifyStopper nschk( extrapars.unitchkfld_->activated );
    setType( desc.getType() );
    if ( desc.isVelocity() || desc.isUdf() )
    {
	const UnitOfMeasure* velstoruom =
			     UnitOfMeasure::surveyDefVelStorageUnit();
	const UnitOfMeasure* descuom = UoMR().get( desc.velunit_.buf() );
	const bool hasunit = descuom && descuom->isCompatibleWith(*velstoruom);
	extrapars.unitchkfld_->setChecked( !hasunit );
	if ( desc.isVelocity() )
	    extrapars.unitfld_->setUnit( hasunit ? descuom : velstoruom );
	extrapars.wasguessed_ = !hasunit;
    }

    hasstaticsfld_->setValue( !desc.statics_.horizon_.isUdf() );
    staticsfld_->set( desc.statics_ );
    typeChgCB( nullptr );
}


bool uiVelocityDesc::isVelocity() const
{
    const OD::VelocityType type = getType();
    return VelocityDesc::isVelocity( type );
}


bool uiVelocityDesc::get( VelocityDesc& res, bool disperr ) const
{
    res.setType( getType() );
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

    if ( unitfld_()->isDisplayed() )
    {
	const UnitOfMeasure* veluom = UoMR().get( unitfld_()->getUnitName() );
	Vel::Worker::setUnit( veluom, res );
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


mStartAllowDeprecatedSection

uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p, const IOObj* sel,
				      const uiVelocityDesc::Setup* vsu )
  : uiVelocityDescDlg(p,sel,vsu ? *vsu : uiVelocityDesc::Setup())
{
}

mStopAllowDeprecatedSection


uiVelocityDescDlg::~uiVelocityDescDlg()
{
    detachAllNotifiers();
}


const UnitOfMeasure* uiVelocityDescDlg::velUnit() const
{
    VelocityDesc desc;
    return veldescfld_->get( desc, false ) ? Vel::Worker::getUnit( desc )
					   : nullptr;
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
	    oldveldesc_.setType( OD::VelocityType::Unknown );

	const IOPar& par = ioobj->pars();
	if ( !VelocityStretcherNew::getRange(par,oldveldesc_,
					     true,toprange_) )
	    toprange_.setUdf();

	if ( !VelocityStretcherNew::getRange(par,oldveldesc_,
					     false,bottomrange_) )
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
    VelocityModelScannerNew scanner( ioobj, desc );
    if ( !TaskRunner::execute(taskrunner,scanner) )
	return false;

    topvelrg = scanner.getTopVAvg();
    botvelrg = scanner.getBotVAvg();
    if ( writergs )
    {
	VelocityStretcherNew::setRange( topvelrg, desc, true, ioobj.pars() );
	VelocityStretcherNew::setRange( botvelrg, desc, false, ioobj.pars() );
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

    VelocityStretcherNew::setRange( toprange_, desc, true, ioobj->pars() );
    VelocityStretcherNew::setRange( bottomrange_, desc, false, ioobj->pars() );

    return veldescfld_->updateAndCommit( *cCast(IOObj*,ioobj), true );
}


MultiID uiVelocityDescDlg::getSelection_() const
{
    MultiID ret;
    PtrMan<IOObj> ioobj = volselfld_->getIOObj( true );
    if ( ioobj )
	ret = ioobj->key();

    return ret;
}


IOObj* uiVelocityDescDlg::getSelection() const
{
    return volselfld_->getIOObj(true);
}


bool uiVelocityDescDlg::isVelocity() const
{
    return veldescfld_->isVelocity();
}


namespace Vel
{

class VelEntriesConvertManager : public CallBacker
{
public:

VelEntriesConvertManager()
{
    mAttachCB( IOM().surveyToBeChanged,
	       VelEntriesConvertManager::surveyChangedCB );
    mAttachCB( IOM().applicationClosing,
	       VelEntriesConvertManager::appClosingCB );
}

~VelEntriesConvertManager()
{
    detachAllNotifiers();
}

void convertLegacyTypes()
{
    if ( !IOM().isOK() )
	return;

    PtrMan<IODir> seisdir = IOM().getDir( IOObjContext::Seis );
    if ( !seisdir )
	return;

    if ( wasconverted_ )
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
	if ( desc.isVelocity() )
	{
	    const UnitOfMeasure* veluom = Worker::getUnit( desc );
	    if ( veluom )
		Worker::setUnit( veluom, desc );
	}

	desc.fillPar( newiop );
	if ( newiop == curiop )
	    continue;

	mNonConst(seisobj)->pars() = newiop;
	seisdir->commitChanges( seisobj );
    }

    wasconverted_ = true;
}

private:

void surveyChangedCB( CallBacker* )
{
    wasconverted_ = false;
}

void appClosingCB( CallBacker* )
{
    detachAllNotifiers();
}

    bool wasconverted_ = false;

};


VelEntriesConvertManager& convertManager()
{
    static VelEntriesConvertManager man_;
    return man_;
}

} // namespace Vel


// uiVelSel

static HiddenParam<uiVelSel,char> uivelselonlyvelmgr_(0);

uiVelSel::uiVelSel( uiParent* p, const uiString& lbltxt,
		    bool is2d, bool enabotherdomain )
    : uiVelSel(p,ioContext(is2d),
	    (uiSeisSel::Setup&)uiSeisSel::Setup(is2d ? Seis::Line : Seis::Vol)
					.enabotherdomain(enabotherdomain)
					.seltxt(lbltxt))
{
    uivelselonlyvelmgr_.setParam( this, true );
}


uiVelSel::uiVelSel( uiParent* p, const IOObjContext& ctxt,
		    const uiSeisSel::Setup& setup, bool /* weditbutton */ )
    : uiSeisSel(p,ctxt,setup)
    , trg_(Interval<float>::udf())
    , brg_(Interval<float>::udf())
    , velrgchanged( this )
{
    uivelselonlyvelmgr_.setParam( this, true );
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
    uivelselonlyvelmgr_.removeParam( this );
}


bool uiVelSel::isOnlyVelocity() const
{
    return uivelselonlyvelmgr_.getParam( this );
}


void uiVelSel::initGrpCB( CallBacker* )
{
    selectionDoneCB( nullptr );
    if ( trg_.isUdf() || brg_.isUdf() )
    {
	const UnitOfMeasure* veluom = velUnit();
	const Interval<float> defvelrg =
				VelocityStretcherNew::getDefaultVAvg( veluom );
	if ( trg_.isUdf() )
	    trg_.set( defvelrg.start, defvelrg.start );
	if ( brg_.isUdf() )
	    brg_.set( defvelrg.stop, defvelrg.stop );

	velChanged().trigger();
    }
}


const IOObjContext& uiVelSel::ioContext( bool is2d )
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, velctxt, = nullptr );
    mDefineStaticLocalObject( PtrMan<IOObjContext>, linectxt, = nullptr );
    Vel::convertManager().convertLegacyTypes();
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


void uiVelSel::fillDefault()
{
    uiSeisSel::fillDefault();
}


void uiVelSel::setInput( const MultiID& mid )
{
    setInput_( mid );
}


void uiVelSel::setInput_( const IOObj& ioobj )
{
    uiSeisSel::setInput( ioobj );
    updateEditButton();
}


void uiVelSel::setInput_( const MultiID& mid )
{
    uiSeisSel::setInput( mid );
    updateEditButton();
}


void uiVelSel::setVelocityOnly( bool yn )
{
    uivelselonlyvelmgr_.setParam( this, yn );
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
    else if ( isOnlyVelocity() && !desc.isVelocity() )
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
    if ( !ioobj(true) )
	return UnitOfMeasure::surveyDefVelUnit();

    VelocityDesc desc;
    const uiRetVal uirv = get( desc );
    return uirv.isOK() ? Vel::Worker::getUnit( desc )
		       : UnitOfMeasure::surveyDefVelUnit();
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
	    VelocityStretcherNew::getRange( ioobj->pars(), desc, true, trg );
	    VelocityStretcherNew::getRange( ioobj->pars(), desc, false, brg );
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

	    velChanged().trigger();
	}
    }

    updateEditButton();
}


void uiVelSel::editCB( CallBacker* )
{
    const uiVelocityDesc::Setup vsu( workctio_.ioobj_, is2D(),isOnlyVelocity());
    uiVelocityDescDlg dlg( this, workctio_.ioobj_, vsu );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    const MultiID selid = dlg.getSelection_();
    if ( selid.isUdf() )
	return;

    setInput( selid );
    if ( dlg.isVelocity() )
    {
	trg_ = dlg.getVelocityTopRange();
	brg_ = dlg.getVelocityBottomRange();
	velChanged().trigger();
    }

    updateEditButton();
}


void uiVelSel::updateEditButton()
{
    editcubebutt_->setText( ioobj(true) ? m3Dots(uiStrings::sEdit())
					: m3Dots(uiStrings::sCreate()) );
}


const IOObjContext& uiVelSel::ioContext()
{
    return ioContext( false );
}


void uiVelSel::setIs2D( bool yn )
{
    seissetup_.geom_ = yn ? Seis::Line : Seis::Vol;
    IOObjContext ctxt = uiSeisSel::ioContext( seissetup_.geom_, true );
    ctxt.toselect_.require_.set( sKey::Type(), sKey::Velocity() );
    workctio_.ctxt_ = inctio_.ctxt_ = ctxt;
    updateInput();
    fillEntries();
    selectionDoneCB(0);
}


// uiVelModelZAxisTransform

static HiddenParam<uiVelModelZAxisTransform,ZAxisTransform*>
						uivelzaxistfmgr_(nullptr);

uiVelModelZAxisTransform::uiVelModelZAxisTransform( uiParent* p, bool t2d )
    : uiTime2DepthZTransformBase(p,t2d)
    , transform_(nullptr)
{
    const bool issurv2d = SI().has2D() && !SI().has3D();
    init( issurv2d );
}


uiVelModelZAxisTransform::uiVelModelZAxisTransform( uiParent* p, bool t2d,
								bool is2d )
    : uiTime2DepthZTransformBase(p,t2d)
    , transform_(nullptr)
{
    init( is2d );
}


void uiVelModelZAxisTransform::init( bool is2d )
{
    uivelzaxistfmgr_.setParam( this, nullptr );
    velsel_ = new uiVelSel( this, VelocityDesc::getVelVolumeLabel(),
								is2d, true );
    setHAlignObj( velsel_ );
    mAttachCB( postFinalize(), uiVelModelZAxisTransform::finalizeCB );
}


uiVelModelZAxisTransform::~uiVelModelZAxisTransform()
{
    detachAllNotifiers();
    unRefPtr( transform_ );
    ZAxisTransform* transform = uivelzaxistfmgr_.getParam( this );
    uivelzaxistfmgr_.setParam( this, nullptr );
    unRefPtr( transform );
    uivelzaxistfmgr_.removeParam( this );
}


ZAxisTransform* uiVelModelZAxisTransform::getSelection()
{
    return uivelzaxistfmgr_.getParam( this );
}


void uiVelModelZAxisTransform::enableTargetSampling()
{
    uiTime2DepthZTransformBase::enableTargetSampling();
}


void uiVelModelZAxisTransform::finalizeCB( CallBacker* )
{
    setZRangeCB( nullptr );
    uiTime2DepthZTransformBase::finalizeDoneCB( nullptr );
    mAttachCB( velsel_->velChanged(), uiVelModelZAxisTransform::setZRangeCB );
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
    const ZSampling zrg = VelocityStretcherNew::getWorkZSampling( zsamp,
						 from, to,
						 topvelrg, botvelrg, veluom );
    rangefld_->setZRange( zrg );
}


const char* uiVelModelZAxisTransform::selName() const
{ return selname_.buf(); }


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiVelModelZAxisTransform::acceptOK()
{
    ZAxisTransform* transform = uivelzaxistfmgr_.getParam( this );
    uivelzaxistfmgr_.setParam( this, nullptr );
    unRefAndNullPtr( transform );
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
	transform = new Time2DepthStretcherNew( mid );
    else
	transform = new Depth2TimeStretcherNew( mid );

    if ( !transform->isOK() )
    {
	uiRetVal msgs( tr("Internal: Could not initialize transform") );
	if ( !transform->errMsg().isEmpty() )
	    msgs.add( transform->errMsg() );

	uiMSG().errorWithDetails( msgs.messages() );
	return false;
    }

    transform->ref();
    uivelzaxistfmgr_.setParam( this, transform );

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
	    Time2DepthStretcherNew::sFactoryKeyword(), tr("Velocity volume") );
}


uiZAxisTransform* uiTime2Depth::createInstance( uiParent* p,
					const char* domainstr,
					const char* is2dstr )
{
    const FileMultiString fms( domainstr );
    const StringView str2d( is2dstr );
    if ( fms.isEmpty() || fms.size() < 2 || str2d.isEmpty() )
	return nullptr;

    const BufferString fromdomain = fms[0];
    const BufferString todomain = fms[1];

    if ( fromdomain != ZDomain::sKeyTime() ||
					todomain != ZDomain::sKeyDepth() )
	return nullptr;

    const bool is2d = str2d.isEqual( sKey::TwoD() );

    return new uiTime2Depth( p, is2d );
}


uiTime2Depth::uiTime2Depth( uiParent* p )
    : uiVelModelZAxisTransform(p,true,(SI().has2D() && !SI().has3D()))
{}


uiTime2Depth::uiTime2Depth( uiParent* p, bool is2d )
    : uiVelModelZAxisTransform(p,true,is2d)
{}


uiTime2Depth::~uiTime2Depth()
{}


void uiDepth2Time::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance,
	    Depth2TimeStretcherNew::sFactoryKeyword(), tr("Velocity Model") );
}


uiZAxisTransform* uiDepth2Time::createInstance( uiParent* p,
				const char* domainstr, const char* is2dstr )
{
    const FileMultiString fms( domainstr );
    const StringView str2d( is2dstr );
    if ( fms.isEmpty() || fms.size() < 2 || str2d.isEmpty() )
	return nullptr;

    const BufferString fromdomain = fms[0];
    const BufferString todomain = fms[1];

    if ( fromdomain != ZDomain::sKeyDepth() ||
					    todomain != ZDomain::sKeyTime() )
	return nullptr;

    const bool is2d = str2d.isEqual( sKey::TwoD() );
    return new uiDepth2Time( p, is2d );
}


uiDepth2Time::uiDepth2Time( uiParent* p )
    : uiVelModelZAxisTransform(p,false,(SI().has2D() && !SI().has3D()))
{}


uiDepth2Time::uiDepth2Time( uiParent* p, bool is2d )
    : uiVelModelZAxisTransform(p,false,is2d)
{}


uiDepth2Time::~uiDepth2Time()
{}
