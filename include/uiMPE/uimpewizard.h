#ifndef uimpewizard_h
#define uimpewizard_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.h,v 1.4 2005-03-24 16:32:17 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uiwizard.h"

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

    void			setTrackingType(const char* typestr);
    void			startAt(int);
    void			setSurfaceColor(const Color&);

protected:

    uiGenInput*			namefld;

    uiColorInput*		colorfld;
    uiLabeledSpinBox*		markerszbox;

    uiSetupSel*			setupgrp;

    uiGenInput*			anotherfld;
    uiGenInput*			typefld;

    uiGroup*			createPage1();
    uiGroup*			createPage2();
    uiGroup*			createPage3();
    uiGroup*			createPage4();

    bool			processPage1();
    bool			processPage2();
    bool			processPage3();
    bool			processPage4();

    bool			addTracker(const char* objnm);
    bool			newObjectPresent(const char* objnm) const;
    void			updateDialogTitle();

    void			stickSetChange(CallBacker*);
    void			anotherSel(CallBacker*);
    
    void			nextPage(CallBacker*);
    void			cancelWizard(CallBacker*);
    void			finishWizard(CallBacker*);

    uiMPEPartServer*		mpeserv;
    static int			defcolnr;
    bool			pickmode;
    bool			currentfinished;
    int				curtrackid;
    BufferString		trackertype;
};

}; // namespace Tracking

#endif
