#ifndef uimsg_H
#define uimsg_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/04/2000
 RCS:           $Id: uimsg.h,v 1.12 2004-12-20 12:16:15 dgb Exp $
________________________________________________________________________

-*/

#include <callback.h>
#include <msgh.h>
class uiObject;
class MsgClass;
class uiMainWin;
class uiStatusBar;
class QWidget;


class uiMsg : public UsrIoMsg
{
    friend class uiMain;
    friend uiMsg& uiMSG();

public:

    void 	message(const char*,const char* caption=0);
    void 	warning(const char*,const char* cn=0);
    void 	error(const char*,const char* cn=0);
    void 	about(const char*,const char* cn=0);
    bool	askGoOn(const char*,bool withyesno=true,const char* cn=0);
    		//!< withyesno false: 'Ok' and 'Cancel', true: 'Yes' and 'No'
    int		askGoOnAfter(const char*,const char* cnclmsg=0,
	    		     const char* cn=0);

    uiMainWin*	setMainWin(uiMainWin*);	//!< return old

    uiStatusBar* statusBar();

protected:

		uiMsg();
    void	handleMsg(CallBacker*);
    void	toStatusbar(MsgClass*);

    QWidget*	popParnt();

    static uiMsg* theinst_;

private:

    uiMainWin*	uimainwin_;
};


inline uiMsg& uiMSG()
{
    if ( !uiMsg::theinst_ )
    {
	uiMsg::theinst_ = new uiMsg;
	UsrIoMsg::theUsrIoMsg_ = uiMsg::theinst_;
    }
    return *uiMsg::theinst_;
}


#endif
