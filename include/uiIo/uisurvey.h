#ifndef uisurvey_h
#define uisurvey_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvey.h,v 1.22 2007-03-15 16:13:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class uiLabel;
class uiCanvas;
class uiListBox;
class uiTextEdit;
class SurveyInfo;
class uiSurveyMap;
class uiPushButton;


/*!\brief The main survey selection dialog */

class uiSurvey : public uiDialog
{

public:
			uiSurvey(uiParent*,bool isgdi=false);
			~uiSurvey();

    static void		updateViewsGlobal();
    			//!< updates caption on main window
    static void		getSurveyList(BufferStringSet&);

    static bool		survTypeOKForUser(bool is2d);
    			//!< checks whether given type has support
    			//!< returns whether user wants to continue

protected:

    SurveyInfo*		survinfo;
    BufferStringSet	dirlist;
    BufferString	initialdatadir;
    uiSurveyMap*	survmap;

    uiListBox*		listbox;
    uiCanvas*		mapcanvas;
    uiPushButton*	newbut;
    uiPushButton*	editbut;
    uiPushButton*	rmbut;
    uiPushButton*	datarootbut;
    uiPushButton*	convbut;
    uiPushButton*	copybut;
    uiLabel*		inllbl;
    uiLabel*		crllbl; 
    uiLabel*		zlbl;
    uiLabel*		binlbl;
    uiTextEdit*		notes;

    bool		acceptOK(CallBacker*);  
    bool		rejectOK(CallBacker*);  
    void		newButPushed(CallBacker*);
    void		editButPushed(CallBacker*);
    void		copyButPushed(CallBacker*);
    void		rmButPushed(CallBacker*);
    void		dataRootPushed(CallBacker*);
    void		convButPushed(CallBacker*);
    void		tutButPushed(CallBacker*);
    void 		getSurvInfo();
    bool		survInfoDialog();
    void		updateSvyList();
    void 		updateInfo(CallBacker*);
    void		mkInfo();
    void		writeComments();
    bool		updateSvyFile();
    bool		writeSurveyName(const char*);
    void		selChange();
    void		doCanvas(CallBacker*);
    void		newSurvey();
    void		mkDirList();
};

#endif
