#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uidialog.h,v 1.6 2001-05-28 10:52:33 arend Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "bufstring.h"

class IOPar;
class QDialog;
class QWidget;
class i_LayoutMngr;
class uiPushButton;
class uiCheckBox;
class uiSeparator;
class uiLabel;
template <class T> class i_QObjWrapper;
mTemplTypeDefT( i_QObjWrapper, QDialog, i_QDialog )
mTemplTypeDefT( i_QObjWrapper, QWidget, i_QWidget )


/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/


class uiDialog : public uiWrapObj<i_QDialog>
{ 	
public:
			uiDialog( uiParent* =0, const char* nm="uiDialog", 
				  bool modal=true, bool separator=true,
				  int border=0, int spacing=10);
    virtual		~uiDialog();

    int			go(); 
    virtual void	reject( CallBacker* s)	{ if( rejectOK(s) ) done_(0); }
                        //!< to be called by a 'cancel' button
    virtual void	accept( CallBacker* s)	{ if( acceptOK(s) ) done_(1); }
                        //!< to be called by a 'ok' button
    virtual void	done( CallBacker* s )	{ if( doneOK(s) ) done_(2); }

    virtual void	hide(CallBacker* =0);

    void		setResult( int v ) { reslt = v; }
    int			result(){ return reslt; }
   
    void		setSpacing( int ); 
    void		setBorder( int ); 

    void		setOkText( const char* txt );
			//!< OK button disabled when set to empty
    void		setCancelText( const char* txt );
			//!< cancel button disabled when set to empty
    void		enableSaveButton( const char* txt="Save defaults" )
			    { saveText = txt; }
    bool		saveButtonChecked();
    uiCheckBox*		saveButton()			{ return saveBut; }

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn )		{ separ = yn; }
    bool		separator()			{ return separ; }

protected:

    const QWidget*	qWidget_() const;

    virtual i_LayoutMngr* mLayoutMngr() 	
			{ return initing ? &dlgMngr : mLoMngr; } 
    virtual i_LayoutMngr* prntLayoutMngr() 	{ return 0; } 
			//!< a dialog is not managed by a parent's manager

    virtual const uiParent& clientWidget_() const;

    virtual bool	rejectOK(CallBacker*){ return true;}//!< confirm reject 
    virtual bool	acceptOK(CallBacker*){ return true;}//!< confirm accept 
    virtual bool	doneOK(CallBacker*){ return true; }   //!< confirm exit 
    void		done_(int);

    virtual void	finalise_();

    // don't change the order of these 5 attributes!
    bool		initing;
    bool		childrenInited;
    i_LayoutMngr*	mLoMngr;
    i_QDialog*          mqth__;
    i_LayoutMngr& 	dlgMngr;

    int 		reslt;

    uiGroup*            mCentralWidget;
    BufferString	okText;
    BufferString	cnclText;
    BufferString	saveText;
    bool		separ;

    uiPushButton*	okBut;
    uiPushButton*	cnclBut;
    uiCheckBox*		saveBut;
    uiSeparator*	horSepar;
    uiLabel*		title;

};

#endif
