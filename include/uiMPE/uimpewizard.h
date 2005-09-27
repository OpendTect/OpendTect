#ifndef uimpewizard_h
#define uimpewizard_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.h,v 1.13 2005-09-27 22:02:58 cvskris Exp $
________________________________________________________________________

-*/


#include "uiwizard.h"

#include "cubesampling.h"
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

    void		reset();

    void		setObject( const MultiID&, const EM::SectionID& );
    void		setTrackingType(const char* typestr);

    static const int	sNamePage;
    static const int	sSeedSetupPage;
    static const int	sFinalizePage;

protected:

    uiGenInput*		namefld;

    uiColorInput*	colorfld;

    uiSetupSel*		setupgrp;

    uiGenInput*		anotherfld;
    uiGenInput*		typefld;

    uiGroup*		createNamePage();
    uiGroup*		createSeedSetupPage();
    uiGroup*		createFinalizePage();

    bool		leaveNamePage(bool);
    bool		leaveSeedSetupPage(bool);
    bool		leaveFinalizePage(bool);

    bool		prepareNamePage();
    bool		prepareSeedSetupPage();
    bool		prepareFinalizePage();

    void		isStarting();
    bool		isClosing(bool);

    bool		preparePage(int);
    bool		leavePage(int,bool);

    bool		createTracker();
    void		updateDialogTitle();
    void		adjustSeedBox();

    void		colorChangeCB(CallBacker*);
    void		anotherSel(CallBacker*);
    void		setupChange(CallBacker*);

    bool		allowpicking;
    bool		ispicking;
    void		updatePickingStatus();

    CubeSampling	seedbox;

    MultiID		currentobject;
    EM::SectionID	sid;
    bool		objectcreated;
    bool		trackercreated;
    
    uiMPEPartServer*	mpeserv;
    static int		defcolnr;
    BufferString	trackertype;
};

}; // namespace MPE

#endif
