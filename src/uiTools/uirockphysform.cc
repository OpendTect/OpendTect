/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert / Helene
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uirockphysform.h"
#include "rockphysics.h"
#include "mathexpression.h"
#include "mathproperty.h"
#include "pixmap.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitextedit.h"

#define mMaxNrCsts	5

uiRockPhysForm::uiRockPhysForm( uiParent* p )
    : uiGroup(p,"RockPhyics Formula Selector")
    , fixedtype_(PropertyRef::Den)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
	    			PropertyRef::StdTypeNames(), "Property Type" );
    typfld_ = lcb->box();
    typfld_->selectionChanged.notify( mCB(this,uiRockPhysForm,typSel) );

    createFlds( lcb->attachObj() );
}


uiRockPhysForm::uiRockPhysForm( uiParent* p, PropertyRef::StdType typ )
    : uiGroup(p,"RockPhyics Formula Selector")
    , fixedtype_(typ)
    , typfld_(0)
{
    createFlds( 0 );
}


void uiRockPhysForm::createFlds( uiObject* attobj )
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Formula" );
    lcb->box()->setHSzPol( uiObject::WideMax );
    lcb->label()->setPrefWidthInChar( 35 );
    lcb->label()->setAlignment( Alignment::Right );

    nmfld_ = lcb->box();
    nmfld_->selectionChanged.notify( mCB(this,uiRockPhysForm,nameSel) );

    formulafld_ = new uiTextEdit( this, "Formula", true );
    formulafld_->setPrefHeightInChar( 2 );
    formulafld_->setPrefWidthInChar( 100 );
    formulafld_->setStretch(2,0);
    formulafld_->attach( ensureBelow, lcb->attachObj() );

    if ( attobj )
	attobj->attach( alignedAbove, lcb );

    descriptionfld_ = new uiTextEdit( this, "Formula Desc", true );
    descriptionfld_->setPrefHeightInChar( 3 );
    descriptionfld_->setPrefWidthInChar( 100 );
    descriptionfld_->setStretch(2,0);
    descriptionfld_->attach( alignedBelow, formulafld_ );

    for ( int idx=0; idx<mMaxNrCsts; idx++ )
     {
	uiRockPhysCstFld* rpcfld = new uiRockPhysCstFld( this );
	if ( idx )
	    rpcfld->attach( alignedBelow, cstflds_[idx-1] );
	else
	    rpcfld->attach( alignedBelow, descriptionfld_ );

	cstflds_ += rpcfld;
    }

    setHAlignObj( lcb );
    setType( fixedtype_ );
}


PropertyRef::StdType uiRockPhysForm::getType() const
{
    if ( !typfld_ )
	return fixedtype_;

    const char* txt = typfld_->text();
    if ( !txt || !*txt ) return PropertyRef::Other;

    return PropertyRef::parseEnumStdType(txt);
}


void uiRockPhysForm::setType( PropertyRef::StdType typ )
{
    if ( typfld_ )
	typfld_->setText( PropertyRef::toString(typ) );

    BufferStringSet nms;
    ROCKPHYSFORMS().getRelevant( typ, nms );

    NotifyStopper ns( nmfld_->selectionChanged );
    nmfld_->setEmpty();
    nmfld_->addItems( nms );
    if ( !nms.isEmpty() )
	nmfld_->setCurrentItem( 0 );
    else
    {
	BufferString msg = "The category '";
	msg += PropertyRef::toString(typ);
	msg += "' does not contain any formula yet.\n";
	msg += "Please add a formula to the repository ";
	msg += "or select another category.";
	uiMSG().warning( msg );

	descriptionfld_->setText("");
	formulafld_->setText("");
	for ( int idx=0; idx<cstflds_.size(); idx++ )
	    cstflds_[idx]->display( false );

	return;
    }

    nameSel( 0 );
}


const char* uiRockPhysForm::formulaName() const
{
    return nmfld_->text();
}


void uiRockPhysForm::setFormulaName( const char* fmnm )
{
    nmfld_->setText( fmnm );
}


void uiRockPhysForm::typSel( CallBacker* cb )
{
    if ( !typfld_ ) return;

    const char* txt = typfld_->text();
    if ( !txt || !*txt ) return;

    setType( PropertyRef::parseEnumStdType(txt) );
}


void uiRockPhysForm::nameSel( CallBacker* cb )
{
    const char* txt = nmfld_->text();
    if ( !txt || !*txt ) return;

    const RockPhysics::Formula* fm = ROCKPHYSFORMS().get( txt );
    if ( !fm )
	{ uiMSG().error( "Internal [impossible?]: formula not found" ); return;}

    MathProperty* mp = fm->getProperty();
    if ( !mp )
	{ uiMSG().error( "No property defined for this type" ); return; }

    int nrconsts = mp->nrConsts();
    if ( nrconsts != fm->constdefs_.size() )
	{ uiMSG().error( "Formula doesn't match repository [c]!" ); return; }
    if ( mp->nrInputs() != fm->vardefs_.size() )
	{ uiMSG().error( "Formula doesn't match repository [v]!" ); return; }

    for ( int idx=0; idx<cstflds_.size(); idx++ )
    {
	bool dodisp = idx<nrconsts;
	cstflds_[idx]->display( dodisp );
	if ( dodisp )
	    cstflds_[idx]->updField( fm->constdefs_[idx]->name(),
		    		     fm->constdefs_[idx]->typicalrg_,
		   		     fm->constdefs_[idx]->desc_,
		   		     fm->constdefs_[idx]->defaultval_ );
    }

    descriptionfld_->setText( fm->desc_ );

    formulafld_->setText( getText(false).buf() );
}


const char* uiRockPhysForm::getText() const
{
    return "Please use 'getText(bool)' function instead";
}


BufferString uiRockPhysForm::getText( bool usecstvals ) const
{
    BufferString formula;
    BufferString formulaunit;
    BufferString outunit;
    BufferStringSet varsunits;
    TypeSet<PropertyRef::StdType> varstypes;
    if ( getFormulaInfo( formula, formulaunit, outunit, varsunits, 
			 varstypes, usecstvals ) )
	return formula;

    return 0;
}


//Will be removed shortly, please do not use.
bool uiRockPhysForm::getFormulaInfo( BufferString& cleanformula,
				     BufferString& outputunit,
				     BufferStringSet& varsunits,
				     bool usecstvals ) const
{
    BufferString formulaunit;
    TypeSet<PropertyRef::StdType> varstypes;
    return getFormulaInfo( cleanformula, formulaunit, outputunit, varsunits,
	    		   varstypes, usecstvals );
}


bool uiRockPhysForm::getFormulaInfo( BufferString& cleanformula,
				     BufferString& formulaunit,
				     BufferString& outputunit,
				     BufferStringSet& varsunits,
					 TypeSet<PropertyRef::StdType>& varstypes,
				     bool usecstvals ) const
{
    varsunits.setEmpty();
    varstypes.erase();
    char* txt = const_cast<char*>(nmfld_->text());
    if ( !txt || !*txt )
    {
	uiMSG().error( "No formula name selected" );
	return false;
    }

    const RockPhysics::Formula* fm = ROCKPHYSFORMS().get( txt );
    if ( !fm )
    {
	uiMSG().error( "Internal [impossible?]: formula not found" );
	return false;
    }

    BufferString ret(fm->def_);
    MathProperty* mp = fm->getProperty();
    if ( !mp )
	{ uiMSG().error( "No property defined for this type" ); return false; }

    for ( int idx=0; idx<mp->nrInputs(); idx++ )
    {
	char* cleanvarnm = const_cast<char*>(fm->vardefs_[idx]->name().buf());
	cleanupString( cleanvarnm, false, false, false );
	replaceString( ret.buf(), mp->inputName( idx ), cleanvarnm );

	varsunits += new BufferString( fm->vardefs_[idx]->unit_ );
	varstypes += fm->vardefs_[idx]->type_;
    }
    for ( int idx=0; idx<mp->nrConsts(); idx++ )
    {
	if ( usecstvals )
	    replaceString( ret.buf(), mp->constName(idx),
			   toString(cstflds_[idx]->getCstVal()) );
	else
	{
	    char* cleancstnm = const_cast<char*>(
		    			fm->constdefs_[idx]->name().buf());
	    cleanupString( cleancstnm, false, false, false );
	    replaceString( ret.buf(), mp->constName( idx ), cleancstnm );
	}
    }

    cleanformula = ret;
    formulaunit = fm->getFormulaUnit();
    outputunit = fm->unit_;
    return true;
}


bool uiRockPhysForm::isOK()
{
    for ( int idx=0; idx<cstflds_.size(); idx++ )
    {
	if ( cstflds_[idx]->attachObj()->isDisplayed()
	     && mIsUdf( cstflds_[idx]->getCstVal() ) )
	{
	    errmsg_ += "Please provide a value for constant '";
	    errmsg_ += cstflds_[idx]->cstnm_; errmsg_ += "'\n";
	    return false;
	}
    }
    return true;
}


//-----------------------------------------------------------------------------

uiRockPhysCstFld::uiRockPhysCstFld( uiParent* p )
    : uiGroup(p,"Rock Physics Constant Field")
{
    nmlbl_ = new uiLabel( this, 0 );
    nmlbl_->setPrefWidthInChar( 35 );
    nmlbl_->setAlignment( Alignment::Right );

    valfld_ = new uiGenInput( this, 0, FloatInpSpec() );
    valfld_->attach( rightOf, nmlbl_ );

    rangelbl_ = new uiLabel( this, 0 );
    rangelbl_->setPrefWidthInChar( 35 );
    rangelbl_->attach( rightOf, valfld_ );

    CallBack cb = mCB(this,uiRockPhysCstFld,descPush);
    descbutton_ = new uiPushButton( this, "", ioPixmap("info.png"), cb, true );
    descbutton_->setPrefWidthInChar( 5 );
    descbutton_->attach( rightOf, rangelbl_ );
}


float uiRockPhysCstFld::getCstVal() const
{
    return valfld_->getfValue();
}


void uiRockPhysCstFld::updField( BufferString nm, Interval<float> range,
				 BufferString desc, float val )
{
    cstnm_ = nm;
    desc_ = desc;

    BufferString prefix = "Value for '"; prefix += nm; prefix += "'";
    nmlbl_->setText( prefix.buf() );

    if ( !mIsUdf(val) )
	valfld_->setValue( val );
    else
	valfld_->clear();

    BufferString suffix = "Typical range is ["; suffix += range.start;
    suffix += ","; suffix += range.stop; suffix += "]";
    rangelbl_->setText( suffix.buf() );

    rangelbl_->display( !range.isUdf() );
    descbutton_->display( !desc_.isEmpty() );
}


void uiRockPhysCstFld::descPush( CallBacker* )
{
    uiMSG().message( desc_.buf() );
}
