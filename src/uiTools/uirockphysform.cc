/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert / Helene
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uirockphysform.cc,v 1.3 2012-02-16 15:25:29 cvshelene Exp $";

#include "uirockphysform.h"
#include "rockphysics.h"
#include "mathproperty.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"

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
    setHAlignObj( lcb );

    for ( int idx=0; idx< mMaxNrCsts; idx++ )
    {
	uiRockPhysCstFld* rpcfld = new uiRockPhysCstFld( this );
	if ( idx )
	    rpcfld->attach( alignedBelow, *(cstflds_[idx-1]) );
	else
	    rpcfld->attach( alignedBelow, nmfld_ );

	cstflds_ += rpcfld;
    }

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

    const int nrconsts = mp->nrConsts();
    if ( nrconsts != fm->constdefs_.size() )
	{ uiMSG().error( "Formula doesn't match repository [c]!" ); return; }
    BufferString msg( "TODO ...\nNr consts: ", nrconsts );
    for ( int idx=0; idx<nrconsts; idx++ )
	msg.add( "\n" ).add( fm->constdefs_[idx]->name() );
    const int nrvars = mp->nrInputs();
    if ( nrvars != fm->vardefs_.size() )
	{ uiMSG().error( "Formula doesn't match repository [v]!" ); return; }
    msg.add( "\nNr Vars:" ).add( nrvars );
    for ( int idx=0; idx<nrvars; idx++ )
	msg.add( "\n" ).add( fm->vardefs_.get(idx) );

    uiMSG().message( msg );
}


BufferString uiRockPhysForm::getText() const
{
    BufferString ret( "TODO * implement" );

    //TODO construct it, remember to replace ' ' with '_' in variable names

    return ret;
}


//-----------------------------------------------------------------------------

uiRockPhysCstFld::uiRockPhysCstFld( uiParent* p )
    : uiGroup(p,"Rock Physics Constant Field")
{
    nmlbl_ = new uiLabel( p, 0 );
    nmlbl_->setPrefWidthInChar( 40 );

    valfld_ = new uiGenInput( p, 0, FloatInpSpec() );
    valfld_-> attach( rightOf, nmlbl_ );

    rangelbl_ = new uiLabel( p, 0 );
    rangelbl_->setPrefWidthInChar( 40 );
    rangelbl_-> attach( rightOf, valfld_ );
}


float uiRockPhysCstFld::getCstVal() const
{
    return valfld_->getfValue();
}


void uiRockPhysCstFld::updField( BufferString nm,
				 Interval<float> range, float val )
{
    BufferString prefix = "Value for '"; prefix += nm; prefix += "'";
    nmlbl_->setText( prefix.buf() );

    if ( !mIsUdf(val) )
	valfld_->setValue( val );

    BufferString suffix = "Typical range is ["; suffix += range.start;
    suffix += ","; suffix += range.stop; suffix += "]";
    rangelbl_->setText( suffix.buf() );
}
