#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uigroup.h"
#include "stratseisevent.h"
#include "uistring.h"

class uiCheckBox;
class uiGenInput;
class uiIOObjSel;
class uiStratLevelSel;

/*!\brief selector for Strat::Level and a horizon to go with it.

  It assumes there is something outside that determines whether to select
  2D or 3D horizons.
*/


mExpClass(uiWellAttrib) uiStratLevelHorSel : public uiGroup
{ mODTextTranslationClass(uiStratLevelHorSel);
public:

			uiStratLevelHorSel(uiParent*,const Strat::LevelID&);
			~uiStratLevelHorSel();

    void		set2D(bool);

    bool		is2D() const	{ return is2d_; }
    Strat::LevelID	levelID() const;
    MultiID		horID() const;

    Notifier<uiStratLevelHorSel> levelSel;
    Notifier<uiStratLevelHorSel> horSel;

protected:

    bool		is2d_ = false;
    Strat::LevelID	lvlid_;

    uiStratLevelSel*	lvlsel_;
    uiIOObjSel*		horsel2d_ = nullptr;
    uiIOObjSel*		horsel3d_;

    void		initGrp(CallBacker*);
    void		lvlSelCB(CallBacker*);
    void		horSelCB(CallBacker*);
    void		setHorFromLvl();

};


/*!\brief allows user to specify an auto-pick event on synthetic traces.

  You can fix the level. If not, the user can select one.

 */


mExpClass(uiWellAttrib) uiStratSeisEvent : public uiGroup
{ mODTextTranslationClass(uiStratSeisEvent);
public:

    mExpClass(uiWellAttrib) Setup
    {
    public:
			Setup(bool wew=false);
			~Setup();

	mDefSetupMemb(bool,sellevel)		// def: true
	mDefSetupMemb(Strat::LevelID,levelid)
	mDefSetupMemb(bool,withextrwin)
	mDefSetupMemb(bool,allowlayerbased)	// def: false
    };

			uiStratSeisEvent(uiParent*,const Setup&);
			~uiStratSeisEvent();

    bool		getFromScreen();
    void		setLevel(const Strat::LevelID&);
    void		setLevel(const char* lvlnm);
    void		putToScreen();
    Strat::LevelID	levelID() const;
    BufferString	levelName() const;
    bool		snapToEvent() const;
    bool		hasExtrWin() const;
    bool		hasStep() const;
    bool		layerBased() const	{ return !hasStep(); }

			    // step may be undefined
    Strat::SeisEvent&	event()			{ return ev_; }
    const Strat::SeisEvent& event() const	{ return ev_; }

			// step may be undefined
    const StepInterval<float> getFullExtrWin() const;

    Notifier<uiStratSeisEvent> anyChange;

protected:

    Strat::SeisEvent	ev_;
    Setup		setup_;
    EnumDefImpl<VSEvent::Type> evtype_;

    uiGenInput*		evfld_;
    uiGenInput*		snapoffsfld_;
    uiStratLevelSel*	levelfld_	= nullptr;
    uiGenInput*		extrwinfld_	= nullptr;
    uiGenInput*		extrstepfld_	= nullptr;
    uiGenInput*		uptolvlfld_	= nullptr;
    uiCheckBox*		layerbasedfld_	= nullptr;

    void		initGrp(CallBacker*);
    void		evSnapCheck(CallBacker*);
    void		extrWinCB(CallBacker*);
    void		stopAtCB(CallBacker*);
    void		layBasedCB(CallBacker*);
    void		anyChgCB( CallBacker* )     { anyChange.trigger(); }

};
