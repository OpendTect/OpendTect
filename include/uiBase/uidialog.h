#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uidialog.h,v 1.31 2003-01-15 15:34:35 bert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "bufstring.h"

#include "errh.h"
class uiGroup;
class uiButton;

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

It is meant to be the base class for 'normal' dialog windows.

*/


class uiDialog : public uiMainWin
{ 	
    // impl: uimainwin.cc
    friend class	uiDialogBody;

public:

    /*!\brief description of properties of dialog.
     
	For convenience, the 'set' functions return a reference to the object.
	This allows constructions like:
	Setup P( "xx", "yy", "zz" ).oktext("").canceltext("Quit").modal(false);
	This is _very_ helpful when a uiDialog base class is constructed.
     */

    class Setup
    {
    public:

    enum                SaveButType { None, CheckBox, PushButton };

			Setup( const char* window_title,
			       const char* dialog_title =0,
			       const char* help_id =0 )
			: wintitle_(window_title)
			, dlgtitle_(dialog_title ? dialog_title : window_title)
			, helpid_(help_id), savetext_("Save defaults")
			, oktext_("Ok"), canceltext_("Cancel")
			, modal_(true)
			, savebutton_(None), separator_(true)
			, menubar_(false), toolbar_(false), nrstatusflds_(0)
			, mainwidgcentered_(false), savechecked_(false)
			, fixedsize_(false)
			{}

	BufferString	wintitle_, dlgtitle_, helpid_;
	BufferString	savetext_, oktext_, canceltext_;
	bool		modal_, separator_, savechecked_;
	bool		menubar_, toolbar_, mainwidgcentered_, fixedsize_;
	SaveButType	savebutton_;
	int		nrstatusflds_;

	Setup&	savetext( const char* s )   { savetext_ = s;    return *this; }
	Setup&	oktext( const char* s )     { oktext_ = s;      return *this; }
	Setup&	canceltext( const char* s ) { canceltext_ = s;  return *this; }
	Setup&	modal( bool yn=true )       { modal_ = yn;      return *this; }
	Setup&	savebutton( SaveButType tp=CheckBox )
					    { savebutton_ = tp; return *this; }
	Setup&	separator( bool yn=true )   { separator_ = yn;  return *this; }
	Setup&	menubar( bool yn=true )     { menubar_ = yn;    return *this; }
	Setup&	toolbar( bool yn=true )     { toolbar_ = yn;    return *this; }
	//! nrstatusflds == -1: Do make a statusbar, but don't add msg fields.
	Setup&	nrstatusflds( int nr=1 )    { nrstatusflds_= nr; return *this; }
	Setup&	savechecked( bool yn=true ) { savechecked_ = yn; return *this; }
	Setup&	mainwidgcentered( bool yn=false ) 
				    { mainwidgcentered_ = yn; return *this; }
	Setup&	fixedsize( bool yn=true )  { fixedsize_ = yn; return *this; }

    };

    enum                Button { OK, SAVE, CANCEL, HELP };

			uiDialog(uiParent*,const Setup&);

    int			go(); 

    void		reject( CallBacker* cb =0);
    void		accept( CallBacker* cb =0);
    void		done(int ret=0);
    			//!< 0=Cancel, 1=OK, other=user defined

    void		setHSpacing( int ); 
    void		setVSpacing( int ); 
    void		setBorder( int ); 

    void		setCaption( const char* txt );

    uiButton*		button( Button but );
    void		setButtonText( Button but, const char* txt )
			{
			    switch ( but )
			    {
			    case OK	: setOkText( txt ); break;
			    case CANCEL	: setCancelText( txt ); break;
			    case SAVE	: enableSaveButton( txt ); break;
			    case HELP	: pErrMsg("can't set txt on help but");
			    }
			}

			//! OK button disabled when set to empty
    void		setOkText( const char* txt );
			//! cancel button disabled when set to empty
    void		setCancelText( const char* txt );
			//! Save button enabled when set to non-empty
    void		enableSaveButton( const char* txt="Save defaults" );
			//! 0: cancel; 1: OK
    int			uiResult() const;
    			

    void		setButtonSensitive( Button, bool);
    void		setSaveButtonChecked(bool);
    void		setTitleText( const char* txt );
    bool		saveButtonChecked() const;

    void		setSeparator( bool yn );
			//!< Separator between central dialog and Ok/Cancel bar?
    bool		separator() const;
    void		setHelpID(const char*);
    const char*		helpID() const;

protected:


    virtual bool        rejectOK(CallBacker*){ return true;}//!< confirm reject 
    virtual bool        acceptOK(CallBacker*){ return true;}//!< confirm accept 
    virtual bool        doneOK(int)	     { return true; } //!< confirm exit 
};

#endif
