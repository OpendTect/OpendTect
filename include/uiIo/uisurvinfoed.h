#ifndef uisurvinfoed_h
#define uisurvinfoed_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.h,v 1.29 2009-01-13 13:52:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"
class IOPar;
class SurveyInfo;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiLabel;
class uiGroup;
class uiPushButton;
class uiLabeledComboBox;
class uiSurvInfoProvider;


/*\brief The survey info editor */

mClass uiSurveyInfoEditor : public uiDialog
{

public:

			uiSurveyInfoEditor(uiParent*,SurveyInfo&);
			~uiSurveyInfoEditor();

    bool		dirnmChanged() const	{ return dirnamechanged; }
    const char*		dirName();
    Notifier<uiSurveyInfoEditor> survparchanged;

    static int		addInfoProvider(uiSurvInfoProvider*);
    static bool		copySurv(const char* frompath,const char* fromdirnm,
	    			 const char* topath,const char* todirnm);
    static bool		renameSurv(const char* path,const char* fromdirnm,
				   const char* todirnm);
    static const char*	newSurvTempDirName();
    			
protected:

    SurveyInfo&		si_;
    BufferString	orgdirname;
    BufferString	orgstorepath;
    const FileNameString rootdir;
    bool		isnew;
    IOPar*		impiop_;
    uiSurvInfoProvider*	lastsip_;

    uiGenInput*		survnmfld;
    uiGenInput*		dirnmfld;
    uiGenInput*		pathfld;
    uiGenInput*		inlfld;
    uiGenInput*		crlfld;
    uiGenInput*		zfld;
    uiLabeledComboBox*	zunitfld;
    uiLabeledComboBox*	pol2dfld;

    uiGenInput*		x0fld;
    uiGenInput*		xinlfld;
    uiGenInput*		xcrlfld;
    uiGenInput*		y0fld;
    uiGenInput*		yinlfld;
    uiGenInput*		ycrlfld;
    uiGenInput*		ic0fld;
    uiGenInput*		ic1fld;
    uiGenInput*		ic2fld;
    uiGenInput*		xy0fld;
    uiGenInput*		xy1fld;
    uiGenInput*		xy2fld;
    uiGenInput*		coordset;
    uiGroup*		crdgrp;
    uiGroup*		trgrp;
    uiComboBox*		sipfld;
    uiCheckBox*		overrulefld;
    uiCheckBox*		xyinftfld;
    uiPushButton*	applybut;
    ObjectSet<CallBacker> sipbuts;

    bool		dirnamechanged;
    uiObject*		mkSIPFld(uiObject*);
    uiGroup*		mkRangeGrp();
    void		mkCoordGrp();
    void		mkTransfGrp();
    void		setValues();
    bool		setRanges();
    bool		setCoords();
    bool		setRelation();
    bool		appButPushed();
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    void		sipbutPush(CallBacker*);
    void		doFinalise(CallBacker*);
    void		setInl1Fld(CallBacker*);
    void		chgSetMode(CallBacker*);
    void		pathbutPush(CallBacker*);
    void		updStatusBar(const char*);

    friend class	uiSurvey;
};

#endif
