#ifndef uiwelldisplay_h
#define uiwelldisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
 RCS:           $Id: uiwelldisplay.h,v 1.4 2010-10-29 12:43:11 cvsbruno Exp $
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
				    : nrlogdisplay_(1)
				    , nobackground_(false)  
				    , displaystrat_(false)
				    , isstratbelow_(false)
				    , nologborder_(false)
				    , noxgridline_(false)
				    , noygridline_(false)
				    , withcontrol_(true)
				    , preflogsz_(uiSize(100,400))
				    {}

	mDefSetupMemb(bool,nobackground)
	mDefSetupMemb(bool,noxgridline)
	mDefSetupMemb(bool,noygridline)
	mDefSetupMemb(int,nrlogdisplay)
	mDefSetupMemb(int,nologborder)
	mDefSetupMemb(bool,withcontrol) //will add a control 
	mDefSetupMemb(bool,displaystrat) //will make strat display visible
	mDefSetupMemb(bool,isstratbelow) //in case transparent backgrnd
	mDefSetupMemb(uiSize,preflogsz) //the actual uiSize will be computed on it 

	void copyFrom(const Setup& su)
	{
	    nrlogdisplay_ = su.nrlogdisplay_;
	    nobackground_ = su.nobackground_;
	    displaystrat_ = su.displaystrat_;
	    isstratbelow_ = su.isstratbelow_;
	    nologborder_  = su.nologborder_;
	    withcontrol_  = su.withcontrol_;
	    preflogsz_ 	  = su.preflogsz_;
	}
    };

				uiWellDisplay(uiParent*,const Well::Data& wd,
							const Setup& su );
				~uiWellDisplay();

    Interval<float>		zRange() const	{ return zrg_; }
    void 			setZRange(Interval<float> zrg)
				{ zrg_ = zrg; resetDahData(); }
    void 			setZIsTime( bool yn )
				{ zistime_ = yn; resetDahData(); }
    void 			setZInFeet( bool yn )
				{ dispzinft_ = yn; resetDahData(); }

    void			setControl(uiWellDisplayControl&);
    uiWellDisplayControl*	control() 	{ return control_; }
    const uiWellDisplayControl*	control() const	{ return control_; }

    const Setup&		setup() const	{ return setup_; }
    const uiSize&		size()		{ return size_; }

    void			applyWDChanged();

    uiWellStratDisplay*		stratDisplay() 	{ return stratdisp_; }
    const uiWellStratDisplay*	stratDisplay() const 
						{ return stratdisp_; }

    void			setDragMode(uiGraphicsViewBase::ODDragMode&);

    //Only if more than 1 logdisplay, this should take place in wd ( TODO ) 
    void			setDisplayProperties(int,
	    				const Well::DisplayProperties&);
    void			getDisplayProperties(
				    ObjectSet<Well::DisplayProperties>&) const;

protected:

    const Well::Data& 		wd_;

    Interval<float>		zrg_;
    bool			dispzinft_;
    bool			zistime_;
    uiSize			size_;
    const Setup 		setup_;

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellStratDisplay*		stratdisp_;
    uiWellDisplayControl*	control_;

    void			setInitialSize();
    void			resetDahData();
    void			resetWDDisplayProperties();

public :

    void                        setNewWidth(int); //do not use, 
						  //only for dynamic redraw
};


mClass uiWellDisplayWin : public uiMainWin
{
public :
			    	uiWellDisplayWin(uiParent*,Well::Data&);

protected:                  

    Well::Data& 		wd_;
    uiWellDisplay* 		welldisp_;
    uiWellDispInfoPanel*	wellinfo_;

    void 			mkInfoPanel(CallBacker*);
    void			dispInfoMsg(CallBacker*);
    void			updateProperties(CallBacker*);
    void 			closeWin(CallBacker*);
};

#endif
