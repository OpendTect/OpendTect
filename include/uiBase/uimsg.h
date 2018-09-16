#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          26/04/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiusershowwait.h"
#include "typeset.h"
#include "callback.h"
#include "enums.h"
class uiMainWin;
class BufferStringSet;
class FileMultiString;
class MouseCursorChanger;
mFDQtclass(QWidget)


#define uiMSG() gUiMsg( this )

class uiMsg;
mGlobal(uiBase) uiMsg& gUiMsg(const uiParent* p=0);

/*!\brief pops up messages that must be clicked away by user.
  Usually, you use the global uiMsg instance through uiMSG. */


mExpClass(uiBase) uiMsg	: public CallBacker
{ mODTextTranslationClass(uiMsg)
public:

    // Messages / info
    bool	message(const uiString&,
			const uiString& part2=uiString::empty(),
			const uiString& part3=uiString::empty(),
			bool withdontshowagain=false);
		/*!<If withdontshowgain is true, the user will be prompted
		    to not see this again. Return true if the user
		    does not want to see it again. */
    void	info(const uiStringSet&);
    void	about(const uiString&);
    void	aboutOpendTect(const uiString&);

    // Warning
    bool	warning(const uiString&,
			const uiString& part2=uiString::empty(),
			const uiString& part3=uiString::empty(),
			bool withdontshowagain=false);
		/*!<If withdontshowgain is true, the user will be prompted
		    to not see this again. Return true if the user
		    does not want to see it again. */
    void	warning(const uiRetVal&);
    void	handleWarnings(const uiRetVal&); //!< does nothing if isOK()

    // Error
    bool	error(const uiString&,
		      const uiString& part2=uiString::empty(),
		      const uiString& part3=uiString::empty(),
		      bool withdontshowagain=false);
		/*!<If withdontshowagain is true, the user will be prompted
		    to not see this again. Return true if the user
		    does not want to see it again. */
    void	error(const uiRetVal&);
    void	handleErrors(const uiRetVal&); //!< does nothing if isOK()
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
			 const uiString& textyes=uiString::empty(),
			 const uiString& textno=uiString::empty(),
			 const uiString& textcncl=uiString::empty(),
			 const uiString& caption=uiString::empty(),
			 bool* dontaskagain=0);
		/*!<If dontaskagain is given, the user will have the
		   option to not see this again, and the boolean will
		   be filled in. */
    int		askSave(const uiString&,bool cancelbut=true);
		//!<\retval 0=Don't save 1=Save -1=Cancel
    int		askRemove(const uiString&,bool cancelbut=false);
		//!<\retval 0=Don't remove 1=Remove -1=Cancel
    int		ask2D3D(const uiString&,bool cancelbut=false);
		//!<\retval 0=3D 1=2D -1=Cancel
    bool	askContinue(const uiString&);
		//!<\retval true: yes, go ahead, false: oh no, don't want that!
    bool	askOverwrite(const uiString&);
		//!<\retval true: yes, go ahead, false: oh no, don't want that!

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
			     const uiString& cnclmsg=uiString::empty(),
			     const uiString& textyes=uiString::empty(),
			     const uiString& textno=uiString::empty(),
			     bool* dontaskagain=0);

    static void setNextCaption(const uiString&);
		//!< Sets the caption for the next call to any of the msg fns
		//!< After that, caption will be reset to default

    uiMainWin*	setMainWin(uiMainWin*);	//!< return old

    enum Icon	{ NoIcon, Information, Warning, Critical, Question };
    int		showMessageBox(Icon icon,QWidget* parent,
			   const uiString& txt,const uiString& yestxtinp,
			   const uiString& notxtinp,const uiString& cncltxtinp,
			   const uiString& title=uiString::empty(),
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

    Threads::Lock	lock_;
    Threads::Lock	msgdisplock_;

private:

    int			beginCmdRecEvent( const char* wintitle );
    void		endCmdRecEvent(int refnr,int retval,const char* buttxt0,
				const char* buttxt1=0,const char* buttxt2=0);

    uiMainWin*		uimainwin_;


    mGlobal(uiBase) friend uiMsg&	gUiMsg(const uiParent*);
    friend class			uiMain;

};
