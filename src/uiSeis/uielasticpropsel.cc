/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uielasticpropsel.cc,v 1.14 2012-07-17 13:45:20 cvsbruno Exp $";

#include "uielasticpropsel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uiseparator.h"
#include "uitabstack.h"
#include "uitoolbutton.h"

#include "elasticpropseltransl.h"
#include "ioman.h"
#include "mathexpression.h"
#include "propertyref.h"
#include "strmprov.h"

uiElasticPropSelGrp::uiSelInpGrp::uiSelInpGrp( uiParent* p, 
				const BufferStringSet& ppnms, int idx ) 
    : uiGroup( p, "Inp data group" )
    , idx_(idx)
    , propnms_(ppnms)
    , isactive_(false)		     
{
    varnmfld_ = new uiGenInput( this, "For" );
    varnmfld_->setElemSzPol( uiObject::Small );

    uiLabeledComboBox* lbl = new uiLabeledComboBox( this, "use");
    lbl->attach( rightOf, varnmfld_ );

    inpfld_ = lbl->box();
    inpfld_->selectionChanged.notify( 
		    mCB(this,uiElasticPropSelGrp::uiSelInpGrp,selVarCB ) );

    ctefld_ = new uiGenInput( this, "Value", IntInpSpec() );
    ctefld_->attach( rightOf, lbl );
    ctefld_->setElemSzPol( uiObject::Small );

    display( false );

    setHAlignObj( lbl );
    selVarCB(0);
}


void uiElasticPropSelGrp::uiSelInpGrp::fillList()
{
    BufferString seltxt = inpfld_->text();
    inpfld_->setEmpty();
    inpfld_->addItems( propnms_ );
    inpfld_->addItem( "Constant" );
    inpfld_->setCurrentItem( seltxt );
}


void uiElasticPropSelGrp::uiSelInpGrp::selVarCB( CallBacker* )
{
    const int selidx = inpfld_->currentItem();
    isconstant_ = selidx == inpfld_->size() -1;
    ctefld_->display( isconstant_ );
}


const char* uiElasticPropSelGrp::uiSelInpGrp::textOfVariable() const
{
    return isactive_ ? isconstant_ ? toString( ctefld_->getfValue() ) 
				   : inpfld_->text() 
		     : 0; 
}


void uiElasticPropSelGrp::uiSelInpGrp::setVariable( const char* txt, float val )
{
    isactive_ = txt;
    isconstant_ = !mIsUdf( val );
    ctefld_->display( isconstant_ );
    if ( isconstant_ )
    {
	ctefld_->setValue( val );
	inpfld_->setCurrentItem( inpfld_->size()-1 ); 
    }
    else
	inpfld_->setCurrentItem( txt ); 
}


void uiElasticPropSelGrp::uiSelInpGrp::use( MathExpression* expr )
{
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    isactive_ =  idx_ < nrvars;
    display( isactive_ );

    if ( !isactive_ ) 
	return;

    const BufferString varnm = expr->uniqueVarName( idx_ );
    varnmfld_->setText( varnm );
    const int nearidx = propnms_.nearestMatch( varnm );
    if ( nearidx >= 0 )
	inpfld_->setCurrentItem( nearidx );
}



uiElasticPropSelGrp::uiElasticPropSelGrp( uiParent* p,
					const BufferStringSet& prs,
       					ElasticPropertyRef& elprop,
       					const TypeSet<ElasticFormula>& el )
    : uiGroup( p, "Prop Grp" )
    , propnms_(prs)  
    , elpropref_(elprop)			
    , elformsel_(elprop.formula())
    , availableformulas_(el)
    , expr_(0) 
{
    BufferStringSet predeftitles;
    for ( int idx=0; idx<availableformulas_.size(); idx++ )
	predeftitles.add( availableformulas_[idx].name() );

    const CallBack selformcb( mCB(this,uiElasticPropSelGrp,selFormulaChgCB) );
    const CallBack selcompcb( mCB(this,uiElasticPropSelGrp,selComputeFldChgCB));
    selmathfld_ = new uiLabeledComboBox( this, "Compute from" );
    selmathfld_->box()->addItem( "Defined quantity" );
    selmathfld_->box()->addItems( predeftitles );
    selmathfld_->box()->addItem( "Formula" );
    selmathfld_->box()->selectionChanged.notify(selcompcb); 

    formfld_ = new uiGenInput( this, "Formula " );
    formfld_->attach( alignedBelow, selmathfld_ );

    singleinpfld_ = new uiLabeledComboBox( this, "use" );
    singleinpfld_->attach( alignedBelow, selmathfld_ );
    singleinpfld_->box()->addItems( propnms_ );

    for ( int idx=0; idx<propnms_.size(); idx++ )
    {
	inpgrps_ += new uiElasticPropSelGrp::uiSelInpGrp( this, propnms_, idx );
	if ( idx )
	    inpgrps_[idx]->attach( alignedBelow, inpgrps_[idx-1] );
	else
	    inpgrps_[idx]->attach( alignedBelow, formfld_ );
    }

    formfld_->valuechanged.notify( selformcb );

    storenamesep_ = new uiSeparator( this, "sep" );
    storenamesep_->attach( stretchedBelow, inpgrps_[inpgrps_.size()-1]  );

    storenamefld_ = new uiGenInput( this, "Quantity name:" );
    storenamefld_->attach( alignedBelow, inpgrps_[inpgrps_.size()-1] );
    storenamefld_->attach( ensureBelow, storenamesep_ );

    updateRefPropNames();
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

    selFormulaChgCB(0);
}


void uiElasticPropSelGrp::selFormulaChgCB( CallBacker* )
{
    const int selidx = selmathfld_->box()->currentItem();
    elformsel_.setExpression( "" );

    if ( selidx == selmathfld_->box()->size() -1 )
    {
	elformsel_.setExpression( formfld_->text() );
    }
    else
    {
	const ElasticFormula* ef = availableformulas_.validIdx(selidx-1) ? 
					    &availableformulas_[selidx-1] : 0;
	formfld_->setText( ef ? ef->expression() : "" );
	if ( ef ) 
	{
	    elformsel_.setExpression( ef->expression() );
	    elformsel_.variables() = ef->variables();
	}
    }
    BufferString formulanm( selmathfld_->box()->text() );
    elformsel_.setName( formulanm.buf() );

    putToScreen();
}


void uiElasticPropSelGrp::getMathExpr()
{
    delete expr_; expr_ = 0;
    const BufferString inp( formfld_->text() );
    if ( inp.isEmpty() ) return;

    MathExpressionParser mep( inp );
    expr_ = mep.parse();

    if ( !expr_ )
	uiMSG().warning( 
	BufferString("The provided expression cannot be used:\n",mep.errMsg()));
}


bool uiElasticPropSelGrp::isDefinedQuantity() const
{
    return selmathfld_->box()->currentItem() == 0;
}


const char* uiElasticPropSelGrp::quantityName() const
{
    return isDefinedQuantity() ? 0 : storenamefld_->text();
}


void uiElasticPropSelGrp::getFromScreen()
{
    elformsel_.variables().erase();
    elformsel_.setExpression( "" );

    if ( isDefinedQuantity() )
	elformsel_.variables().add( singleinpfld_->box()->text() );
    else
    {
	elformsel_.setExpression( formfld_->text() );
	for ( int idx=0; idx<inpgrps_.size(); idx++ )
	{
	    const char* txt = inpgrps_[idx]->textOfVariable();
	    if ( txt ) 
		elformsel_.variables().add( txt ); 
	}
    }
    elpropref_.setName( storenamefld_->text() );
}


void uiElasticPropSelGrp::putToScreen()
{
    const BufferString& expr = elformsel_.expression();
    const bool hasexpr = !expr.isEmpty() 
	|| selmathfld_->box()->currentItem() == selmathfld_->box()->size()-1;

    if ( elformsel_.name().isEmpty() )
	selmathfld_->box()->setCurrentItem( 0 );
    else    
	selmathfld_->box()->setCurrentItem( elformsel_.name() );
    formfld_->setText( expr );
    storenamefld_->setText( elpropref_.name() ); 

    getMathExpr();

    float val = mUdf(float);
    formfld_->setText( expr );
    for ( int idx=0; idx<inpgrps_.size(); idx++ )
    {
	inpgrps_[idx]->use( expr_ );
	if ( !inpgrps_[idx]->isActive() ) 
	    continue;

	const char* vartxt = elformsel_.parseVariable( idx, val );
	inpgrps_[idx]->setVariable( vartxt, val ); 
    }
    if ( !hasexpr )
    {
	const char* vartxt = elformsel_.parseVariable( 0, val );
	singleinpfld_->box()->setCurrentItem( vartxt );
    }

    formfld_->display( hasexpr );
    singleinpfld_->display( !hasexpr );
    storenamefld_->display( hasexpr );
    storenamesep_->display( hasexpr );
}



static const char** props = ElasticFormula::TypeNames();

#define mErrRet(s,act) { uiMSG().error(s); act; }
uiElasticPropSelDlg::uiElasticPropSelDlg( uiParent* p, 
					const PropertyRefSelection& prs,
					ElasticPropSelection& elsel )
    : uiDialog(p,uiDialog::Setup("Synthetic layers property selection",
		"Select quantities to compute density, p-wave and s-wave"
		,"110.1.3"))
    , ctio_(*mMkCtxtIOObj(ElasticPropSelection)) 
    , elpropsel_(elsel)
    , orgelpropsel_(elsel)
    , propsaved_(false)			  
{
    orgpropnms_.erase();
    for ( int idx=0; idx<prs.size(); idx++ )
    {
	const PropertyRef* ref = prs[idx];
	orgpropnms_.addIfNew( ref->name() );
    }
    if ( orgpropnms_.isEmpty() )
	mErrRet( "No property found", return );
    propnms_ = orgpropnms_;

    ts_ = new uiTabStack( this, "Property selection tab stack" );
    ObjectSet<uiGroup> tgs;

    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp; 
	ElasticFormula::parseEnumType( props[idx], tp );
	tgs += new uiGroup( ts_->tabGroup(), props[idx] );
	TypeSet<ElasticFormula> formulas;
	ElFR().getByType( tp, formulas );
	ElasticPropertyRef& epr = elpropsel_.get(tp);
	propflds_ += new uiElasticPropSelGrp(tgs[idx], propnms_, epr, formulas);
	ts_->addTab( tgs[idx], props[idx] );
    }
    ts_->selChange().notify( 
	    		mCB(this,uiElasticPropSelDlg,screenSelectionChanged) );

    uiGroup* gengrp = new uiGroup( this, "buttons" );
    gengrp->attach( ensureBelow, ts_ );
    uiToolButton* opentb = new uiToolButton( gengrp, "open",
				"Open stored property selection",
				mCB(this,uiElasticPropSelDlg,openPropSelCB) );
    uiToolButton* stb = new uiToolButton( gengrp, "save",
				"Save property selection",
				mCB(this,uiElasticPropSelDlg,savePropSelCB) );
    stb->attach( rightOf, opentb );

    elasticPropSelectionChanged(0);
}


uiElasticPropSelDlg::~uiElasticPropSelDlg()
{
   delete ctio_.ioobj; delete &ctio_; 
}


bool uiElasticPropSelDlg::screenSelectionChanged( CallBacker* )
{ 
    NotifyStopper ns( ts_->selChange() );
    propnms_ = orgpropnms_;
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	propflds_[idx]->getFromScreen();

	if ( !propflds_[idx]->quantityName() )
	    continue;

	BufferString quantitynm( propflds_[idx]->quantityName() );
	if ( quantitynm.isEmpty() )
	    mErrRet( "Please select a name for the new quantity", 
		    	ts_->setCurrentPage(idx); return false; )
	else if( propnms_.isPresent( quantitynm.buf() ) )
	{
	    BufferString msg( quantitynm.buf() );
	    msg += " already exists";
	    msg += ", please select another name for this property";
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
	propflds_[idx]->setPropRef( elpropsel_.get( idx ) );

    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->putToScreen();
}


bool uiElasticPropSelDlg::rejectOK( CallBacker* )
{
    elpropsel_ = orgelpropsel_;
    propsaved_ = false;
    return true;
}


bool uiElasticPropSelDlg::acceptOK( CallBacker* )
{
    if ( !screenSelectionChanged(0) )
	return false;

    if( ctio_.ioobj )
	doStore( *ctio_.ioobj );

    BufferString msg;
    if ( !elpropsel_.isValidInput( &msg ) )
	mErrRet( msg, return false; );

    return true;
}


bool uiElasticPropSelDlg::savePropSel() 
{
    ctio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    ctio_.setObj( dlg.ioObj()->clone() );
    return doStore( *ctio_.ioobj );
}


bool uiElasticPropSelDlg::doStore( const IOObj& ioobj )
{
    const BufferString fnm( ioobj.fullUserExpr(false) );
    StreamData sd( StreamProvider(fnm).makeOStream() );
    bool rv = false;
    if ( !sd.usable() )
	uiMSG().error( "Cannot open output file" );
    else if ( !elpropsel_.put(&ioobj) )
	uiMSG().error( "Cann not write file" );
    else
	rv = true;

    sd.close();
    storedmid_ = ioobj.key(); 
    propsaved_ = true;
    return rv;
}


bool uiElasticPropSelDlg::openPropSel()
{
    ctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    ctio_.setObj( dlg.ioObj()->clone() );
    const BufferString fnm( ctio_.ioobj->fullUserExpr(true) );
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file", return false; )
    sd.close();

    if ( !doRead( ctio_.ioobj->key() ) )
	mErrRet( "Unable to read elastic property selection", return false; );
    return true;
}


bool uiElasticPropSelDlg::doRead( const MultiID& mid )
{
    ElasticPropSelection* elp = elpropsel_.get( mid );
    ctio_.setObj( IOM().get( mid ) );

    if ( !elp ) return false;

    elpropsel_ = *elp; delete elp;
    propnms_ = orgpropnms_;
    for ( int idx=0; idx<elpropsel_.size(); idx++ )
    {
	const ElasticPropertyRef& epr = elpropsel_.get(idx);
	propnms_.addIfNew( epr.name() );
    }

    storedmid_ = mid;
    elasticPropSelectionChanged(0);

    return true;
}
