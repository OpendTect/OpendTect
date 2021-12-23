/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
________________________________________________________________________

-*/


#include "uiwellpropertyrefsel.h"

#include "uicombobox.h"
#include "uimsg.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "uiwelllogcalc.h"
#include "uiwelllogdisplay.h"

#include "propertyref.h"
#include "unitofmeasure.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"

static const char* sKeyPlsSel = "Please select";


uiWellSinglePropSel::uiWellSinglePropSel( uiParent* p, const Mnemonic& mn )
    : uiGroup(p,mn.name())
    , logtypename_(mn.name())
{
    logunits_.setNullAllowed();
    altlogunits_.setNullAllowed();

    makeLogNameFld( mn.unit() );
    const Mnemonic* altmn = guessAltMnemonic( mn );
    if ( altmn )
    {
	altlogtypename_.set( altmn->name() );
	makeAltLogNameFld( altmn->unit() );
    }

    setHAlignObj( lognmfld_->box() );
}


uiWellSinglePropSel::uiWellSinglePropSel( uiParent* p, const PropertyRef& pr )
    : uiGroup(p,pr.name())
    , logtypename_(pr.name())
    , propref_(&pr)
{
    logunits_.setNullAllowed();
    altlogunits_.setNullAllowed();

    makeLogNameFld( pr.unit() );
    altpropref_ = guessAltProp( pr );
    if ( altpropref_ )
    {
	altlogtypename_.set( altpropref_->name() );
	makeAltLogNameFld( altpropref_->unit() );
    }

    setHAlignObj( lognmfld_->box() );
}


uiWellSinglePropSel::~uiWellSinglePropSel()
{
    detachAllNotifiers();
}


void uiWellSinglePropSel::makeLogNameFld( const UnitOfMeasure* uom )
{
    const Mnemonic& mn = normMn();
    const char* nm = logtypeName();
    lognmfld_ = new uiLabeledComboBox( this, toUiString(nm) );
    mAttachCB( lognmfld_->box()->selectionChanged,
	       uiWellSinglePropSel::switchPropCB );

    uiUnitSel::Setup ussu( mn.stdType(), uiString::empty(), &mn );
    ussu.mode( uiUnitSel::Setup::SymbolsOnly ).withnone( true );
    unfld_ = new uiUnitSel( this, ussu );
    unfld_->setUnit( uom );
    unfld_->attach( rightOf, lognmfld_ );
}


void uiWellSinglePropSel::makeAltLogNameFld( const UnitOfMeasure* uom )
{
    const Mnemonic& mn = *altMn();
    const char* nm = altLogtypeName();
    altlognmfld_ = new uiLabeledComboBox( this, toUiString(nm) );
    mAttachCB( altlognmfld_->box()->selectionChanged,
	       uiWellSinglePropSel::switchPropCB );
    altlognmfld_->attach( alignedWith, lognmfld_ );

    uiUnitSel::Setup altussu( mn.stdType(), uiString::empty(), &mn );
    altussu.mode( uiUnitSel::Setup::SymbolsOnly ).withnone( true );
    altunfld_ = new uiUnitSel( this, altussu );
    altunfld_->setUnit( uom );
    altunfld_->attach( rightOf, altlognmfld_ );

    altbox_ = new uiCheckBox( this, toUiString(altlogtypename_) );
    altbox_->attach( rightOf, unfld_ );
    mAttachCB( altbox_->activated, uiWellSinglePropSel::updateSelCB );
}


bool uiWellSinglePropSel::setAvailableLogs( const Well::LogSet& wls )
{
    BufferStringSet normnms;
    normnms.add( sKeyPlsSel );
    logunits_.setEmpty();

    const TypeSet<int> logidxs = wls.getSuitable( normMn() );
    for ( const auto& idx : logidxs )
    {
	const Well::Log& wl = wls.getLog( idx );
	normnms.add( wl.name() );
	logunits_.add( wl.unitOfMeasure() );
    }

    uiComboBox* lognmfld = lognmfld_->box();
    BufferString prevlognm;
    if ( lognmfld->currentItem() > 0 )
	prevlognm.set( lognmfld->text() );

    lognmfld->setEmpty();
    lognmfld->addItems( normnms );
    if ( !setDefaultLog(wls,normMn()) )
    {
	if ( !prevlognm.isEmpty() && normnms.isPresent(prevlognm.str()) )
		lognmfld->setCurrentItem( normnms.indexOf(prevlognm.str()) );
	else if ( normnms.size() > 1 )
		lognmfld->setCurrentItem( 1 );
    }

    switchPropCB( lognmfld );
    if ( !altMn() )
    {
	updateSelCB( nullptr );
	return normnms.size() > 1;
    }

    BufferStringSet altnms;
    altnms.add( sKeyPlsSel );
    altlogunits_.setEmpty();

    const TypeSet<int> altlogidxs = wls.getSuitable( *altMn() );
    for ( const auto& idx : altlogidxs )
    {
	const Well::Log& wl = wls.getLog( idx );
	altnms.add( wl.name() );
	altlogunits_.add( wl.unitOfMeasure() );
    }

    uiComboBox* altlognmfld = altlognmfld_->box();
    altlognmfld->setEmpty();
    altlognmfld->addItems( altnms );
    if ( !setDefaultLog(wls, *altMn()) )
    {
	if ( altnms.size() > 1 )
	    altlognmfld->setCurrentItem( 1 );
    }

    switchPropCB( altlognmfld );

    altbox_->setChecked( normnms.size() < 2 );
    updateSelCB( nullptr );

    return normnms.size() + altnms.size() > 2;
}


bool uiWellSinglePropSel::setDefaultLog( const Well::LogSet& wls,
					 const Mnemonic& mnem )
{
    const Well::Log* deflog = wls.getLog( mnem );
    if ( !deflog )
	return false;

    setCurrent( deflog->name() );
    return true;
}


void uiWellSinglePropSel::switchPropCB( CallBacker* cb )
{
    mDynamicCastGet(uiComboBox*,lognmfld,cb);
    if ( !lognmfld )
	return;

    const bool isalt = altlognmfld_ && lognmfld == altlognmfld_->box();
    const ObjectSet<const UnitOfMeasure>& logunits = isalt ? altlogunits_
							   : logunits_;
    const int itmidx = lognmfld->currentItem() - 1;
    const UnitOfMeasure* uom = logunits.validIdx(itmidx)
			     ? logunits.get( itmidx ) : nullptr;
    uiUnitSel* unfld = isalt ? altunfld_ : unfld_;
    unfld->setSensitive( !uom );
    unfld->setUnit( uom );
}


void uiWellSinglePropSel::updateSelCB( CallBacker* )
{
    const bool isalt = altPropSelected();
    lognmfld_->display( !isalt );
    unfld_->display( !isalt );
    if ( altMn() )
    {
	altlognmfld_->display( isalt );
	altunfld_->display( isalt );
    }
}


void uiWellSinglePropSel::selectAltProp( bool yn )
{
    if ( !altbox_ )
	return;

    altbox_->setChecked( yn );
    updateSelCB( nullptr );
}


void uiWellSinglePropSel::setCurrent( const char* lnm )
{
    uiComboBox* lognmfld = curLogNmFld();
    lognmfld->setCurrentItem( lnm );
    switchPropCB( lognmfld );
}


void uiWellSinglePropSel::setUOM( const UnitOfMeasure* uom )
{
    uiUnitSel* unfld = curUnitFld();
    unfld->setUnit( uom );
    unfld->setSensitive( !uom );
}


void uiWellSinglePropSel::set( const char* txt, bool alt,
			       const UnitOfMeasure* uom )
{
    selectAltProp( alt );
    setCurrent( txt );
    setUOM( uom );
}


bool uiWellSinglePropSel::isOK() const
{
    return curLogNmFld()->currentItem() > 0 && curUnitFld()->getUnit();
}


bool uiWellSinglePropSel::altPropSelected() const
{
    return altbox_ ? altbox_->isChecked() : false;
}


uiComboBox* uiWellSinglePropSel::curLogNmFld() const
{
    return altPropSelected() ? altlognmfld_->box() : lognmfld_->box();

}


const char* uiWellSinglePropSel::logName() const
{
    return curLogNmFld()->text();
}


uiUnitSel* uiWellSinglePropSel::curUnitFld() const
{
    return altPropSelected() ? altunfld_ : unfld_;
}


const UnitOfMeasure* uiWellSinglePropSel::getUnit() const
{
    return curUnitFld()->getUnit();
}


const char* uiWellSinglePropSel::selLogtypeName() const
{
    return altPropSelected() ? altLogtypeName() : logtypeName();
}


const Mnemonic& uiWellSinglePropSel::normMn() const
{
    return propref_ ? propref_->mn()
		    : *MNC().getByName( logtypename_, false );
}


const Mnemonic* uiWellSinglePropSel::altMn() const
{
    if ( altlogtypename_.isEmpty() )
	return nullptr;

    return altpropref_ ? &altpropref_->mn()
		       : MNC().getByName( altlogtypename_, false );
}


const Mnemonic& uiWellSinglePropSel::selMn() const
{
    return altPropSelected() ? *altMn() : normMn();
}


const Mnemonic* uiWellSinglePropSel::guessAltMnemonic( const Mnemonic& mn )
{
    if ( mn.stdType() != Mnemonic::Vel && mn.stdType() != Mnemonic::Son )
	return nullptr;

    const Mnemonic& dtmn = Mnemonic::defDT();
    const Mnemonic& pvelmn = Mnemonic::defPVEL();
    const bool issonic = &mn == &dtmn;
    const bool ispvel = &mn == &pvelmn;
    if ( issonic || ispvel )
	return issonic ? &pvelmn : &dtmn;

    const Mnemonic& dtsmn = Mnemonic::defDTS();
    const Mnemonic& svelmn = Mnemonic::defSVEL();
    const bool isshearsonic = &mn == &dtsmn;
    const bool issvel = &mn == &svelmn;
    if ( isshearsonic || issvel )
	return isshearsonic ? &svelmn : &dtsmn;

    return nullptr;
}


const PropertyRef* uiWellSinglePropSel::guessAltProp( const PropertyRef& pr )
{
    const Mnemonic* mn = guessAltMnemonic( pr.mn() );
    return mn ? PROPS().getByMnemonic( *mn ) : nullptr;
}



uiWellPropSel::uiWellPropSel( uiParent* p, const MnemonicSelection& mnsel )
    : uiGroup(p," mnemonic selection from well logs")
    , logCreated(this)
{
    for ( const auto* mn : mnsel )
    {
	auto* fld = new uiWellSinglePropSel( this, *mn );
	if ( propflds_.isEmpty() )
	    setHAlignObj( fld );
	else
	    fld->attach( alignedBelow, propflds_[propflds_.size()-1] );

	propflds_ += fld;
    }

    addButtons();
}


uiWellPropSel::uiWellPropSel( uiParent* p, const PropertyRefSelection& prs )
    : uiGroup(p," property selection from well logs")
    , logCreated(this)
{
    for ( const auto* pr : prs )
    {
	if ( pr->isThickness() || pr->hasFixedDef() )
	    continue;

	auto* fld = new uiWellSinglePropSel( this, *pr );
	if ( propflds_.isEmpty() )
	    setHAlignObj( fld );
	else
	    fld->attach( alignedBelow, propflds_[propflds_.size()-1] );

	propflds_ += fld;
    }

    addButtons();
}


void uiWellPropSel::addButtons()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	auto* createbut = new uiToolButton( this, "newlog",
		tr("Create a log from other logs"),
		mCB(this,uiWellPropSel,createLogPushed) );
	auto* viewbut = new uiToolButton( this, "view_log",
		tr("View log"), mCB(this,uiWellPropSel,viewLogPushed) );
	if ( idx )
	    createbut->attach( ensureBelow, createbuts_[idx-1] );
	for ( int idotherpr=0; idotherpr<propflds_.size(); idotherpr++ )
	    createbut->attach( ensureRightOf, propflds_[idotherpr] );
	viewbut->attach( rightOf, createbut );
	createbuts_ += createbut;
	viewbuts_ += viewbut;
    }
}


uiWellPropSel::~uiWellPropSel()
{
    detachAllNotifiers();
}


bool uiWellPropSel::setAvailableLogs( const Well::LogSet& logs,
				      BufferStringSet& notokpropnms  )
{
    bool allok = true;
    notokpropnms.erase();
    for ( auto* propfld : propflds_ )
    {
	if ( !propfld->setAvailableLogs(logs) )
	{
	    notokpropnms.add( propfld->logtypeName() );
	    allok = false;
	}
    }
    return allok;
}


bool uiWellPropSel::isOK() const
{
    for ( const auto* propfld : propflds_ )
    {
	if ( !propfld->isOK() )
	{
	    uiMSG().error( tr("Please create/select a log for %1")
			   .arg(propfld->logtypeName()) );
	    return false;
	}
    }
    return true;
}


void uiWellPropSel::setLog( const Mnemonic* mn, const char* nm, bool usealt,
			    const UnitOfMeasure* uom, int idx )
{
    if ( !propflds_.validIdx(idx) )
    {
	pErrMsg("Idx failure");
	return;
    }

    const Mnemonic* propmn = usealt ? propflds_[idx]->altMn()
				    : &propflds_[idx]->normMn();
    if ( propmn != mn )
    {
	pErrMsg("Type failure");
	return;
    }

    propflds_[idx]->set( nm, usealt, uom );
}


const Mnemonic* uiWellPropSel::getMnRef( int idx ) const
{
    if ( !propflds_.validIdx(idx) )
	return nullptr;

    return &propflds_.get( idx )->normMn();
}


const char* uiWellPropSel::getLogTypename( int idx, bool selected ) const
{
    if ( !propflds_.validIdx(idx) )
	return nullptr;

    const uiWellSinglePropSel& selfld = *propflds_.get( idx );
    return selected ? selfld.selLogtypeName() : selfld.logtypeName();
}


bool uiWellPropSel::getLog( const Mnemonic& mn,	BufferString& retlognm,
			    bool& retisrev, const UnitOfMeasure*& retuom,
			    int idx ) const
{
    if ( !propflds_.validIdx(idx) )
	{ pErrMsg("Idx failure"); return false; }

    const uiWellSinglePropSel& fld = *propflds_[idx];
    const bool isrev = fld.altPropSelected();
    const Mnemonic& fldmn = fld.normMn();
    const Mnemonic* fldaltmn = isrev ? fld.altMn() : nullptr;
    if ( &fldmn != &mn && !(fldaltmn && fldaltmn == &mn) )
    {
	pErrMsg("Type failure");
	return false;
    }

    if ( !fld.isOK() )
	return false;

    retlognm = fld.logName();
    retisrev = isrev;
    retuom = fld.getUnit();

    return true;
}


void uiWellPropSel::createLogPushed( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb);
    const int idxofbut = createbuts_.indexOf( but );
    if ( !propflds_.validIdx( idxofbut ) )
	return;

    TypeSet<MultiID> idset; idset += wellid_;
    uiWellLogCalc dlg( this, idset );
    dlg.setOutputLogName( propflds_[idxofbut]->selLogtypeName() );
    dlg.go();

    if ( dlg.haveNewLogs() )
    {
	logCreated.trigger();
	propflds_[idxofbut]->setCurrent( dlg.getOutputLogName() );
    }
}


void uiWellPropSel::viewLogPushed( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb);
    const int idxofbut = viewbuts_.indexOf( but );
    if ( !propflds_.validIdx( idxofbut ) )
	return;

    const BufferString lognm( propflds_[idxofbut]->logName() );
    if ( lognm == sKeyPlsSel )
	return;

    const Well::Data* wd = Well::MGR().get( wellid_ );
    if  ( !wd ) return;

    const Well::LogSet& logs = wd->logs();
    const Well::Log* wl = logs.getLog( lognm );
    if ( !wl )
	return; // the log was removed since popup of the window ... unlikely

    (void)uiWellLogDispDlg::popupNonModal( this, wl );
}


//Deprecated implementations:

uiWellSinglePropSel::uiWellSinglePropSel( uiParent* p, const PropertyRef& pr,
					  const PropertyRef* altpr )
    : uiWellSinglePropSel(p,pr)
{
    if ( altpr )
    {
	altpropref_ = altpr;
	altlogtypename_.set( altpropref_->name() );
	makeAltLogNameFld( altpropref_->unit() );
    }
}


const PropertyRef& uiWellSinglePropSel::normPropRef() const
{
    if ( propref_ )
	return *propref_;

    const PropertyRef* pr = PROPS().getByMnemonic( normMn() );
    return pr ? *pr : PropertyRef::thickness();
}


const PropertyRef* uiWellSinglePropSel::altPropRef() const
{
    return altpropref_;
}


const PropertyRef& uiWellSinglePropSel::selPropRef() const
{
    const Mnemonic& mn = altPropSelected() ? *altMn() : normMn();
    const PropertyRef* pr = PROPS().getByMnemonic( mn );
    return pr ? *pr : PropertyRef::thickness();
}


void uiWellPropSel::setLog( const Mnemonic::StdType tp, const char* nm,
			    bool usealt, const UnitOfMeasure* uom, int idx )
{
    if ( !propflds_.validIdx(idx) )
	{ pErrMsg("Idx failure"); return; }

    const BufferStringSet hintnms( nm );
    const Mnemonic& mn = MNC().getGuessed( tp, &hintnms );
    setLog( &mn, nm, usealt, uom, idx );
}


bool uiWellPropSel::getLog( const Mnemonic::StdType tp, BufferString& retlognm,
			    bool& retisrev, BufferString& uomstr,
			    int idx ) const
{
    if ( !propflds_.validIdx(idx) )
	{ pErrMsg("Idx failure"); return false; }

    const Mnemonic& mn = MNC().getGuessed( tp );
    const UnitOfMeasure* uom = nullptr;
    const bool res = getLog( mn, retlognm, retisrev, uom, idx );
    if ( res )
	uomstr.set( UnitOfMeasure::getUnitLbl( uom ) );

    return res;
}


uiWellSinglePropSel* uiWellPropSel::getPropSelFromListByName(
						const BufferString& propnm )
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	uiWellSinglePropSel* fld = propflds_[idx];
	if ( propnm == fld->logtypeName()
	  || (fld->altMn() && propnm == fld->altLogtypeName()) )
	    return fld;
    }

    return nullptr;
}


uiWellSinglePropSel* uiWellPropSel::getPropSelFromListByIndex( int idx )
{
    return propflds_.validIdx(idx) ? propflds_[idx] : nullptr;
}
