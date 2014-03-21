#ifndef uistratseisevent_h
#define uistratseisevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "stratseisevent.h"
class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiStratLevelSel;


mExpClass(uiWellAttrib) uiStratSeisEvent : public uiGroup
{
public:

    mExpClass(uiWellAttrib) Setup
    {
    public:
			Setup( bool wew=false )
			    : withextrwin_(wew)
			    , allowlayerbased_(false)
			    , fixedlevel_(0)		{}

	mDefSetupMemb(const Strat::Level*,fixedlevel)
	mDefSetupMemb(bool,withextrwin)
	mDefSetupMemb(bool,allowlayerbased)
    };

			uiStratSeisEvent(uiParent*,const Setup&);

    bool		getFromScreen();
    void		setLevel(const char* lvlnm);
    void		putToScreen();
    const char*		levelName() const;
    bool		doAllLayers() const;
    bool		hasExtrWin() const;
    bool		hasStep() const;

    Strat::SeisEvent&	event()		{ return ev_; }
			// step may be undefined
    const StepInterval<float> getFullExtrWin() const;

protected:

    Strat::SeisEvent	ev_;
    Setup		setup_;

    uiStratLevelSel*	levelfld_;
    uiGenInput*		evfld_;
    uiGenInput*		snapoffsfld_;
    uiGenInput*		extrwinfld_;
    uiCheckBox*		usestepfld_;
    uiGenInput*		extrstepfld_;
    uiLabel*		nosteplbl_;
    uiGenInput*		uptolvlfld_;

    void		evSnapCheck(CallBacker*);
    void		extrWinCB(CallBacker*);
    void		stopAtCB(CallBacker*);
    void		stepSelCB(CallBacker*);

};


#endif

