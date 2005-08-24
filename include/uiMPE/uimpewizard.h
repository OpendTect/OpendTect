#ifndef uimpewizard_h
#define uimpewizard_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.h,v 1.10 2005-08-24 21:59:11 cvskris Exp $
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
    static const int	sSeedPage;
    static const int	sSetupPage;
    static const int	sFinalizePage;

protected:

    uiGenInput*		namefld;

    uiColorInput*	colorfld;
    uiLabeledSpinBox*	markerszbox;
    uiLabeledSpinBox*	linewidthbox;

    uiSetupSel*		setupgrp;

    uiGenInput*		anotherfld;
    uiGenInput*		typefld;

    uiGroup*		createNamePage();
    uiGroup*		createSeedPage();
    uiGroup*		createSetupPage();
    uiGroup*		createFinalizePage();

    bool		leaveNamePage(bool);
    bool		leaveSeedPage(bool);
    bool		leaveSetupPage(bool);
    bool		leaveFinalizePage(bool);

    bool		prepareNamePage();
    bool		prepareSeedPage();
    bool		prepareSetupPage();
    bool		prepareFinalizePage();

    void		isStarting();
    bool		isClosing(bool);

    bool		preparePage(int);
    bool		leavePage(int,bool);

    void		updateDialogTitle();
    void		adjustSeedBox();

    void		stickSetChange(CallBacker*);
    void		anotherSel(CallBacker*);

    CubeSampling	seedbox;

    MultiID		currentobject;
    EM::SectionID	sid;
    bool		objectcreated;
    bool		trackercreated;
    bool		reloadattribdata;
    
    uiMPEPartServer*	mpeserv;
    static int		defcolnr;
    BufferString	trackertype;
};

}; // namespace MPE

#endif
