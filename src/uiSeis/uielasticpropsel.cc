/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uielasticpropsel.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiioobjseldlg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitabstack.h"
#include "uitoolbutton.h"

#include "elasticpropseltransl.h"
#include "ioman.h"
#include "propertyref.h"
#include "strmprov.h"
#include "od_helpids.h"

uiElasticPropSelGrp::uiSelInpGrp::uiSelInpGrp( uiParent* p,
				const BufferStringSet& ppnms, int idx )
    : uiGroup( p, "Inp data group" )
    , idx_(idx)
    , propnms_(ppnms)
    , isactive_(false)
{
    varnmfld_ = new uiGenInput( this, tr("For") );
    varnmfld_->setElemSzPol( uiObject::Small );

    auto* lbl = new uiLabeledComboBox( this, uiStrings::sUse() );
    lbl->attach( rightOf, varnmfld_ );

    inpfld_ = lbl->box();
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
    display( false );
    selVarCB( nullptr );
}


void uiElasticPropSelGrp::uiSelInpGrp::fillList()
{
    const BufferString seltxt = inpfld_->text();
    inpfld_->setEmpty();
    inpfld_->addItems( propnms_ );
    inpfld_->addItem( tr("Constant") );
    inpfld_->setCurrentItem( seltxt.buf() );
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
    isactive_ = true;
    isconstant_ = true;
    inpfld_->setCurrentItem( inpfld_->size()-1 );
    ctefld_->setValue( val );
    ctefld_->display( isconstant_ );
}


void uiElasticPropSelGrp::uiSelInpGrp::setVariable( const char* txt )
{
    isactive_ = txt;
    isconstant_ = false;
    ctefld_->display( isconstant_ );
    const int nearidx = propnms_.nearestMatch( txt );
    if ( nearidx >= 0 )
	inpfld_->setCurrentItem( nearidx );
    else
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
    const int nearidx = propnms_.nearestMatch( varnm );
    if ( nearidx >= 0 )
	inpfld_->setCurrentItem( nearidx );
}



uiElasticPropSelGrp::uiElasticPropSelGrp( uiParent* p,
					const BufferStringSet& prs,
					ElasticPropertyRef& elprop,
				    const ObjectSet<const ElasticFormula>& el )
    : uiGroup( p, "Elastic Prop Sel Grp" )
    , propnms_(prs)
    , elpropref_(elprop)
    , availableformulas_(el)
{
    BufferStringSet predeftitles;
    for ( const auto* availableformula : availableformulas_ )
	predeftitles.add( availableformula->name() );

    selmathfld_ = new uiLabeledComboBox( this, tr("Compute from") );
    selmathfld_->box()->addItem( tr("Defined quantity") );
    selmathfld_->box()->addItems( predeftitles );
    selmathfld_->box()->addItem( tr("Formula") );
    mAttachCB( selmathfld_->box()->selectionChanged,
	       uiElasticPropSelGrp::selComputeFldChgCB );

    formfld_ = new uiGenInput( this, tr("Formula ") );
    formfld_->attach( alignedBelow, selmathfld_ );
    mAttachCB( formfld_->valueChanged, uiElasticPropSelGrp::selFormulaChgCB );

    singleinpfld_ = new uiLabeledComboBox( this, uiStrings::sUse() );
    singleinpfld_->attach( alignedBelow, selmathfld_ );
    const PropertyRefSet& props = PROPS();
    BufferStringSet propnms;
    for ( const auto* propnm : propnms_ )
    {
	const PropertyRef* pr = props.getByName( propnm->buf(), false );
	if ( !pr )
	    continue;

	if ( elprop.isCompatibleWith(pr->mn()) )
	    propnms.addIfNew( propnm->buf() );
    }

    singleinpfld_->box()->addItems( propnms );

    for ( int idx=0; idx<propnms_.size(); idx++ )
    {
	inpgrps_.add( new uiElasticPropSelGrp::uiSelInpGrp(this,propnms_,idx) );
	if ( idx )
	    inpgrps_[idx]->attach( alignedBelow, inpgrps_[idx-1] );
	else
	    inpgrps_[idx]->attach( alignedBelow, formfld_ );
    }

    storenamesep_ = new uiSeparator( this, "sep" );
    storenamesep_->attach( stretchedBelow, inpgrps_[inpgrps_.size()-1]  );

    storenamefld_ = new uiGenInput( this, tr("Quantity name") );
    storenamefld_->attach( alignedBelow, inpgrps_[inpgrps_.size()-1] );
    storenamefld_->attach( ensureBelow, storenamesep_ );

    updateRefPropNames();
}


uiElasticPropSelGrp::~uiElasticPropSelGrp()
{
    detachAllNotifiers();
}


void uiElasticPropSelGrp::updateRefPropNames()
{
    for ( int idx=0; idx<inpgrps_.size(); idx++ )
	inpgrps_[idx]->fillList();
}


void uiElasticPropSelGrp::selComputeFldChgCB( CallBacker* )
{
    if ( selmathfld_->box()->currentItem() == selmathfld_->box()->size() -1 )
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


const char* uiElasticPropSelGrp::quantityName() const
{
    return isDefinedQuantity() ? nullptr : storenamefld_->text();
}


void uiElasticPropSelGrp::getFromScreen()
{
    const BufferString prname( storenamefld_->text() );
    if ( prname != elpropref_.name() )
	elpropref_.setName( prname );

    if ( isDefinedQuantity() )
    {
	const PropertyRef* pr = PROPS().getByName( singleinpfld_->box()->text(),
						   false );
	if ( !pr || elpropref_.ref() == pr )
	    return;

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

	for ( int iinp=0; iinp<inpgrps_.size(); iinp++ )
	{
	    const BufferString inptxt( inpgrps_[iinp]->textOfVariable() );
	    if ( inptxt != eform.inputDef(iinp) )
		eform.setInputDef( iinp, inptxt );
		//TODO: Add missing UoM field
	}

	if ( !inpform || (inpform && eform != *inpform) )
	    elpropref_.setFormula( eform );
    }
}


void uiElasticPropSelGrp::putToScreen()
{
    const ElasticFormula* eform = elpropref_.formula();
    BufferString expr;
    if ( eform )
	expr = eform->text();

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

    storenamefld_->setText( elpropref_.name() );
    if ( elpropref_.ref() )
	singleinpfld_->box()->setCurrentItem( elpropref_.ref()->name().buf() );
    else if ( eform )
    {
	formfld_->setText( expr );
	for ( int idx=0; idx<inpgrps_.size(); idx++ )
	{
	    inpgrps_[idx]->use( *eform );
	    if ( !inpgrps_[idx]->isActive() )
		continue;

	    if ( eform->isConst(idx) )
		inpgrps_[idx]->setConstant( eform->getConstVal(idx) );
	    else if ( !eform->isSpec(idx) )
		inpgrps_[idx]->setVariable( eform->inputDef(idx) );
	}
    }

    singleinpfld_->display( !hasexpr );
    storenamefld_->display( hasexpr );
    storenamesep_->display( hasexpr );
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
    for ( const auto* pr : prs_ )
    {
	if ( !pr->isThickness() )
	    orgpropnms_.addIfNew( pr->name() );
    }

    if ( orgpropnms_.isEmpty() )
	mErrRet( tr("No property found"), return );

    propnms_ = orgpropnms_;

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

	propflds_ += new uiElasticPropSelGrp( tgs[idx], propnms_, *epr,
					      elasformulas );
	ts_->addTab( tgs[idx], toUiString(props[idx]) );
    }

    mAttachCB( ts_->selChange(), uiElasticPropSelDlg::screenSelectionChanged );

    auto* gengrp = new uiGroup( this, "buttons" );
    gengrp->attach( ensureBelow, ts_ );
    auto* opentb = new uiToolButton( gengrp, "open",
				tr("Open stored property selection"),
				mCB(this,uiElasticPropSelDlg,openPropSelCB) );
    auto* stb = new uiToolButton( gengrp, "save",
				tr("Save property selection"),
				mCB(this,uiElasticPropSelDlg,savePropSelCB) );
    stb->attach( rightOf, opentb );

    elasticPropSelectionChanged( nullptr );
}


uiElasticPropSelDlg::~uiElasticPropSelDlg()
{
    detachAllNotifiers();
    delete ctio_.ioobj_; delete &ctio_;
}


void uiElasticPropSelDlg::screenSelectionChanged( CallBacker* )
{
    screenSelectionChanged();
}


bool uiElasticPropSelDlg::screenSelectionChanged()
{
    if ( !ts_ ) // when no properties found
	return false;

    NotifyStopper ns( ts_->selChange() );
    propnms_ = orgpropnms_;
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	propflds_[idx]->getFromScreen();

	if ( !propflds_[idx]->quantityName() )
	    continue;

	BufferString quantitynm( propflds_[idx]->quantityName() );
	if ( quantitynm.isEmpty() )
	    mErrRet( tr("Please select a name for the new quantity"),
			ts_->setCurrentPage(idx); return false; )
	else if( propnms_.isPresent( quantitynm.buf() ) )
	{
	    uiString msg = tr("%1 already exists, please select "
			      "another name for this property")
			 .arg(quantitynm.buf());
	    mErrRet( msg, ts_->setCurrentPage(idx); return false; )
	}

	propnms_.addIfNew( quantitynm );
    }

    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->updateRefPropNames();

    return true;
}


void uiElasticPropSelDlg::elasticPropSelectionChanged( CallBacker* )
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->putToScreen();
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

    if( ctio_.ioobj_ )
	doStore( *ctio_.ioobj_ );

    uiString msg;
    if ( !elpropsel_.isOK(&prs_) || !elpropsel_.isValidInput(&msg) )
	mErrRet( msg, return false; );

    return true;
}


bool uiElasticPropSelDlg::savePropSel()
{
    ctio_.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    ctio_.setObj( dlg.ioObj()->clone() );
    return doStore( *ctio_.ioobj_ );
}


bool uiElasticPropSelDlg::doStore( const IOObj& ioobj )
{
    const BufferString fnm( ioobj.fullUserExpr(false) );
    StreamData sd = StreamProvider::createOStream( fnm );
    bool rv = false;
    if ( !sd.usable() )
	uiMSG().error( uiStrings::sCantOpenOutpFile() );
    else if ( !elpropsel_.put(&ioobj) )
	uiMSG().error( tr("Cann not write file") );
    else
	rv = true;

    sd.close();
    return rv;
}


bool uiElasticPropSelDlg::openPropSel()
{
    ctio_.ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    ctio_.setObj( dlg.ioObj()->clone() );
    const BufferString fnm( ctio_.ioobj_->fullUserExpr(true) );
    StreamData sd = StreamProvider::createIStream( fnm );
    if ( !sd.usable() )
	mErrRet( uiStrings::sCantOpenInpFile(), return false; )
    sd.close();

    if ( !doRead( ctio_.ioobj_->key() ) )
	mErrRet( tr("Unable to read elastic property selection"),
		 return false; );
    return true;
}


bool uiElasticPropSelDlg::doRead( const MultiID& mid )
{
    PtrMan<ElasticPropSelection> elp = elpropsel_.getByDBKey( mid );
    ctio_.setObj( IOM().get( mid ) );

    if ( !elp )
	return false;

    elpropsel_ = *elp;
    propnms_ = orgpropnms_;
    for ( const auto* elprop : elpropsel_ )
	propnms_.addIfNew( elprop->name() );

    elasticPropSelectionChanged( nullptr );

    return true;
}
