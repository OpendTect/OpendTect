#ifndef uisurvey_h
#define uisurvey_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvey.h,v 1.12 2003-09-26 16:24:48 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class DirList;
class SurveyInfo;
class uiCanvas;
class uiListBox;
class uiPushButton;
class uiSurveyMap;
class uiTextEdit;
class uiLabel;

class uiSurvey : public uiDialog
{

public:
			uiSurvey(uiParent*,bool isgdi=false);
			~uiSurvey();

    static void		updateViewsGlobal();
    			//!< updates caption on main window

protected:

    SurveyInfo*		survinfo;
    DirList*		dirlist;
    uiListBox*		listbox;
    uiCanvas*		mapcanvas;
    uiSurveyMap*	survmap;
    uiPushButton*	newbut;
    uiPushButton*	editbut;
    uiPushButton*	rmbut;
    uiPushButton*	convbut;
    uiLabel*		inllbl;
    uiLabel*		crllbl; 
    uiLabel*		zlbl;
    uiLabel*		binlbl;
    uiTextEdit*		notes;

    bool		acceptOK(CallBacker*);  
    bool		rejectOK(CallBacker*);  
    void		newButPushed(CallBacker*);
    void		editButPushed(CallBacker*);
    void		rmButPushed(CallBacker*);
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

};

#endif
