/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfunctionsel.cc,v 1.11 2009-07-22 16:01:43 cvsbert Exp $";

#include "uiwindowfunctionsel.h"

#include "bufstringset.h"
#include "uigeninput.h"
#include "windowfunction.h"
#include "uiwindowfuncseldlg.h"
#include "uibutton.h"


uiWindowFunctionSel::uiWindowFunctionSel( uiParent* p, const char* label,
					  const char* prevwinname,
					  float prevwinparam )
    : uiGroup( p, "Window function selector" )
    , winfuncseldlg_(0)
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

    uiPushButton* viewbut = new uiPushButton( this, "View", true );
    viewbut->activated.notify( mCB(this,uiWindowFunctionSel,winfuncseldlgCB) );
    viewbut->attach( rightTo, windowtypefld_ );

    uiGenInput* firstvarfld = 0;
    for ( int idx=1; idx<funcnames.size(); idx++ )
    {
	WindowFunction* winfunc = WinFuncs().create( funcnames[idx]->buf());
	if ( winfunc && winfunc->hasVariable() )
	{
	    BufferString varname( winfunc->variableName() );
	    varname += " (%)";
	    uiGenInput* varinp = new uiGenInput( this,
		varname.buf(), FloatInpSpec(winfunc->getVariable() * 100) );

	    if ( prevwinname && !strcmp(prevwinname, winfunc->name() ) )
		varinp->setValue( prevwinparam * 100 );

	    varinp->attach( alignedBelow, windowtypefld_ );
	    windowvariable_ += varinp;
	    if ( !firstvarfld )
		firstvarfld = varinp;
	    varinp->valuechanged.notify( mCB(this,uiWindowFunctionSel,
					     windowChangedCB) );
	}
	else
	    windowvariable_ += 0;

	windowfuncs_ += winfunc;
    }

    setHAlignObj( windowtypefld_ );

    windowChangedCB( 0 );
}


uiWindowFunctionSel::~uiWindowFunctionSel()
{ deepErase( windowfuncs_ ); }

NotifierAccess& uiWindowFunctionSel::typeChange()
{ return windowtypefld_->valuechanged; }

const char* uiWindowFunctionSel::windowName() const
{ return windowtypefld_->text( 0 ); }

void uiWindowFunctionSel::setWindowName( const char* nm )
{
    windowtypefld_->setText( nm );
}


void uiWindowFunctionSel::setWindowParamValue( float val )
{
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( !windowfuncs_.validIdx(winidx) ) return;

    if (  windowvariable_[winidx] )
	windowvariable_[winidx]->setValue( val * 100 );
    for ( int idx=0; idx<windowvariable_.size(); idx++ )
    {
	if ( windowvariable_[idx] )
	     windowvariable_[idx]->display( idx==winidx );
    }
}


float uiWindowFunctionSel::windowParamValue() const
{ 
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( winidx<0 || !windowvariable_[winidx] )
	return mUdf(float);

    return windowvariable_[winidx]->getfValue( 0 )/100;
}


const char* uiWindowFunctionSel::windowParamName() const
{ 
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( winidx<0 || !windowvariable_[winidx] )
	return 0;

    return windowfuncs_[winidx]->variableName();
}

void uiWindowFunctionSel::winfuncseldlgCB( CallBacker* )
{
    if ( !winfuncseldlg_ )
	winfuncseldlg_ = new uiWindowFuncSelDlg( this, windowName(), windowParamValue() );
    else
    {
	winfuncseldlg_->setCurrentWindowFunc( windowName(),windowParamValue() );
	windowParamValue() ? winfuncseldlg_->setVariable( windowParamValue() ) :
	    		     winfuncseldlg_->setVariable( 0.05 );
    }
    winfuncseldlg_->windowClosed.notify( mCB(this,uiWindowFunctionSel,
					     windowClosed) );
    winfuncseldlg_->show();
}


void uiWindowFunctionSel::windowClosed( CallBacker* )
{
    BufferString winname( windowName() );
    if ( winfuncseldlg_->getCurrentWindowName( winname ) )
	setWindowName( winname );
    if( !mIsUdf(winfuncseldlg_->getVariable()) )
	setWindowParamValue( winfuncseldlg_->getVariable() );
    
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    for ( int idx=0; idx<windowvariable_.size(); idx++ )
    {
	if ( windowvariable_[idx] )
	     windowvariable_[idx]->display( idx==winidx );
    }
}


void uiWindowFunctionSel::windowChangedCB( CallBacker* )
{
    if ( !windowtypefld_ )
	return;

    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    for ( int idx=0; idx<windowvariable_.size(); idx++ )
    {
	if ( windowvariable_[idx] )
	     windowvariable_[idx]->display( idx==winidx );
    }

};
