#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
#include "ranges.h"


class Grid2D;
class SurveyInfo;
class uiCheckBox;
class uiComboBox;
class uiLabel;
class uiLabeledComboBox;
class uiGenInput;
class uiGrid2DMapObject;
class uiGroup;
class uiSurvInfoProvider;
class uiSurveyMap;
class uiTabStack;

namespace Coords{ class uiCoordSystemSelGrp; }

/*!
\brief The survey info editor.
*/

mExpClass(uiIo) uiSurveyInfoEditor : public uiDialog
{ mODTextTranslationClass(uiSurveyInfoEditor);

public:

			uiSurveyInfoEditor(uiParent*,bool isnewborn=false);
    bool		isOK() const		{ return gengrp_; }
			//!<Must be checked before 'go'
			~uiSurveyInfoEditor();

    BufferString	dirName() const;

    static ObjectSet<uiSurvInfoProvider>& survInfoProvs();
    static int		addInfoProvider(uiSurvInfoProvider*);

    static bool		renameSurv(const char* path,const char* fromdirnm,
				   const char* todirnm);

protected:

    SurveyInfo&		si_;
    BufferString	basepath_;
    BufferString	orgdirname_;
    const BufferString	rootdir_;
    bool		isnew_;
    IOPar*		impiop_;
    ObjectSet<uiSurvInfoProvider> sips_;
    uiSurvInfoProvider*	lastsip_;
    uiString		errmsg_;

    uiGenInput*		survnmfld_;
    uiGenInput*		pathfld_;
    uiGenInput*		inlfld_;
    uiGenInput*		crlfld_;
    uiGenInput*		zfld_;
    uiLabel*		nrinlslbl_;
    uiLabel*		nrcrlslbl_;
    uiComboBox*		zunitfld_;
    uiComboBox*		pol2d3dfld_;

    uiGenInput*		x0fld_;
    uiGenInput*		xinlfld_;
    uiGenInput*		xcrlfld_;
    uiGenInput*		y0fld_;
    uiGenInput*		yinlfld_;
    uiGenInput*		ycrlfld_;
    uiGenInput*		ic0fld_;
    uiGenInput*		ic1fld_;
    uiGenInput*		ic2fld_;
    uiGenInput*		xy0fld_;
    uiGenInput*		xy1fld_;
    uiGenInput*		xy2fld_;
    Coords::uiCoordSystemSelGrp* latlongsel_;

    uiGroup*		gengrp_;
    uiGroup*		crdgrp_;
    uiGroup*		trgrp_;
    uiGroup*		rangegrp_;
    uiGroup*		latlonggrp_;
    uiLabeledComboBox*	sipfld_;
    uiCheckBox*		overrulefld_;
    uiGenInput*		depthdispfld_;
    uiGenInput*		refdatumfld_;
    uiSurveyMap*	surveymap_;
    uiGrid2DMapObject*  inlgridview_;
    Grid2D*		inlgrid_;
    uiTabStack*		tabs_;

    void		mkSIPFld(uiObject*,bool);
    void		mkRangeGrp();
    void		mkCoordGrp();
    void		mkTransfGrp();
    void		mkLatLongGrp();

    void		setValues();
    void		updateLabels();
    bool		setInlCrlRange();
    bool		setZRange();
    bool		setSurvName();
    bool		setCoords();
    bool		setRelation();
    bool		getFromScreen();
    void		updateMap();
    void                updStatusBar(const char*);

    bool		acceptOK();
    void		sipCB(CallBacker*);
    void		doFinalise(CallBacker*);
    void		setInl1Fld(CallBacker*);
    void		rangeChg(CallBacker*);
    void		coordsChg(CallBacker*);
    void		transformChg(CallBacker*);
    void		depthDisplayUnitSel(CallBacker*);
    void		updZUnit(CallBacker*);
    void		pathbutPush(CallBacker*);
    void		overruleCB(CallBacker*);

    static uiString	getSRDString(bool infeet);

    friend class	uiSurveyManager;

};
