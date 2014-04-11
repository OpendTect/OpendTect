/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert / Helene
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uirockphysform.h"
#include "rockphysics.h"
#include "mathformula.h"
#include "mathproperty.h"
#include "unitofmeasure.h"
#include "pixmap.h"

#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitextedit.h"

#define mMaxNrCsts	5




class uiRockPhysCstFld : public uiGroup
{
public:

			uiRockPhysCstFld(uiParent*);

    float		getCstVal() const;
    void		updField(BufferString,Interval<float>,BufferString,
				 float val = mUdf(float));

    BufferString	cstnm_;
    BufferString	desc_;

protected:

    void		descPush(CallBacker*);

    uiGenInput*		valfld_;
    uiLabel*		nmlbl_;
    uiLabel*		rangelbl_;
    uiToolButton*	descbutton_;

};



uiRockPhysForm::uiRockPhysForm( uiParent* p )
    : uiGroup(p,"RockPhyics Formula Selector")
    , fixedtype_(PropertyRef::Den)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
				PropertyRef::StdTypeNames(), "Property Type" );
    typfld_ = lcb->box();
    typfld_->setHSzPol( uiObject::MedMax );
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
    if ( !txt || !*txt )
	return PropertyRef::Other;

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
	BufferString msg( "The category '", PropertyRef::toString(typ),
		"' does not contain any formula yet.\n"
		"Please add a formula to the repository "
		"or select another category." );
	uiMSG().warning( msg );

	descriptionfld_->setText("");
	formulafld_->setText("");
	for ( int idx=0; idx<cstflds_.size(); idx++ )
	    cstflds_[idx]->display( false );

	return;
    }

    nameSel( 0 );
}


void uiRockPhysForm::typSel( CallBacker* cb )
{
    if ( !typfld_ ) return;

    const char* txt = typfld_->text();
    if ( !txt || !*txt ) return;

    setType( PropertyRef::parseEnumStdType(txt) );
}


BufferString uiRockPhysForm::getFormText( const RockPhysics::Formula& rpfm,
					  bool fortxtdisp ) const
{
    BufferString formstr( rpfm.def_ );
    Math::Formula form( formstr );
    int ivardef = 0; int iconstdef = 0;
    for ( int iinp=0; iinp<form.nrInputs(); iinp++ )
    {
	BufferString formval;
	if ( form.isConst(iinp) )
	{
	    if ( fortxtdisp )
		formval = rpfm.constdefs_[iconstdef]->name();
	    else
		formval.set( cstflds_[iconstdef]->getCstVal() );
	    iconstdef++;
	}
	else
	{
	    formval = rpfm.vardefs_[ivardef]->name();
	    formval.clean();
	    ivardef++;
	}
	formstr.replace( form.variableName(iinp), formval );
    }

    return formstr;
}


void uiRockPhysForm::nameSel( CallBacker* cb )
{
    const char* txt = nmfld_->text();
    if ( !txt || !*txt ) return;

    const RockPhysics::Formula* fm = ROCKPHYSFORMS().get( txt );
    if ( !fm )
	{ uiMSG().error( "Internal: formula not found" ); return;}
    const RockPhysics::Formula& rpfm = *fm;

    const int nrconsts = rpfm.constdefs_.size();
    for ( int idx=0; idx<cstflds_.size(); idx++ )
    {
	bool dodisp = idx < nrconsts;
	cstflds_[idx]->display( dodisp );
	if ( dodisp )
	    cstflds_[idx]->updField( rpfm.constdefs_[idx]->name(),
				     rpfm.constdefs_[idx]->typicalrg_,
				     rpfm.constdefs_[idx]->desc_,
				     rpfm.constdefs_[idx]->defaultval_ );
    }

    formulafld_->setText( getFormText(rpfm,true) );
    descriptionfld_->setText( rpfm.desc_ );
}


const char* uiRockPhysForm::getText( bool replcst ) const
{
    if ( !replcst )
	return nmfld_->text();

    Math::Formula form; getFormulaInfo( form );
    mDeclStaticString(ret);
    ret = form.text();
    return ret;
}


bool uiRockPhysForm::getFormulaInfo( Math::Formula& form,
			     TypeSet<PropertyRef::StdType>* sttypes ) const
{
    if ( sttypes )
	sttypes->setEmpty();

    const char* txt = nmfld_->text();
    if ( !txt || !*txt )
	{ uiMSG().error( "No formula name selected" ); return false; }

    const RockPhysics::Formula* fm = ROCKPHYSFORMS().get( txt );
    if ( !fm )
	{ uiMSG().error( "Internal: formula not found" ); return false; }
    const RockPhysics::Formula& rpfm = *fm;

    form.setText( getFormText(rpfm,false) );

    int nrinps = form.nrInputs();
    if ( nrinps != fm->vardefs_.size() )
    {
	BufferString msg; msg.set(nrinps).add("!=").add(fm->vardefs_.size());
	pErrMsg( msg );
	if ( nrinps > fm->vardefs_.size() )
	    nrinps = fm->vardefs_.size();
    }

    for ( int idx=0; idx<nrinps; idx++ )
    {
	const RockPhysics::Formula::VarDef& vd = *fm->vardefs_[idx];
	form.setInputDef( idx, vd.name() );
	form.setInputUnit( idx, UoMR().get(vd.unit_) );
	if ( sttypes )
	    sttypes += vd.type_;
    }

    form.setOutputUnit( UoMR().get(fm->unit_) );
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
    nmlbl_ = new uiLabel( this, "" );
    nmlbl_->setPrefWidthInChar( 35 );
    nmlbl_->setAlignment( Alignment::Right );

    valfld_ = new uiGenInput( this, 0, FloatInpSpec() );
    valfld_->attach( rightOf, nmlbl_ );

    rangelbl_ = new uiLabel( this, "" );
    rangelbl_->setPrefWidthInChar( 35 );
    rangelbl_->attach( rightOf, valfld_ );

    CallBack cb = mCB(this,uiRockPhysCstFld,descPush);
    descbutton_ = new uiToolButton( this, "info", "Info on this constant", cb );
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

    BufferString suffix( "Typical range: [", range.start, "," );
    suffix.add( range.stop ).add( "]" );
    rangelbl_->setText( suffix );

    rangelbl_->display( !range.isUdf() );
    descbutton_->display( !desc_.isEmpty() );
}


void uiRockPhysCstFld::descPush( CallBacker* )
{
    uiMSG().message( desc_.buf() );
}
