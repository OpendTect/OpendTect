#ifndef uimsg_H
#define uimsg_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          26/04/2000
 RCS:           $Id: uimsg.h,v 1.8 2003-01-23 16:15:24 bert Exp $
________________________________________________________________________

-*/

#include <callback.h>
class uiObject;
class MsgClass;
class uiMainWin;
class uiStatusBar;
class QWidget;


class uiMsg : public CallBacker
{
    friend class uiMainWinBody;
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

    void	setParent(QWidget*);
    void	setMainWin(uiMainWin*);

    uiStatusBar* statusBar();

protected:

		uiMsg();
    void	handleMsg(CallBacker*);
    void	toStatusbar(MsgClass*);

    QWidget*	popParnt();

    static uiMsg* theinst_;

private:

    uiMainWin*	uimainwin_;
    QWidget*	parent_;
};


inline uiMsg& uiMSG()
{
    if ( !uiMsg::theinst_ )
	uiMsg::theinst_ = new uiMsg;
    return *uiMsg::theinst_;
}


#endif
