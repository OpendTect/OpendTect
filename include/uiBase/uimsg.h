#ifndef uimsg_H
#define uimsg_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          26/04/2000
 RCS:           $Id: uimsg.h,v 1.6 2002-04-26 15:01:43 bert Exp $
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

    void 	message(const char*,const char* caption="Information");
    void 	warning(const char*,const char* cn="Warning");
    void 	error(const char*,const char* cn="Error");
    void 	about(const char*,const char* cn="About");
    bool	askGoOn(const char*,const char* cn="Please specify",
	    		bool withyesno=true);
    int		askGoOnAfter(const char*,const char* cnclmsg="Cancel",
	    		     const char* cn="Please specify");

    void	setParent(QWidget*);
    void	setMainWin(uiMainWin*);

protected:

		uiMsg();
    void	handleMsg(CallBacker*);
    void	toStatusbar(MsgClass*);

    uiStatusBar* statusBar();
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
