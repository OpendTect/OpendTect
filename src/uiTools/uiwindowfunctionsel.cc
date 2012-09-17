/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfunctionsel.cc,v 1.24 2009/11/27 15:34:42 cvsbruno Exp $";

#include "uiwindowfunctionsel.h"

#include "bufstringset.h"
#include "uigeninput.h"
#include "windowfunction.h"
#include "uiwindowfuncseldlg.h"
#include "uibutton.h"


uiWindowFunctionSel::uiWindowFunctionSel( uiParent* p, const Setup& su )
    : uiGroup( p, "Window function selector" )
    , onlytaper_(su.onlytaper_)				 
    , winfuncseldlg_(0)
{
    windowfuncs_.allowNull();

    BufferStringSet funcnames; 
    if ( onlytaper_ ) 
	funcnames.add( "CosTaper" ); 
    else
	funcnames = WinFuncs().getNames();
    funcnames.insertAt( new BufferString( sNone() ), 0 );
    const StringListInpSpec funclist( funcnames );
    windowtypefld_ =  new uiGenInput( this, su.label_, funclist );
    if ( su.winname_ ) windowtypefld_->setText( su.winname_ );

    windowtypefld_->valuechanged.notify(
			mCB( this, uiWindowFunctionSel, windowChangedCB ) );

    viewbut_ = new uiPushButton( this, "View", true );
    viewbut_->activated.notify( mCB(this,uiWindowFunctionSel,winfuncseldlgCB) );
    viewbut_->attach( rightTo, windowtypefld_ );

    for ( int idx=1; idx<funcnames.size(); idx++ )
    {
	WindowFunction* winfunc = WinFuncs().create( funcnames[idx]->buf());
	if ( winfunc && winfunc->hasVariable() )
	{
	    taperidx_ = idx-1;
	    BufferString varname ( winfunc->variableName() );
	    varname += " (%)";
	    float v = winfunc->getVariable() * 100;
	    if ( su.with2fldsinput_ )
	    {
		BufferString twosidedvarname( su.inpfldtxt_ );
		varinpfld_ = new uiGenInput( this, twosidedvarname.buf(), 
					FloatInpSpec(v), FloatInpSpec(v) );
	    }
	    else
		varinpfld_ = new uiGenInput(this,varname.buf(),FloatInpSpec(v));
	    if ( su.winname_ && !strcmp(su.winname_, winfunc->name() ) )
		varinpfld_->setValue( su.winparam_ * 100 );

	    varinpfld_->attach( alignedBelow, windowtypefld_ );
	    varinpfld_->valuechanged.notify( mCB(this,uiWindowFunctionSel,
					     windowChangedCB) );
	}

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


void uiWindowFunctionSel::setWindowParamValue( float val, int fldnr )
{
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( !windowfuncs_.validIdx(winidx) && !onlytaper_ ) return;

    varinpfld_->setValue( val * 100, fldnr );
    windowChangedCB(0);
}


float uiWindowFunctionSel::windowParamValue() const
{ 
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( winidx<0 || winidx != taperidx_ )
	return mUdf(float);

    return varinpfld_->getfValue( 0 )/100;
}


const char* uiWindowFunctionSel::windowParamName() const
{ 
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( winidx<0 ) return 0;

    return windowfuncs_[winidx]->variableName();
}


void uiWindowFunctionSel::winfuncseldlgCB( CallBacker* )
{
    if ( !winfuncseldlg_ )
	winfuncseldlg_ = new uiWindowFuncSelDlg( this, windowName(), 
						       windowParamValue() );
    else
    {
	winfuncseldlg_->setCurrentWindowFunc( windowName(),windowParamValue() );
	windowParamValue() ? winfuncseldlg_->setVariable( windowParamValue() ) :
			     winfuncseldlg_->setVariable( 0.05 );
    }
    winfuncseldlg_->windowClosed.notify( mCB(
				this,uiWindowFunctionSel,windowClosed) );
    winfuncseldlg_->show();
}


void uiWindowFunctionSel::windowClosed( CallBacker* )
{
    BufferString winname( windowName() );
    winname = winfuncseldlg_->getCurrentWindowName();
    if ( winname )
    {
	setWindowName( winname );
	windowtypefld_->valuechanged.trigger();
    }
    const float variable = winfuncseldlg_->getVariable();
    if( !mIsUdf(variable) && variable >= 0 )
	setWindowParamValue( variable );
    
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    varinpfld_->display( winidx == taperidx_ );
    viewbut_->display( !onlytaper_ || winidx == taperidx_  );
}


void uiWindowFunctionSel::windowChangedCB( CallBacker* )
{
    if ( !windowtypefld_ )
	return;

    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    varinpfld_->display( winidx == taperidx_ );
    viewbut_->display( !onlytaper_ || winidx == taperidx_  );
}

