#ifndef uimpewizard_h
#define uimpewizard_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.h,v 1.18 2005-10-10 21:57:07 cvskris Exp $
________________________________________________________________________

-*/


#include "uiwizard.h"

#include "cubesampling.h"
#include "emposid.h"

class Color;
class CtxtIOObj;
class IOObj;
class uiColorInput;
class uiGenInput;
class uiGroup;
class uiIOObjSel;
class uiIOObjSelGrp;
class uiLabeledSpinBox;
class uiPushButton;
class uiSelLineStyle;
class uiMPEPartServer;

namespace MPE {

class uiSetupSel;

class Wizard : public uiWizard
{
public:
			Wizard(uiParent*,uiMPEPartServer*);
			~Wizard();

    void		reset();

    void		setObject( const EM::ObjectID&, const EM::SectionID& );
    void		setTrackingType(const char* typestr);

    static const int	sNamePage;
    static const int	sSeedSetupPage;
    static const int	sFinalizePage;

protected:
    void		restoreObject();

    uiIOObjSelGrp*	objselgrp;

    uiColorInput*	colorfld;

    uiSetupSel*		setupgrp;

    uiGenInput*		anotherfld;
    uiGenInput*		typefld;

    uiIOObjSelGrp*	createNamePage();
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

    void		emObjectChange(CallBacker*);

    bool		allowpicking;
    bool		ispicking;
    void		updatePickingStatus();

    CubeSampling	seedbox;

    EM::ObjectID	currentobject;
    EM::SectionID	sid;
    bool		ioparentrycreated;
    bool		objectcreated;
    bool		trackercreated;
    int			initialhistorynr;
    
    uiMPEPartServer*	mpeserv;
    static int		defcolnr;
    BufferString	trackertype;
};

}; // namespace MPE

#endif
