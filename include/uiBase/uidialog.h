#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uidialog.h,v 1.7 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uiparent.h"
#include "bufstring.h"

class uiDialogBody;

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/
class uiDialog : public uiParent
{ 	
friend class uiDialogBody;

public:
			uiDialog( uiParent* p =0, const char* nm="uiDialog", 
				  bool modal=true, bool separator=true,
				  int border=0, int spacing=10);


    int			go(); 

    void		reject( CallBacker* cb =0);
    void		accept( CallBacker* cb =0);
    void		done( CallBacker* cb =0);

    void		setSpacing( int ); 
    void		setBorder( int ); 

			//! OK button disabled when set to empty
    void		setOkText( const char* txt );
			//! cancel button disabled when set to empty
    void		setCancelText( const char* txt );
			//! Save button enabled when set to non-empty
    void		enableSaveButton( const char* txt="Save defaults" );
    bool		saveButtonChecked();

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn );
    bool		separator();

    Notifier<uiDialog>	finalising;

protected:


    virtual bool        rejectOK(CallBacker*){ return true;}//!< confirm reject 
    virtual bool        acceptOK(CallBacker*){ return true;}//!< confirm accept 
    virtual bool        doneOK(CallBacker*){ return true; } //!< confirm exit 

private:

    uiDialogBody*	body_;
    uiDialogBody&	mkbody(uiParent*, const char*, bool, bool, int, int);
};

#endif
