/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uimathattrib.h"
#include "mathattrib.h"
#include "mathformula.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uimathexpression.h"
#include "uimathexpressionvariable.h"
#include "uimathformula.h"
#include "uimsg.h"
#include "od_helpids.h"

#include <math.h>

using namespace Attrib;

mInitAttribUI(uiMathAttrib,Attrib::Mathematics,"Mathematics",sKeyBasicGrp())

uiMathAttrib::uiMathAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mMathAttribHelpID) )
	, form_(new Math::Formula(true,Attrib::Mathematics::getSpecVars()))
{
    uiAttrSelData asd( is2d );
    BufferStringSet inpnms_;
    asd.attrSet().fillInUIInputList( inpnms_ );
    uiMathFormula::Setup mfsu( "Formula (like 'nearstk + c0 * farstk')" );
    mfsu.withunits( false ).maxnrinps( 8 )
	.stortype( "Attribute calculation" );
    uimathform_ = new uiMathFormula( this, *form_, mfsu );
    uimathform_->formSet.notify( mCB(this,uiMathAttrib,formSel) );
    uimathform_->setNonSpecInputs( inpnms_ );
    setHAlignObj( uimathform_ );
}


#define mErrRet(msg,retval) \
{ uiMSG().error( "Could not parse this equation" ); return retval; }


void uiMathAttrib::formSel( CallBacker* cb )
{
    if ( !ads_ ) return;	//?

    BufferStringSet inpnms_;
    ads_->fillInUIInputList( inpnms_ );
    uimathform_->setNonSpecInputs( inpnms_ );
}


bool uiMathAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Attrib::Mathematics::attribName() )
	return false;

    mIfGetString( Attrib::Mathematics::expressionStr(), expression,
		  uimathform_->setText(expression) );

    formSel(0);

    if ( desc.getParam(Attrib::Mathematics::cstStr()) )
    {
	mDescGetConstParamGroup(Attrib::DoubleParam,cstset,desc,
				Attrib::Mathematics::cstStr());
	for ( int idx=0; idx<cstset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*cstset)[idx];
	    for ( int iinp=0; iinp<form_->nrInputs(); iinp++ )
	    {
		BufferString cststr ( "c", idx );
		if ( (BufferString) form_->variableName(iinp) == cststr )
		{
		    form_->setInputDef( iinp, toString(param.getdValue()) );
		    uimathform_->inpFld(iinp)->use( *form_ );
	}
    }
    }
    }

    if ( form_->isRecursive()
	    && desc.getValParam(Attrib::Mathematics::recstartvalsStr()) )
    {
	FileMultiString recvalsstr = desc.getValParam(
		Attrib::Mathematics::recstartvalsStr())->getStringValue(0);
	for ( int idx=0; idx<recvalsstr.size(); idx++ )
	{
	    const double val = recvalsstr.getDValue( idx );
	    form_->recStartVals()[idx] = mIsUdf(val) ? 0 : val;
    }
    }

    return true;
}


bool uiMathAttrib::setInput( const Desc& desc )
{
    int varinplastidx = 0;
    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	const Desc* inpdsc = desc.getInput( idx );
	if ( inpdsc )
	{
	    BufferString refstr = inpdsc->isStored()
				? BufferString ( "[",inpdsc->userRef(),"]")
				: BufferString( inpdsc->userRef() );
	    for ( int varinpidx = varinplastidx; varinpidx<form_->nrInputs();
		  varinpidx ++ )
		if ( !form_->isConst(varinpidx) && !form_->isSpec(varinpidx) )
		{
		    form_->setInputDef( varinpidx, refstr );
		    uimathform_->inpFld(idx)->selectInput( refstr.buf() );
		    varinplastidx = varinpidx+1;
		    break;
		}
	}
    }

    return true;
}


bool uiMathAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Attrib::Mathematics::attribName() )
	return false;

    mSetString( Attrib::Mathematics::expressionStr(),
		uimathform_->exprFld()->text() );

    int nrconsts = form_->nrConsts();
    if ( nrconsts )
    {
	mDescGetParamGroup(DoubleParam,cstset,desc,
			   Attrib::Mathematics::cstStr())
	cstset->setSize( nrconsts );
	int constidx = -1;
	for ( int idx=0; idx<form_->nrInputs(); idx++ )
	{
	    if ( form_->isConst(idx) )
	    {
		constidx++;
		DoubleParam& dparam = (DoubleParam&)(*cstset)[constidx];
		dparam.setValue( uimathform_->getConstVal(idx) );
    }
	}
    }

    if ( form_->isRecursive() )
    {
	FileMultiString fms;
	for ( int idx=0; idx<form_->maxRecShift(); idx++ )
	    fms.add( form_->recStartVals()[idx] );

	mSetString( Attrib::Mathematics::recstartvalsStr(), fms );
    }

    return true;
}


bool uiMathAttrib::getInput( Desc& desc )
{
    int attrinpidx = -1;
    for ( int idx=0; idx<form_->nrInputs(); idx++ )
    {
	if ( !form_->isConst(idx) && !form_->isSpec(idx) )
	{
	    attrinpidx++;
	    if ( attrinpidx >= desc.nrInputs() )
		return false;

	    Desc* inpdesc = desc.descSet()->getDescFromUIListEntry(
					uimathform_->inpFld(idx)->getInput() );
	    desc.setInput( attrinpidx, inpdesc );
    }
    }

    return true;
}


void uiMathAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    if ( !curDesc() ) return;

    int cstinpidx = -1;
    for ( int idx=0; idx<form_->nrInputs(); idx++ )
    {
	if ( form_->isConst(idx) )
	{
	    cstinpidx++;
	    params += EvalParam( form_->variableName(idx),
				 Attrib::Mathematics::cstStr(), 0,
			     idx );
    }
}
}
