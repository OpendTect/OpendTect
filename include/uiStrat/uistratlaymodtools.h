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
#include "uistring.h"

class uiComboBox;
class uiLabel;
class uiSpinBox;
class uiToolButton;
namespace Strat { class Level; }


mExpClass(uiStrat) uiStratGenDescTools : public uiGroup
{ mODTextTranslationClass(uiStratGenDescTools);
public:

		uiStratGenDescTools(uiParent*);

    int		nrModels() const;
    void	setNrModels(int);
    void	enableSave(bool);

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

    uiSpinBox*	nrmodlsfld_;
    uiToolButton* savetb_;

    void	openCB(CallBacker*)	{ openReq.trigger(); }
    void	saveCB(CallBacker*)	{ saveReq.trigger(); }
    void	propEdCB(CallBacker*)	{ propEdReq.trigger(); }
    void	genCB(CallBacker*)	{ genReq.trigger(); }
    void	nrModelsChangedCB(CallBacker*);

};


mExpClass(uiStrat) uiStratLayModEditTools : public uiGroup
{ mODTextTranslationClass(uiStratLayModEditTools);
public:

		uiStratLayModEditTools(uiParent*);

    void	setProps(const BufferStringSet&);
    void	setLevelNames(const BufferStringSet&);
    void	setContentNames(const BufferStringSet&);

    const char*	selProp() const;		//!< May return null
    const char*	selLevel() const;		//!< May return null
    const char*	selContent() const;		//!< May return null
    int		dispEach() const;
    bool	dispZoomed() const;
    bool	dispLith() const;
    bool	showFlattened() const;
    bool	mkSynthetics() const;

    void	setSelProp(const char*);
    void	setSelLevel(const char*);
    void	setSelContent(const char*);
    void	setDispEach(int);
    void	setDispZoomed(bool);
    void	setDispLith(bool);
    void	setShowFlattened(bool);
    void	setMkSynthetics(bool);
    void	setFlatTBSensitive(bool);

    void	setNoDispEachFld();

    Notifier<uiStratLayModEditTools>	selPropChg;
    Notifier<uiStratLayModEditTools>	selLevelChg;
    Notifier<uiStratLayModEditTools>	selContentChg;
    Notifier<uiStratLayModEditTools>	dispEachChg;
    Notifier<uiStratLayModEditTools>	dispZoomedChg;
    Notifier<uiStratLayModEditTools>	dispLithChg;
    Notifier<uiStratLayModEditTools>	flattenChg;
    Notifier<uiStratLayModEditTools>	mkSynthChg;

    int		selPropIdx() const;		//!< May return -1
    int		selLevelIdx() const;		//!< May return -1
    const Strat::Level*	selStratLevel() const;	//!< May return null
    OD::Color	selLevelColor() const;		//!< May return NoColor

    uiToolButton* lithButton()		{ return lithtb_; }
    uiToolButton* zoomButton()		{ return zoomtb_; }

    void	fillPar(IOPar&) const;
    bool	usePar(const IOPar&);

    bool	allownoprop_;

protected:

    static const char*		sKeyDisplayedProp();
    static const char*		sKeyDecimation();
    static const char*		sKeySelectedLevel();
    static const char*		sKeySelectedContent();
    static const char*		sKeyZoomToggle();
    static const char*		sKeyDispLith();
    static const char*		sKeyShowFlattened();

    uiComboBox*	propfld_;
    uiComboBox*	lvlfld_;
    uiComboBox*	contfld_;
    uiSpinBox*	eachfld_;
    uiLabel*	eachlbl_;
    uiToolButton* zoomtb_;
    uiToolButton* lithtb_;
    uiToolButton* flattenedtb_;
    uiToolButton* mksynthtb_;

    void	selPropCB( CallBacker* )	{ selPropChg.trigger(); }
    void	selLevelCB( CallBacker* )	{ selLevelChg.trigger(); }
    void	selContentCB( CallBacker* )	{ selContentChg.trigger(); }
    void	dispEachCB( CallBacker* )	{ dispEachChg.trigger(); }
    void	dispZoomedCB( CallBacker* )	{ dispZoomedChg.trigger(); }
    void	dispLithCB( CallBacker* )	{ dispLithChg.trigger(); }
    void	showFlatCB( CallBacker* )	{ flattenChg.trigger(); }
    void	mkSynthCB( CallBacker* )	{ mkSynthChg.trigger(); }

};


class PropertyRefSelection;

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

    virtual bool	acceptOK(CallBacker*);

    uiComboBox*		vpfld_;
    uiComboBox*		vsfld_ = nullptr;
    uiComboBox*		denfld_;
    uiComboBox*		initialsatfld_ = nullptr;
    uiComboBox*		finalsatfld_ = nullptr;
    uiComboBox*		porosityfld_ = nullptr;
    mutable uiString	errmsg_;

};


