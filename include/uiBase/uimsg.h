#ifndef uimsg_H
#define uimsg_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          26/04/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "gendefs.h"
#include "uistring.h"
class uiMainWin;
class uiStatusBar;
mFDQtclass(QWidget)
class BufferStringSet;
class FileMultiString;
class uiString;


mExpClass(uiBase) uiMsg
{
    mTextTranslationClass(uiMsg);
    friend class uiMain;
    mGlobal(uiBase) friend uiMsg& uiMSG();

public:

    // Messages
    void	message(const char*,const char* part2,const char* part3=0);
    void	message(const uiString&);
    void	warning(const char*,const char* part2,const char* part3=0);
    void	warning(const uiString&);
    void	error(const char*,const char* part2,const char* part3=0);
    void	error(const uiString&);
    void	errorWithDetails(const FileMultiString&);
    		/*!<If input has multiple parts, the first will be displayed
		    directly, while the complete message is available under a
		    'Details ...' button, separated by new lines. */
    void	errorWithDetails(const BufferStringSet&,const char* firstmsg=0);

    // Interaction
    int		question(const uiString&,const uiString& textyes=0,
			 const uiString& textno=0,const uiString& textcncl=0,
			 const uiString& caption=0);
    int		askSave(const uiString&,bool cancelbutt=true);
    		//!<\retval 0=Don't save 1=Save -1=Cancel
    int		notSaved( const uiString& txt, bool cancelbutt=true )
		{ return askSave(txt,cancelbutt); } // Remove in v4.0
    int		askRemove(const uiString&,bool cancelbutt=false);
    		//!<\retval 0=Don't remove 1=Remove -1=Cancel
    int		askContinue(const uiString&);
    		//!<\retval 0=Abort 1=Continue
    int		askOverwrite(const uiString&);

    bool	askGoOn(const uiString&,bool withyesno=true);
    		//!< withyesno false: 'Ok' and 'Cancel', true: 'Yes' and 'No'
    bool	askGoOn(const uiString& msg,const uiString& textyes,
			const uiString& textno);
    int		askGoOnAfter(const uiString&,const uiString& cnclmsg=0,
			     const uiString& textyes=0,
			     const uiString& textno=0);
    		//!< 0=yes, 1=no, 2=cancel
    bool	showMsgNextTime(const uiString&,const uiString& msg=0);
    		//!< The msg must be negative, like "Don't show msg again"
    		//!< Be sure to store the ret val in the user settings

    static void setNextCaption(const uiString&);
    		//!< Sets the caption for the next call to any of the msg fns
    		//!< After that, caption will be reset to default

    uiMainWin*	setMainWin(uiMainWin*);	//!< return old

    bool	toStatusbar(const char*,int fld=0,int msec=-1);
    		//!< returns false if there is none
    uiStatusBar* statusBar();

    void	about(const uiString&);

protected:

			uiMsg();

    mQtclass(QWidget*)	popParnt();

    static uiMsg*	theinst_;

private:

    int			beginCmdRecEvent( const char* wintitle );
    void		endCmdRecEvent(int refnr,int retval,const char* buttxt0,
				const char* buttxt1=0,const char* buttxt2=0); 

    uiMainWin*		uimainwin_;
};

mGlobal(uiBase) uiMsg& uiMSG();


//!Sets the uiMSG's main window temporary during the scope of the object
mExpClass(uiBase) uiMsgMainWinSetter
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

