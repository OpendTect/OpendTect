#ifndef uimsg_H
#define uimsg_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          26/04/2000
 RCS:           $Id: uimsg.h,v 1.2 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include <callback.h>
class uiObject;
class MsgClass;
class uiMainWin;


class uiMsg : public CallBacker
{
    friend class uiMainWinBody;
    friend uiMsg& uiMSG();

public:

    void 	message(const char*,const char* caption="Information");
    void 	warning(const char*,const char* cn="Warning");
    void 	error(const char*,const char* cn="Error");
    void 	about(const char*,const char* cn="About");
    bool	askGoOn(const char*,const char* cn="Please specify");
    int		askGoOnAfter(const char*,const char* cn="Please specify");

protected:

		uiMsg();
    void	handleMsg(CallBacker*);
    void	toStatusbar(MsgClass*);

    uiMainWin*	mainwin_;
    static uiMsg* theinst_;

};


inline uiMsg& uiMSG()
{
    if ( !uiMsg::theinst_ )
	uiMsg::theinst_ = new uiMsg;
    return *uiMsg::theinst_;
}


#endif
