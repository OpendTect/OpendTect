/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.cc,v 1.28 2002-01-07 13:17:01 arend Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "uidialog.h"

#include "msgh.h"
#include "errh.h"
#include "helpview.h"
#include "uimsg.h"

#include "uibody.h"
#include "uiobjbody.h"
#include "uiparentbody.h"

#include "uigroup.h"
#include "uibutton.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uiseparator.h"
#include "uimenu.h"
#include "uilabel.h"


#include <iostream>

#include <qmainwindow.h>
#include <qwidget.h>
#include <qstatusbar.h>
#include <qapplication.h>

#ifdef __msvc__
#include <qpopupmenu.h>
#endif


#ifdef __debug__
#include <qmenubar.h>
#endif

#include <qlayout.h>


class uiMainWinBody : public uiParentBody, public UserIDObject
		    , public QMainWindow
{
public:
			uiMainWinBody( uiMainWin& handle, uiParent* parnt,
				       const char* nm, bool modal );

    void		construct( bool wantStatusBar, bool wantMenuBar, 
				   bool wantToolBar );

    virtual		~uiMainWinBody();

#define mHANDLE_OBJ     uiMainWin
#define mQWIDGET_BASE   QMainWindow
#define mQWIDGET_BODY   QMainWindow
#define UIBASEBODY_ONLY
#include                "i_uiobjqtbody.h"

public:

    uiStatusBar* 	uistatusbar();
    uiMenuBar* 		uimenubar();
    uiToolBar*		uitoolbar();

    virtual void        polish()
                        {
			    MsgClass::theCB = mCB(&uiMSG(),uiMsg,handleMsg);
                            QMainWindow::polish();
                        }

    uiGroup*		uiCentralWidg()		{ return centralWidget_; }

    virtual void        manageChld_( uiObjHandle& o, uiObjectBody& b )
			{ 
			    if ( !initing && centralWidget_ ) 
				centralWidget_->manageChld( o, b );

			}

    virtual void  	attachChild ( constraintType tp,
                                              uiObject* child,
                                              uiObject* other, int margin )
                        {
                            if ( !child || initing ) return;

			    centralWidget_->attachChild( tp, child, other,
							margin); 
                        }

    void		reDraw( bool deep )
			{
			    update();
			    centralWidget_->reDraw(deep);
			}

    virtual int		minTextWidgetHeight() const
			{ return centralWidget_->minTextWidgetHeight(); }

    void		go()			
			{ 
			    finalise(); 
			    show(); 
			}

    virtual void	show() 
			{
			    QMainWindow::show();
			    if( modal_ )	
				looplevel__ = qApp->enter_loop();
			    else 
				looplevel__ = -1;
			}


    virtual void	hide() 
			{
			int ll = qApp->loopLevel();
			    if( modal_ )	qApp->exit_loop();
			    QMainWindow::hide();
			}

protected:

    virtual void	finalise();

    bool		initing;

    uiGroup*		centralWidget_;

    uiStatusBar* 	statusbar;
    uiMenuBar* 		menubar;
    uiToolBar* 		toolbar;


protected:

    virtual const QWidget* managewidg_() const 
			{ 
			    if ( !initing ) 
				return centralWidget_->body()->managewidg();
			    return qwidget_();
			}

private:

    bool		modal_;
    int			looplevel__;
};



uiMainWinBody::uiMainWinBody( uiMainWin& handle__, uiParent* parnt, 
			      const char* nm, bool modal )
	: uiParentBody()
	, UserIDObject( nm )
	, QMainWindow( parnt && parnt->body() ?  parnt->body()->qwidget() : 0, 
		       nm, 
//		       modal ?  WType_TopLevel | WShowModal| WGroupLeader :
		       modal ?  WType_TopLevel | WShowModal :
				WType_TopLevel )
	, handle_( handle__ )
	, initing( true )
	, centralWidget_( 0 )
	, statusbar(0), menubar(0), toolbar(0)  
	, modal_( modal )
{
    if ( *nm ) setCaption( nm );
}

void uiMainWinBody::construct(  bool wantStatusBar, bool wantMenuBar, 
				bool wantToolBar )
{
    centralWidget_ = new uiGroup( &handle(), "uiMainWin central widget", 7 );
    setCentralWidget( centralWidget_->body()->qwidget() ); 

    centralWidget_->setIsMain(true);

    if( wantStatusBar )
    {
	QStatusBar* mbar= statusBar();
	if( mbar )
	    statusbar = new uiStatusBar( &handle(),
					  "MainWindow StatusBar handle", *mbar);
	else
	    pErrMsg("No statusbar returned from Qt");
    }
    if( wantMenuBar )
    {   
	QMenuBar* myBar =  menuBar();

	if( myBar )
	    menubar = new uiMenuBar( &handle(), "MainWindow MenuBar handle", 
				      *myBar);
	else
	    pErrMsg("No menubar returned from Qt");
    }
    if( wantToolBar )
    {
	toolbar = uiToolBar::getNew( *this );
    }

    initing = false;
}


uiMainWinBody::~uiMainWinBody( )
{
    delete centralWidget_;
}


void uiMainWinBody::finalise()
    { centralWidget_->finalise();  finaliseChildren(); }



uiStatusBar* uiMainWinBody::uistatusbar()
{
    return statusbar;
}


uiMenuBar* uiMainWinBody::uimenubar()
{
    if ( !menubar ) pErrMsg("No menuBar. See uiMainWinBody's constructor"); 
    return menubar;
}

uiToolBar* uiMainWinBody::uitoolbar()
{
    if ( !toolbar ) pErrMsg("No toolBar. See uiMainWinBody's constructor"); 
    return toolbar;
}

uiMainWin::uiMainWin( uiParent* parnt, const char* nm,
		      bool wantSBar, bool wantMBar, bool wantTBar, bool modal )
    : uiParent( nm, 0 )
    , body_( 0 )
{ 
    body_= new uiMainWinBody( *this, parnt, nm, modal ); 
    setBody( body_ );
    body_->construct(wantSBar,wantMBar,wantTBar);
//    body_->uiCentralWidg()->setBorder(10);
}

uiMainWin::uiMainWin( const char* nm )
    : uiParent( nm, 0 )
    , body_( 0 )			
{}

uiMainWin::~uiMainWin()
{ delete body_; }

uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }
uiToolBar* uiMainWin::toolBar()			{ return body_->uitoolbar(); }
void uiMainWin::show()				{ body_->go(); }
void uiMainWin::setCaption( const char* txt )	{ body_->setCaption(txt); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }

uiObject* uiMainWin::uiObj()
    { return body_->uiCentralWidg()->uiObj(); }

const uiObject* uiMainWin::uiObj() const
    { return body_->uiCentralWidg()->uiObj(); }

void uiMainWin::toStatusBar( const char* txt )
{
    if ( !txt ) txt = "";
    uiStatusBar* sb = statusBar();
    if ( sb )
	sb->message( txt );
    else
    	UsrMsg(txt);
}

uiMainWin* uiMainWin::activeWindow()
{
    QWidget* _aw = qApp->activeWindow();
    if( !_aw )		return 0;

    uiMainWinBody* _awb = dynamic_cast<uiMainWinBody*>(_aw);
    if( !_awb )		return 0;

    return &_awb->handle();

}

uiMainWin* uiMainWin::gtUiWinIfIsBdy(QWidget* mwimpl)
{
    if( !mwimpl )	return 0;

    uiMainWinBody* _mwb = dynamic_cast<uiMainWinBody*>( mwimpl );
    if( !_mwb )		return 0;

    return &_mwb->handle();
}

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/

#define mHandle static_cast<uiDialog&>(handle_)

class uiDialogBody : public uiMainWinBody
{ 	
public:
			uiDialogBody(uiDialog&,uiParent*,const char*, 
				      bool,bool,bool,const char*);
			uiDialogBody(uiDialog&,uiParent*,
				     const uiDialog::Setup&);

    int			exec(); 

    void		reject( CallBacker* s)	
			    { if( mHandle.rejectOK(s) ) done_(0); }
                        //!< to be called by a 'cancel' button
    void		accept( CallBacker* s)	
			    { if( mHandle.acceptOK(s) ) done_(1); }
                        //!< to be called by a 'ok' button
    void		done( int i )
			    { if( mHandle.doneOK(i) ) done_(i); }

    void		uiSetResult( int v ) { reslt = v; }
    int			uiResult(){ return reslt; }

    void		setOkText( const char* txt );
			//!< OK button disabled when set to empty
    void		setCancelText( const char* txt );
			//!< cancel button disabled when set to empty
    void		enableSaveButton( const char* txt )
			    { saveText = txt; withsavebut = true; }

    void		setTitleText( const char* txt );

    bool		saveButtonChecked();
    uiCheckBox*		saveButton()			{ return saveBut; }

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn )		{ separ = yn; }
    bool		separator() const		{ return separ; }

    void		setDlgGrp( uiGroup* cw )	{ dlgGroup=cw; }

    void		setSpacing( int spc )	{ dlgGroup->setSpacing(spc); }
    void		setBorder( int b )	{ dlgGroup->setBorder( b ); }

    virtual void        manageChld_( uiObjHandle& o, uiObjectBody& b )
			{ 
			    if ( !initing ) 
				dlgGroup->manageChld( o, b );
			}

    virtual void  	attachChild ( constraintType tp,
                                              uiObject* child,
                                              uiObject* other, int margin )
                        {
                            if ( !child || initing ) return;
			    dlgGroup->attachChild( tp, child, other, margin); 
                        }
    void		provideHelp(CallBacker*);

protected:

    virtual const QWidget* managewidg_() const 
			{ 
			    if ( !initing ) 
				return dlgGroup->body()->managewidg();
			    return uiMainWinBody::managewidg_();
			}

    int 		reslt;
    bool		childrenInited;


    uiGroup*            dlgGroup;
    BufferString	okText;
    BufferString	cnclText;
    BufferString	saveText;
    BufferString	titleText;
    BufferString	helpId;
    bool		separ;
    bool		withmenubar;
    bool		withsavebut;

    uiPushButton*	okBut;
    uiPushButton*	cnclBut;
    uiPushButton*	helpBut;
    uiCheckBox*		saveBut;
    uiSeparator*	horSepar;
    uiLabel*		title;

    void		done_(int);

    virtual void	finalise();

};


uiDialogBody::uiDialogBody( uiDialog& handle, uiParent* parnt, const char* nm, 
			    bool modal, bool separator, bool withmb, 
			    const char* hid )
    : uiMainWinBody(handle,parnt,nm,modal)
    , dlgGroup( 0 )
    , okText("Ok"), cnclText("Cancel"), saveText("Save defaults"), titleText("")
    , okBut( 0 ), cnclBut( 0 ), saveBut( 0 ), helpBut( 0 ), title( 0 )
    , reslt( 0 )
    , separ( separator ), horSepar( 0 )
    , childrenInited(false)
    , withmenubar(withmb)
    , withsavebut(false)
    , helpId(hid)
{
}

uiDialogBody::uiDialogBody( uiDialog& handle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : uiMainWinBody(handle,parnt,s.wintitle_,s.modal_)
    , dlgGroup( 0 )
    , okText(s.oktext_), cnclText(s.canceltext_), saveText(s.savetext_)
    , titleText(s.dlgtitle_)
    , okBut( 0 ), cnclBut( 0 ), saveBut( 0 ), helpBut( 0 ), title( 0 )
    , reslt( 0 )
    , separ( s.separator_ ), horSepar( 0 )
    , childrenInited(false)
    , withmenubar(s.menubar_)
    , withsavebut(s.savebutton_)
    , helpId(s.helpid_)
{
}

int uiDialogBody::exec()
{ 
    uiSetResult( 0 );

    go();

    return uiResult();
}



void uiDialogBody::setOkText( const char* txt )    
{ 
    okText = txt; 
    if( okBut ) okBut->setText(txt);
}


void uiDialogBody::setTitleText( const char* txt )    
{ 
    titleText = txt; 
    if( title ) 
    { 
	title->setText(txt); 
	title->setPrefWidthInChar( strlen(txt) + 2 ); 
    }
}

void uiDialogBody::setCancelText( const char* txt ) 
{ 
    cnclText = txt; 
    if( cnclBut ) cnclBut->setText(txt);
}


bool uiDialogBody::saveButtonChecked()
{ 
    return saveBut ? saveBut->isChecked() : false;
}


/*!
    Hides the box, which also exits the event loop in case of a modal box.
*/
void uiDialogBody::done_( int v )
{
    uiSetResult( v );
    hide();
}


/*!
    Construct OK and Cancel buttons just before the first show.
    This gives chance not to construct them in case OKtext and CancelText have
    been set to ""
*/
void uiDialogBody::finalise() 
{
    uiMainWinBody::finalise(); 

    mHandle.finaliseStart.trigger(mHandle);

    dlgGroup->finalise();

    if( !childrenInited ) 
    {
	uiObject* alignObj = dlgGroup->uiObj();

	int prefbutwdt = 10;


	if ( okText != "" )
	{
	    okBut = new uiPushButton( centralWidget_, okText );
	    prefbutwdt = mMAX( prefbutwdt, okText.size());
	}
	if ( cnclText != "" )
	{
	    cnclBut = new uiPushButton( centralWidget_, cnclText );
	    prefbutwdt = mMAX( prefbutwdt, cnclText.size());
	}
	if ( withsavebut && saveText != "" )
	    saveBut = new uiCheckBox( centralWidget_, saveText );
	if ( helpId != "" )
	    helpBut = new uiPushButton( centralWidget_, "?" );

        if ( !withmenubar )
	{
	    title = new uiLabel( centralWidget_, titleText );
	    title->setPrefWidthInChar( titleText.size() + 2 ); 

	    uiSeparator* sep = new uiSeparator( centralWidget_ );

	    title->attach( centeredAbove, sep );
	    sep->attach( stretchedBelow, title, -2 );
	    dlgGroup->attach( stretchedBelow, sep );
	}

	if ( separ && (okBut || cnclBut || saveBut || helpBut) )
	{
	    horSepar = new uiSeparator( centralWidget_ );
	    horSepar->attach( stretchedBelow, dlgGroup, -2 );
	    alignObj = horSepar;
	}

	if ( okBut )
	{
	    if ( !cnclBut )
		okBut->attach( centeredBelow, alignObj );
	    else
	    {
		okBut->attach( leftBorder );
		okBut->attach( ensureBelow, alignObj );
	    }
	    okBut->attach( bottomBorder, 0 );
	    okBut->activated.notify( mCB( this, uiDialogBody, accept ));
	    okBut->setDefault();

	    okBut->setPrefWidthInChar( prefbutwdt );
	}

	if ( cnclBut )
	{
	    if ( !okBut )
	    {
		cnclBut->attach( centeredBelow, alignObj );
		cnclBut->setDefault();
	    }
	    else
	    {
		cnclBut->attach( rightBorder );
		cnclBut->attach( ensureBelow, alignObj );
		cnclBut->attach( ensureRightOf, okBut );
	    }
	    cnclBut->attach( bottomBorder, 0 );

	    cnclBut->activated.notify( mCB( this, uiDialogBody, reject ));

	    cnclBut->setPrefWidthInChar( prefbutwdt );
	}

	if( saveBut )
	{
	    if( okBut )
		saveBut->attach(rightOf, okBut);
	    if( cnclBut )
		cnclBut->attach(ensureRightOf, saveBut);

	    saveBut->attach( bottomBorder, 0 );
	}

	if ( helpBut )
	{
	    if ( saveBut )
		helpBut->attach(rightOf, saveBut);
	    else if ( (!cnclBut && !okBut) || (cnclBut && okBut) )
	    {
		helpBut->attach( centeredBelow,
				 horSepar ? horSepar : centralWidget_->uiObj());
		if ( cnclBut ) helpBut->attach( ensureLeftOf, cnclBut );
		if ( okBut ) helpBut->attach( ensureRightOf, okBut );
	    }
	    else if ( cnclBut )
		helpBut->attach( leftOf, cnclBut );
	    else if ( okBut )
		helpBut->attach( rightOf, okBut );


	    helpBut->attach( bottomBorder, 0 );
	    helpBut->activated.notify( mCB( this, uiDialogBody, provideHelp ));
	}

	childrenInited = true;
    }

    finaliseChildren();

    mHandle.finaliseDone.trigger(mHandle);
}


void uiDialogBody::provideHelp( CallBacker* )
{
    HelpViewer::use( HelpViewer::getURLForWinID(helpId) );
}

#define mBody static_cast<uiDialogBody*>(body_)

uiDialog::uiDialog( uiParent* parnt, const char* nm, bool modal, bool sep,
		    bool wantMBar, bool wantSBar, bool wantTBar,
		    const char* hid)
	: uiMainWin( nm )
	, finaliseStart( this )
	, finaliseDone( this )
{
    body_= new uiDialogBody( *this, parnt, nm, modal, sep,
	    			wantMBar, hid );
    setBody( body_ );
    body_->construct( wantSBar, wantMBar, wantTBar );

    uiGroup* cw= new uiGroup( body_->uiCentralWidg(), "Dialog box client area");

    cw->setStretch( 1, 1 );
    mBody->setDlgGrp( cw );

    setTitleText( nm );
}


uiDialog::uiDialog( uiParent* p, const uiDialog::Setup& s )
	: uiMainWin(s.wintitle_)
    	, finaliseStart(this)
    	, finaliseDone(this)
{
    body_= new uiDialogBody( *this, p, s );
    setBody( body_ );
    body_->construct( s.statusbar_, s.menubar_, s.toolbar_ );
    uiGroup* cw= new uiGroup( body_->uiCentralWidg(), "Dialog box client area");
    cw->setStretch( 1, 1 );
    mBody->setDlgGrp( cw );
    setTitleText( s.dlgtitle_ );
}


int uiDialog::go()				{ return mBody->exec(); }
void uiDialog::reject( CallBacker* cb)		{ mBody->reject( cb ); }
void uiDialog::accept( CallBacker*cb)		{ mBody->accept( cb ); }
void uiDialog::done( int i )			{ mBody->done( i ); }
void uiDialog::setSpacing( int s )		{ mBody->setSpacing(s); }
void uiDialog::setBorder( int b )		{ mBody->setBorder(b); }
void uiDialog::setCaption( const char* txt )	{ mBody->setCaption(txt); }
void uiDialog::setTitleText( const char* txt )	{ mBody->setTitleText(txt); }
void uiDialog::setOkText( const char* txt )	{ mBody->setOkText(txt); }
void uiDialog::setCancelText( const char* txt )	{ mBody->setCancelText(txt);}
void uiDialog::enableSaveButton(const char* t)  { mBody->enableSaveButton(t); }
bool uiDialog::saveButtonChecked()	{ return mBody->saveButtonChecked(); }
void uiDialog::setSeparator( bool yn )		{ mBody->setSeparator(yn); }
bool uiDialog::separator() const		{ return mBody->separator(); }
uiGroup* uiDialog::topGroup()			{return mBody->uiCentralWidg();}
