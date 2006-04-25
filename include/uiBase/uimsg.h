#ifndef uimsg_H
#define uimsg_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/04/2000
 RCS:           $Id: uimsg.h,v 1.16 2006-04-25 16:53:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class uiMainWin;
class uiStatusBar;
class QWidget;


class uiMsg
{
    friend class uiMain;
    friend uiMsg& uiMSG();

public:

    void 	message(const char*,const char* caption=0);
    void 	warning(const char*,const char* cn=0);
    void 	error(const char*,const char* cn=0);
    void 	about(const char*,const char* cn=0);
    int 	notSaved(const char*, const char* cn=0,bool cancelbutt=true);
    		//!<\ retval 0 Don't save
    		//!<\ retval 1 Save
    		//!<\ retval -1 Cancel
    bool	askGoOn(const char*,bool withyesno=true,const char* cn=0);
    		//!< withyesno false: 'Ok' and 'Cancel', true: 'Yes' and 'No'
    int		askGoOnAfter(const char*,const char* cnclmsg=0,
	    		     const char* cn=0);
    bool	showMsgNextTime(const char*,const char* cn=0,const char* msg=0);
    		//!< The msg must be negative, like "Don't show msg again"
    		//!< Be sure to store the ret val in the user settings

    uiMainWin*	setMainWin(uiMainWin*);	//!< return old

    bool	toStatusbar(const char*); //!< returns false if there is none
    uiStatusBar* statusBar();

protected:

		uiMsg();

    QWidget*	popParnt();

    static uiMsg* theinst_;

private:

    uiMainWin*	uimainwin_;
};


inline uiMsg& uiMSG()
{
    if ( !uiMsg::theinst_ )
	uiMsg::theinst_ = new uiMsg;
    return *uiMsg::theinst_;
}


#endif
