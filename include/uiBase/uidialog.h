#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uidialog.h,v 1.14 2001-11-20 12:17:19 arend Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "bufstring.h"

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/
class uiDialog : public uiMainWin
{ 	
// impl: uimainwin.cc
friend class uiDialogBody;

public:
			uiDialog( uiParent* p =0, const char* nm="uiDialog", 
				  bool modal=true, bool separator=true,
				  bool wantMBar=false, bool wantSBar=false );
//				  int border=7, int spacing=10);


    int			go(); 

    void		reject( CallBacker* cb =0);
    void		accept( CallBacker* cb =0);
    void		done( CallBacker* cb =0);

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

    Notifier<uiDialog>	finaliseStart; //! triggered when about to start finalising
    Notifier<uiDialog>	finaliseDone;  //! triggered when finalising finished

protected:


    virtual bool        rejectOK(CallBacker*){ return true;}//!< confirm reject 
    virtual bool        acceptOK(CallBacker*){ return true;}//!< confirm accept 
    virtual bool        doneOK(CallBacker*){ return true; } //!< confirm exit 
};

#endif
