#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uidialog.h,v 1.1 2000-11-27 10:19:27 bert Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "bufstring.h"

template <class T> class i_QObjWrapper;
class QDialog;
class QWidget;

mTemplTypeDefT( i_QObjWrapper, QDialog, i_QDialog )
mTemplTypeDefT( i_QObjWrapper, QWidget, i_QWidget )

class i_LayoutMngr;
class uiButton;
class uiCheckBox;

class uiDialog : public uiWrapObj<i_QDialog>
{ 	
public:
			uiDialog( uiObject* =0, const char* nm="uiDialog", 
				  bool modal=true,
				  int border=10, int spacing=10);
    virtual		~uiDialog();

    int			go(); 
    virtual void	reject( CallBacker* s) { if( rejectOK(s) ) done_(0); }
                        //!< to be called by a 'cancel' button
    virtual void	accept( CallBacker* s){ if( acceptOK(s) ) done_(1); }
                        //!< to be called by a 'ok' button
    virtual void	done( CallBacker* s ) { if( doneOK(s) ) done_(2); }

    virtual void	hide(CallBacker* =0);

    void		setResult( int v ) { reslt = v; }
    int			result(){ return reslt; }
   
    void		setSpacing( int ); 
    void		setBorder( int ); 

    void		enableSaveButton( const char* txt="Save defaults" )
			    { saveText = txt; }
    bool		saveButtonChecked();
    uiCheckBox*		saveButton() { return saveBut; }

protected:
    const QWidget*	qWidget_() const;

    virtual i_LayoutMngr* mLayoutMngr() 	
			{ return initing ? &dlgMngr : mLoMngr; } 
    virtual i_LayoutMngr* prntLayoutMngr() 	{ return 0; } 
			//!< a dialog is not managed by a parent's manager

    virtual const uiObject& clientWidget_() const;

    virtual bool	rejectOK(CallBacker*){ return true;}//!< confirm reject 
    virtual bool	acceptOK(CallBacker*){ return true;}//!< confirm accept 
    virtual bool	doneOK(CallBacker*){ return true; }   //!< confirm exit 
    void		done_(int);

    virtual void        preShow();

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

    uiButton*		okBut;
    uiButton*		cnclBut;
    uiCheckBox*		saveBut;

};



#endif
