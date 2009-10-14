/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfunctionsel.cc,v 1.14 2009-10-14 14:37:32 cvsbruno Exp $";

#include "uiwindowfunctionsel.h"

#include "bufstringset.h"
#include "uigeninput.h"
#include "windowfunction.h"
#include "uiwindowfuncseldlg.h"
#include "uibutton.h"


uiWindowFunctionSel::uiWindowFunctionSel( uiParent* p, const Setup& su )
    : uiGroup( p, "Window function selector" )
    , winfuncseldlg_(0)
{
    windowvariable_.allowNull();
    windowfuncs_.allowNull();

    BufferStringSet funcnames; 
    if ( su.isminfreq_ || su.ismaxfreq_ ) 
	funcnames.add( "CosTaper" ); 
    else
	funcnames = WinFuncs().getNames();
    funcnames.insertAt( new BufferString( sNone() ), 0 );
    const StringListInpSpec funclist( funcnames );
    windowtypefld_ =  new uiGenInput( this, su.label_, funclist );
    if ( su.winname_ ) windowtypefld_->setText( su.winname_ );

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

	    if ( su.winname_ && !strcmp(su.winname_, winfunc->name() ) )
		varinp->setValue( su.winparam_ * 100 );

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
	setWindowName( winname );
    const float variable = winfuncseldlg_->getVariable();
    if( !mIsUdf(variable) && variable >= 0 )
	setWindowParamValue( variable );
    
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
}


uiFreqTaperSel::uiFreqTaperSel( uiParent* p, const Setup& su )
    : uiWindowFunctionSel( p, su )
    , isminfreq_(su.isminfreq_)
    , ismaxfreq_ (su.ismaxfreq_)     	
    , freqtaperdlg_(0)
{
}


void uiFreqTaperSel::winfuncseldlgCB( CallBacker* )
{
    delete freqtaperdlg_;
    uiFreqTaperDlg::Setup su; 		 
    su.hasmin_ = isminfreq_; 		su.hasmax_ = ismaxfreq_;
    su.minfreqrg_.set( freqrg_.start-18, freqrg_.start );
    su.maxfreqrg_.set( freqrg_.stop, freqrg_.stop+18 );

    freqtaperdlg_ = new uiFreqTaperDlg( this, su );
    freqtaperdlg_->setVariable( windowParamValue() ? windowParamValue() : 20 );
    freqtaperdlg_->windowClosed.notify( mCB(this,uiFreqTaperSel,windowClosed));
    freqtaperdlg_->show();
}


void uiFreqTaperSel::windowClosed( CallBacker* )
{
    const float maxvariable = freqtaperdlg_->getVariable( false )/100;
    const float minvariable = freqtaperdlg_->getVariable( true )/100;
    if( !mIsUdf(maxvariable) && maxvariable >= 0 )
	setWindowParamValue( maxvariable );
}
