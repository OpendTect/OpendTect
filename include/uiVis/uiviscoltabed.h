#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
________________________________________________________________________


-*/

#include "uivismod.h"
#include "uistring.h"
#include "coltabmappersetup.h"

namespace visSurvey { class SurveyObject; }
namespace ColTab { class Sequence; }
class uiColTabToolBar;
class uiColTabSelTool;


//TODO: this class should go, or at least be demoted to monitoring
// and checking a few things.


mExpClass(uiVis) uiVisColTabEd : public CallBacker
{ mODTextTranslationClass(uiVisColTabEd);
public:

				uiVisColTabEd(uiColTabToolBar&);
				~uiVisColTabEd();

    bool			isDisplayed() const;
    void			display(bool);

    void			setColTab(const ColTab::Sequence&,
					  ColTab::Mapper&);
    void			setColTab(visSurvey::SurveyObject*,int ch);
    const ColTab::Sequence&	getColTabSequence() const;
    const ColTab::Mapper&	getColTabMapper() const;

    int				getChannel() const { return channel_; }
    const visSurvey::SurveyObject* getSurvObj() const { return survobj_; }

    NotifierAccess&		seqChange();

    uiColTabSelTool&		colTabSel()	{ return coltabsel_; }

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

    Notifier<uiVisColTabEd>	isDisplayedChange;

protected:

    void			removeAllVisCB(CallBacker*);


    uiColTabSelTool&		coltabsel_;
    int				channel_;
    visSurvey::SurveyObject*	survobj_;

};
