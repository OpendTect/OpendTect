#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uidialog.h,v 1.19 2002-01-04 16:16:49 bert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "bufstring.h"
class uiGroup;

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/
class uiDialog : public uiMainWin
{ 	
// impl: uimainwin.cc
friend class uiDialogBody;

public:

    class Setup
    {
    public:
			Setup( const char* window_title,
			       const char* dialog_title,
			       const char* help_id )
			: wintitle(window_title)
			, dlgtitle(dialog_title)
			, helpid(help_id)
			, oktext("Ok")
			, canceltext("Cancel")
			, modal(true)
			, savebutton(false)
			, separator(true)
			, menubar(false)
			, toolbar(false)
			, statusbar(false)		{}

	BufferString	wintitle;
	BufferString	dlgtitle;
	BufferString	helpid;
	BufferString	oktext;
	BufferString	canceltext;
	BufferString	savetext;
	bool		modal;
	bool		savebutton;
	bool		separator;
	bool		menubar;
	bool		toolbar;
	bool		statusbar;
    };

			uiDialog(uiParent*,const Setup&);
    			//TODO remove the old crappy constructor
			uiDialog( uiParent* p =0, const char* nm="uiDialog", 
				  bool modal=true, bool separator=true,
				  bool wantMBar=false, bool wantSBar=false,
				  bool wantTBar=false, const char* helpid=0 );

    int			go(); 

    void		reject( CallBacker* cb =0);
    void		accept( CallBacker* cb =0);
    void		done(int ret=0);
    			//!< 0=Cancel, 1=OK, other=user defined

    void		setSpacing( int ); 
    void		setBorder( int ); 

    void		setCaption( const char* txt );

			//! OK button disabled when set to empty
    void		setOkText( const char* txt );
			//! cancel button disabled when set to empty
    void		setCancelText( const char* txt );
			//! Save button enabled when set to non-empty
    void		enableSaveButton( const char* txt="Save defaults" );
			//! title text. Default equal to name
    void		setTitleText( const char* txt );
    bool		saveButtonChecked();

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn );
    bool		separator();
    uiGroup*		topGroup();

    Notifier<uiDialog>	finaliseStart; //! triggered when about to start finalising
    Notifier<uiDialog>	finaliseDone;  //! triggered when finalising finished

protected:


    virtual bool        rejectOK(CallBacker*){ return true;}//!< confirm reject 
    virtual bool        acceptOK(CallBacker*){ return true;}//!< confirm accept 
    virtual bool        doneOK(int)	     { return true; } //!< confirm exit 
};

#endif
