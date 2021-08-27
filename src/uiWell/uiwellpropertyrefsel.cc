/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
________________________________________________________________________

-*/


#include "uiwellpropertyrefsel.h"
#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uiunitsel.h"
#include "uimsg.h"
#include "uiwelllogdisplay.h"
#include "uiwelllogcalc.h"

#include "elasticprop.h"
#include "elasticpropsel.h"
#include "unitofmeasure.h"
#include "property.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "wellman.h"

static const char* sKeyPlsSel = "Please select";


uiWellSinglePropSel::uiWellSinglePropSel( uiParent* p, const PropertyRef& pr,
					const PropertyRef* alternatepr )
    : uiGroup(p,pr.name())
    , propref_(pr)
    , altpropref_(alternatepr)
    , altPropChosen(this)
{
    auto* lcb = new uiLabeledComboBox(this, toUiString(pr.name()));
    lognmfld_ = lcb->box();
    mAttachCB( lognmfld_->selectionChanged, uiWellSinglePropSel::updateSelCB );

    uiUnitSel::Setup ussu( propref_.stdType() ); ussu.withnone( true );
    unfld_ = new uiUnitSel( this, ussu );
    unfld_->setPropType( propref_.stdType() );
    unfld_->attach( rightOf, lcb );

    if ( altpropref_ )
    {
	altbox_ = new uiCheckBox( this, toUiString(altpropref_->name()) );
	altbox_->attach( rightOf, unfld_ );
	mAttachCB( altbox_->activated, uiWellSinglePropSel::switchPropCB );
    }

    setHAlignObj( lognmfld_ );
}


uiWellSinglePropSel::~uiWellSinglePropSel()
{
    detachAllNotifiers();
}


static const char* sNoUnMeasLbl = "-";


bool uiWellSinglePropSel::setAvailableLogs( const Well::LogSet& wls )
{
    normnms_.setEmpty(); altnms_.setEmpty();
    normnms_.add( sKeyPlsSel ); altnms_.add( sKeyPlsSel );
    normunmeaslbls_.setEmpty(); altunmeaslbls_.setEmpty();
    normunmeaslbls_.add( sNoUnMeasLbl ); altunmeaslbls_.add( sNoUnMeasLbl );

    BoolTypeSet arealt;
    TypeSet<int> logidxs = wls.getSuitable( propref_.stdType(),
					    altpropref_, &arealt );
    for ( int idx=0; idx<logidxs.size(); idx++ )
    {
	const Well::Log& wl = wls.getLog( logidxs[idx] );
	const OD::String& lognm = wl.name();
	const BufferString& unmeaslbl = wl.unitMeasLabel();
	if ( arealt[idx] )
	    { altnms_.add( lognm ); altunmeaslbls_.add( unmeaslbl ); }
	else
	    { normnms_.add( lognm ); normunmeaslbls_.add( unmeaslbl ); }
    }

    const int nrnorm = normnms_.size();
    const int nralt = altnms_.size();

    if ( altpref_ < 0 )
    {
	altpref_ = nralt > nrnorm ? 1 : 0;
	if ( altbox_ )
	{
	    NotifyStopper ns( altbox_->activated );
	    altbox_->setChecked( altpref_ );
	}
    }

    updateLogInfo();
    return nrnorm + nralt > 2;
}


void uiWellSinglePropSel::updateSelCB( CallBacker* )
{
    altPropChosen.trigger();
}


void uiWellSinglePropSel::switchPropCB( CallBacker* )
{
    if ( altpropref_ )
	updateLogInfo();
}


void uiWellSinglePropSel::updateLogInfo()
{
    const bool isalt = altPropSelected();
    const BufferStringSet& nms = isalt ? altnms_ : normnms_;
    const int sz = nms.size();
    if ( sz < 1 )
	return;

    const BufferString selpropnm = selPropRef().name();
    int selidx = 0;
    if ( sz < 2 || nms.get(0) != sKeyPlsSel )
	selidx = nms.nearestMatch( selpropnm );
    else
    {
	BufferStringSet matchnms = nms;
	matchnms.removeSingle( 0 );
	selidx = matchnms.nearestMatch( selpropnm ) + 1;
    }

    lognmfld_->setEmpty(); lognmfld_->addItems( nms );
    BufferString selunstr;
    if ( selidx >= 0 )
    {
	lognmfld_->setCurrentItem( selidx );
	selunstr.set( (isalt ? altunmeaslbls_ : normunmeaslbls_).get(selidx) );
    }

    if ( !selunstr.isEmpty() )
	unfld_->setUnit( selunstr );

    updateSelCB( nullptr );
}


void uiWellSinglePropSel::setCurrent( const char* lnm )
{
    lognmfld_->setCurrentItem( lnm );
}


void uiWellSinglePropSel::setUOM( const UnitOfMeasure& um )
{
    unfld_->setUnit( &um );
}


void uiWellSinglePropSel::set( const char* txt, bool alt,
				const UnitOfMeasure* um)
{
    selectAltProp( alt );
    setCurrent( txt );
    unfld_->setUnit( um );
}


const char* uiWellSinglePropSel::logName() const
{
    return lognmfld_->text();
}


const UnitOfMeasure* uiWellSinglePropSel::getUnit() const
{
    return unfld_->getUnit();
}


void uiWellSinglePropSel::selectAltProp( bool yn )
{
    if ( altbox_ )
	altbox_->setChecked( yn );
}


bool uiWellSinglePropSel::altPropSelected() const
{
    return altbox_ ? altbox_->isChecked() : false;
}


const PropertyRef& uiWellSinglePropSel::selPropRef() const
{
    return altPropSelected() ? *altpropref_ : propref_;
}



uiWellPropSel::uiWellPropSel( uiParent* p, const PropertyRefSelection& prs )
    : uiGroup(p," property selection from well logs")
    , logCreated(this)
{
    for ( const auto* pr : prs )
    {
	if ( pr->isThickness() || pr->hasFixedDef() )
	    continue;

	const PropertyRef* altpr = nullptr;
	const bool issonic = pr->isCompatibleWith( Mnemonic::defDT() );
	const bool isvel = pr->isCompatibleWith( Mnemonic::defPVEL() );
	if ( issonic || isvel )
	    altpr = PROPS().getByMnemonic( issonic ? Mnemonic::defPVEL()
						   : Mnemonic::defDT() );

	auto* fld = new uiWellSinglePropSel( this, *pr, altpr );
	mAttachCB( fld->altPropChosen, uiWellPropSel::updateSelCB );
	if ( propflds_.isEmpty() )
	    setHAlignObj( fld );
	else
	    fld->attach( alignedBelow, propflds_[propflds_.size()-1] );

	propflds_ += fld;
    }

    for ( int idx=0; idx<propflds_.size(); idx ++ )
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


void uiWellPropSel::updateSelCB( CallBacker* c )
{
    if ( !c )
	return;

    mDynamicCastGet(uiWellSinglePropSel*, fld, c);
    if ( !fld )
	return;
    const Well::Data* wd = Well::MGR().get( wellid_ );
    if  ( !wd )
	return;

    const Well::Log* log = wd->logs().getLog( fld->logName() );
    const UnitOfMeasure* logun = log ? log->unitOfMeasure() : nullptr;
    if ( !logun )
    {
	const UnitOfMeasure* emptyuom = nullptr;
	fld->setUOM( *emptyuom );
	return;
    }
    const PropertyRef& propref = fld->normPropRef();
    const PropertyRef* altpropref = fld->altPropRef();
    bool isreverted;
    if ( propref.stdType() == logun->propType() )
	isreverted = false;
    else if ( altpropref && altpropref->stdType() == logun->propType() )
	isreverted = true;
    else
	return;
    fld->selectAltProp( isreverted );
    fld->setUOM ( *logun );
}


bool uiWellPropSel::setAvailableLogs( const Well::LogSet& logs,
				      BufferStringSet& notokpropnms  )
{
    bool allok = true;
    notokpropnms.erase();
    for ( int iprop=0; iprop<propflds_.size(); iprop++ )
    {
	if ( !propflds_[iprop]->setAvailableLogs(logs) )
	{
	    notokpropnms.add( propflds_[iprop]->normPropRef().name() );
	    allok = false;
	}
    }
    return allok;
}


bool uiWellPropSel::isOK() const
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	if ( FixedString(propflds_[idx]->logName()) == sKeyPlsSel )
	{
	    uiMSG().error(tr("Please create/select a log for %1")
			.arg(propflds_[idx]->normPropRef().name()));
	    return false;
	}
    }
    return true;
}


void uiWellPropSel::setLog( const Mnemonic::StdType tp,
				const char* nm, bool usealt,
				const UnitOfMeasure* uom, int idx )
{
    if ( !propflds_.validIdx(idx) )
	{ pErrMsg("Idx failure"); return; }
    if ( !propflds_[idx]->normPropRef().hasType( tp ) )
	{ pErrMsg("Type failure"); return; }
    propflds_[idx]->set( nm, usealt, uom );
}


bool uiWellPropSel::getLog( const Mnemonic::StdType proptyp,
		BufferString& retlognm, bool& retisrev, BufferString& retuom,
		int idx ) const
{
    if ( !propflds_.validIdx(idx) )
	{ pErrMsg("Idx failure"); return false; }

    const uiWellSinglePropSel& fld = *propflds_[idx];
    const bool isrev = fld.altPropSelected();
    const PropertyRef& pr = fld.normPropRef();
    const PropertyRef* altpr = isrev ? fld.altPropRef() : 0;
    if ( !pr.hasType(proptyp) && !(altpr && altpr->hasType(proptyp)) )
    {
	pErrMsg("Type failure");
	return false;
    }

    const BufferString lognm = fld.logName();
    if ( lognm.isEmpty() || lognm == sKeyPlsSel )
	return false;

    retlognm = lognm;
    retisrev = isrev;
    const UnitOfMeasure* uom = fld.getUnit();
    if ( !uom )
	retuom.setEmpty();
    else
	retuom = *uom->symbol() ? uom->symbol() : uom->name().buf();

    return true;
}


uiWellSinglePropSel* uiWellPropSel::getPropSelFromListByName(
						const BufferString& propnm )
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	uiWellSinglePropSel* fld = propflds_[idx];
	if ( propnm == fld->normPropRef().name()
	  || (fld->altPropRef() && propnm == fld->altPropRef()->name()) )
	    return fld;
    }

    return nullptr;
}


uiWellSinglePropSel* uiWellPropSel::getPropSelFromListByIndex( int idx )
{
    return propflds_.validIdx(idx) ? propflds_[idx] : nullptr;
}


void uiWellPropSel::createLogPushed( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb);
    const int idxofbut = createbuts_.indexOf( but );
    if ( !propflds_.validIdx( idxofbut ) )
	return;

    TypeSet<MultiID> idset; idset += wellid_;
    uiWellLogCalc dlg( this, idset );
    dlg.setOutputLogName( propflds_[idxofbut]->selPropRef().name() );
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
