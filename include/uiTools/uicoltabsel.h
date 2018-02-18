#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicolseqsel.h"
#include "coltabmapper.h"
class uiColSeqUseModeSel;
class uiEdMapperSetupDlg;
class uiManipMapper;
class uiMapperScaleTextInput;
class uiToolButton;


/*!\brief The color table selection tool on the OD main window. */

class uiColTabSelTool;
mGlobal(uiTools) uiColTabSelTool& uiCOLTAB();


/*!\brief Full Color Table = Sequence + Mapper Setup selection tool. */

mExpClass(uiTools) uiColTabSelTool : public uiColSeqSelTool
{ mODTextTranslationClass(uiColTabSelTool);
public:

    typedef ColTab::Mapper	Mapper;

				~uiColTabSelTool();

				// access current
    Mapper&			mapper()		{ return *mapper_; }
    const Mapper&		mapper() const		{ return *mapper_; }
    void			setMapper(Mapper&);

    void			setRange(Interval<float>);

    Notifier<uiColTabSelTool>	mappingChanged;
    Notifier<uiColTabSelTool>	mapperMenuReq;
				//!< CallBacker* is the uiMenu about to pop up

    virtual void		addObjectsToToolBar(uiToolBar&);
    virtual void		orientationChanged();

    static const char*		sKeyShowTextManipulators();
    static const char*		sKeyShowUseModeSel();
    static const char*		sKeyShowHistEqBut();
    static const char*		sKeyEnableAsymmetricClipping();

    static void			initClass();

protected:

				uiColTabSelTool();

    RefMan<Mapper>		mapper_;

    uiManipMapper*		manip_;
    uiMapperScaleTextInput*	txtscalefld_;
    uiColSeqUseModeSel*		usemodesel_;
    uiToolButton*		histeqbut_;

    void			initialise(OD::Orientation);
    void			handleMapperSetupChange();
    void			handleDistribChange();

    void			modeSelChgCB(CallBacker*);
    void			histeqButChgCB(CallBacker*);
    void			mapSetupChgCB(CallBacker*);
    void			mapRangeChgCB(CallBacker*);
    void			distribChgCB(CallBacker*);

    friend class		uiColTabSelToolHelper;
    friend class		uiManipMapper;
    friend class		uiMapperScaleTextInput;
    friend class		uiEdMapperSetupDlg;

};


/* uiGroup for color table selection */

mExpClass(uiTools) uiColTabSel : public uiGroup
			       , public uiColTabSelTool
{
public:

			uiColTabSel(uiParent*,OD::Orientation,
				uiString lbl=uiString::empty());

			mImpluiColSeqSelGroup();

};


/* uiToolBar for color table selection */

mExpClass(uiTools) uiColTabToolBar : public uiToolBar
{ mODTextTranslationClass(uiColTabToolBar);
public:

			uiColTabToolBar(uiParent*);

			mImpluiColSeqSelToolBar(uiColTabSelTool);

};
