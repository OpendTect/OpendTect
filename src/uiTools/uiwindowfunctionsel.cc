/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          July 2007
 RCS:           $Id: uiwindowfunctionsel.cc,v 1.1 2007-07-23 16:51:20 cvskris Exp $
________________________________________________________________________

-*/

#include "uiwindowfunctionsel.h"

#include "bufstringset.h"
#include "uigeninput.h"
#include "windowfunction.h"


uiWindowFunctionSel::uiWindowFunctionSel( uiParent* p, const char* label,
					  const char* prevwinname,
					  float prevwinparam )
    : uiGroup( p, "Window function selector" )
{
    windowvariable_.allowNull();
    windowfuncs_.allowNull();

    BufferStringSet funcnames = WinFuncs().getNames();
    funcnames.insertAt( new BufferString( sNone() ), 0 );
    const StringListInpSpec funclist( funcnames );
    windowtypefld_ =  new uiGenInput( this, label, funclist );
    if ( prevwinname ) windowtypefld_->setText( prevwinname );

    windowtypefld_->valuechanged.notify(
			mCB( this, uiWindowFunctionSel, windowChangedCB ) );

    uiGenInput* firstvarfld = 0;
    for ( int idx=1; idx<funcnames.size(); idx++ )
    {
	WindowFunction* winfunc = WinFuncs().create( funcnames[idx]->buf());
	if ( winfunc && winfunc->hasVariable() )
	{
	    uiGenInput* varinp = new uiGenInput( this,
		winfunc->variableName(), FloatInpSpec(winfunc->getVariable()) );

	    if ( prevwinname && !strcmp(prevwinname, winfunc->name() ) )
	    {
		varinp->setValue( prevwinparam );
	    }

	    varinp->attach( alignedBelow, windowtypefld_ );
	    windowvariable_ += varinp;
	    if ( !firstvarfld )
		firstvarfld = varinp;
	}
	else
	    windowvariable_ += 0;

	windowfuncs_ += winfunc;
    }

    setHAlignObj( windowtypefld_ );

    windowChangedCB( 0 );
}


uiWindowFunctionSel::~uiWindowFunctionSel()
{
    deepErase( windowfuncs_ );
}


NotifierAccess& uiWindowFunctionSel::typeChange()
{
    return windowtypefld_->valuechanged;
}


const char* uiWindowFunctionSel::windowName() const
{
    return windowtypefld_->text( 0 );
}


float uiWindowFunctionSel::windowParamValue() const
{ 
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( winidx<0 || !windowvariable_[winidx] )
	return mUdf(float);

    return windowvariable_[winidx]->getfValue( 0 );
}


const char* uiWindowFunctionSel::windowParamName() const
{ 
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( winidx<0 || !windowvariable_[winidx] )
	return 0;

    return windowfuncs_[winidx]->variableName();
}


void uiWindowFunctionSel::windowChangedCB(CallBacker*)
{
    if ( !windowtypefld_ )
	return;

    const int winidx = windowtypefld_->getIntValue( 0 )-1;

    for ( int idx=0; idx<windowvariable_.size(); idx++ )
    {
	if ( windowvariable_[idx] )
	     windowvariable_[idx]->display( idx==winidx );
    }

}
