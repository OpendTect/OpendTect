/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/


#include <uiprogressbar.h>
#include <qprogressbar.h> 
#include <i_qobjwrap.h>


uiProgressBar::uiProgressBar( uiObject* p, const char* txt, 
			      int totalSteps, int progress )
	: uiWrapObj<i_QProgressBar>(new i_QProgressBar( *this, p, txt ), p,txt)
{
    setProgress( progress );
    setTotalSteps( totalSteps );
}

const QWidget* 	uiProgressBar::qWidget_() const 	{ return mQtThing(); } 


void uiProgressBar::setProgress( int progr )
{ 
    mQtThing()->setProgress( progr );
} 


int uiProgressBar::Progress() const
{
    return mQtThing()->progress();
}


void uiProgressBar::setTotalSteps( int tstp )
{ 
    mQtThing()->setTotalSteps( tstp );
} 


int uiProgressBar::totalSteps() const
{
    return mQtThing()->totalSteps();
}
