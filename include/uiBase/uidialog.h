#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uidialog.h,v 1.21 2002-01-07 13:53:19 arend Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "bufstring.h"
class uiGroup;

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
			Setup( const char* window_title,
			       const char* dialog_title,
			       const char* help_id )
			: wintitle_(window_title), dlgtitle_(dialog_title)
			, helpid_(help_id), savetext_("Save defaults")
			, oktext_("Ok"), canceltext_("Cancel")
			, modal_(true)
			, savebutton_(false), separator_(true)
			, menubar_(false), toolbar_(false), statusbar_(false)
			, mainwidgcentered_( false )
			{}

	BufferString	wintitle_, dlgtitle_, helpid_;
	BufferString	savetext_, oktext_, canceltext_;
	bool		modal_, savebutton_, separator_;
	bool		menubar_, toolbar_, statusbar_, mainwidgcentered_;

	Setup&	savetext( const char* s )   { savetext_ = s;    return *this; }
	Setup&	oktext( const char* s )     { oktext_ = s;      return *this; }
	Setup&	canceltext( const char* s ) { canceltext_ = s;  return *this; }
	Setup&	modal( bool yn=true )       { modal_ = yn;      return *this; }
	Setup&	savebutton( bool yn=true )  { savebutton_ = yn; return *this; }
	Setup&	separator( bool yn=true )   { separator_ = yn;  return *this; }
	Setup&	menubar( bool yn=true )     { menubar_ = yn;    return *this; }
	Setup&	toolbar( bool yn=true )     { toolbar_ = yn;    return *this; }
	Setup&	statusbar( bool yn=true )   { statusbar_ = yn;  return *this; }
	Setup&	mainwidgcentered( bool yn=false ) 
				    { mainwidgcentered_ = yn; return *this; }

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

    void		setSeparator( bool yn );
			//!< Separator between central dialog and Ok/Cancel bar?
    bool		separator() const;
    uiGroup*		topGroup();

    Notifier<uiDialog>	finaliseStart;
    			//!< triggered when about to start finalising
    Notifier<uiDialog>	finaliseDone;
    			//!< triggered when finalising finished

protected:


    virtual bool        rejectOK(CallBacker*){ return true;}//!< confirm reject 
    virtual bool        acceptOK(CallBacker*){ return true;}//!< confirm accept 
    virtual bool        doneOK(int)	     { return true; } //!< confirm exit 
};

#endif
