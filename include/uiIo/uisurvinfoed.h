#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uisip.h"

#include "bufstringset.h"
#include "coordsystem.h"
#include "ranges.h"

class SurveyInfo;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiGroup;
class uiLabel;
class uiPushButton;
class uiSurvInfoProvider;
class uiTabStack;
namespace Coords { class uiCoordSystemSelGrp; }

/*!
\brief The survey info editor.
*/

mExpClass(uiIo) uiSurveyInfoEditor : public uiDialog
{ mODTextTranslationClass(uiSurveyInfoEditor)

public:
			uiSurveyInfoEditor(uiParent*,SurveyInfo&,
					   bool isnew=false);
			~uiSurveyInfoEditor();

    bool		isOK() const		{ return topgrp_; }
			//!<Must be checked before 'go'

    bool		dirnmChanged() const	{ return dirnamechanged; }
    const char*		dirName() const;
    void		setNameandPathSensitive(bool, bool);
    IOPar*		getImportPars();
    uiSurvInfoProvider* getSIP();

    static ObjectSet<uiSurvInfoProvider>& survInfoProvs();
    static int		addInfoProvider(uiSurvInfoProvider*);
    static bool		copySurv(const char* frompath,const char* fromdirnm,
				 const char* topath,const char* todirnm);
    static bool		renameSurv(const char* path,const char* fromdirnm,
				   const char* todirnm);

    Notifier<uiSurveyInfoEditor> survParChanged;

protected:

    SurveyInfo&		si_;
    BufferString	orgdirname_;
    BufferString	orgstorepath_;
    const BufferString	rootdir_;
    bool		isnew_;
    IOPar*		impiop_;
    ObjectSet<uiSurvInfoProvider> sips_;
    uiSurvInfoProvider*	lastsip_;

    RefMan<Coords::CoordSystem>	coordsystem_;

    uiGenInput*		survnmfld_;
    uiGenInput*		pathfld_;
    uiGenInput*		inlfld_;
    uiGenInput*		crlfld_;
    uiGenInput*		zfld_;
    uiLabel*		nrinlslbl_;
    uiLabel*		nrcrlslbl_;
    uiComboBox*		zunitfld_;
    uiComboBox*		pol2dfld_;

    uiTabStack*		tabs_;
    uiGenInput*		x0fld_;
    uiGenInput*		xinlfld_;
    uiGenInput*		xcrlfld_;
    uiGenInput*		y0fld_;
    uiGenInput*		yinlfld_;
    uiGenInput*		ycrlfld_;
    uiGenInput*		ic0fld_;
    uiGenInput*		ic1fld_;
    uiGenInput*		ic2fld_;
    uiGenInput*		ic3fld_;
    uiGenInput*		xy0fld_;
    uiGenInput*		xy1fld_;
    uiGenInput*		xy2fld_;
    uiGenInput*		xy3fld_;
    uiGroup*		topgrp_;
    uiGroup*		crdgrp_;
    uiGroup*		trgrp_;
    uiGroup*		rangegrp_;
    uiGroup*		crsgrp_;
    uiComboBox*		sipfld_;
    uiCheckBox*		overrulefld_;
    uiPushButton*	coordsysfld_;
    uiCheckBox*		xyinftfld_;
    uiGenInput*		depthdispfld_;
    uiGenInput*		refdatumfld_;
    uiLabel*		xyunitlbl_;
    Coords::uiCoordSystemSelGrp* crssel_;

    bool		xyInFeet() const;
    bool		dirnamechanged;
    void		mkSIPFld(uiObject*);
    void		mkRangeGrp();
    void		mkCoordGrp();
    void		mkTransfGrp();
    void		mkCRSGrp();
    void		setValues();
    void		updateLabels();
    void		updStatusBar(const char*);
    bool		setRanges();
    bool		setSurvName();
    bool		setCoords();
    bool		setRelation();
    bool		doApply();

    bool		acceptOK(CallBacker*) override;
    bool		rejectOK(CallBacker*) override;
    void		updatePar(CallBacker*);
    void		sipCB(CallBacker*);
    void		doFinalize(CallBacker*);
    void		ic0ChgCB(CallBacker*);
    void		ic2ChgCB(CallBacker*);
    void		rangeChg(CallBacker*);
    void		depthDisplayUnitSel(CallBacker*);
    void		updZUnit(CallBacker*);
    void		pathbutPush(CallBacker*);
    void		appButPushed(CallBacker*);

    static uiString	getSRDString(bool infeet);
    static uiString	getCoordString(bool infeet);

    friend class	uiSurvey;

};


mExpClass(uiIo) uiCopySurveySIP : public uiSurvInfoProvider
{
public:
			uiCopySurveySIP();
			~uiCopySurveySIP();

    const char*		usrText() const override
			    { return "Copy from other survey"; }
    uiDialog*		dialog(uiParent*) override;
    bool		getInfo(uiDialog*,
				TrcKeyZSampling&,Coord crd[3]) override;

    void		fillLogPars(IOPar&) const override;

    TDInfo		tdInfo() const override		{ return tdinf_; }
    bool		xyInFeet() const override	{ return inft_; }
    const char*		iconName() const override	{ return "copyobj"; }

    IOPar*		getCoordSystemPars() const override;

protected:

    BufferString	othersurvey_;
    TDInfo		tdinf_;
    bool		inft_;
    BufferStringSet	survlist_;
    IOPar*		crspars_			= nullptr;

public:
    void		reset();
};


mExpClass(uiIo) uiSurveyFileSIP : public uiSurvInfoProvider
{ mODTextTranslationClass(uiSurveyFileSIP)
public:
			uiSurveyFileSIP();
			~uiSurveyFileSIP();

    const char*		usrText() const override;
    uiDialog*		dialog(uiParent*) override;
    bool		getInfo(uiDialog*,
				TrcKeyZSampling&,Coord crd[3]) override;

    void		fillLogPars(IOPar&) const override;

    TDInfo		tdInfo() const override		{ return tdinf_; }
    bool		xyInFeet() const override	{ return inft_; }
    const char*		iconName() const override	{ return "ascii"; }

    IOPar*		getCoordSystemPars() const override;

protected:

    BufferString	filenm_;
    TDInfo		tdinf_;
    bool		inft_;
    BufferString	surveynm_;
    RefMan<Coords::CoordSystem> coordsystem_;
};
