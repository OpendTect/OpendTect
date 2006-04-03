/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.cc,v 1.6 2006-04-03 13:30:32 cvshelene Exp $
________________________________________________________________________

-*/

#include "uimathattrib.h"
#include "mathattrib.h"
#include "mathexpression.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"

using namespace Attrib;

const int cNrVars = 6;

uiMathAttrib::uiMathAttrib( uiParent* p )
	: uiAttrDescEd(p)
	, nrvariables(0)
{
    inpfld = new uiGenInput( this, "Formula (e.g. x0+x1)", StringInpSpec() );

    parsebut = new uiPushButton( this, "Set", true );
    parsebut->activated.notify( mCB(this,uiMathAttrib,parsePush) );
    parsebut->attach( rightTo, inpfld );

    for ( int idx=0; idx<cNrVars; idx++ )
    {
        BufferString str( "Selection for x" );
	str += idx;
	uiAttrSel* attrbox = new uiAttrSel( this, 0, str );
	attribflds += attrbox;
	attrbox->display( false );
	attrbox->attach( alignedBelow, idx ? (uiObject*)attribflds[idx-1] 
					   : (uiObject*)inpfld );
    }

    setHAlignObj( inpfld );
}


void uiMathAttrib::parsePush( CallBacker* )
{
    MathExpression* expr = MathExpression::parse( inpfld->text() );
    nrvariables = expr ? expr->getNrVariables() : 0;
    if ( !expr && strcmp( inpfld->text(), "" ) )
    {
	uiMSG().error( "Could not parse this equation" );
	return;
    }

    if ( nrvariables > cNrVars )
    {
	uiMSG().error( "Max. nr of variables you can use is 6" );
	nrvariables = 0;
	return;
    }

    for ( int idx=0; idx<nrvariables; idx++ )
    {
	bool found = false;
	BufferString xstr = "x"; xstr += idx;
	for ( int idy=0; idy<nrvariables; idy++ )
	{
	    if ( !strcmp( expr->getVariableStr(idy), xstr.buf() ) )
		found = true;
	} 

	if ( !found )
	{
	    uiMSG().error( "Formula should have x0, x1, x2 ..." );
	    nrvariables = 0;
	    return;
	}
    }

    for ( int idx=0; idx<cNrVars; idx++ )
	attribflds[idx]->display( idx<nrvariables );
}


bool uiMathAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Math::attribName()) )
	return false;

    mIfGetString( Math::expressionStr(), expression, 
	    	  inpfld->setText(expression) );
    parsePush(0);
    return true;
}


bool uiMathAttrib::setInput( const Desc& desc )
{
    for ( int idx=0; idx<cNrVars; idx++ )
	putInp( attribflds[idx], desc, idx );

    return true;
}


bool uiMathAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Math::attribName()) )
	return false;

    mSetString( Math::expressionStr(), inpfld->text() );
    return true;
}


bool uiMathAttrib::getInput( Desc& desc )
{
    for ( int idx=0; idx<nrvariables; idx++ )
    {
	attribflds[idx]->processInput();
	fillInp( attribflds[idx], desc, idx );
    }

    return true;
}
