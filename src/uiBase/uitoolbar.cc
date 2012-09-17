/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitoolbar.cc,v 1.62 2011/04/21 13:09:13 cvsbert Exp $";

#include "uitoolbar.h"

#include "uiaction.h"
#include "uitoolbutton.h"
#include "uimainwin.h"
#include "uiparentbody.h"

#include "bufstringset.h"
#include "menuhandler.h"
#include "pixmap.h"
#include "separstr.h"

#include <QToolBar>
#include "i_qtoolbut.h"
#include "i_qtoolbar.h"

const char* uiIcon::save()		{ return "save.png"; }
const char* uiIcon::saveAs()		{ return "saveas.png"; }
const char* uiIcon::openObject()	{ return "openstorage.png"; }
const char* uiIcon::newObject()		{ return "newstorage.png"; }
const char* uiIcon::removeObject()	{ return "trashcan.png"; }
const char* uiIcon::None()		{ return "-"; }


class uiToolBarBody : public uiParentBody
{
public:

			uiToolBarBody(uiToolBar&,QToolBar&);
			~uiToolBarBody();

    int 		addButton(const uiToolButtonSetup&);
    int 		addButton(const char*,const char*,const CallBack&,bool);
    int			addButton(const MenuItem&);
    int			getButtonID(QAction*); // QAction from MenuItem

    void		addObject(uiObject*);
    void		clear();
    void		turnOn(int idx, bool yn );
    bool		isOn(int idx ) const;
    void		setSensitive(int idx, bool yn );
    void		setSensitive(bool yn);
    bool		isSensitive() const;
    void		setToolTip(int,const char*);
    void		setShortcut(int,const char*);
    void		setPixmap(int,const char*);
    void		setPixmap(int,const ioPixmap&);
    void		setButtonMenu(int,uiPopupMenu*);

    void		display(bool yn=true);
			//!< you must call this after all buttons are added

    void		reLoadPixMaps();

    const ObjectSet<uiObject>&	objectList() const	{ return objects_; }

protected:

    virtual const QWidget*      managewidg_() const	{ return qbar_; }
    virtual const QWidget*	qwidget_() const	{ return qbar_; }
    QToolBar*			qbar_;
    uiToolBar&			tbar_;
    int				iconsz_;

    virtual void		attachChild( constraintType tp,
					     uiObject* child,
					     uiObject* other, int margin,
					     bool reciprocal )
				{
				    pErrMsg(
				      "Cannot attach uiObjects in a uiToolBar");
				    return;
				}

private:
    BufferStringSet		pmsrcs_;
    
    ObjectSet<uiObject>		objects_; 
    TypeSet<int>		butindex_;

    // MenuItems
    ObjectSet<QAction>		qactions_;
    TypeSet<int>		mnuids_;

};


uiToolBarBody::uiToolBarBody( uiToolBar& handle, QToolBar& bar )
    : uiParentBody("ToolBar")
    , qbar_(&bar)
    , tbar_(handle)
{
}


uiToolBarBody::~uiToolBarBody()	
{ clear(); }


int uiToolBarBody::addButton( const char* fnm, const char* tt,
			      const CallBack& cb, bool toggle )
{
    uiToolButtonSetup su( fnm, tt, cb );
    su.istoggle( toggle );
    return addButton( su );
}


int uiToolBarBody::addButton( const uiToolButtonSetup& su )
{
    uiToolButton* toolbarbut = new uiToolButton( &tbar_, su );
    butindex_ += objects_.size();
    addObject( toolbarbut );
    pmsrcs_.add( su.filename_ );

    const int butid = butindex_.size()-1;
    toolbarbut->setID( butid );
    return butid;
}


int uiToolBarBody::addButton( const MenuItem& itm )
{
    uiAction* action = new uiAction( itm );
    qbar_->addAction( action->qaction() );
    qactions_ += action->qaction();
    mnuids_ += itm.id;
    return itm.id >=0 ? itm.id : qbar_->actions().size()-1;
}


int uiToolBarBody::getButtonID( QAction* qaction )
{
    const int idx = qactions_.indexOf( qaction );
    return mnuids_.validIdx(idx) ? mnuids_[idx] : -1;
}


void uiToolBarBody::addObject( uiObject* obj )
{
    QWidget* qw = obj && obj->body() ? obj->body()->qwidget() : 0;
    if ( qw )
    {
	qbar_->addWidget( qw );
	mDynamicCastGet(uiToolButton*,button,obj)
	if ( !button ) qw->setMaximumHeight( uiObject::iconSize() );
	objects_ += obj;
    }
}


void uiToolBarBody::clear()
{
    qbar_->clear();

    for ( int idx=0; idx<butindex_.size(); idx++ )
	delete objects_[butindex_[idx]];

    objects_.erase();
    butindex_.erase();
    qactions_.erase();
    mnuids_.erase();
    pmsrcs_.erase();
}

#define mToolBarBut(idx) dynamic_cast<uiToolButton*>(objects_[butindex_[idx]])
#define mConstToolBarBut(idx) \
	dynamic_cast<const uiToolButton*>(objects_[butindex_[idx]])

void uiToolBarBody::turnOn( int idx, bool yn )
{ mToolBarBut(idx)->setOn( yn ); }


void uiToolBarBody::reLoadPixMaps()
{
    for ( int idx=0; idx<pmsrcs_.size(); idx++ )
    {
	const BufferString& pmsrc = pmsrcs_.get( idx );
	if ( pmsrc[0] == '[' )
	    continue;

	FileMultiString fms( pmsrc );
	const int len = fms.size();
	const BufferString fnm( fms[0] );
	const ioPixmap pm( fnm.buf(), len > 1 ? fms[1] : 0 );
	if ( pm.isEmpty() )
	    continue;

	mToolBarBut(idx)->setPixmap( pm );
    }
}


bool uiToolBarBody::isOn( int idx ) const
{ return mConstToolBarBut(idx)->isOn(); }

void uiToolBarBody::setSensitive( int idx, bool yn )
{ mToolBarBut(idx)->setSensitive( yn ); }

void uiToolBarBody::setPixmap( int idx, const char* fnm )
{ setPixmap( idx, ioPixmap(fnm) ); }

void uiToolBarBody::setPixmap( int idx, const ioPixmap& pm )
{ mToolBarBut(idx)->setPixmap( pm ); }

void uiToolBarBody::setToolTip( int idx, const char* txt )
{ mToolBarBut(idx)->setToolTip( txt ); }

void uiToolBarBody::setShortcut( int idx, const char* sc )
{ mToolBarBut(idx)->setShortcut( sc ); }

void uiToolBarBody::setSensitive( bool yn )
{ if ( qwidget() ) qwidget()->setEnabled( yn ); }

bool uiToolBarBody::isSensitive() const
{ return qwidget() ? qwidget()->isEnabled() : false; }

void uiToolBarBody::setButtonMenu( int idx, uiPopupMenu* mnu )
{ mToolBarBut(idx)->setMenu( mnu ); }


ObjectSet<uiToolBar>& uiToolBar::toolBars()
{
    static ObjectSet<uiToolBar>* ret = 0;
    if ( !ret )
    ret = new ObjectSet<uiToolBar>;
    return *ret;
}



uiToolBar::uiToolBar( uiParent* parnt, const char* nm, ToolBarArea tba,
		      bool newline )
    : uiParent(nm,0)
    , parent_(parnt)
    , tbarea_(tba)
    , buttonClicked(this)
{
    qtoolbar_ = new QToolBar( QString(nm) );
    qtoolbar_->setObjectName( nm );
    setBody( &mkbody(nm,*qtoolbar_) );
    msgr_ = new i_ToolBarMessenger( qtoolbar_, this );

    mDynamicCastGet(uiMainWin*,uimw,parnt)
    if ( uimw )
    {
	if ( newline ) uimw->addToolBarBreak();
	uimw->addToolBar( this );
    }

    toolBars() += this;
}


uiToolBar::~uiToolBar()
{
    mDynamicCastGet(uiMainWin*,uimw,parent_)
    if ( uimw ) uimw->removeToolBar( this );

    delete body_;
    delete qtoolbar_;
    delete msgr_;

    toolBars() -= this;
}


uiToolBarBody& uiToolBar::mkbody( const char* nm, QToolBar& qtb )
{ 
    body_ = new uiToolBarBody( *this, qtb );
    return *body_; 
}


int uiToolBar::addButton( const char* fnm, const char* tt, const CallBack& cb,
			  bool toggle )
{ return body_->addButton( fnm, tt, cb, toggle ); }


int uiToolBar::addButton( const uiToolButtonSetup& su )
{ return body_->addButton( su ); }


int uiToolBar::addButton( const MenuItem& itm )
{ return body_->addButton( itm ); }


void uiToolBar::addButton( uiToolButton* tb )
{ body_->addObject( tb ); };


void uiToolBar::addObject( uiObject* obj )
{ body_->addObject( obj ); };


void uiToolBar::setLabel( const char* lbl )
{
    qtoolbar_->setWindowTitle( QString(lbl) );
    setName( lbl );
}


void uiToolBar::turnOn( int idx, bool yn )
{ body_->turnOn( idx, yn ); }

bool uiToolBar::isOn( int idx ) const
{ return body_->isOn( idx ); }

void uiToolBar::setSensitive( int idx, bool yn )
{ body_->setSensitive( idx, yn ); }

void uiToolBar::setSensitive( bool yn )
{ body_->setSensitive( yn ); }

bool uiToolBar::isSensitive() const
{ return body_->isSensitive(); }

void uiToolBar::setToolTip( int idx, const char* tip )
{ body_->setToolTip( idx, tip ); }

void uiToolBar::setShortcut( int idx, const char* sc )
{ body_->setShortcut( idx, sc ); }

void uiToolBar::setPixmap( int idx, const char* fnm )
{ body_->setPixmap( idx, fnm ); }

void uiToolBar::setPixmap( int idx, const ioPixmap& pm )
{ body_->setPixmap( idx, pm ); }

void uiToolBar::setButtonMenu( int idx, uiPopupMenu* mnu )
{ body_->setButtonMenu( idx, mnu ); }


void uiToolBar::display( bool yn, bool, bool )
{
    if ( yn )
	qtoolbar_->show();
    else
	qtoolbar_->hide();

    mDynamicCastGet(uiMainWin*,uimw,parent_)
    if ( uimw ) uimw->updateToolbarsMenu();
}


bool uiToolBar::isHidden() const
{ return qtoolbar_->isHidden(); }

bool uiToolBar::isVisible() const
{ return qtoolbar_->isVisible(); }

void uiToolBar::addSeparator()
{ qtoolbar_->addSeparator(); }

void uiToolBar::reLoadPixMaps()
{ body_->reLoadPixMaps(); }

void uiToolBar::clear()
{ body_->clear(); }

const ObjectSet<uiObject>& uiToolBar::objectList() const
{ return body_->objectList(); }

int uiToolBar::getButtonID( QAction* qaction )
{ return body_->getButtonID( qaction ); }

uiMainWin* uiToolBar::mainwin()
{ 
    mDynamicCastGet(uiMainWin*,uimw,parent_)
    return uimw;
}
