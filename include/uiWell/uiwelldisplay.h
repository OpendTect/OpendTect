#ifndef uiwelldisplay_h
#define uiwelldisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
 RCS:           $Id: uiwelldisplay.h,v 1.9 2011-05-27 07:51:05 cvsbruno Exp $
________________________________________________________________________

-*/

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

mClass uiWellDisplay : public uiGroup
{
public:

    mStruct Setup
    {
				Setup()
				    : nobackground_(false)  
				    , nologborder_(false)
				    , noxannot_(false)
				    , noyannot_(false)
				    , withcontrol_(true)
				    , preflogsz_(uiSize(150,600))
				    {}

	mDefSetupMemb(bool,nobackground)
	mDefSetupMemb(bool,noxannot)
	mDefSetupMemb(bool,noyannot)
	mDefSetupMemb(int,nrlogdisplay)
	mDefSetupMemb(int,nologborder)
	mDefSetupMemb(bool,withcontrol) //will add a control 
	mDefSetupMemb(uiSize,preflogsz) //the actual uiSize will be computed on it 

	void copyFrom(const Setup& su)
	{
	    nrlogdisplay_ = su.nrlogdisplay_;
	    nobackground_ = su.nobackground_;
	    nologborder_  = su.nologborder_;
	    withcontrol_  = su.withcontrol_;
	    preflogsz_ 	  = su.preflogsz_;
	    noxannot_	  = su.noxannot_;
	    noyannot_	  = su.noyannot_;
	}
    };

				uiWellDisplay(uiParent*,Well::Data& wd,
							const Setup& su );
				~uiWellDisplay();

    Interval<float>		zRange() const	{ return zrg_; }
    void 			setZRange(Interval<float> zrg)
				{ zrg_ = zrg; setDahData(); }
    void 			setZIsTime( bool yn )
				{ zistime_ = yn; setDahData(); }
    void 			setZInFeet( bool yn )
				{ dispzinft_ = yn; setDahData(); }

    void			setControl(uiWellDisplayControl&);
    uiWellDisplayControl*	control() 	{ return control_; }
    const uiWellDisplayControl*	control() const	{ return control_; }
    const Setup&		setup() const	{ return setup_; }
    const uiWellStratDisplay*	stratDisplay() const { return stratdisp_; }

    const uiSize&		size() const 	{ return size_; }

protected:

    Well::Data& 		wd_;

    Interval<float>		zrg_;
    bool			dispzinft_;
    bool			zistime_;
    uiSize			size_;
    const Setup 		setup_;

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellDisplayControl*	control_;
    uiWellStratDisplay*		stratdisp_; 

    void			setInitialSize();
    void			setDahData();
    void			setDisplayProperties();

    void			applyWDChanges(CallBacker*);
};


mClass uiWellDisplayWin : public uiMainWin
{
public :
			    	uiWellDisplayWin(uiParent*,Well::Data&);

protected:                  

    Well::Data& 		wd_;
    uiWellDisplay* 		welldisp_;

    void			dispInfoMsg(CallBacker*);
    void 			closeWin(CallBacker*);
};

#endif
