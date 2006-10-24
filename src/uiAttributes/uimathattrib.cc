/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.cc,v 1.9 2006-10-24 15:21:36 cvshelene Exp $
________________________________________________________________________

-*/

#include "uimathattrib.h"
#include "mathattrib.h"
#include "mathexpression.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"

using namespace Attrib;

const int cNrVars = 6;
const int cNrXVars = 6;
const int cNrConstVars = 6;

mInitAttribUI(uiMathAttrib,Math,"Mathematics",sKeyBasicGrp)

uiMathAttrib::uiMathAttrib( uiParent* p )
	: uiAttrDescEd(p)
	, nrvariables_(0)
{
    inpfld_ = new uiGenInput( this, "Formula (e.g. x0 + c0 * x1)",
	    		     StringInpSpec() );

    parsebut_ = new uiPushButton( this, "Set", true );
    parsebut_->activated.notify( mCB(this,uiMathAttrib,parsePush) );
    parsebut_->attach( rightTo, inpfld_ );

    for ( int idx=0; idx<cNrXVars; idx++ )
    {
        BufferString str( "Selection for x" );
	str += idx;
	uiAttrSel* attrbox = new uiAttrSel( this, 0, str );
	attribflds_ += attrbox;
	attrbox->display( false );
	attrbox->attach( alignedBelow, idx ? (uiObject*)attribflds_[idx-1] 
					   : (uiObject*)inpfld_ );
    }

    for ( int idx=0; idx<cNrConstVars; idx++ )
    {
	BufferString str( "Selection for c" );
	str += idx;
	uiGenInput* constbox = new uiGenInput( this, str, FloatInpSpec() );
	cstsflds_ += constbox;
	constbox->display( false );
	if ( idx )
	    constbox->attach( alignedBelow, (uiObject*)cstsflds_[idx-1] );
    }

    setHAlignObj( inpfld_ );
}


void uiMathAttrib::parsePush( CallBacker* )
{
    MathExpression* expr = MathExpression::parse( inpfld_->text() );
    nrvariables_ = expr ? expr->getNrVariables() : 0;
    if ( !expr && strcmp( inpfld_->text(), "" ) )
    {
	uiMSG().error( "Could not parse this equation" );
	return;
    }

    if ( nrvariables_ > cNrVars )
    {
	uiMSG().error( "Max. nr of variables you can use is 6" );
	nrvariables_ = 0;
	return;
    }

    bool found = false;
    int nrxvars = 0;
    int nrcstvars = 0;
    for ( int idx=0; idx<nrvariables_; idx++ )
    {
	BufferString xstr = "x"; xstr += idx;
	for ( int idy=0; idy<nrvariables_; idy++ )
	{
	    if ( !strcmp( expr->getVariableStr(idy), xstr.buf() ) )
	    {
		nrxvars++;
		found = true;
	    }
	} 
    }
    for ( int idx=0; idx<nrvariables_-nrxvars; idx++ )
    {
	BufferString varstr = "c"; varstr += idx;
	for ( int idy=0; idy<nrvariables_; idy++ )
	{
	    if ( !strcmp( expr->getVariableStr(idy), varstr.buf() ) )
	    {
		nrcstvars++;
		found = true;
	    }
	}
    }

    if ( !found && nrvariables_ )
    {
	uiMSG().error( "Formula should have x0, x1, x2 ... or c0, c1, c2 ..." );
	nrvariables_ = 0;
	return;
    }

    for ( int idx=0; idx<cNrXVars; idx++ )
	attribflds_[idx]->display( idx<nrxvars );
    
    cstsflds_[0]->attach( alignedBelow, 
	    nrxvars ? (uiObject*)attribflds_[nrxvars-1] : (uiObject*)inpfld_ );

    for ( int idx=0; idx<cNrConstVars; idx++ )
	cstsflds_[idx]->display( idx<nrcstvars );
}


bool uiMathAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Math::attribName()) )
	return false;

    mIfGetString( Math::expressionStr(), expression, 
	    	  inpfld_->setText(expression) );
    parsePush(0);

    if ( desc.getParam(Math::cstStr()) )
    {
	mDescGetConstParamGroup(FloatParam,cstset,desc,Math::cstStr());
	for ( int idx=0; idx<cstset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*cstset)[idx];
	    cstsflds_[idx]->setValue(param.getfValue(0));
	}
    }
    
    return true;
}


bool uiMathAttrib::setInput( const Desc& desc )
{
    for ( int idx=0; idx<cNrVars; idx++ )
	putInp( attribflds_[idx], desc, idx );

    return true;
}


bool uiMathAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Math::attribName()) )
	return false;

    mSetString( Math::expressionStr(), inpfld_->text() );
    
    MathExpression* expr = MathExpression::parse( inpfld_->text() );
    TypeSet<int> inptable;
    Math::getInputTable( expr, inptable, true );
    int nrcsts = inptable.size();
    mDescGetParamGroup(FloatParam,cstset,desc,Math::cstStr())
    cstset->setSize( nrcsts );
    for ( int idx=0; idx<nrcsts; idx++ )
    {
	FloatParam& fparam = (FloatParam&)(*cstset)[idx];
	fparam.setValue( cstsflds_[idx]->getfValue(0) );
    }
    
    return true;
}


bool uiMathAttrib::getInput( Desc& desc )
{
    for ( int idx=0; idx<nrvariables_; idx++ )
    {
	attribflds_[idx]->processInput();
	fillInp( attribflds_[idx], desc, idx );
    }

    return true;
}


void uiMathAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    mDescGetConstParamGroup(FloatParam,cstset,(*curDesc()),Math::cstStr());
    BufferString constantbase = "constant c";
    for ( int idx=0; idx<cstset->size(); idx++ )
    {
	BufferString constantstr = constantbase;
	constantstr +=idx;
	params += EvalParam( constantstr, Math::cstStr(), 0, idx );
    }
}
