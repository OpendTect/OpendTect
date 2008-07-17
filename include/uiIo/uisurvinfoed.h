#ifndef uisurvinfoed_h
#define uisurvinfoed_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.h,v 1.26 2008-07-17 15:10:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "ranges.h"

class Coord;
class CubeSampling;
class SurveyInfo;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiLabel;
class uiPushButton;
class uiLabeledComboBox;


/*\brief Interface for survey info provider

  The survey setup can be delivered by scanning files (SEG-Y) or by querying
  data stores like SeisWorks and GeoFrame. Another idea not implemented may be
  to read a simple ascii file. In any case, this is the interface. The
  implementation should be added to the factory using 
  uiSurveyInfoEditor::addInfoProvider.
 
 */

class uiSurvInfoProvider
{
public:

    virtual const char*		usrText() const		= 0;
    virtual uiDialog*		dialog(uiParent*)	= 0;
    virtual bool		getInfo(uiDialog*,CubeSampling&,
					Coord crd[3])	= 0;

    virtual const char*		scanFile() const	{ return 0; }

    enum TDInfo			{ Uknown, Time, Depth, DepthFeet };
    virtual TDInfo		tdInfo() const		{ return Uknown; }
};


/*\brief The survey info editor */

class uiSurveyInfoEditor : public uiDialog
{

public:
			uiSurveyInfoEditor(uiParent*,SurveyInfo&);

    bool		dirnmChanged() const	{ return dirnamechanged; }
    const char*		dirName();
    Notifier<uiSurveyInfoEditor> survparchanged;

    static int		addInfoProvider(uiSurvInfoProvider*);
    			//!< See uiSurvInfoProvider class comments
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
};

#endif
