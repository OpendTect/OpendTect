/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Arend Lammertink
 Date:		2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiprogressbar.cc,v 1.18 2009-07-22 16:01:38 cvsbert Exp $";


#include "uiprogressbar.h"
#include "uiobjbody.h"

#include	<QProgressBar>


class uiProgressBarBody : public uiObjBodyImpl<uiProgressBar,QProgressBar>
{
public:

                        uiProgressBarBody( uiProgressBar& handle, 
					   uiParent* parnt, const char* nm )
			    : uiObjBodyImpl<uiProgressBar,QProgressBar>
				(handle,parnt,nm)
			    { 
				setStretch( 1, 0 );
				setHSzPol( uiObject::MedVar );
			    }

    virtual int 	nrTxtLines() const			{ return 1; }
};



uiProgressBar::uiProgressBar( uiParent* p, const char* txt, 
			      int totalSteps, int progress )
    : uiObject(p,txt,mkbody(p,txt))
{
    setProgress( progress );
    setTotalSteps( totalSteps );
}


uiProgressBarBody& uiProgressBar::mkbody( uiParent* p, const char* txt )
{
    body_= new uiProgressBarBody( *this, p, txt );
    return *body_; 
}


void uiProgressBar::setProgress( int progr )
{ body_->setValue( progr ); } 

int uiProgressBar::progress() const
{ return body_->value(); }

void uiProgressBar::setTotalSteps( int tstp )
{ body_->setMaximum( tstp > 2 ? tstp : 2 ); } 

int uiProgressBar::totalSteps() const
{ return body_->maximum(); }
