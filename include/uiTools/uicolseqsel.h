#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uitoolbar.h"
#include "coltabsequence.h"

class uiMenu;
class uiLabel;
class uiCheckBox;
class uiColSeqMan;
class uiColSeqDisp;


/* Color sequence selection object. */

mExpClass(uiTools) uiColSeqSelTool : public CallBacker
{ mODTextTranslationClass(uiColSeqSelTool);
public:

    typedef ColTab::Sequence	Sequence;

    virtual			~uiColSeqSelTool();

    ConstRefMan<Sequence>	sequence() const;
    void			setSequence(const Sequence&);
    const char*			seqName() const;
    void			setSeqName(const char*);
    ColTab::SeqUseMode		seqUseMode() const;
    void			setSeqUseMode(ColTab::SeqUseMode);
    void			setNonSeisDefault();

    Notifier<uiColSeqSelTool>	seqChanged;
    Notifier<uiColSeqSelTool>	menuReq;	//!< only when !usingBasicMenu()
    Notifier<uiColSeqSelTool>	newManDlg;
    Notifier<uiColSeqSelTool>	seqModified;
				//!< no change details, if needed start
				//!< watching the current sequence for yourself
    Notifier<uiColSeqSelTool>	refreshReq;
				//!< Only to be picked up by viewers that do
				//!< no automatic updating

    bool			usingBasicMenu() const	{ return usebasicmenu_;}
    void			setUseBasicMenu( bool yn )
				{ usebasicmenu_ = yn; }
    uiMenu*			getBasicMenu();	//!< starting point

    void			setCurrentAsDefault();
    void			showManageDlg();

    OD::Orientation		orientation() const;
    virtual bool		isGroup() const			{ return false;}
    virtual uiParent*		asParent()			= 0;
    virtual bool		isFinalised() const		= 0;
    virtual void		addObjectsToToolBar(uiToolBar&);

protected:

				uiColSeqSelTool();

    uiColSeqDisp*		disp_;
    uiColSeqMan*		mandlg_;
    bool			usebasicmenu_;

    void			initDisp(CallBacker*);
    void			seqModifCB(CallBacker*);
    void			selectCB(CallBacker*);
    void			menuCB(CallBacker*);
    void			colSeqModifiedCB(CallBacker*);
    void			upCB(CallBacker*)	{ nextColSeq(true); }
    void			downCB(CallBacker*)	{ nextColSeq(false); }
    void			setAsDefaultCB(CallBacker*);
    void			manageCB(CallBacker*)	{ showManageDlg(); }
    void			manDlgCloseCB(CallBacker*) { mandlg_ = 0; }

    void			initialise(OD::Orientation);
    virtual void		setIsVertical(bool);
    void			nextColSeq(bool prev);
    void			setToolTip();

};


#define mImpluiColSeqSelGroup() \
    virtual uiParent*		asParent()		{ return this; } \
    virtual bool		isGroup() const		{ return true; } \
    virtual bool		isFinalised() const	{ return finalised(); }\
    void			setLabelText(const uiString&); \
    void			setOrientation( OD::Orientation orient ) \
				{ setIsVertical(orient==OD::Vertical); } \
protected: \
    uiLabel*			lbl_

/* uiGroup for color sequence selection */

mExpClass(uiTools) uiColSeqSel : public uiGroup
				, public uiColSeqSelTool
{
public:

			uiColSeqSel(uiParent*,OD::Orientation,
				uiString lbl=uiString::emptyString());

			mImpluiColSeqSelGroup();

};


#define mImpluiColSeqSelToolBar(seltoolclssnm) \
\
    seltoolclssnm&	selTool()			{ return seltool_; } \
    operator		seltoolclssnm&()		{ return seltool_; } \
\
protected: \
\
    seltoolclssnm&	seltool_;



/* uiToolBar for color sequence selection */

mExpClass(uiTools) uiColSeqToolBar : public uiToolBar
{ mODTextTranslationClass(uiColSeqToolBar);
public:

			uiColSeqToolBar(uiParent*);

			mImpluiColSeqSelToolBar(uiColSeqSelTool)

};


mExpClass(uiTools) uiColSeqUseMode : public uiGroup
{ mODTextTranslationClass(uiColSeqUseMode);
public:

			uiColSeqUseMode(uiParent*,uiString lbltxt
							=tr("Use table"));

    ColTab::SeqUseMode	mode() const;
    void		setMode(ColTab::SeqUseMode);

    Notifier<uiColSeqUseMode>    modeChange;

protected:

    uiCheckBox*		flippedbox_;
    uiCheckBox*		cyclicbox_;

    void		boxChgCB( CallBacker* )		{ modeChange.trigger();}

};


/* Defines toolbar tool class in .cc file. */

#define mImpluiColSeqSelToolBarTool(tbarclssnm,baseclssnm) \
\
class tbarclssnm##Tool : public baseclssnm \
{ \
public: \
\
tbarclssnm##Tool( tbarclssnm* tb ) \
    : tbar_(tb) \
{ \
    initialise( tbar_->getOrientation() ); \
    addObjectsToToolBar( *tbar_ ); \
    mAttachCB( tbar_->orientationChanged, tbarclssnm##Tool::orChgCB ); \
} \
\
void orChgCB( CallBacker* ) \
{ \
    setIsVertical( tbar_->getOrientation() == OD::Vertical ); \
} \
\
virtual uiParent* asParent() \
{ \
    return tbar_; \
} \
\
virtual bool isFinalised() const \
{ \
    return tbar_->finalised(); \
} \
\
    tbarclssnm*	tbar_; \
\
};
