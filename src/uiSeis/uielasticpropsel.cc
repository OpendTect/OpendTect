/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uielasticpropsel.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitabstack.h"
#include "uitoolbutton.h"

#include "elasticpropseltransl.h"
#include "ioman.h"
#include "od_helpids.h"
#include "propertyref.h"


// uiElasticPropSelGrp::uiSelInpGrp

uiElasticPropSelGrp::uiSelInpGrp::uiSelInpGrp( uiParent* p,
				const PropertyRefSelection& prs, int idx )
    : uiGroup( p, "Inp data group" )
    , idx_(idx)
    , prs_(prs)
{
    varnmfld_ = new uiGenInput( this, tr("For") );
    varnmfld_->setElemSzPol( uiObject::Small );

    auto* lbl = new uiLabeledComboBox( this, uiStrings::sUse() );
    inpfld_ = lbl->box();
    for ( const auto* pr : prs_ )
	if ( !pr->isThickness() )
	    inpfld_->addItem( pr->name() );
    inpfld_->addItem( tr("Constant") );
    lbl->attach( rightOf, varnmfld_ );

    mAttachCB( inpfld_->selectionChanged,
	       uiElasticPropSelGrp::uiSelInpGrp::selVarCB );

    ctefld_ = new uiGenInput( this, uiStrings::sValue(), FloatInpSpec() );
    ctefld_->attach( rightOf, lbl );
    ctefld_->setElemSzPol( uiObject::Small );

    setHAlignObj( lbl );
    mAttachCB( postFinalize(), uiElasticPropSelGrp::uiSelInpGrp::initGrp );
}


uiElasticPropSelGrp::uiSelInpGrp::~uiSelInpGrp()
{
    detachAllNotifiers();
}


void uiElasticPropSelGrp::uiSelInpGrp::initGrp( CallBacker* )
{
    selVarCB( nullptr );
}


void uiElasticPropSelGrp::uiSelInpGrp::selVarCB( CallBacker* )
{
    const int selidx = inpfld_->currentItem();
    isconstant_ = selidx == inpfld_->size() -1;
    ctefld_->display( isconstant_ );
}


const char* uiElasticPropSelGrp::uiSelInpGrp::textOfVariable() const
{
    return isactive_ ? isconstant_ ? toString( ctefld_->getFValue() )
				   : inpfld_->text()
		     : nullptr;
}


void uiElasticPropSelGrp::uiSelInpGrp::setConstant( double val )
{
    isconstant_ = true;
    inpfld_->setCurrentItem( inpfld_->size()-1 );
    ctefld_->setValue( val );
    ctefld_->display( isconstant_ );
}


void uiElasticPropSelGrp::uiSelInpGrp::setVariable( const char* txt )
{
    isconstant_ = false;
    ctefld_->display( isconstant_ );
    inpfld_->setCurrentItem( txt );
}


void uiElasticPropSelGrp::uiSelInpGrp::use( const Math::Formula& form )
{
    const int nrvars = form.isOK() ? form.nrInputs() : 0;
    isactive_ =  idx_ < nrvars;
    display( isactive_ );
    if ( !isactive_ )
	return;

    const BufferString varnm = form.variableName( idx_ );
    varnmfld_->setText( varnm );
    if ( form.isConst(idx_) )
	setConstant( form.getConstVal(idx_) );
    else if ( !form.isSpec(idx_) )
    {
	const PropertyRef* pr = prs_.getByName( form.inputDef(idx_), false );
	if ( !pr && form.inputMnemonic(idx_) )
	    pr = prs_.getByMnemonic( *form.inputMnemonic(idx_) );
	if ( pr )
	    setVariable( pr->name().str() );
    }
}


void uiElasticPropSelGrp::uiSelInpGrp::set( Math::Formula& form ) const
{
    if ( !isDisplayed() )
	return;

    const BufferString inptxt( textOfVariable() );
    if ( !inptxt.isEmpty() && inptxt != form.inputDef(idx_) )
	form.setInputDef( idx_, inptxt );
    //TODO: Add missing UoM field
}


// uiElasticPropSelGrp

uiElasticPropSelGrp::uiElasticPropSelGrp( uiParent* p,
					const PropertyRefSelection& prs,
					ElasticPropertyRef& elprop,
				    const ObjectSet<const ElasticFormula>& el )
    : uiGroup( p, "Elastic Prop Sel Grp" )
    , prs_(prs)
    , elpropref_(elprop)
    , availableformulas_(el)
{
    BufferStringSet predeftitles;
    int maxnrinputs = 0;
    for ( const auto* availableformula : availableformulas_ )
    {
	predeftitles.add( availableformula->name() );
	const int nrinputs = availableformula->nrInputs();
	if ( nrinputs > maxnrinputs )
	    maxnrinputs = nrinputs;
    }

    if ( maxnrinputs < 8 ) // Allow enough variables for free formulas
	maxnrinputs = 8;

    selmathfld_ = new uiLabeledComboBox( this, tr("Compute from") );
    selmathfld_->box()->addItem( tr("Defined quantity") );
    selmathfld_->box()->addItems( predeftitles );
    selmathfld_->box()->addItem( tr("Formula") );
    mAttachCB( selmathfld_->box()->selectionChanged,
	       uiElasticPropSelGrp::selComputeFldChgCB );

    formfld_ = new uiGenInput( this, tr("Formula") );
    formfld_->attach( alignedBelow, selmathfld_ );
    mAttachCB( formfld_->valueChanged, uiElasticPropSelGrp::selFormulaChgCB );

    singleinpfld_ = new uiLabeledComboBox( this, uiStrings::sUse() );
    singleinpfld_->attach( alignedBelow, selmathfld_ );
    BufferStringSet propnms;
    for ( const auto* pr : prs_ )
    {
	if ( elprop.isCompatibleWith(pr->mn()) )
	    propnms.addIfNew( pr->name() );
    }

    singleinpfld_->box()->addItems( propnms );

    uiObject* lastobj = (uiObject*) formfld_;
    for ( int idx=0; idx<maxnrinputs; idx++ )
    {
	auto* inpgrp = new uiElasticPropSelGrp::uiSelInpGrp( this, prs_, idx );
	inpgrp->attach( alignedBelow, lastobj );
	inpgrps_.add( inpgrp );
	lastobj = (uiObject*) inpgrp;
    }

    mAttachCB( postFinalize(), uiElasticPropSelGrp::initGrpCB );
}


uiElasticPropSelGrp::~uiElasticPropSelGrp()
{
    detachAllNotifiers();
}


void uiElasticPropSelGrp::initGrpCB( CallBacker* )
{
    putToScreen();
}


void uiElasticPropSelGrp::selComputeFldChgCB( CallBacker* )
{
    if ( selmathfld_->box()->currentItem() == selmathfld_->box()->size()-1 )
	formfld_->setText( 0 );

    selFormulaChgCB( nullptr );
}


void uiElasticPropSelGrp::selFormulaChgCB( CallBacker* )
{
    const int selidx = selmathfld_->box()->currentItem();
    ElasticFormula eform( elpropref_.name(), nullptr, elpropref_.elasticType());

    if ( selidx == selmathfld_->box()->size()-1 )
	eform.setText( formfld_->text() );
    else
    {
	const ElasticFormula* ef = availableformulas_.validIdx(selidx-1) ?
				    availableformulas_[selidx-1] : nullptr;
	formfld_->setText( ef ? ef->text() : "" );
	if ( ef )
	    eform = *ef;
    }

    const BufferString formulanm( selmathfld_->box()->text() );
    eform.setName( formulanm.buf() );
    elpropref_.setFormula( eform );

    putToScreen();
}


bool uiElasticPropSelGrp::isDefinedQuantity() const
{
    return selmathfld_->box()->currentItem() == 0;
}


bool uiElasticPropSelGrp::getFromScreen()
{
    if ( isDefinedQuantity() )
    {
	const PropertyRef* pr = prs_.getByName( singleinpfld_->box()->text(),
						false );
	if ( !pr )
	    return false;

	if ( pr != elpropref_.ref() )
	    elpropref_.setRef( pr );
    }
    else
    {
	const bool isuserform = selmathfld_->box()->currentItem() ==
				selmathfld_->box()->size()-1;
	const BufferString formnm( isuserform ? elpropref_.name()
					      : selmathfld_->box()->text() );
	const BufferString formtxt( formfld_->text() );
	ElasticFormula eform( formnm, formtxt, elpropref_.elasticType() );
	const ElasticFormula* inpform = elpropref_.formula();
	if ( inpform )
	{
	    eform = *inpform;
	    if ( !isuserform && formnm != eform.name() )
		eform.setName( formnm );
	    if ( formtxt != eform.text() )
		eform.setText( formfld_->text() );
	}

	for ( const auto* inpgrp : inpgrps_ )
	    inpgrp->set( eform );

	if ( !inpform || (inpform && eform != *inpform) )
	    elpropref_.setFormula( eform );
    }

    return true;
}


void uiElasticPropSelGrp::putToScreen()
{
    const ElasticFormula* eform = elpropref_.formula();
    BufferString expr;
    if ( eform )
	expr.set( eform->text() );

    uiComboBox* mathbox = selmathfld_->box();
    const bool hasexpr = !expr.isEmpty() ||
			 mathbox->currentItem() == mathbox->size()-1;

    if ( eform )
    {
	const BufferString formnm( eform->name() );
	if ( !formnm.isEmpty() && mathbox->isPresent(formnm) )
	    mathbox->setCurrentItem( formnm.buf() );
	else
	    mathbox->setCurrentItem( mathbox->size()-1 );
    }
    else
	mathbox->setCurrentItem( 0 );

    if ( elpropref_.ref() )
    {
	singleinpfld_->box()->setCurrentItem( elpropref_.ref()->name().buf() );
	for ( auto* inpgrp : inpgrps_ )
	    inpgrp->display( false );
    }
    else if ( eform )
    {
	formfld_->setText( expr );
	for ( auto* inpgrp : inpgrps_ )
	    inpgrp->use( *eform );
    }

    singleinpfld_->display( !hasexpr );
    formfld_->display( hasexpr );
}



static const char** props = ElasticFormula::TypeNames();

#define mErrRet(s,act) { uiMSG().error(s); act; }

uiElasticPropSelDlg::uiElasticPropSelDlg( uiParent* p,
					const PropertyRefSelection& prs,
					ElasticPropSelection& elsel )
    : uiDialog(p,uiDialog::Setup(tr("Elastic Model"),
	       tr("Specify how to obtain density and "
		  "p-wave and s-wave velocities")
		,mODHelpKey(mElasticPropSelDlgHelpID) ))
    , ctio_(*mMkCtxtIOObj(ElasticPropSelection))
    , prs_(prs)
    , elpropsel_(elsel)
    , orgelpropsel_(elsel)
{
    if ( prs_.size() < 2 )
	mErrRet( tr("No property found"), return );

    ts_ = new uiTabStack( this, "Property selection tab stack" );
    ObjectSet<uiGroup> tgs;

    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	tgs += new uiGroup( ts_->tabGroup(), props[idx] );
	ObjectSet<const Math::Formula> formulas;
	ElFR().getByType( tp, formulas );
	ElasticPropertyRef* epr = elpropsel_.getByType( tp );
	if ( !epr )
	    continue;

	ObjectSet<const ElasticFormula> elasformulas;
	for ( const auto* fm : formulas )
	    elasformulas.add( sCast(const ElasticFormula*,fm) );

	propflds_ += new uiElasticPropSelGrp( tgs[idx], prs_, *epr,
					      elasformulas );
	ts_->addTab( tgs[idx], toUiString(props[idx]) );
    }

    mAttachCB( ts_->selChange(), uiElasticPropSelDlg::screenSelectionChangedCB);

    auto* gengrp = new uiGroup( this, "buttons" );
    gengrp->attach( ensureBelow, ts_ );
    auto* opentb = new uiToolButton( gengrp, "open",
				tr("Open stored property selection"),
				mCB(this,uiElasticPropSelDlg,openPropSelCB) );
    auto* stb = new uiToolButton( gengrp, "save",
				tr("Save property selection"),
				mCB(this,uiElasticPropSelDlg,savePropSelCB) );
    stb->attach( rightOf, opentb );
}


uiElasticPropSelDlg::~uiElasticPropSelDlg()
{
    detachAllNotifiers();
    delete ctio_.ioobj_;
    delete &ctio_;
}


void uiElasticPropSelDlg::screenSelectionChangedCB( CallBacker* )
{
    screenSelectionChanged();
}


bool uiElasticPropSelDlg::screenSelectionChanged()
{
    for ( auto* propfld : propflds_ )
	if ( !propfld->getFromScreen() )
	    return false;

    return true;
}


void uiElasticPropSelDlg::elasticPropSelectionChanged( CallBacker* )
{
    for ( auto* propfld : propflds_ )
	propfld->putToScreen();
}


bool uiElasticPropSelDlg::rejectOK( CallBacker* )
{
    elpropsel_ = orgelpropsel_;
    return true;
}


bool uiElasticPropSelDlg::acceptOK( CallBacker* )
{
    if ( !screenSelectionChanged() )
	return false;

    uiString msg;
    if ( !elpropsel_.isOK(&prs_) || !elpropsel_.isValidInput(&msg) )
	mErrRet( msg, return false );

    if ( ctio_.ioobj_ )
	doStore( *ctio_.ioobj_ );

    return true;
}


bool uiElasticPropSelDlg::savePropSel()
{
    ctio_.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() != uiDialog::Accepted || !dlg.ioObj() )
	return false;

    ctio_.setObj( dlg.ioObj()->clone() );
    return doStore( *ctio_.ioobj_ );
}


bool uiElasticPropSelDlg::doStore( const IOObj& ioobj )
{
    if ( !screenSelectionChanged() )
	return false;

    if ( !elpropsel_.put(&ioobj) )
	mErrRet( ioobj.phrCannotWriteObj(), return false )

    return true;
}


bool uiElasticPropSelDlg::openPropSel()
{
    ctio_.ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() != uiDialog::Accepted || !dlg.ioObj() )
	return false;

    ctio_.setObj( dlg.ioObj()->clone() );
    return doRead( *ctio_.ioobj_ );
}


bool uiElasticPropSelDlg::doRead( const IOObj& ioobj )
{
    PtrMan<ElasticPropSelection> elp = elpropsel_.getByIOObj( &ioobj, &prs_ );
    if ( !elp )
	mErrRet( ioobj.phrCannotReadObj(), return false )

    elpropsel_ = *elp;
    elasticPropSelectionChanged( nullptr );

    return true;
}
