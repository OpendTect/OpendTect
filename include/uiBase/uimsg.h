#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          26/04/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "gendefs.h"
#include "uistring.h"
#include "uiparent.h"
#include "typeset.h"
class uiMainWin;
class uiStatusBar;
mFDQtclass(QWidget)
class BufferStringSet;
class FileMultiString;
class uiString;


mExpClass(uiBase) uiMsg	: public CallBacker
{ mODTextTranslationClass(uiMsg)

friend class uiMain;
mGlobal(uiBase) friend uiMsg& uiMSG();

public:

    // Messages
    bool	message(const uiString&,
			const uiString& part2=uiString::emptyString(),
			const uiString& part3=uiString::emptyString(),
			bool withdontshowatain=false);
		/*!<If withdontshowgain is true, the user will be prompted
		    to not see this again. Return true if the user
		    does not want to see it again. */
    bool	warning(const uiString&,
			const uiString& part2=uiString::emptyString(),
			const uiString& part3=uiString::emptyString(),
			bool withdontshowatain=false);
		/*!<If withdontshowgain is true, the user will be prompted
		    to not see this again. Return true if the user
		    does not want to see it again. */
    void	warning(const uiRetVal&);
    bool	error(const uiString&,
		      const uiString& part2=uiString::emptyString(),
		      const uiString& part3=uiString::emptyString(),
		      bool withdontshowgain=false);
		/*!<If withdontshowgain is true, the user will be prompted
		    to not see this again. Return true if the user
		    does not want to see it again. */
    void	error(const uiRetVal&);
    void	errorWithDetails(const FileMultiString&);
		/*!<If input has multiple parts, the first will be displayed
		    directly, while the complete message is available under a
		    'Details ...' button, separated by new lines. */
    void	errorWithDetails(const uiStringSet&,
				 const uiString& firstmsg);
    void	errorWithDetails(const uiStringSet&);
    void	errorWithDetails(const BufferStringSet&);

    // Interaction
    int		question(const uiString&,
			 const uiString& textyes=uiString::emptyString(),
			 const uiString& textno=uiString::emptyString(),
			 const uiString& textcncl=uiString::emptyString(),
			 const uiString& caption=uiString::emptyString(),
			 bool* dontaskagain=0);
		/*!<If dontaskagain is given, the user will have the
		   option to not see this again, and the boolean will
		   be filled in. */
    int		askSave(const uiString&,bool cancelbut=true);
		//!<\retval 0=Don't save 1=Save -1=Cancel
    int		askRemove(const uiString&,bool cancelbut=false);
		//!<\retval 0=Don't remove 1=Remove -1=Cancel
    int		askContinue(const uiString&);
		//!<\retval 0=Abort 1=Continue
    int		askOverwrite(const uiString&);
		//!<\retval 0=Abort 1=Overwrite
    int		ask2D3D(const uiString&,bool cancelbut=false);
		//!<\retval 0=3D 1=2D -1=Cancel

    bool	askGoOn(const uiString&,bool withyesno=true,
			bool* dontaskagain=0);
		/*!<withyesno false: 'OK' and 'Cancel', true: 'Yes' and 'No'
		   If dontaskagain is given, the user will have the
		   option to not see this again, and the boolean will
		   be filled in. */
    bool	askGoOn(const uiString& msg,const uiString& textyes,
			const uiString& textno,
			bool* dontaskagain=0);
		/*!<If don't askagain is given, the user will have the
		   option to not see this again, and the boolean will
		   be filled in. */
    int		askGoOnAfter(const uiString&,
			     const uiString& cnclmsg=uiString::emptyString(),
			     const uiString& textyes=uiString::emptyString(),
			     const uiString& textno=uiString::emptyString(),
			     bool* dontaskagain=0);

    static void setNextCaption(const uiString&);
		//!< Sets the caption for the next call to any of the msg fns
		//!< After that, caption will be reset to default

    uiMainWin*	setMainWin(uiMainWin*);	//!< return old

    bool	toStatusbar(uiString,int fld=0,int msec=-1);
		//!< returns false if there is none
    uiStatusBar* statusBar();

    void	about(const uiString&);
    void	aboutOpendTect(const uiString&);

    enum Icon	{ NoIcon, Information, Warning, Critical, Question };
    int		showMessageBox(Icon icon,QWidget* parent,
			   const uiString& txt,const uiString& yestxtinp,
			   const uiString& notxtinp,const uiString& cncltxtinp,
			   const uiString& title=uiString::emptyString(),
			   bool* notagain=0);
		/*!<If don't askagain is given, the user will have the
		   option to not see this again, and the boolean will
		   be filled in. */

    static uiString	sDontShowAgain();
    enum msgType	{ WarningMsg, ErrorWithDetails, ErrorMsg, Info };
			    mDeclareEnumUtils(msgType);
    void		showMsg(uiMainWin*, msgType, const uiStringSet&);
    void		dispErrMsgCB(CallBacker*);
    void		dispWarnMsgCB(CallBacker*);
    void		errorWithDetailProc(uiStringSet&);
protected:

			uiMsg();

    mQtclass(QWidget*)	popParnt();

    static uiMsg*	theinst_;
    Threads::Lock	lock_;
    Threads::Lock	msgdisplock_;

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


