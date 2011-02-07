#ifndef uistratseisevent_h
#define uistratseisevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratseisevent.h,v 1.2 2011-02-07 16:17:43 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "stratseisevent.h"
class uiComboBox;
class uiGenInput;


mClass uiStratSeisEvent : public uiGroup
{
public:

    mClass Setup
    {
    public:
			Setup( bool wew=false )
			    : withextrwin_(wew)
			    , fixedlevel_(0)		{}

	mDefSetupMemb(const Strat::Level*,fixedlevel)
	mDefSetupMemb(bool,withextrwin)
    };

    			uiStratSeisEvent(uiParent*,const Setup&);

    bool		getFromScreen();
    void		setLevel(const char* lvlnm);
    void		putToScreen();

    Strat::SeisEvent&	event()		{ return ev_; }

protected:

    Strat::SeisEvent	ev_;
    Setup		setup_;

    uiComboBox*		levelfld_;
    uiGenInput*		evfld_;
    uiGenInput*		snapoffsfld_;
    uiGenInput*		extrwinfld_;

    void		evSnapCheck(CallBacker*);

};


#endif
