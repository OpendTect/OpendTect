#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "stratlevel.h"
#include "uistring.h"
#include "uicompoundparsel.h"

class PropertyRefSelection;
class uiComboBox;
class uiLabel;
class uiSpinBox;
class uiStratLevelSel;
class uiToolButton;


mExpClass(uiStrat) uiStratGenDescTools : public uiGroup
{ mODTextTranslationClass(uiStratGenDescTools);
public:

		uiStratGenDescTools(uiParent*);

    int		nrModels() const;
    void	setNrModels(int);
    void	enableSave(bool);
    void	setGenWarning( const uiString& msg )
		    { genaskcontinuemsg_ = msg; } // cleared if user continues

    Notifier<uiStratGenDescTools>	openReq;
    Notifier<uiStratGenDescTools>	saveReq;
    Notifier<uiStratGenDescTools>	propEdReq;
    Notifier<uiStratGenDescTools>	genReq;
    Notifier<uiStratGenDescTools>	nrModelsChanged;

    int		getNrModelsFromPar(const IOPar&) const;
    void	fillPar(IOPar&) const;
    bool	usePar(const IOPar&);

protected:

    static const char*	sKeyNrModels();
    uiString	genaskcontinuemsg_;

    uiSpinBox*	nrmodlsfld_;
    uiToolButton* savetb_;

    void	openCB(CallBacker*)	{ openReq.trigger(); }
    void	saveCB(CallBacker*)	{ saveReq.trigger(); }
    void	propEdCB(CallBacker*)	{ propEdReq.trigger(); }
    void	genCB(CallBacker*);
    void	nrModelsChangedCB(CallBacker*)	{ nrModelsChanged.trigger(); }

};


mExpClass(uiStrat) uiStratLayModEditTools : public uiGroup
{ mODTextTranslationClass(uiStratLayModEditTools);
public:

		uiStratLayModEditTools(uiParent*);

    void	setProps(const BufferStringSet&);
    void	setContentNames(const BufferStringSet&);
    const BufferStringSet& levelNames() const	{ return lvlnms_; }

    Strat::Level selLevel() const;
    Strat::Level::ID selLevelID() const;
    int		selLevelIdx() const;
    BufferString selLevelName() const;
    Color	selLevelColor() const;

    const char*	selProp() const;			//!< May return null
    const char*	selContent() const;			//!< May return null
    int		dispEach() const;
    bool	dispZoomed() const;
    bool	dispLith() const;
    bool	showFlattened() const;

    void	setSelProp(const char*,bool notif=true);
    void	setSelLevel(const char*,bool notif=true);
    void	setSelContent(const char*,bool notif=true);
    void	setDispEach(int,bool notif=true);
    void	setDispZoomed(bool,bool notif=true);
    void	setDispLith(bool,bool notif=true);
    void	setShowFlattened(bool,bool notif=true);
    void	setFlatTBSensitive(bool);

    void	addEachFld();
    void	addLithFld();

    uiToolButton* lithButton()		{ return lithtb_; }
    uiToolButton* zoomButton()		{ return zoomtb_; }

    void	fillPar(IOPar&) const;
    bool	usePar(const IOPar&);

#define mDefSLMETNotif(nm) \
    Notifier<uiStratLayModEditTools>		nm; \
    void	nm##CB(CallBacker*)		{ nm.trigger(); }

    mDefSLMETNotif(	selPropChg )
    mDefSLMETNotif(	selLevelChg )
    mDefSLMETNotif(	selContentChg )
    mDefSLMETNotif(	dispZoomedChg )
    mDefSLMETNotif(	dispLithChg )
    mDefSLMETNotif(	showFlatChg )

    Notifier<uiStratLayModEditTools>	dispEachChg;
    void				dispEachChgCB(CallBacker*);

protected:

    uiGroup*		leftgrp_;
    uiGroup*		rightgrp_;
    uiComboBox*		propfld_;
    uiStratLevelSel*	lvlfld_;
    uiComboBox*		contfld_;
    uiToolButton*	zoomtb_;
    uiToolButton*	flattenedtb_;

    uiToolButton*	lithtb_			= 0;
    uiSpinBox*		eachfld_		= 0;
    int			prevdispeach_		= -1;

    BufferStringSet	lvlnms_;

    static const char*	sKeyDisplayedProp();
    static const char*	sKeyDecimation();
    static const char*	sKeySelectedLevel();
    static const char*	sKeySelectedContent();
    static const char*	sKeyZoomToggle();
    static const char*	sKeyDispLith();
    static const char*	sKeyShowFlattened();

    void		initGrp(CallBacker*);
    int			selPropIdx() const;

    friend class	uiStratLayerModelDisp;

};



mExpClass(uiStrat) uiStratLayModFRPropSelector : public uiDialog
{ mODTextTranslationClass(uiStratLayModFRPropSelector)
public:

    mExpClass(uiStrat) Setup
    {
	public:
			Setup(bool needswave=true,bool needpor=true,
			      bool needinitsat=true,bool needfinalsat=true)
			    : withswave_(needswave)
			    , withpor_(needpor)
			    , withinitsat_(needinitsat)
			    , withfinalsat_(needfinalsat)
			{}
	mDefSetupMemb(bool,withswave)
	mDefSetupMemb(bool,withpor)
	mDefSetupMemb(bool,withinitsat)
	mDefSetupMemb(bool,withfinalsat)
    };

			uiStratLayModFRPropSelector(uiParent*,
						  const PropertyRefSelection&,
						  const Setup&);

    void		setDenProp(const char*);
    void		setVPProp(const char*);
    void		setVSProp(const char*);
    void		setPorProp(const char*);
    void		setInitialSatProp(const char*);
    void		setFinalSatProp(const char*);

    bool		needsDisplay() const;
    bool		isOK() const;
    const char*		getSelVPName() const;
    const char*		getSelVSName() const;
    const char*		getSelDenName() const;
    const char*		getSelInitialSatName() const;
    const char*		getSelFinalSatName() const;
    const char*		getSelPorName() const;

    const uiString&	errMsg() const	{ return errmsg_; }

protected:

    virtual bool	acceptOK();

    uiComboBox*		vpfld_;
    uiComboBox*		vsfld_;
    uiComboBox*		denfld_;
    uiComboBox*		initialsatfld_;
    uiComboBox*		finalsatfld_;
    uiComboBox*		porosityfld_;
    mutable uiString	errmsg_;

};
