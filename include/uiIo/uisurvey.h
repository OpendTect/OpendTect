#ifndef uisurvey_h
#define uisurvey_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvey.h,v 1.29 2008-10-09 17:43:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class uiLabel;
class uiGraphicsScene;
class uiGraphicsView;
class uiListBox;
class uiTextEdit;
class SurveyInfo;
class uiSurveyMap;
class uiPushButton;
class uiToolButton;


/*!\brief The main survey selection dialog */

class uiSurvey : public uiDialog
{

public:
			uiSurvey(uiParent*);
			~uiSurvey();

    static void		updateViewsGlobal();
    			//!< updates caption on main window
    static void		getSurveyList(BufferStringSet&);

    static bool		survTypeOKForUser(bool is2d);
    			//!< checks whether given type has support
    			//!< returns whether user wants to continue

    /*!\brief 'Menu' item on window. First is always 'X,Y <-> I/C' */
    struct Util
    {
			Util( const char* pixmap, const char* tooltip,
				const CallBack& cb )
			    : cb_(cb)
			    , pixmap_(pixmap)
			    , tooltip_(tooltip)		{}

	CallBack	cb_;
	BufferString	pixmap_;
	BufferString	tooltip_;
    };
    static void		add(const Util&);

    SurveyInfo*		curSurvInfo()		{ return survinfo_; }
    const SurveyInfo*	curSurvInfo() const	{ return survinfo_; }

protected:

    SurveyInfo*		survinfo_;
    BufferStringSet	dirlist_;
    BufferString	initialdatadir_;
    BufferString	initialsurvey_;
    uiSurveyMap*	survmap_;

    uiListBox*		listbox_;
    uiPushButton*	newbut_;
    uiPushButton*	editbut_;
    uiPushButton*	rmbut_;
    uiPushButton*	datarootbut_;
    uiPushButton*	copybut_;
    ObjectSet<uiToolButton> utilbuts_;
    uiLabel*		inllbl_;
    uiLabel*		crllbl_; 
    uiLabel*		zlbl_;
    uiLabel*		binlbl_;
    uiTextEdit*		notes_;

    bool		acceptOK(CallBacker*);  
    bool		rejectOK(CallBacker*);  
    void		newButPushed(CallBacker*);
    void		editButPushed(CallBacker*);
    void		copyButPushed(CallBacker*);
    void		rmButPushed(CallBacker*);
    void		dataRootPushed(CallBacker*);
    void		utilButPush(CallBacker*);
    void 		getSurvInfo();
    bool		survInfoDialog();
    void		updateSvyList();
    void 		updateInfo(CallBacker*);
    void		mkInfo();
    void		writeComments();
    bool		updateSvyFile();
    bool		writeSurveyName(const char*);
    void		selChange(CallBacker*);
    void		newSurvey();
    void		mkDirList();

};

#endif
