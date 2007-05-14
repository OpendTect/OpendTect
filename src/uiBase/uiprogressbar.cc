/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Arend Lammertink
 Date:		2001
 RCS:		$Id: uiprogressbar.cc,v 1.14 2007-05-14 06:55:01 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uiprogressbar.h"
#include "uiobjbody.h"

#ifdef USEQT3
# include	<qprogressbar.h>
#else
# include	<QProgressBar>
#endif


#ifdef USEQT3
class uiProgressBarBody : public uiObjBodyImpl<uiProgressBar,QProgressBar>
{
public:

                        uiProgressBarBody( uiProgressBar& handle, 
					   uiParent* parnt, const char* nm )
			    : uiObjBodyImpl<uiProgressBar,QProgressBar>
				( handle, parnt,nm)
			    { 
				setStretch( 1, 0 );
				setHSzPol( uiObject::MedVar );
				setCenterIndicator( true );
			    }

    virtual int 	nrTxtLines() const			{ return 1; }
};
#else
class uiProgressBarBody : public uiObjBodyImplNoQtNm<uiProgressBar,QProgressBar>
{
public:

                        uiProgressBarBody( uiProgressBar& handle, 
					   uiParent* parnt, const char* nm )
			    : uiObjBodyImplNoQtNm<uiProgressBar,QProgressBar>
				(handle,parnt,nm)
			    { 
				setStretch( 1, 0 );
				setHSzPol( uiObject::MedVar );
			    }

    virtual int 	nrTxtLines() const			{ return 1; }
};

#endif


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


#ifdef USEQT3

void uiProgressBar::setProgress( int progr )
{ body_->setProgress( progr ); } 

int uiProgressBar::progress() const
{ return body_->progress(); }

void uiProgressBar::setTotalSteps( int tstp )
{ body_->setTotalSteps( tstp > 2 ? tstp : 2 ); } 

int uiProgressBar::totalSteps() const
{ return body_->totalSteps(); }

#else

void uiProgressBar::setProgress( int progr )
{ body_->setValue( progr ); } 

int uiProgressBar::progress() const
{ return body_->value(); }

void uiProgressBar::setTotalSteps( int tstp )
{ body_->setMaximum( tstp > 2 ? tstp : 2 ); } 

int uiProgressBar::totalSteps() const
{ return body_->maximum(); }

#endif
