#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
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

    typedef Strat::Level::ID	LevelID;

			uiStratLevelHorSel(uiParent*,const LevelID&);

    void		set2D(bool);

    bool		is2D() const	{ return is2d_; }
    LevelID		levelID() const;
    DBKey		horID() const;

    Notifier<uiStratLevelHorSel>    levelSel;
    Notifier<uiStratLevelHorSel>    horSel;

protected:

    bool		is2d_;
    LevelID		lvlid_;

    uiStratLevelSel*	lvlsel_;
    uiIOObjSel*		horsel2d_;
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

    typedef Strat::Level::ID	LevelID;
    typedef Strat::SeisEvent	SeisEvent;

    mExpClass(uiWellAttrib) Setup
    {
    public:
			Setup( bool wew=false )
			    : withextrwin_(wew)
			    , allowlayerbased_(false)
			    , sellevel_(true)	    {}

	mDefSetupMemb(bool,sellevel)
	mDefSetupMemb(LevelID,levelid)
	mDefSetupMemb(bool,withextrwin)
	mDefSetupMemb(bool,allowlayerbased)
    };

			uiStratSeisEvent(uiParent*,const Setup&);

    bool		getFromScreen();
    void		setLevel(const LevelID&);
    void		setLevel(const char* lvlnm);
    void		putToScreen();
    LevelID		levelID() const;
    BufferString	levelName() const;
    bool		snapToEvent() const;
    bool		hasExtrWin() const;
    bool		hasStep() const;
    bool		layerBased() const	{ return !hasStep(); }

			    // step may be undefined
    SeisEvent&		event()		{ return ev_; }
    const SeisEvent&	event() const	{ return ev_; }

    const StepInterval<float> getFullExtrWin() const;

    Notifier<uiStratSeisEvent>	anyChange;

protected:

    SeisEvent		ev_;
    Setup		setup_;
    EnumDefImpl<VSEvent::Type>	evtype_;

    uiGenInput*		evfld_;
    uiGenInput*		snapoffsfld_;
    uiStratLevelSel*	levelfld_		= 0;
    uiGenInput*		extrwinfld_		= 0;
    uiGenInput*		extrstepfld_		= 0;
    uiGenInput*		uptolvlfld_		= 0;
    uiCheckBox*		layerbasedfld_		= 0;

    void		initGrp(CallBacker*);
    void		evSnapCheck(CallBacker*);
    void		extrWinCB(CallBacker*);
    void		stopAtCB(CallBacker*);
    void		layBasedCB(CallBacker*);
    void		anyChgCB( CallBacker* )	    { anyChange.trigger(); }

};
