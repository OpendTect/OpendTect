#ifndef uimsg_H
#define uimsg_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/04/2000
 RCS:           $Id: uimsg.h,v 1.22 2009-01-09 04:26:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class uiMainWin;
class uiStatusBar;
class QWidget;


mClass uiMsg
{
    friend class uiMain;
    friend uiMsg& uiMSG();

public:

    // Messages
    void 	message(const char*,const char* part2=0,const char* part3=0);
    void 	warning(const char*,const char* part2=0,const char* part3=0);
    void 	error(const char*,const char* part2=0,const char* part3=0);

    // Interaction
    int 	notSaved(const char*,bool cancelbutt=true);
    		//!<\retval 0=Don't save 1=Save -1=Cancel
    bool	askGoOn(const char*,bool withyesno=true);
    		//!< withyesno false: 'Ok' and 'Cancel', true: 'Yes' and 'No'
    int		askGoOnAfter(const char*,const char* cnclmsg=0,
	    		     const char* textyes=0,const char* textno=0);
    		//!< 0=yes, 1=no, 2=cancel
    bool	showMsgNextTime(const char*,const char* msg=0);
    		//!< The msg must be negative, like "Don't show msg again"
    		//!< Be sure to store the ret val in the user settings

    static void	setNextCaption(const char*);
    		//!< Sets the caption for the next call to any of the msg fns
    		//!< After that, caption will be reset to default

    uiMainWin*	setMainWin(uiMainWin*);	//!< return old

    bool	toStatusbar(const char*,int fld=0,int msec=-1);
    		//!< returns false if there is none
    uiStatusBar* statusBar();

    void 	about(const char*);

protected:

			uiMsg();

    QWidget*		popParnt();

    static uiMsg*	theinst_;

private:

    uiMainWin*		uimainwin_;

};

inline uiMsg& uiMSG()
{
    if ( !uiMsg::theinst_ )
	uiMsg::theinst_ = new uiMsg;
    return *uiMsg::theinst_;
}


//!Sets the uiMSG's main window temporary during the scope of the object
mClass uiMsgMainWinSetter
{
public:
    			uiMsgMainWinSetter( uiMainWin* np )
			    : isset_( np )
			    , oldparent_( 0 )
			{
			    if ( np ) oldparent_ = ::uiMSG().setMainWin( np );
			}

			~uiMsgMainWinSetter()
			{ if ( isset_ ) ::uiMSG().setMainWin( oldparent_ ); }
protected:
    uiMainWin*		oldparent_;
    bool		isset_;
};


#endif
