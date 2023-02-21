/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
    varinpflds_.allowNull();

    BufferStringSet funcnames;
    if ( onlytaper_ )
	funcnames.add( "CosTaper" );
    else
	funcnames = WINFUNCS().getNames();
    funcnames.insertAt( new BufferString( sNone() ), 0 );
    const StringListInpSpec funclist( funcnames );
    windowtypefld_ =  new uiGenInput( this, mToUiStringTodo(su.label_), 
				      funclist );
    if ( su.winname_ ) windowtypefld_->setText( su.winname_ );

    windowtypefld_->valueChanged.notify(
			mCB(this,uiWindowFunctionSel,windowChangedCB) );

    viewbut_ = new uiPushButton( this, uiStrings::sView(), true );
    viewbut_->activated.notify( mCB(this,uiWindowFunctionSel,winfuncseldlgCB) );
    viewbut_->attach( rightTo, windowtypefld_ );

    for ( int idx=1; idx<funcnames.size(); idx++ )
    {
	WindowFunction* winfunc = WINFUNCS().create( funcnames[idx]->buf());
	uiGenInput* varinpfld = 0;
	if ( winfunc && winfunc->hasVariable() )
	{
	    uiString varname = 
		toUiString("%1 (%)").arg(winfunc->variableName());
	    const float v = winfunc->getVariable() * 100;
	    if ( su.with2fldsinput_ )
	    {
		uiString twosidedvarname( mToUiStringTodo(su.inpfldtxt_) );
		varinpfld = new uiGenInput( this, twosidedvarname,
					FloatInpSpec(v), FloatInpSpec(v) );
	    }
	    else
		varinpfld = new uiGenInput(this,varname,FloatInpSpec(v));

	    if ( StringView(su.winname_) == winfunc->name() )
		varinpfld->setValue( su.winparam_ * 100 );

	    varinpfld->attach( alignedBelow, windowtypefld_ );
	    varinpfld->valueChanged.notify(
			mCB(this,uiWindowFunctionSel,windowChangedCB) );
	}

	windowfuncs_ += winfunc;
	varinpflds_ += varinpfld;
    }

    setHAlignObj( windowtypefld_ );
    windowChangedCB( 0 );
}


uiWindowFunctionSel::~uiWindowFunctionSel()
{ deepErase( windowfuncs_ ); }


NotifierAccess& uiWindowFunctionSel::typeChange()
{ return windowtypefld_->valueChanged; }


const char* uiWindowFunctionSel::windowName() const
{ return windowtypefld_->text( 0 ); }


void uiWindowFunctionSel::setWindowName( const char* nm )
{
    windowtypefld_->setText( nm );
    windowChangedCB(0);
}


uiGenInput* uiWindowFunctionSel::getVariableFld( int winidx )
{ return varinpflds_.validIdx(winidx) ? varinpflds_[winidx] : 0; }

const uiGenInput* uiWindowFunctionSel::getVariableFld( int winidx ) const
{ return const_cast<uiWindowFunctionSel*>(this)->getVariableFld( winidx ); }


void uiWindowFunctionSel::setWindowParamValue( float val, int fldnr )
{
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    uiGenInput* varinpfld = getVariableFld( winidx );
    if ( !varinpfld ) return;

    varinpfld->setValue( val*100, fldnr );
    windowChangedCB(0);
}


float uiWindowFunctionSel::windowParamValue() const
{
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    const uiGenInput* varinpfld = getVariableFld( winidx );
    return varinpfld ? varinpfld->getFValue( 0 )/100 : mUdf(float);
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
    if ( winname.buf() )
    {
	setWindowName( winname );
	windowtypefld_->valueChanged.trigger();
	windowtypefld_->valueChanged.trigger();
    }

    const float variable = winfuncseldlg_->getVariable();
    if( !mIsUdf(variable) && variable >= 0 )
	setWindowParamValue( variable );
}


void uiWindowFunctionSel::windowChangedCB( CallBacker* )
{
    if ( !windowtypefld_ )
	return;

    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    for ( int idx=0; idx<varinpflds_.size(); idx++ )
	if ( varinpflds_[idx] ) varinpflds_[idx]->display( idx==winidx );

    const bool hasparam = varinpflds_.validIdx( winidx ) && varinpflds_[winidx];
    viewbut_->display( !onlytaper_ || hasparam );
}
