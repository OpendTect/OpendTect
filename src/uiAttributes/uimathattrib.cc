/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimathattrib.cc,v 1.30 2009-06-19 13:02:30 cvshelene Exp $";


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
	, nrvars_(0)
	, nrcsts_(0)
	, nrspecs_(0)

{
    inpfld_ = new uiGenInput( this, "Formula (e.g. nearstk + c0 * farstk)",
	    		     StringInpSpec().setName("Formula") );

    parsebut_ = new uiPushButton( this, "Set", true );
    parsebut_->activated.notify( mCB(this,uiMathAttrib,parsePush) );
    parsebut_->attach( rightTo, inpfld_ );

    xtable_ = new uiTable( this,uiTable::Setup().minrowhgt(1.5)
					.maxrowhgt(2)
					.mincolwdt(3*uiObject::baseFldSize())
					.maxcolwdt(3.5*uiObject::baseFldSize())
					.defrowlbl("")
					.fillcol(true)
					.fillrow(true)
					.defrowstartidx(0),
				"Variable attribute table" );
    const char* xcollbls[] = { "Select input for", 0 };
    xtable_->setColumnLabels( xcollbls );
    xtable_->setNrRows( 3 );
    xtable_->setStretch( 2, 0 );
    xtable_->setRowResizeMode( uiTable::Fixed );
    xtable_->setColumnResizeMode( uiTable::Fixed );
    xtable_->attach( alignedBelow, inpfld_ );

    ctable_ = new uiTable( this,uiTable::Setup().minrowhgt(1)
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
    const BufferString usrinp( inpfld_->text() );
    if ( usrinp.isEmpty() ) return;

    MathExpressionParser mep( usrinp );
    MathExpression* expr = mep.parse();
    if ( !expr )
    {
	BufferString errmsg = "Invalid formula:\n";
	errmsg += mep.errMsg();
	errmsg += "\nFormula should have variable names";
	errmsg += "or constants c0, c1, c2 ...\n";
	errmsg += "Please read documentation for detailed examples\n ";
	errmsg += "over recursive formulas, shift....";
	uiMSG().error( errmsg.buf() );
	return;
    }

    getVarsNrAndNms( expr );
    updateDisplay( expr && expr->isRecursive() );
}


void uiMathAttrib::getVarsNrAndNms( MathExpression* expr )
{
    //TODO check what extra for specs
    nrvars_ = 0;
    nrcsts_ = 0;
    nrspecs_ = 0;
    varnms.erase();
    cstnms.erase();
    for ( int idx=0; idx<expr->nrUniqueVarNames(); idx++ )
    {
	MathExpression::VarType vtyp =
		MathExpressionParser::varTypeOf( expr->uniqueVarName(idx) );
	switch ( vtyp )
	{
	    case MathExpression::Variable :
	    nrvars_++;
	    varnms.add( expr->uniqueVarName(idx) );
	    break;
	    case MathExpression::Constant :
	    nrcsts_++;
	    cstnms.add( expr->uniqueVarName(idx) );
	    break;
	}
    }
}


void uiMathAttrib::updateDisplay( bool userecfld )
{
    if ( attribflds_.size() != nrvars_ )
    {
	attribflds_.erase();

	xtable_->setNrRows( nrvars_ );
	const uiAttrSelData asd( is2d_, false );
	for ( int idx=0; idx<nrvars_; idx++ )
	{
	    uiAttrSel* attrbox = new uiAttrSel( 0, 0, asd );
	    attrbox->setDescSet( ads_ );
	    attribflds_ += attrbox;
	    xtable_->setCellGroup( RowCol(idx,0), attrbox );
	    xtable_->setRowLabel( idx, varnms.get(idx) );
	}
    }
    xtable_->display( nrvars_ );
    
    ctable_->setNrRows( nrcsts_ );
    ctable_->setRowLabels( cstnms );
    ctable_->display( nrcsts_ );
    
    recstartfld_->display( userecfld );
    recstartposfld_->display( userecfld );
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
	    ctable_->setValue( RowCol(idx,0), param.getfValue(0) );
	    ctable_->setRowLabel( idx, cstnms.get(idx) );
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

    MathExpressionParser mep( inpfld_->text() );
    MathExpression* expr = mep.parse();
    if ( !expr )
	mErrRet( BufferString("Incorrect formula:\n",mep.errMsg()), false )

    int nrconsts = 0;
    for ( int idx=0; idx<expr->nrUniqueVarNames(); idx++ )
    {
	MathExpression::VarType vtyp =
		    MathExpressionParser::varTypeOf( expr->uniqueVarName(idx) );
	if ( vtyp == MathExpression::Constant )
	    nrconsts++;
    }

    mDescGetParamGroup(FloatParam,cstset,desc,Attrib::Math::cstStr())
    cstset->setSize( nrconsts );
    if ( ctable_->nrRows() < nrconsts ) return false;
    
    for ( int idx=0; idx<nrconsts; idx++ )
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
    for ( int idx=0; idx<nrvars_; idx++ )
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
