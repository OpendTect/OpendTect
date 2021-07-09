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


uiWellSingleMnemSel::uiWellSingleMnemSel( uiParent* p, const Mnemonic& mn,
					const Mnemonic* alternatemn )
    : uiGroup( p, mn.name() )
    , mnem_(mn)
    , altmnem_(alternatemn)
    , altbox_(nullptr)
    , altmn_(-1)
    , altMnemChosen(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox(this, toUiString(mn.name()));
    lognmfld_ = lcb->box();
    lognmfld_->selectionChanged.notify(
			       mCB(this,uiWellSingleMnemSel,updateSelCB) );

    uiUnitSel::Setup ussu( mnem_.stdType() ); ussu.withnone( true );
    unfld_ = new uiUnitSel( this, ussu );
    unfld_->setPropType( mnem_.stdType() );
    unfld_->attach( rightOf, lcb );

    if ( altmnem_ )
    {
	altbox_ = new uiCheckBox( this, toUiString(altmnem_->name()) );
	altbox_->attach( rightOf, unfld_ );
	altbox_->activated.notify( mCB(this,uiWellSingleMnemSel,switchMnemCB) );
    }

    setHAlignObj( lognmfld_ );
}


static const char* sNoUnMeasLbl = "-";


bool uiWellSingleMnemSel::setAvailableLogs( const Well::LogSet& wls )
{
    normnms_.setEmpty(); altnms_.setEmpty();
    normnms_.add( sKeyPlsSel ); altnms_.add( sKeyPlsSel );
    normunmeaslbls_.setEmpty(); altunmeaslbls_.setEmpty();
    normunmeaslbls_.add( sNoUnMeasLbl ); altunmeaslbls_.add( sNoUnMeasLbl );

    BoolTypeSet arealt;
    TypeSet<int> logidxs = wls.getSuitable( mnem_.stdType(),
					     altmnem_, &arealt );
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

    if ( altmn_ < 0 )
    {
	altmn_ = nralt > nrnorm ? 1 : 0;
	if ( altbox_ )
	{
	    NotifyStopper ns( altbox_->activated );
	    altbox_->setChecked( altmn_ );
	}
    }

    updateLogInfo();
    return nrnorm + nralt > 2;
}


void uiWellSingleMnemSel::updateSelCB( CallBacker* )
{
    altMnemChosen.trigger();
}


void uiWellSingleMnemSel::switchMnemCB( CallBacker* )
{
    if ( altmnem_ )
	updateLogInfo();
}


void uiWellSingleMnemSel::updateLogInfo()
{
    const bool isalt = altMnemSelected();
    const BufferStringSet& nms = isalt ? altnms_ : normnms_;
    const int sz = nms.size();
    if ( sz < 1 )
	return;

    const BufferString selmn = selMnem().name();
    int selidx = 0;
    if ( sz < 2 || nms.get(0) != sKeyPlsSel )
	selidx = nms.nearestMatch( selmn );
    else
    {
	BufferStringSet matchnms = nms;
	matchnms.removeSingle( 0 );
	selidx = matchnms.nearestMatch( selmn ) + 1;
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


void uiWellSingleMnemSel::setCurrent( const char* lnm )
{
    lognmfld_->setCurrentItem( lnm );
}


void uiWellSingleMnemSel::setUOM( const UnitOfMeasure& um )
{
    unfld_->setUnit( &um );
}


void uiWellSingleMnemSel::set( const char* txt, bool alt,
				const UnitOfMeasure* um)
{
    selectAltMnem( alt );
    setCurrent( txt );
    unfld_->setUnit( um );
}


const char* uiWellSingleMnemSel::logName() const
{
    return lognmfld_->text();
}


const UnitOfMeasure* uiWellSingleMnemSel::getUnit() const
{
    return unfld_->getUnit();
}


void uiWellSingleMnemSel::selectAltMnem( bool yn )
{
    if ( altbox_ )
	altbox_->setChecked( yn );
}


bool uiWellSingleMnemSel::altMnemSelected() const
{
    return altbox_ ? altbox_->isChecked() : false;
}


const Mnemonic& uiWellSingleMnemSel::selMnem() const
{
    return altMnemSelected() ? *altmnem_ : mnem_;
}



uiWellMnemSel::uiWellMnemSel( uiParent* p, const MnemonicSelection& mns )
    : uiGroup(p," property selection from well logs")
    , logCreated(this)
{
    for ( int idx=0; idx<mns.size(); idx ++ )
    {
	const Mnemonic& mn = *mns[idx];
	/*if ( mn.isThickness() || mn.hasFixedDef() )
	    continue;*/

	const Mnemonic* altmn = nullptr;
	const bool issonic = mn.hasType( PropertyRef::Son );
	const bool isvel = mn.hasType( PropertyRef::Vel );
	if ( issonic || isvel )
	{
	    const int pidx = PROPS().indexOf( issonic ? PropertyRef::Vel
						      : PropertyRef::Son );
	    if ( pidx >= 0 )
		altmn = &PROPS().get(pidx).mnem();
	}

	uiWellSingleMnemSel* fld = new	uiWellSingleMnemSel( this, mn, altmn );
	fld->altMnemChosen.notify( mCB(this,uiWellMnemSel,updateSelCB) );
	if ( mnemflds_.size() > 0 )
	    fld->attach( alignedBelow, mnemflds_[mnemflds_.size()-1] );
	else
	    setHAlignObj( fld );

	mnemflds_ += fld;
    }

    for ( int idx=0; idx<mnemflds_.size(); idx ++ )
    {
	uiToolButton* createbut = new uiToolButton( this, "newlog",
		tr("Create a log from other logs"),
		mCB(this,uiWellMnemSel,createLogPushed) );
	uiToolButton* viewbut = new uiToolButton( this, "view_log",
		tr("View log"), mCB(this,uiWellMnemSel,viewLogPushed) );
	if ( idx )
	    createbut->attach( ensureBelow, createbuts_[idx-1] );
	for ( int idothermn=0; idothermn<mnemflds_.size(); idothermn++ )
	    createbut->attach( ensureRightOf, mnemflds_[idothermn] );
	viewbut->attach( rightOf, createbut );
	createbuts_ += createbut;
	viewbuts_ += viewbut;
    }
}


void uiWellMnemSel::updateSelCB( CallBacker* c )
{
    if ( !c )
	return;

    mDynamicCastGet(uiWellSingleMnemSel*, fld, c);
    if ( !fld )
	return;
    const Well::Data* wd = Well::MGR().get( wellid_ );
    if  ( !wd )
	return;

    const Well::Log* log = wd->logs().getLog( fld->logName() );
    const char* logunitnm = log ? log->unitMeasLabel() : 0;
    const UnitOfMeasure* logun = UnitOfMeasure::getGuessed( logunitnm );
    if ( !logun )
    {
	const UnitOfMeasure* emptyuom = nullptr;
	fld->setUOM( *emptyuom );
	return;
    }
    const Mnemonic& mnem = fld->normMnem();
    const Mnemonic* altmnem = fld->altMnem();
    bool isreverted;
    if ( mnem.stdType() == logun->propType() )
	isreverted = false;
    else if ( altmnem && altmnem->stdType() == logun->propType() )
	isreverted = true;
    else
	return;
    fld->selectAltMnem( isreverted );
    fld->setUOM ( *logun );
}


bool uiWellMnemSel::setAvailableLogs( const Well::LogSet& logs,
				      BufferStringSet& notokmns  )
{
    bool allok = true;
    notokmns.erase();
    for ( int idx=0; idx<mnemflds_.size(); idx++ )
    {
	if ( !mnemflds_[idx]->setAvailableLogs(logs) )
	{
	    notokmns.add( mnemflds_[idx]->normMnem().name() );
	    allok = false;
	}
    }
    return allok;
}


bool uiWellMnemSel::isOK() const
{
    for ( int idx=0; idx<mnemflds_.size(); idx++ )
    {
	if ( FixedString(mnemflds_[idx]->logName()) == sKeyPlsSel )
	{
	    uiMSG().error(tr("Please create/select a log for %1")
			.arg(mnemflds_[idx]->normMnem().name()));
	    return false;
	}
    }
    return true;
}


void uiWellMnemSel::setLog( const PropertyRef::StdType tp,
				const char* nm, bool usealt,
				const UnitOfMeasure* uom, int idx )
{
    if ( !mnemflds_.validIdx(idx) )
	{ pErrMsg("Idx failure"); return; }
    if ( !mnemflds_[idx]->normMnem().hasType( tp ) )
	{ pErrMsg("Type failure"); return; }
    mnemflds_[idx]->set( nm, usealt, uom );
}


bool uiWellMnemSel::getLog( const PropertyRef::StdType proptyp,
		BufferString& retlognm, bool& retisrev, BufferString& retuom,
		int idx ) const
{
    if ( !mnemflds_.validIdx(idx) )
	{ pErrMsg("Idx failure"); return false; }

    const uiWellSingleMnemSel& fld = *mnemflds_[idx];
    const bool isrev = fld.altMnemSelected();
    const Mnemonic& pr = fld.normMnem();
    const Mnemonic* altpr = isrev ? fld.altMnem() : nullptr;
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


uiWellSingleMnemSel* uiWellMnemSel::getMnemSelFromListByName(
						const BufferString& propnm )
{
    for ( int idx=0; idx<mnemflds_.size(); idx++ )
    {
	uiWellSingleMnemSel* fld = mnemflds_[idx];
	if ( propnm == fld->normMnem().name()
	  || (fld->altMnem() && propnm == fld->altMnem()->name()) )
	    return fld;
    }

    return nullptr;
}


uiWellSingleMnemSel* uiWellMnemSel::getMnemSelFromListByIndex( int idx )
{
    return mnemflds_.validIdx(idx) ? mnemflds_[idx] : nullptr;
}


void uiWellMnemSel::createLogPushed( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb);
    const int idxofbut = createbuts_.indexOf( but );
    if ( !mnemflds_.validIdx( idxofbut ) )
	return;

    TypeSet<MultiID> idset; idset += wellid_;
    uiWellLogCalc dlg( this, idset );
    dlg.setOutputLogName( mnemflds_[idxofbut]->selMnem().name() );
    dlg.go();

    if ( dlg.haveNewLogs() )
    {
	logCreated.trigger();
	mnemflds_[idxofbut]->setCurrent( dlg.getOutputLogName() );
    }
}


void uiWellMnemSel::viewLogPushed( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb);
    const int idxofbut = viewbuts_.indexOf( but );
    if ( !mnemflds_.validIdx( idxofbut ) )
	return;

    const BufferString lognm( mnemflds_[idxofbut]->logName() );
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
