#ifndef uipsviewerscalingtab_h
#define uipsviewerscalingtab_h

/*+
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          May 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewproptabs.h"

class uiPushButton;
namespace visSurvey { class PreStackDisplay; }

namespace PreStackView
{

class uiViewer3DMgr; 

mClass(uiPreStackViewer) uiViewer3DScalingTab : public uiFlatViewDataDispPropTab
{
public:
				uiViewer3DScalingTab(uiParent*,
						 visSurvey::PreStackDisplay&,
  						 uiViewer3DMgr&);
    virtual void		putToScreen();
    virtual void		setData()		{ doSetData(true); }

    bool			acceptOK();
    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }
    void			saveAsDefault(bool yn)  { savedefault_ = yn; }
    bool			saveAsDefault()         { return savedefault_; }

protected:

    bool			applyButPushedCB(CallBacker*);
    bool			settingCheck();

    virtual const char* 	dataName() const;
    void                	dispSel(CallBacker*);
    virtual void		handleFieldDisplay(bool) {}
    FlatView::DataDispPars::Common& commonPars();
    
    uiPushButton*		applybut_;
    uiViewer3DMgr&		mgr_;
    bool			applyall_;
    bool			savedefault_;
};


} // namespace

#endif
