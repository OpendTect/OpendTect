/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.cc,v 1.17 2008-01-22 16:24:39 cvshelene Exp $
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

static const int cNrVars = 6;
static const int cNrXVars = 6;
static const int cNrConstVars = 6;

mInitAttribUI(uiMathAttrib,Math,"Mathematics",sKeyBasicGrp)

uiMathAttrib::uiMathAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.9")
	, nrvariables_(0)
	, nrxvars_(0)
	, nrcstvars_(0)

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
	uiAttrSel* attrbox = new uiAttrSel( this, 0, is2d, str );
	attribflds_ += attrbox;
	attrbox->display( false );
	attrbox->attach( alignedBelow, idx ? (uiObject*)attribflds_[idx-1] 
					   : (uiObject*)inpfld_ );
    }

    for ( int idx=0; idx<cNrConstVars; idx++ )
    {
	uiGenInput* constbox = new uiGenInput( this, "Selection for c0",
					       FloatInpSpec() );
	cstsflds_ += constbox;
	constbox->display( false );
	constbox->attach( alignedBelow, idx ? (uiObject*)cstsflds_[idx-1] 
					    : (uiObject*)inpfld_ );
    }

    BufferString str = "Provide a starting value\n";
    str += "for recursive function";
    recstartfld_ = new uiGenInput( this, str, FloatInpSpec() );
    recstartfld_->setPrefHeightInChar(2);
    recstartfld_->attach( alignedBelow,
	    cstsflds_.isEmpty() ? (uiObject*)attribflds_[attribflds_.size()-1]
				: (uiObject*)cstsflds_[cstsflds_.size()-1] );

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
    if ( nrvariables_ > cNrVars )
    {
	uiMSG().error( "Max. nr of variables you can use is 6" );
	nrvariables_ = 0;
	return;
    }

    bool found = false;
    nrxvars_= nrcstvars_ = 0;
    for ( int idx=0; idx<nrvariables_; idx++ )
    {
	BufferString xstr = "x"; xstr += idx;
	for ( int idy=0; idy<nrvariables_; idy++ )
	{
	    if ( !strcmp( expr->getVarPrefixStr(idy), xstr.buf() ) )
	    {
		nrxvars_++;
		found = true;
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
		found = true;
	    }
	}
    }

    if ( ( !found && nrvariables_ ) || ( nrxvars_+nrcstvars_!=nrvariables_) )
    {
	BufferString errmsg = "Formula should have x0, x1, x2 ...";
	errmsg += "or c0, c1, c2 ...\n";
	errmsg += "Please take care of the numbering:\n";
       	errmsg += "first x0, then x1...\n";
	errmsg += "For recursive formula please use 'THIS[-1]'.";
	uiMSG().error( errmsg.buf() );
	nrvariables_ = 0;
	return;
    }

    for ( int idx=0; idx<cNrXVars; idx++ )
	attribflds_[idx]->display( idx<nrxvars_ );
    
    for ( int idx=0; idx<cNrConstVars; idx++ )
    {
	bool dodisplay = idx>nrxvars_-1 && idx<nrvariables_;
	if ( dodisplay )
	{
	    BufferString str( "Selection for c" );
	    str += idx-nrxvars_;
	    cstsflds_[idx]->setTitleText(str);
	}
	
	cstsflds_[idx]->display( dodisplay );
    }
    
    recstartfld_->display( expr && expr->isRecursive() );
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
	    if ( cstsflds_.size() <= idx+nrxvars_ ) return false;
	    
	    const ValParam& param = (ValParam&)(*cstset)[idx];
	    cstsflds_[idx+nrxvars_]->setValue(param.getfValue(0));
	}
    }
    
    if ( desc.getValParam( Math::recstartStr() ) )
    {
	float recstart = desc.getValParam( Math::recstartStr() )->getfValue(0);
	if ( !mIsUdf( recstart ) )
	    recstartfld_->setValue( recstart );
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
    if ( !expr )
	mErrRet( "Could not parse this equation", false )

    TypeSet<int> cstinptable, xinptable;
    Math::getInputTable( expr, cstinptable, true );
    Math::getInputTable( expr, xinptable, false );
    int nrcsts = cstinptable.size();
    int nrxvars = expr->getNrDiffVariables();
    mDescGetParamGroup(FloatParam,cstset,desc,Math::cstStr())
    cstset->setSize( nrcsts );
    if ( cstsflds_.size() < nrxvars ) return false;
    
    for ( int idx=0; idx<nrcsts; idx++ )
    {
	FloatParam& fparam = (FloatParam&)(*cstset)[idx];
	fparam.setValue( cstsflds_[idx+nrxvars-nrcsts]->getfValue(0) );
    }
    
    mSetFloat( Math::recstartStr(), recstartfld_->getfValue() );
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
