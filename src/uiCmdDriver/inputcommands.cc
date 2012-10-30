/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "inputcommands.h"
#include "cmddriverbasics.h"
#include "cmdrecorder.h"

#include <limits.h>
#include <math.h>

#include "uicombobox.h"
#include "uifileinput.h"
#include "uilabel.h"


namespace CmdDrive
{


#define mParEnterHold( objnm, parstr, parnext, enter, update ) \
\
    BufferString enterword; \
    const char* parnext = getNextWord( parstr, enterword.buf() ); \
    mSkipBlanks( parnext ); \
\
    bool enter = true; \
    if ( mMatchCI(enterword,"Hold") ) \
	enter = false; \
    else if ( !mMatchCI(enterword,"Enter") ) \
        parnext = parstr; \
\
    if ( !enter && !update ) \
    { \
	mParseWarnStrm << "Holding is void without (valid) argument " \
	    	       << "to update " << objnm << std::endl; \
    }

bool InputCmd::act( const char* parstr )
{
    mParKeyStrInit( "input field", parstr, parnext, keys, selnr );
    mParInputStr( "input field", parnext, parnexxt, inpstr, true );
    mParEnterHold( "input field", parnexxt, partail, enter, inpstr );
    mParTail( partail );

    mFindObjects3( objsfound, uiLineEdit, uiSpinBox, uiComboBox, keys, nrgrey );
    for ( int idx=objsfound.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( const uiComboBox*, uicombo, objsfound[idx] );
	if ( uicombo && uicombo->isReadOnly() )
	    objsfound.removeSingle( idx );
    }

    mParKeyStrPre( "input field", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiLineEdit*, uilined, objsfound[0] );
    mDynamicCastGet( const uiSpinBox*, uispin, objsfound[0] );
    mDynamicCastGet( const uiComboBox*, uicombo, objsfound[0] );

    if ( uilined )
    {
	if ( uilined->isReadOnly() )
	{
	    mWinErrStrm << "This field is read-only" << std::endl;
	    return false;
	}

	BufferString inpbs( inpstr );
	mDynamicCastGet( const uiFileInput*, uifileinp, uilined->parent() );
	if ( inpstr && uifileinp )
	{
	    StringProcessor(inpbs).makeDirSepIndep();
	    inpstr = inpbs.buf(); 
	}

	mActivate( Input, Activator(*uilined,inpstr,enter) );
    }

    if ( uispin )
	mActivate( SpinInput, Activator(*uispin,inpstr,enter) );
    if ( uicombo )
	mActivate( ComboInput, Activator(*uicombo,inpstr,enter) );

    return true;
}


void InputActivator::actCB( CallBacker* cb )
{
    if ( !actobj_.isReadOnly() )
    {
	if ( acttxt_ )
	    actobj_.setvalue_( acttxt_ );
	if ( actenter_ )
	    actobj_.returnPressed.trigger();
	if ( acttxt_ || actenter_ )
	    actobj_.editingFinished.trigger();
    }
}


bool GetInputCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "input field", parnext, parnexxt, keys, selnr );
    mParExtraFormInit( parnexxt, partail, form, "FilePath" );
    mParTail( partail );

    mFindObjects3( objsfound, uiLineEdit, uiSpinBox, uiComboBox, keys, nrgrey );
    for ( int idx=objsfound.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( const uiComboBox*, uicombo, objsfound[idx] );
	if ( uicombo && uicombo->isReadOnly() )
	    objsfound.removeSingle( idx );
    }

    mParKeyStrPre( "input field", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiLineEdit*, uilined, objsfound[0] );
    mDynamicCastGet( const uiSpinBox*, uispin, objsfound[0] );
    mDynamicCastGet( const uiComboBox*, uicombo, objsfound[0] );

    BufferString inptxt; 
    if ( uilined )
	inptxt = uilined->getvalue_();
    if ( uispin )
	inptxt = uispin->getFValue();
    if ( uicombo )
	inptxt = uicombo->text();

    mDynamicCastGet( const uiFileInput*, uifileinp, 
		     uilined ? uilined->parent() : 0 );

    const char* filepath = uifileinp ? uifileinp->fileName() : "";
    if ( uifileinp && !*filepath )
	filepath = uifileinp->defaultSelectionDir();

    mParForm( answer, form, inptxt, filepath );
    mParIdentPost( identname, answer, parnext );
    return true;
}


void SpinInputActivator::actCB( CallBacker* cb )
{
    if ( acttxt_ )
	actobj_.setValue( acttxt_ );
    if ( actenter_ )
	actobj_.valueChanged.trigger();
}


void ComboInputActivator::actCB( CallBacker* cb )
{
    if ( !actobj_.isReadOnly() )
    {
	if ( acttxt_ )
	    actobj_.setText( acttxt_ );

	if ( actenter_ )
	{
	    const char* txt = actobj_.text();
	    if ( !actobj_.isPresent(txt) )
		actobj_.addItem( txt );

	    actobj_.setCurrentItem( txt );
	    actobj_.selectionChanged.trigger();
	}
    }
}


bool SpinCmd::act( const char* parstr )
{
    mParKeyStrInit( "spinbox", parstr, parnext, keys, selnr );
    mParSteps( parnext, parnexxt, nrsteps, INT_MIN, 0 );
    mParEnterHold( "spinbox", parnexxt, partail, enter, nrsteps );
    mParTail( partail );

    mFindObjects( objsfound, uiSpinBox, keys, nrgrey );
    mParKeyStrPre( "spinbox", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiSpinBox*, uispin, objsfound[0] );

    const uiMainWin* prevwin = curWin();
    const int dir = nrsteps>0 ? 1 : -1;
    for ( int idx=0; idx<nrsteps*dir; idx++ )
    {
	mPopUpWinInLoopCheck( prevwin );
	const float oldvalue = uispin->getFValue();
	mActivateNoClearance( Spin, Activator(*uispin, dir) );

        if ( mIsEqual(uispin->getFValue(), oldvalue, uispin->fstep()/10) )
	{
	   mWinWarnStrm << "Unable to spin more than " << idx << " step(s) "
			<< (dir>0 ? "up" : "down") << std::endl; 
	   break;
	}
    }
    mActivate( SpinInput, Activator(*uispin, 0, enter) );
    return true;
}


SpinActivator::SpinActivator( const uiSpinBox& uispin, int nrsteps )
    : actspin_( const_cast<uiSpinBox&>(uispin) )
    , actsteps_( nrsteps )
{}


void SpinActivator::actCB( CallBacker* cb )
{ actspin_.stepBy( actsteps_ ); }


bool GetSpinCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "slider", parnext, parnexxt, keys, selnr );
    mParExtraFormInit( parnexxt, partail, form, "Value`Minimum`Maximum`Step" );
    mParTail( partail );

    mFindObjects( objsfound, uiSpinBox, keys, nrgrey );
    mParKeyStrPre( "spinbox", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiSpinBox*, uispin, objsfound[0] );

    mDynamicCastGet( uiLabeledSpinBox*, uilabeledspin,
		     const_cast<uiSpinBox*>(uispin)->parent() );
    BufferString text = uilabeledspin ? uilabeledspin->label()->text() : "";

    mParForm( answer, form, text, uispin->fstep() );
    mParExtraForm( answer, form, Value, uispin->getFValue() );
    mParExtraForm( answer, form, Minimum, uispin->minFValue() );
    mParExtraForm( answer, form, Maximum, uispin->maxFValue() );
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool SliderCmd::act( const char* parstr )
{
    mParKeyStrInit( "slider", parstr, parnext, keys, selnr );

    char* parnexxt;
    const float finalfrac = (float) strtod( parnext, &parnexxt) / 100; 
    if ( parnexxt==parnext || finalfrac<0.0 || finalfrac>1.0 )
    {
	mParseErrStrm << "Positioning a slider requires percentage between "
	    	      << "0.0 and 100.0" << std::endl;
	return false;
    }
    mParSteps( parnexxt, partail, nrsteps, 1, 1 );
    mParTail( partail );
    
    mFindObjects( objsfound, uiSlider, keys, nrgrey );
    mParKeyStrPre( "slider", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiSlider*, uislider, objsfound[0] );

    const uiMainWin* prevwin = curWin();
    float frac = uislider->getLinearFraction();
    const float fracstep = (finalfrac - frac) / nrsteps;
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	mPopUpWinInLoopCheck( prevwin );
	frac += fracstep;
	mActivateNoClearance( Slider, Activator(*uislider,frac) );
    }
    waitForClearance();
    return true;
}


SliderActivator::SliderActivator( const uiSlider& uislider, float frac )
    : actslider_( const_cast<uiSlider&>(uislider) )
    , actfrac_( frac )
{}


void SliderActivator::actCB( CallBacker* cb )
{
    if ( actfrac_>=0.0 && actfrac_<=1.0 )
    {
	actslider_.sliderPressed.trigger();
	actslider_.setLinearFraction( actfrac_ );
	actslider_.sliderMoved.trigger();
	actslider_.sliderReleased.trigger();
    }
}


bool GetSliderCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "slider", parnext, parnexxt, keys, selnr );
    mParExtraFormInit( parnexxt, partail, form,
	    				  "Value`Minimum`Maximum`Percentage" );
    mParTail( partail );

    mFindObjects( objsfound, uiSlider, keys, nrgrey );
    mParKeyStrPre( "slider", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiSlider*, uislider, objsfound[0] );

    mDynamicCastGet( uiSliderExtra*, uiextraslider,
		     const_cast<uiSlider*>(uislider)->parent() );
    BufferString text = uiextraslider ? uiextraslider->label()->text() : "";

    mParForm( answer, form, text, uislider->getLinearFraction()*100 );
    mParExtraForm( answer, form, Value, uislider->getValue() );
    mParExtraForm( answer, form, Minimum, uislider->minValue() );
    mParExtraForm( answer, form, Maximum, uislider->maxValue() );
    mParIdentPost( identname, answer, parnext );
    return true;
}


//====== CmdComposers =========================================================


void InputCmdComposer::init()
{ 
    textchanged_ = false;
    bursteventnames_.add( "textChanged" );
}


bool InputCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( mMatchCI(ev.msg_, "textChanged") )
    {
	textchanged_ = true;
	notDone();
	return true; 
    }

    if ( ignoreflag_ || !ev.begin_ )
	return true;

    bool enter = true;
    if ( mMatchCI(ev.msg_, "editingFinished") )
	enter = false;
    else if ( !mMatchCI(ev.msg_, "returnPressed") )
	return true;

    mDynamicCastGet( const uiLineEdit*, uilined, ev.object_ ); 
    mWriteInputCmd( textchanged_, uilined->getvalue_(), enter );
    return true;
}


void SpinCmdComposer::init()
{ 
    pendingsteps_ = 0;
    pendinginput_ = mUdf(float);
    bursteventnames_.add( "valueChanging" );
}


bool SpinCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    const char* msgnext;
    const float oldval = (float) strtod( ev.msg_, const_cast<char**>(&msgnext) );
    mSkipBlanks( msgnext );
    const bool valuechanging = mMatchCI( msgnext, "valueChanging" );

    if ( valuechanging )
	notDone();

    if ( ignoreflag_ || !ev.begin_ )
	return true;

    if ( valuechanging )
    {
	mDynamicCastGet( const uiSpinBox*, uispin, ev.object_ );
	const float newval = uispin->getFValue();
	const float curstep = uispin->fstep();
	const float incr = curstep ? (newval-oldval)/curstep : MAXFLOAT;

	if ( mIsZero(fabs(incr)-1.0, mDefEps) )
	{
	    if ( incr*pendingsteps_>=0 && mIsUdf(pendinginput_) )
	    {
		pendingsteps_ += mNINT32(incr);
		return true;
	    }
	}
	else if ( !pendingsteps_ && mIsUdf(pendinginput_) )
	{
	    pendinginput_ = newval;
	    return true;
	}
    }

    insertWindowCaseExec( ev );
    BufferString enterword = valuechanging ? " Hold" : " Enter";

    if ( !mIsUdf(pendinginput_) )
    {
	mRecOutStrm << "Input \"" << ev.keystr_ << "\" " << pendinginput_ 
		    << enterword << std::endl;
    }
    else if ( pendingsteps_ )
    {
	mRecOutStrm << "Spin \"" << ev.keystr_ << "\" " << pendingsteps_
		    << enterword << std::endl;
    }

    if ( valuechanging )
	mRefuseAndQuit();

    return true;
}


bool SliderCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ignoreflag_ || !ev.begin_ )
	return true;

    if ( mMatchCI(ev.msg_, "sliderReleased") )
    {
	mDynamicCastGet( const uiSlider*, uislider, ev.object_ );
	const float perc = 100 * uislider->getLinearFraction();
	insertWindowCaseExec( ev );
	mRecOutStrm << "Slider \"" << ev.keystr_ << "\" " << perc << std::endl;
    }

    return true;
}


}; // namespace CmdDrive
