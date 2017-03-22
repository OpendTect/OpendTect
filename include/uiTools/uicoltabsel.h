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
class uiManipMapper;
class uiEdMapperSetupDlg;
class uiColSeqUseModeSel;


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

    Notifier<uiColTabSelTool>	mapperMenuReq;
    Notifier<uiColTabSelTool>	mappingChanged;
				//!< CallBacker* is the uiMenu about to pop up

    virtual void		addObjectsToToolBar(uiToolBar&);
    virtual void		orientationChanged();

protected:

				uiColTabSelTool();

    RefMan<Mapper>		mapper_;

    uiManipMapper*		manip_;
    uiColSeqUseModeSel*		usemodesel_;

    void			initialise(OD::Orientation);
    void			addSetupNotifs();
    void			removeSetupNotifs();
    void			handleMapperSetupChange();
    void			handleDistribChange();

    void			modeChgCB(CallBacker*);
    void			mapSetupChgCB(CallBacker*);
    void			mapRangeChgCB(CallBacker*);
    void			distribChgCB(CallBacker*);

    friend class		uiManipMapper;
    friend class		uiEdMapperSetupDlg;

};


/* uiGroup for color table selection */

mExpClass(uiTools) uiColTabSel : public uiGroup
			       , public uiColTabSelTool
{
public:

			uiColTabSel(uiParent*,OD::Orientation,
				uiString lbl=uiString::emptyString());

			mImpluiColSeqSelGroup();

};


/* uiToolBar for color table selection */

mExpClass(uiTools) uiColTabToolBar : public uiToolBar
{ mODTextTranslationClass(uiColTabToolBar);
public:

			uiColTabToolBar(uiParent*);

			mImpluiColSeqSelToolBar(uiColTabSelTool);

};
