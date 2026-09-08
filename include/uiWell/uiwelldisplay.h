#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uigroup.h"
#include "uimainwin.h"
#include "uigeom.h"
#include "uigraphicsview.h"

#include "welldata.h"
#include "welldisp.h"

class uiWellDahDisplay;
class uiWellDispInfoPanel;
class uiWellDisplayControl;
class uiWellLogDisplay;
class uiWellStratDisplay;


/*!
\brief Well display.
*/

mExpClass(uiWell) uiWellDisplay : public uiGroup
{
public:

    mStruct(uiWell) Setup
    {
				Setup();
	virtual			~Setup();

	mDefSetupMembInit(bool,nobackground,false)
	mDefSetupMembInit(bool,noxannot,false)
	mDefSetupMembInit(bool,xaxisinpercents,false)
	mDefSetupMembInit(bool,noyannot,false)
	mDefSetupMembInit(int,nologborder,false)
	mDefSetupMembInit(bool,withcontrol,true)
				//!< will add a control
	mDefSetupMembInit(bool,takedisplayfrom3d,false)
				//!< read 3d scene display pars

	void			copyFrom(const Setup&);
    };

				uiWellDisplay(uiParent*,Well::Data& wd,
					      const Setup& su );
				~uiWellDisplay();

    Interval<float>		zRange() const	{ return zrg_; }
    void			setZRange(Interval<float> zrg)
				{ zrg_ = zrg; setDahData(); }
    void			setZIsTime( bool yn )
				{ zistime_ = yn; setDahData(); }
    void			setZInFeet( bool yn )
				{ dispzinft_ = yn; setDahData(); }

    void			setControl(uiWellDisplayControl&);
    uiWellDisplayControl*	control()	{ return control_; }
    const uiWellDisplayControl* control() const { return control_; }
    const Setup&		setup() const	{ return setup_; }

    const uiWellStratDisplay*	stratDisplay() const { return stratdisp_; }
    bool			hasStrat() const { return stratdisp_; }
    int				nrLogDisps() const { return logdisps_.size(); }

protected:

    RefMan<Well::Data>		wd_;

    Interval<float>		zrg_;
    bool			dispzinft_;
    bool			zistime_;
    bool			use3ddisp_;
    uiSize			size_;
    const Setup			setup_;

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellDisplayControl*	control_	= nullptr;
    uiWellStratDisplay*		stratdisp_	= nullptr;

    void			setDahData();
    void			setDisplayProperties();

    void			applyWDChanges(CallBacker*);
    void			logsChanged(CallBacker*);
};


/*!
\brief Main window to display wells.
*/

mExpClass(uiWell) uiWellDisplayWin : public uiMainWin
{
public :
			    uiWellDisplayWin(uiParent*, const MultiID&);
			    ~uiWellDisplayWin();

protected:

    uiWellDisplay*		welldisp_;

    void			dispInfoMsg(CallBacker*);
};
