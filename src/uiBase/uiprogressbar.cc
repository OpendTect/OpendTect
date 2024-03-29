/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprogressbar.h"

#include "uiobjbodyimpl.h"
#include <QProgressBar>

mUseQtnamespace

class uiProgressBarBody : public uiObjBodyImpl<uiProgressBar,QProgressBar>
{
public:
			uiProgressBarBody( uiProgressBar& hndle,
					   uiParent* parnt, const char* nm )
			    : uiObjBodyImpl<uiProgressBar,QProgressBar>
				(hndle,parnt,nm)
			    {
				setStretch( 1, 0 );
				setHSzPol( uiObject::MedVar );
			    }

    int			nrTxtLines() const override		{ return 1; }
};



uiProgressBar::uiProgressBar( uiParent* p, const char* txt,
			      int totsteps, int progr )
    : uiObject(p,txt,mkbody(p,txt))
{
    setProgress( progr );
    setTotalSteps( totsteps );
}


uiProgressBar::~uiProgressBar()
{}


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
{ body_->setMaximum( tstp > 0 ? tstp : 0 ); }

int uiProgressBar::totalSteps() const
{ return body_->maximum(); }
