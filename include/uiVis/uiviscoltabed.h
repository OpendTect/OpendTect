#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "uistring.h"

namespace visSurvey { class SurveyObject; }
namespace ColTab { class Sequence; class MapperSetup; }
class uiColorTable;


mExpClass(uiVis) uiVisColTabEd : public CallBacker
{ mODTextTranslationClass(uiVisColTabEd);
public:
    				uiVisColTabEd(uiColorTable&);
				~uiVisColTabEd();

    void			setColTab(const ColTab::Sequence*,
	    				  bool editseq,
	    				  const ColTab::MapperSetup*,
					  bool edittrans);
    void			setColTab(visSurvey::SurveyObject*,int ch,
	    				  int version);
    const ColTab::Sequence&	getColTabSequence() const;
    const ColTab::MapperSetup&	getColTabMapperSetup() const;

    int				getChannel() const { return channel_; }
    const visSurvey::SurveyObject* getSurvObj() const { return survobj_; }

    NotifierAccess&		seqChange();
    NotifierAccess&		mapperChange();
    
    void			setHistogram(const TypeSet<float>*);
    uiColorTable&		colTab()	{ return uicoltab_; }

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


    uiColorTable&		uicoltab_;
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
