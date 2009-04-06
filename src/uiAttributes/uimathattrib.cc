/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimathattrib.cc,v 1.28 2009-04-06 09:32:24 cvsranojay Exp $";


#include "uimathattrib.h"
#include "mathattrib.h"
#include "mathexpression.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitable.h"

#include <math.h>

using namespace Attrib;

mInitAttribUI(uiMathAttrib,Attrib::Math,"Mathematics",sKeyBasicGrp())

uiMathAttrib::uiMathAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.9")
	, nrvariables_(0)
	, nrxvars_(0)
	, nrcstvars_(0)

{
    inpfld_ = new uiGenInput( this, "Formula (e.g. x0 + c0 * x1)",
	    		     StringInpSpec().setName("Formula") );

    parsebut_ = new uiPushButton( this, "Set", true );
    parsebut_->activated.notify( mCB(this,uiMathAttrib,parsePush) );
    parsebut_->attach( rightTo, inpfld_ );

    xtable_ = new uiTable( this,uiTable::Setup().rowdesc("X")
					.minrowhgt(1.5)
					.maxrowhgt(2)
					.mincolwdt(3*uiObject::baseFldSize())
					.maxcolwdt(3.5*uiObject::baseFldSize())
					.defrowlbl("")
					.fillcol(true)
					.fillrow(true)
					.defrowstartidx(0),
				"Variable X attribute table" );
    const char* xcollbls[] = { "Select input for", 0 };
    xtable_->setColumnLabels( xcollbls );
    xtable_->setNrRows( 3 );
    xtable_->setStretch( 2, 0 );
    xtable_->setRowResizeMode( uiTable::Fixed );
    xtable_->setColumnResizeMode( uiTable::Fixed );
    xtable_->attach( alignedBelow, inpfld_ );

    ctable_ = new uiTable( this,uiTable::Setup().rowdesc("C")
					.minrowhgt(1)
					.maxrowhgt(1.2)
					.mincolwdt(uiObject::baseFldSize())
					.maxcolwdt(1.5*uiObject::baseFldSize())
					.defrowlbl("")
					.fillcol(true)
					.fillrow(true)
					.defrowstartidx(0),
				"Constants C table" );
    const char* ccollbls[] = { "Choose value for", 0 };
    ctable_->setColumnLabels( ccollbls );
    ctable_->setNrRows( 3 );
    ctable_->setStretch( 1, 0 );
    ctable_->setColumnResizeMode( uiTable::Fixed );
    ctable_->setRowResizeMode( uiTable::Fixed );
    ctable_->attach( alignedBelow, xtable_ );
    
    BufferString str = "Provide a starting value\n";
    str += "for recursive function";
    recstartfld_ = new uiGenInput( this, str, FloatInpSpec() );
    recstartfld_->setPrefHeightInChar(2);
    recstartfld_->attach( alignedBelow, ctable_ );
    
    BufferString lbl = zDepLabel( "Provide a starting",
	    			  "\nfor recursive function" );
    recstartposfld_ = new uiGenInput( this, lbl, FloatInpSpec() );
    recstartposfld_->setPrefHeightInChar(2);
    recstartposfld_->attach( alignedBelow, recstartfld_ );
    setHAlignObj( inpfld_ );
}


#define mErrRet(msg,retval) \
{ uiMSG().error( "Could not parse this equation" ); return retval; }


void uiMathAttrib::parsePush( CallBacker* )
{
    MathExpression* expr = MathExpression::parse( inpfld_->text() );
    if ( !expr && strcmp( inpfld_->text(), "" ) )
	mErrRet( "Could not parse this equation", )

    nrvariables_ = expr ? expr->getNrDiffVariables() : 0;
    bool foundvar = false;
    bool correctshifts = true;
    checkVarSpelAndShift( expr, foundvar, correctshifts );

    const bool varspellingok = ( foundvar || !nrvariables_ )
				&& ( nrxvars_+nrcstvars_==nrvariables_ );
    if ( !varspellingok || !correctshifts )
    {
	BufferString errmsg = "Formula should have x0, x1, x2 ...";
	errmsg += "or c0, c1, c2 ...\n";
	errmsg += "Please take care of the numbering: ";
       	errmsg += "first x0, then x1...\n";
	errmsg += "Please read documentation for detailed examples\n ";
	errmsg += "over recursive formulas, shift....";
	uiMSG().error( errmsg.buf() );
	nrvariables_ = 0;
	return;
    }

    updateDisplay( expr && expr->isRecursive() );
}


void uiMathAttrib::updateDisplay( bool userecfld )
{
    if ( attribflds_.size() != nrxvars_ )
    {
	attribflds_.erase();

	xtable_->setNrRows( nrxvars_ );
	const uiAttrSelData asd( is2d_, false );
	for ( int idx=0; idx<nrxvars_; idx++ )
	{
	    uiAttrSel* attrbox = new uiAttrSel( 0, 0, asd );
	    attrbox->setDescSet( ads_ );
	    attribflds_ += attrbox;
	    xtable_->setCellGroup( RowCol(idx,0), attrbox );
	}
    }
    xtable_->display( nrxvars_ );
    
    ctable_->setNrRows( nrcstvars_ );
    ctable_->display( nrcstvars_ );
    
    recstartfld_->display( userecfld );
    recstartposfld_->display( userecfld );
}


void uiMathAttrib::checkVarSpelAndShift( MathExpression* expr,
					 bool& foundvar, bool& correctshifts )
{
    nrxvars_= nrcstvars_ = 0;
    if ( !expr ) return;

    for ( int idx=0; idx<nrvariables_; idx++ )
    {
	BufferString xstr = "x"; xstr += idx;
	for ( int idy=0; idy<nrvariables_; idy++ )
	{
	    if ( !strcmp( expr->getVarPrefixStr(idy), xstr.buf() ) )
	    {
		nrxvars_++;
		foundvar = true;
	    }
	} 
    }
    for ( int idx=0; idx<nrvariables_-nrxvars_; idx++ )
    {
	BufferString varstr = "c"; varstr += idx;
	for ( int idy=0; idy<nrvariables_; idy++ )
	{
	    if ( !strcmp( expr->getVarPrefixStr(idy), varstr.buf() ) )
	    {
		nrcstvars_++;
		foundvar = true;
	    }
	}
    }

    for ( int idx=0; idx<expr->getNrVariables(); idx++ )
    {
	int shift;
	BufferString testprefix;
	expr->getPrefixAndShift( expr->getVariableStr(idx), testprefix, shift );
	if ( strncmp( testprefix, "c", 1 ) && mIsUdf(shift) )
	    correctshifts = false;
    }
}


bool uiMathAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Attrib::Math::attribName()) )
	return false;

    mIfGetString( Attrib::Math::expressionStr(), expression, 
	    	  inpfld_->setText(expression) );
    parsePush(0);

    if ( desc.getParam(Attrib::Math::cstStr()) )
    {
	mDescGetConstParamGroup(FloatParam,cstset,desc,Attrib::Math::cstStr());
	for ( int idx=0; idx<cstset->size(); idx++ )
	{
	    if ( ctable_->nrRows() < idx+1 )
		ctable_->insertRows( idx, 1 );
	    
	    const ValParam& param = (ValParam&)(*cstset)[idx];
	    ctable_->setValue( idx, param.getfValue(0) );
	}
    }
    
    if ( desc.getValParam( Attrib::Math::recstartStr() ) )
    {
	float recstart =
	    desc.getValParam( Attrib::Math::recstartStr() )->getfValue(0);
	if ( !mIsUdf( recstart ) )
	    recstartfld_->setValue( recstart );
    }
    
    if ( desc.getValParam( Attrib::Math::recstartposStr() ) )
    {
	float recstartpos =
	    desc.getValParam( Attrib::Math::recstartposStr() )->getfValue(0);
	if ( !mIsUdf( recstartpos ) )
	    recstartposfld_->setValue( recstartpos * SI().zFactor() );
    }
    
    return true;
}


bool uiMathAttrib::setInput( const Desc& desc )
{
    for ( int idx=0; idx<attribflds_.size(); idx++ )
	putInp( attribflds_[idx], desc, idx );

    return true;
}


bool uiMathAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Attrib::Math::attribName()) )
	return false;

    mSetString( Attrib::Math::expressionStr(), inpfld_->text() );
    
    MathExpression* expr = MathExpression::parse( inpfld_->text() );
    if ( !expr )
	mErrRet( "Could not parse this equation", false )

    TypeSet<int> cstinptable, xinptable;
    Attrib::Math::getInputTable( expr, cstinptable, true );
    Attrib::Math::getInputTable( expr, xinptable, false );
    int nrcsts = cstinptable.size();
    int nrxvars = expr->getNrDiffVariables();
    mDescGetParamGroup(FloatParam,cstset,desc,Attrib::Math::cstStr())
    cstset->setSize( nrcsts );
    if ( ctable_->nrRows() < nrcsts ) return false;
    
    for ( int idx=0; idx<nrcsts; idx++ )
    {
	FloatParam& fparam = (FloatParam&)(*cstset)[idx];
	fparam.setValue( ctable_->getfValue( RowCol(idx,0) ) );
    }
    
    mSetFloat( Attrib::Math::recstartStr(), recstartfld_->getfValue() );
    mSetFloat( Attrib::Math::recstartposStr(),
	       recstartposfld_->getfValue() / SI().zFactor() );
    return true;
}


bool uiMathAttrib::getInput( Desc& desc )
{
    for ( int idx=0; idx<nrxvars_; idx++ )
    {
	attribflds_[idx]->processInput();
	fillInp( attribflds_[idx], desc, idx );
    }

    return true;
}


void uiMathAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    mDescGetConstParamGroup(FloatParam,cstset,(*curDesc()),
	    		    Attrib::Math::cstStr());
    BufferString constantbase = "constant c";
    for ( int idx=0; idx<cstset->size(); idx++ )
    {
	BufferString constantstr = constantbase;
	constantstr +=idx;
	params += EvalParam( constantstr, Attrib::Math::cstStr(), 0, idx );
    }
}
