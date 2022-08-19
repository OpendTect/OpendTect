#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "uiwizard.h"

#include "trckeyzsampling.h"
#include "emposid.h"

class IOObj;
class uiButtonGroup;
class uiColorInput;
class uiGenInput;
class uiGroup;
class uiIOObjSel;
class uiIOObjSelGrp;
class uiLabel;
class uiSelLineStyle;
class uiMPEPartServer;
class uiTextEdit;

namespace MPE {

class uiSetupGroup;

mExpClass(uiMPE) Wizard : public uiWizard
{
public:
			Wizard( uiParent*, uiMPEPartServer* );
			~Wizard();

    void		reset();

    void		setObject( const EM::ObjectID&, const EM::SectionID& );
    void		setTrackingType(const char* typstr);

    static const int	sNamePage;
    static const int	sTrackModePage;
    static const int	sSeedSetupPage;
    static const int	sFinalizePage;

protected:
    void		restoreObject();

    uiIOObjSelGrp*	objselgrp;

    uiColorInput*	colorfld;

    uiSetupGroup*	hsetupgrp;
    uiSetupGroup*	h2dsetupgrp;
    uiSetupGroup*	fsetupgrp;
    uiSetupGroup*	setupgrp;

    uiButtonGroup*      hmodegrp;
    uiButtonGroup*      h2dmodegrp;
    uiButtonGroup*      fmodegrp;
    uiButtonGroup*      modegrp;
    uiTextEdit*		infofld;

    uiGenInput*		anotherfld;
    uiGenInput*		typefld;

    uiIOObjSelGrp*	createNamePage();
    uiGroup*		createTrackModePage();
    uiGroup*		createSeedSetupPage();
    uiGroup*		createFinalizePage();

    bool		prepareNamePage();
    bool		prepareTrackModePage();
    bool		prepareSeedSetupPage();
    bool		prepareFinalizePage();

    bool		leaveNamePage(bool);
    bool		leaveTrackModePage(bool);
    bool		leaveSeedSetupPage(bool);
    bool		leaveFinalizePage(bool);

    bool		finalizeCycle();

    void		isStarting();
    bool		isClosing(bool);

    bool		preparePage(int);
    bool		leavePage(int,bool);

    bool		createTracker();
    void		updateDialogTitle();
    void		adjustSeedBox();

    void		colorChangeCB(CallBacker*);
    void		anotherSel(CallBacker*);
    void		seedModeChange(CallBacker*);
    void		retrackCB(CallBacker*);
    void		aboutToAddRemoveSeed(CallBacker*);
    void		updateFinishButton(CallBacker*);

    TrcKeyZSampling	seedbox;

    EM::ObjectID	currentobject;
    EM::SectionID	sid;
    bool		ioparentrycreated;
    bool		objectcreated;
    bool		trackercreated;
    int			initialundoid_;
    int			oldsettingsseeds;
    
    uiMPEPartServer*	mpeserv;
    static int		defcolnr;
    BufferString	trackertype;

private:
};

} // namespace MPE
