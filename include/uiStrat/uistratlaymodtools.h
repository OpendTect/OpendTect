#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"

#include "uidialog.h"
#include "uigroup.h"
#include "stratlevel.h"
#include "uistring.h"

class uiComboBox;
class uiLabel;
class uiSpinBox;
class uiStratLevelSel;
class uiToolButton;


mExpClass(uiStrat) uiStratGenDescTools : public uiGroup
{ mODTextTranslationClass(uiStratGenDescTools);
public:

		uiStratGenDescTools(uiParent*);
		~uiStratGenDescTools();

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

    static const char*		sKeyNrModels();
    uiString	genaskcontinuemsg_;

    uiSpinBox*	nrmodlsfld_;
    uiToolButton* savetb_;

    void	openCB(CallBacker*)	{ openReq.trigger(); }
    void	saveCB(CallBacker*)	{ saveReq.trigger(); }
    void	propEdCB(CallBacker*)	{ propEdReq.trigger(); }
    void	genCB(CallBacker*);
    void	nrModelsChangedCB(CallBacker*);

};


mExpClass(uiStrat) uiStratLayModEditTools : public uiGroup
{ mODTextTranslationClass(uiStratLayModEditTools);
public:

		uiStratLayModEditTools(uiParent*);
		~uiStratLayModEditTools();

    void	setProps(const BufferStringSet&);
    void	setContentNames(const BufferStringSet&);

    Strat::Level selLevel() const;
    Strat::LevelID selLevelID() const;
    int		selLevelIdx() const;
    BufferString selLevelName() const;
    OD::Color	selLevelColor() const;

    bool	canSetDispEach() const	{ return eachfld_; }
    const char*	selProp() const;		//!< May return null
    const char*	selContent() const;		//!< May return null
    int		dispEach() const;
    bool	dispZoomed() const;
    bool	dispLith() const;
    bool	showFlattened() const;

    void	setSelProp(const char*,bool notif=true);
    void	setSelLevel(const char*,bool notif=true);
    void	setSelContent(const char*,bool notif=true);
    void	setDispEach(int,bool notif=true);
    void	setMaxDispEach(int,bool notif=false);
    void	setDispZoomed(bool,bool notif=true);
    void	setDispLith(bool,bool notif=true);
    void	setShowFlattened(bool,bool notif=true);
    void	setFlatTBSensitive(bool);

    void	addEachFld();
    void	addLithFld();

    uiToolButton* lithButton()		{ return lithtb_; }
    uiToolButton* zoomButton()		{ return zoomtb_; }

    void	fillPar(IOPar&) const;
    bool	usePar(const IOPar&,bool notif=true);

    Notifier<uiStratLayModEditTools>	selPropChg;
    Notifier<uiStratLayModEditTools>	selLevelChg;
    Notifier<uiStratLayModEditTools>	selContentChg;
    Notifier<uiStratLayModEditTools>	dispEachChg;
    Notifier<uiStratLayModEditTools>	dispZoomedChg;
    Notifier<uiStratLayModEditTools>	dispLithChg;
    Notifier<uiStratLayModEditTools>	showFlatChg;

protected:

    uiGroup*		leftgrp_;
    uiComboBox*		propfld_;
    uiStratLevelSel*	lvlfld_;
    uiComboBox*		contfld_;
    uiLabel*		eachlbl_	= nullptr;
    uiSpinBox*		eachfld_	= nullptr;

    uiGroup*		rightgrp_;
    uiToolButton*	lithtb_		= nullptr;
    uiToolButton*	zoomtb_;
    uiToolButton*	flattenedtb_;

    int			prevdispeach_	= -1;

    static const char*	sKeyDisplayedProp();
    static const char*	sKeyDecimation();
    static const char*	sKeySelectedLevel();
    static const char*	sKeySelectedContent();
    static const char*	sKeyZoomToggle();
    static const char*	sKeyDispLith();
    static const char*	sKeyShowFlattened();

    int		selPropIdx() const;

    void	initGrp(CallBacker*);
    void	selPropChgCB( CallBacker* )	{ selPropChg.trigger(); }
    void	selLevelChgCB( CallBacker* )	{ selLevelChg.trigger(); }
    void	selContentChgCB( CallBacker* )	{ selContentChg.trigger(); }
    void	dispZoomedChgCB( CallBacker* )	{ dispZoomedChg.trigger(); }
    void	dispLithChgCB( CallBacker* )	{ dispLithChg.trigger(); }
    void	showFlatChgCB( CallBacker* )	{ showFlatChg.trigger(); }
    void	dispEachChgCB(CallBacker*);

    friend class uiStratLayerModelDisp;

};


class PropertyRefSelection;

mExpClass(uiStrat) uiStratLayModFRPropSelector : public uiDialog
{ mODTextTranslationClass(uiStratLayModFRPropSelector)
public:

    mExpClass(uiStrat) Setup
    {
	public:
			Setup(bool needswave=true,bool needpor=true,
			      bool needinitsat=true,bool needfinalsat=true);
			~Setup();

	mDefSetupMemb(bool,withswave)
	mDefSetupMemb(bool,withpor)
	mDefSetupMemb(bool,withinitsat)
	mDefSetupMemb(bool,withfinalsat)
    };

			uiStratLayModFRPropSelector(uiParent*,
						  const PropertyRefSelection&,
						  const Setup&);
			~uiStratLayModFRPropSelector();

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

    bool		acceptOK(CallBacker*) override;

    uiComboBox*		vpfld_;
    uiComboBox*		vsfld_ = nullptr;
    uiComboBox*		denfld_;
    uiComboBox*		initialsatfld_ = nullptr;
    uiComboBox*		finalsatfld_ = nullptr;
    uiComboBox*		porosityfld_ = nullptr;
    mutable uiString	errmsg_;

};
