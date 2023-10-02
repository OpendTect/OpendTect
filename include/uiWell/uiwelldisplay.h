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
#include "welldata.h"
#include "uigeom.h"
#include "uigraphicsview.h"
#include "welldisp.h"

class uiWellDahDisplay;
class uiWellDispInfoPanel;
class uiWellDisplayControl;
class uiWellLogDisplay;
class uiWellStratDisplay;

namespace Well { class Data; }

/*!
\brief Well display.
*/

mExpClass(uiWell) uiWellDisplay : public uiGroup
{
public:

    mStruct(uiWell) Setup
    {
				Setup()
				    : nobackground_(false)  
				    , nologborder_(false)
				    , noxannot_(false)
				    , noyannot_(false)
				    , xaxisinpercents_(false)
				    , withcontrol_(true)
				    , takedisplayfrom3d_(false)
				    {}
	virtual		~Setup()	{}

	mDefSetupMemb(bool,nobackground)
	mDefSetupMemb(bool,noxannot)
	mDefSetupMemb(bool,xaxisinpercents)
	mDefSetupMemb(bool,noyannot)
	mDefSetupMemb(int,nologborder)
	mDefSetupMemb(bool,withcontrol) //will add a control 
	mDefSetupMemb(bool,takedisplayfrom3d) //read 3d scene display pars 

	void copyFrom(const Setup& su)
	{
	    nobackground_	= su.nobackground_;
	    nologborder_	= su.nologborder_;
	    withcontrol_	= su.withcontrol_;
	    noxannot_		= su.noxannot_;
	    noyannot_		= su.noyannot_;
	    xaxisinpercents_	= su.xaxisinpercents_;
	    takedisplayfrom3d_	= su.takedisplayfrom3d_;
	}
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

    Well::Data&			wd_;

    Interval<float>		zrg_;
    bool			dispzinft_;
    bool			zistime_;
    bool			use3ddisp_;
    uiSize			size_;
    const Setup			setup_;

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellDisplayControl*	control_;
    uiWellStratDisplay*		stratdisp_; 

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
