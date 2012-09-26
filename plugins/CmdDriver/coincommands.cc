/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          September 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "coincommands.h"
#include "cmdrecorder.h"


namespace CmdDrive
{


bool WheelCmd::act( const char* parstr )
{
    mParKeyStrInit( "thumbwheel", parstr, parnext, keys, selnr );

    char* parnexxt;
    const float angle = (float) (strtod( parnext, &parnexxt) * M_PI / 180); 
    if ( parnexxt == parnext )
    {
	mParseErrStrm << "Turning the wheel requires angle in degrees"
	    	      << std::endl;
	return false;
    }
    mParSteps( parnexxt, partail, nrsteps, 1, 1 );
    mParTail( partail );
    
    mFindObjects( objsfound, uiThumbWheel, keys, nrgrey );
    mParKeyStrPre( "thumbwheel", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiThumbWheel*, uiwheel, objsfound[0] );
	
    const uiMainWin* prevwin = curWin();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	mPopUpWinInLoopCheck( prevwin );
	mActivateNoClearance( Wheel, Activator(*uiwheel, angle/nrsteps) );
    }
    waitForClearance();
    return true;
}


WheelActivator::WheelActivator( const uiThumbWheel& uiwheel, float angle )
    : actwheel_( const_cast<uiThumbWheel&>(uiwheel) )
    , actangle_( angle )
{}


void WheelActivator::actCB( CallBacker* cb )
{
    actwheel_.wheelPressed.trigger();
    actwheel_.move( actangle_ );
    actwheel_.wheelReleased.trigger();
}


bool GetWheelCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "thumbwheel", parnext, parnexxt, keys, selnr );
    mParExtraFormInit( parnexxt, partail, form, "Angle" )
    mParTail( partail );

    mFindObjects( objsfound, uiThumbWheel, keys, nrgrey );
    mParKeyStrPre( "thumbwheel", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiThumbWheel*, uiwheel, objsfound[0] );

    BufferString text;
    if ( mMatchCI(uiwheel->name(), "Dolly") )
	text = "Mov";
    if ( mMatchCI(uiwheel->name(), "vRotate") )
	text = "Rot";
    if ( mMatchCI(uiwheel->name(), "hRotate") )
	text = "Rot";

    mParForm( answer, form, text, uiwheel->getValue()*180/M_PI );
    mParIdentPost( identname, answer, parnext );
    return true;
}


//====== CmdComposers =========================================================


void WheelCmdComposer::init()
{ 
    oldvalue_ = mUdf(float);
    bursteventnames_.add( "wheelMoved" );
}


bool WheelCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( !mMatchCI(ev.msg_, "wheelReleased") )
	notDone();

    if ( ignoreflag_ || !ev.begin_ )
	return true;

    mDynamicCastGet( const uiThumbWheel*, uiwheel, ev.object_ );

    if ( mMatchCI(ev.msg_, "wheelPressed") )
	oldvalue_ = uiwheel->getValue(); 
    else if ( mMatchCI(ev.msg_, "wheelReleased") )
    {
	const float newvalue = uiwheel->getValue();
	const float angle = (float) (180*(newvalue-oldvalue_) / M_PI);
	insertWindowCaseExec( ev );
	mRecOutStrm << "Wheel \"" << ev.keystr_ << "\" " << angle << std::endl;
    }

    return true;
}


}; // namespace CmdDrive
