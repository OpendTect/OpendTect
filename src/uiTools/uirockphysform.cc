/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert / Helene
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uirockphysform.cc,v 1.10 2012-02-29 16:09:44 cvshelene Exp $";

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
    if ( attobj )
	lcb->attach( alignedBelow, attobj );
    nmfld_ = lcb->box();
    nmfld_->selectionChanged.notify( mCB(this,uiRockPhysForm,nameSel) );

    descriptionfld_ = new uiTextEdit( this, "Formula Desc", true );
    descriptionfld_->setPrefHeightInChar( 3 );
    descriptionfld_->setPrefWidthInChar( 100 );
    descriptionfld_->attach( alignedBelow, lcb->attachObj() );

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
		   		     fm->constdefs_[idx]->desc_ );
    }

    descriptionfld_->setText( fm->desc_ );
}


//TODO: remove
const char* uiRockPhysForm::getText() const
{
    BufferString formula;
    BufferString outunit;
    BufferStringSet varsunits;
    if ( getFormulaInfo( formula, outunit, varsunits ) )
	return formula.buf();

    return 0;
}


bool uiRockPhysForm::getFormulaInfo( BufferString& cleanformula,
				     BufferString& outputunit,
				     BufferStringSet& varsunits ) const
{

    char* txt = const_cast<char*>(nmfld_->text());
    if ( !txt || !*txt )
    {
	uiMSG().error( "Internal [impossible?]: no formula name selected" );
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
    }
    for ( int idx=0; idx<mp->nrConsts(); idx++ )
	replaceString( ret.buf(), mp->constName(idx),
		       toString(cstflds_[idx]->getCstVal()) );

    cleanformula = ret;
    //TODO get units
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

    valfld_ = new uiGenInput( this, 0, FloatInpSpec() );
    valfld_->attach( rightOf, nmlbl_ );

    rangelbl_ = new uiLabel( this, 0 );
    rangelbl_->setPrefWidthInChar( 35 );
    rangelbl_->attach( rightOf, valfld_ );

    CallBack cb = mCB(this,uiRockPhysCstFld,descPush);
    descbutton_ = new uiPushButton( this, "", ioPixmap("contexthelp.png"),
	    			    cb, true );
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

    BufferString suffix = "Typical range is ["; suffix += range.start;
    suffix += ","; suffix += range.stop; suffix += "]";
    rangelbl_->setText( suffix.buf() );
}


void uiRockPhysCstFld::descPush( CallBacker* )
{
    uiMSG().message( desc_.buf() );
}
