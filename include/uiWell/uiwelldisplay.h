#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
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
{mODTextTranslationClass(uiWellDisplay);
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

	mDefSetupMemb(bool,nobackground)
	mDefSetupMemb(bool,noxannot)
	mDefSetupMemb(bool,xaxisinpercents)
	mDefSetupMemb(bool,noyannot)
	mDefSetupMemb(int,nologborder)
	mDefSetupMemb(bool,withcontrol) //will add a control
	mDefSetupMemb(bool,takedisplayfrom3d) //read 3d scene display pars

    };

//				uiWellDisplay(uiParent*,Well::Data& wd,
//							const Setup& su );
				uiWellDisplay(uiParent*,const DBKey&,
					      const Setup&);
				~uiWellDisplay();

    ConstRefMan<Well::Data>	wellData() const    { return wd_; }

    Interval<float>		zRange() const	{ return zrg_; }
    void			setZRange(Interval<float> zrg)
				{ zrg_ = zrg; setDahData(); }
    void			setZIsTime( bool yn )
				{ zistime_ = yn; setDahData(); }
    void			setZInFeet( bool yn )
				{ dispzinft_ = yn; setDahData(); }

    void			setControl(uiWellDisplayControl&);
    uiWellDisplayControl*	control()	{ return control_; }
    const uiWellDisplayControl*	control() const	{ return control_; }
    const Setup&		setup() const	{ return setup_; }

    const uiWellStratDisplay*	stratDisplay() const { return stratdisp_; }
    bool			hasStrat() const { return stratdisp_; }
    int				nrLogDisps() const { return logdisps_.size(); }
    void			clearLogDisplay();

protected:

    ConstRefMan<Well::Data>	wd_;
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
    void			updateDisplayFromWellData();

    void			wdChgCB(CallBacker*);
    void			wdLogChgCB(CallBacker*);
    void			wellReloadCB(CallBacker*);

private:

    void			init(const Setup&);

};


/*!
\brief Main window to display wells.
*/

mExpClass(uiWell) uiWellDisplayWin : public uiMainWin
{mODTextTranslationClass(uiWellDisplayWin);
public :
				uiWellDisplayWin(uiParent*,const DBKey&,
						 bool withcontrol=true);
				~uiWellDisplayWin();

protected:

    uiWellDisplay*		welldisp_;

    void			posChgCB(CallBacker*);

private:

    uiString			getWinTitle(const DBKey&,bool);

};
