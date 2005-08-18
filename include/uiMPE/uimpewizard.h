#ifndef uimpewizard_h
#define uimpewizard_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.h,v 1.8 2005-08-18 14:44:16 cvskris Exp $
________________________________________________________________________

-*/


#include "uiwizard.h"

#include "emposid.h"

class uiColorInput;
class uiGenInput;
class uiGroup;
class uiIOObjSel;
class uiLabeledSpinBox;
class uiPushButton;
class uiSelLineStyle;
class uiMPEPartServer;
class Color;
class CtxtIOObj;
class IOObj;

namespace MPE {

class uiSetupSel;

class Wizard : public uiWizard
{
public:
				Wizard(uiParent*,uiMPEPartServer*);
				~Wizard();

    void			reset();

    void			setTrackingType(const char* typestr);
    void			setTrackerID(int);
    void			setSurfaceColor(const Color&);

    static const int		sNamePage;
    static const int		sSeedPage;
    static const int		sSetupPage;
    static const int		sFinalizePage;

protected:

    uiGenInput*			namefld;

    uiColorInput*		colorfld;
    uiLabeledSpinBox*		markerszbox;
    uiLabeledSpinBox*		linewidthbox;

    uiSetupSel*			setupgrp;

    uiGenInput*			anotherfld;
    uiGenInput*			typefld;

    uiGroup*			createNamePage();
    uiGroup*			createSeedPage();
    uiGroup*			createSetupPage();
    uiGroup*			createFinalizePage();

    bool			leaveNamePage(bool);
    bool			leaveSeedPage(bool);
    bool			leaveSetupPage(bool);
    bool			leaveFinalizePage(bool);

    bool			prepareNamePage();
    bool			prepareSeedPage();
    bool			prepareSetupPage();
    bool			prepareFinalizePage() { return true; }

    bool			preparePage(int);
    bool			leavePage(int,bool);

    bool			newObjectPresent(const char* objnm) const;
    void			updateDialogTitle();

    void			stickSetChange(CallBacker*);
    void			anotherSel(CallBacker*);
    
    uiMPEPartServer*		mpeserv;
    bool			dosave;
    EM::SectionID		sid;
    static int			defcolnr;
    bool			pickmode;
    int				curtrackid;
    BufferString		trackertype;
};

}; // namespace MPE

#endif
