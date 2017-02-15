#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
________________________________________________________________________


-*/

#include "uivismod.h"
#include "uidialog.h"
#include "uistring.h"
#include "coltabmappersetup.h"

namespace visSurvey { class SurveyObject; }
namespace ColTab { class Sequence; }
template <class T> class DataDistribution;
class uiColTabToolBar;
class uiColTabSelTool;


mExpClass(uiVis) uiVisColTabEd : public CallBacker
{ mODTextTranslationClass(uiVisColTabEd);
public:

				uiVisColTabEd(uiColTabToolBar&);
				~uiVisColTabEd();

    void			setColTab(const ColTab::Sequence*,
					  const ColTab::MapperSetup*);
    void			setColTab(visSurvey::SurveyObject*,int ch,
					  int version);
    const ColTab::Sequence&	getColTabSequence() const;
    ConstRefMan<ColTab::MapperSetup> getColTabMapperSetup() const;

    int				getChannel() const { return channel_; }
    const visSurvey::SurveyObject* getSurvObj() const { return survobj_; }

    NotifierAccess&		seqChange();
    NotifierAccess&		mapperChange();

    uiColTabSelTool&		colTabSel()	{ return coltabsel_; }
    void			setDistribution(const DataDistribution<float>*);
    void			setHistogram(const TypeSet<float>*);

    bool			usePar(const IOPar&);
    void                        fillPar(IOPar&);
    void			setDefaultColTab();

    static const char*          sKeyColorSeq();
    static const char*          sKeyScaleFactor();
    static const char*          sKeyClipRate();
    static const char*          sKeyAutoScale();
    static const char*          sKeySymmetry();
    static const char*          sKeySymMidval();
    void			colseqChanged(CallBacker*);
    void			colorTabChgdCB(CallBacker*);
    void			clipperChanged(CallBacker*);

protected:

    void			mapperChangeCB(CallBacker*);
    void			removeAllVisCB(CallBacker*);


    uiColTabSelTool&		coltabsel_;
    int				channel_;
    int				version_;
    visSurvey::SurveyObject*	survobj_;

};


mExpClass(uiVis) uiColorBarDialog : public uiDialog
{ mODTextTranslationClass(uiColorBarDialog);
public:
				uiColorBarDialog(uiParent*,
						 const uiString& title);

    uiVisColTabEd&		editor()	{ return *coltabed_; }

    Notifier<uiColorBarDialog>	winClosing;

protected:
    bool			closeOK();
    uiVisColTabEd*		coltabed_;
};
