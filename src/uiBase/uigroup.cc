/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uiobjbody.h"
#include "i_layout.h"
#include "i_layoutitem.h"
#include "envvars.h"

#include <QFrame>
#include <QWidget>
#include <iostream>


class uiGroupObjBody;
class uiGroupParentBody;

mUseQtnamespace

class i_uiGroupLayoutItem : public i_uiLayoutItem
{
public:
			i_uiGroupLayoutItem( i_LayoutMngr& mgr,
					     uiGroupObjBody& obj,
					     uiGroupParentBody& par );

    virtual int		horAlign(LayoutMode) const;
    virtual int		center(LayoutMode,bool hor) const;

    virtual void	invalidate();
    virtual void	updatedAlignment(LayoutMode);
    virtual void	initChildLayout(LayoutMode);

protected:

    i_LayoutMngr*	loMngr();

    uiGroupParentBody&	grpprntbody;

    int			horalign[ nLayoutMode ];

};


class uiGroupObjBody : public uiObjectBody, public QFrame
{
    friend class		uiMainWin;
    friend class		uiDialog;
    friend class		i_LayoutMngr;
    friend class		i_uiGroupLayoutItem;
    friend			uiGroup* gtDynamicCastToGrp(QWidget*);

public:
				uiGroupObjBody(uiGroupObj&,uiParent*,
					       const char*);

    virtual const QWidget*	qwidget_() const { return this; }

    virtual void		reDraw(bool deep);
    void			setPrntBody( uiGroupParentBody* pb )
				{ prntbody_ = pb; }

    virtual int			stretch(bool hor,bool) const;

    uiGroupParentBody*		prntbody_		= nullptr;

    // Hack: Prevents scenewindow movements while trying to rotate
    virtual void		mouseMoveEvent(QMouseEvent*)	{}
    virtual void		setFont(const QFont&);
    virtual void		fontChange(const QFont&);
    virtual void		closeEvent(QCloseEvent*);

protected:

    virtual i_LayoutItem*	mkLayoutItem_( i_LayoutMngr& mgr );

    virtual void		finalise_();
    virtual uiObject&		uiObjHandle()		{ return handle_; }

    uiGroupObj&			handle_;
};


class uiGroupParentBody : public uiParentBody
{
    friend class	uiMainWin;
    friend class	uiDialog;
    friend class	i_LayoutMngr;
    friend class	i_uiGroupLayoutItem;
    friend class	uiGroupObjBody;
    friend		uiGroup* gtDynamicCastToGrp( QWidget*);

public:
			uiGroupParentBody(uiGroup&,uiGroupObjBody&,uiParent*,
					  const char* nm="uiGroupObjBody");

    virtual		~uiGroupParentBody()
			{
			    handle_.body_ = nullptr;
			    delete lomngr_;
			}

    void		setHSpacing( int space )
			    { lomngr_->setHSpacing( space ); }
    void		setVSpacing( int space )
			    { lomngr_->setVSpacing( space ); }
    void		setBorder( int b )
			    { lomngr_->setContentsMargins( b, b, b, b ); }

    uiObject*		hAlignObj()			{ return halignobj_; }
    void		setHAlignObj( uiObject* o );
    uiObject*		hCenterObj()			{ return hcenterobj_; }
    void		setHCenterObj( uiObject* o );


    void		setIsMain( bool yn )
			    { if ( lomngr_ ) lomngr_->setIsMain( yn ); }

    void		updatedAlignment(LayoutMode m )
			    { if ( lomngr_ ) lomngr_->updatedAlignment(m); }


    void		layoutChildren(LayoutMode m )
			    { if ( lomngr_ ) lomngr_->layoutChildren(m); }

    uiGroup&		handle_;

protected:

    i_LayoutMngr*	lomngr_;

    uiObject*		hcenterobj_;
    uiObject*		halignobj_;

    virtual void	finalise()		{ finalise( false ); }
    virtual void	finalise(bool trigger_finalise_start_stop);

    virtual void	manageChld_( uiBaseObject& o, uiObjectBody& b )
			    { lomngr_->addItem( b.mkLayoutItem( *lomngr_ ) ); }

    virtual void	attachChild ( constraintType tp,
				      uiObject* child,
				      uiObject* other, int margin,
				      bool reciprocal )
			{
			    if ( !child  ) return;

			    lomngr_->attach( tp, *child->body()->qwidget(),
				other ? other->body()->qwidget() : 0, margin,
				reciprocal );
			}

    virtual const QWidget* qwidget_() const
			{ return objbody_.qwidget(); }
    virtual const QWidget* managewidg_() const
			{ return objbody_.qwidget();}

    void		mngrDel( CallBacker* cb )
			{
			    if ( cb == lomngr_ )
				lomngr_ = 0;
			    else
				{ pErrMsg("huh?"); }
			}

private:

    uiGroupObjBody&	objbody_;

};


// ----- uiGroupParentBody -----
uiGroupParentBody::uiGroupParentBody( uiGroup& hndle, uiGroupObjBody& objbdy,
				      uiParent* parnt=0, const char* nm )
    : uiParentBody( nm )
    , handle_( hndle )
    , lomngr_( 0 ) , halignobj_( 0 ), hcenterobj_( 0 )
    , objbody_( objbdy )
{
    lomngr_ = new i_LayoutMngr( objbdy.qwidget(), nm, objbdy );
    mAttachCB( lomngr_->objectToBeDeleted(), uiGroupParentBody::mngrDel );
}


void uiGroupParentBody::setHCenterObj( uiObject* obj )
{
    if ( !obj ||( lomngr_ && lomngr_->isChild(obj)) )
    {
	hcenterobj_ = obj;
	return;
    }

#ifdef __debug__
    if ( objbody_.layoutItem() && objbody_.layoutItem()->isAligned() )
    {
	BufferString msg;
	msg = "Set ";
	msg += obj->name();
	msg += "\nas hcenter for ";
	msg += handle_.name();
	msg += ", but attachments were already made!";

	pErrMsg(msg);
    }
#endif

/*
    Ok. So, someone is trying to set an object as HCenterObj, which is not
    a child of this group.
    Let's try to see if the parent of the object is a child and solve the
    problem.
*/

    uiGroup* objpar = dynamic_cast<uiGroup*>(obj->parent());
    if ( objpar && lomngr_ && lomngr_->isChild(objpar->mainObject()) )
    { // good. the object's parent is a child of this group ;-))
	if ( !objpar->hCenterObj() )
	    objpar->setHCenterObj(obj);

	if ( obj == objpar->hCenterObj() )
	{
	    hcenterobj_ = objpar->mainObject();
	    return;
	}
    }

    BufferString msg;
    msg = "Cannot set ";
    msg += obj->name();
    msg += " as hcenter for ";
    msg += handle_.name();
    msg += ". Must be child or child-of-child.";

    pErrMsg(msg);
}

void uiGroupParentBody::setHAlignObj( uiObject* obj )
{
    if ( !obj || (lomngr_ && lomngr_->isChild(obj)) )
    {
	halignobj_ = obj;
	return;
    }

#ifdef __debug__
    if ( objbody_.layoutItem() && objbody_.layoutItem()->isAligned() )
    {
	BufferString msg;
	msg = "Set ";
	msg += obj->name();
	msg += "\nas horalign for ";
	msg += handle_.name();
	msg += ", but attachments were already made!";

	pErrMsg(msg);
    }
#endif

    uiGroup* objpar = dynamic_cast<uiGroup*>(obj->parent());
    if ( objpar && lomngr_ && lomngr_->isChild(objpar->mainObject()) )
    { // good. the object's parent is a child of this group ;-))
	if ( !objpar->hAlignObj() )
	    objpar->setHAlignObj(obj);

	if ( obj == objpar->hAlignObj() )
	{
	    halignobj_ = objpar->mainObject();
	    return;
	}
    }

    BufferString msg;
    msg = "Cannot set ";
    msg += obj->name();
    msg += " as horalign for ";
    msg += handle_.name();
    msg += ". Must be child or child-of-child.";

    pErrMsg(msg);
}


void uiGroupParentBody::finalise( bool trigger_finalise_start_stop )
{
    if ( trigger_finalise_start_stop )
	handle_.preFinalise().trigger( handle_ );

    finaliseChildren();

    if ( trigger_finalise_start_stop )
	handle_.postFinalise().trigger( handle_ );
}


// ----- uiGroupObjBody -----
uiGroupObjBody::uiGroupObjBody( uiGroupObj& hndle, uiParent* parnt,
				    const char* nm )
    : uiObjectBody( parnt, nm )
    , QFrame( parnt && parnt->pbody() ?  parnt->pbody()->managewidg() : 0 )
    , prntbody_( 0 )
    , handle_( hndle )
{}


void uiGroupObjBody::reDraw( bool deep )
{
    prntbody_->handle_.reDraw_( deep );
    prntbody_->lomngr_->forceChildrenRedraw( this, deep );
    uiObjectBody::reDraw( deep ); // calls qWidget().update()
}


int uiGroupObjBody::stretch( bool hor, bool ) const
{
    int strtch = uiObjectBody::stretch( hor, true ); // true: can be undefined
    if ( !mIsUdf(strtch) )
	return strtch;

    if ( !prntbody_->lomngr_ )
	return 0;

    strtch = prntbody_->lomngr_->childStretch( hor );
    if ( strtch>=0 && strtch<=2  )
    {
	if ( hor )
	    const_cast<uiGroupObjBody*>(this)->hstretch = strtch;
	else
	    const_cast<uiGroupObjBody*>(this)->vstretch = strtch;

	return strtch;
    }

    return 0;
}


i_LayoutItem* uiGroupObjBody::mkLayoutItem_( i_LayoutMngr& mgr )
{
#ifdef __debug__
    if ( !prntbody_ )
	{ pErrMsg("Duh. No parentbody yet."); return 0; }
#endif
    i_uiGroupLayoutItem* loitm =
			new i_uiGroupLayoutItem( mgr, *this, *prntbody_ );

    return loitm ;
}

void uiGroupObjBody::finalise_()	{ prntbody_->finalise(); }


void uiGroupObjBody::setFont( const QFont& )
{
    if ( !uifont() ) { pErrMsg("no uifont!"); return; }
    QFrame::setFont( uifont()->qFont() );
}


void uiGroupObjBody::fontChange( const QFont& oldFont )
{
    uiBody::fontchanged();
}


void uiGroupObjBody::closeEvent( QCloseEvent *e )
{
    if ( uiCloseOK() )
	QFrame::closeEvent(e);
}



// ----- i_uiGroupLayoutItem -----
i_uiGroupLayoutItem::i_uiGroupLayoutItem( i_LayoutMngr& mgr,
					  uiGroupObjBody& obj,
					  uiGroupParentBody& par )
    : i_uiLayoutItem( mgr, obj )
    , grpprntbody( par )
{
    for( int idx=0; idx<nLayoutMode; idx++ )
	horalign[idx]=-1;
}

void i_uiGroupLayoutItem::invalidate()
{
    i_uiLayoutItem::invalidate();
    horalign[setGeom]=-1;
}


void i_uiGroupLayoutItem::updatedAlignment( LayoutMode m )
{
    horalign[m]=-1;
    grpprntbody.updatedAlignment(m);
}


void i_uiGroupLayoutItem::initChildLayout( LayoutMode m )
{
     if ( loMngr() )
	loMngr()->initChildLayout(m);
}


i_LayoutMngr* i_uiGroupLayoutItem::loMngr()
{
    return grpprntbody.lomngr_;
}


int i_uiGroupLayoutItem::horAlign( LayoutMode m ) const
{
    const int myleft = curpos(m).left();

    if ( grpprntbody.halignobj_ )
    {
	const i_LayoutItem* halignitm = 0;
	mDynamicCastGet(uiObjectBody*,halobjbody,
			grpprntbody.halignobj_->body());

	if ( halobjbody )
	    halignitm = halobjbody->layoutItem();

	if ( halignitm )
	{
	    if ( horalign[m] < 0 )
	    {
		const_cast<i_uiGroupLayoutItem*>(this)->updatedAlignment(m);
		grpprntbody.layoutChildren(m);
		const_cast<i_uiGroupLayoutItem*>(this)->horalign[m] =
						halignitm->horAlign( m );
	    }

	    return horalign[m] + myleft;
	}
    }

    return myleft;
}


int i_uiGroupLayoutItem::center(LayoutMode m, bool hor) const
{
    if ( !hor )
	return ( curpos(m).top() + curpos(m).bottom() ) / 2;

    if ( grpprntbody.hcenterobj_ )
    {
	const i_LayoutItem* hcenteritm = 0;
	mDynamicCastGet(uiObjectBody*,hcobjbody,grpprntbody.hcenterobj_->body())

	if ( hcobjbody )
	    hcenteritm = hcobjbody->layoutItem();

	if ( hcenteritm )
	    return hcenteritm->center(m);
    }

    return ( curpos(m).left() + curpos(m).right() ) / 2;
}


// uiGroup
uiGroup::uiGroup( uiParent* parnt, const char* nm, bool manage )
    : uiParent( nm, 0 )
    , grpobj_( 0 )
    , body_( 0 )
{
    grpobj_ =  new uiGroupObj( this, parnt, nm, manage );
    uiGroupObjBody* grpbdy = dynamic_cast<uiGroupObjBody*>( grpobj_->body() );

#ifdef __debug__
    if ( !grpbdy )
	{ pErrMsg("Huh"); return; }
#endif

    body_ =  new uiGroupParentBody( *this, *grpbdy, parnt, nm );
    setBody( body_ );

    grpobj_->body_->setPrntBody( body_ );
    if ( parnt )
    {
	if ( manage )
	    parnt->manageChld( *grpobj_, *grpobj_->body_ );
	else
	    parnt->addChild( *this );
    }

    mDefineStaticLocalObject( bool, showgrps__,
			      = GetEnvVarYN("DTECT_SHOW_GROUP_FRAMES") );
    if ( !showgrps__ )
	setFrameStyle( QFrame::NoFrame );
    else
	setFrameStyle( QFrame::Box | QFrame::Plain );
}


uiGroup::~uiGroup()
{
    if ( grpobj_ ) { grpobj_->uigrp_ = 0; delete grpobj_; }
    if ( body_ )
    {
	uiGroupParentBody* bd = body_;
	body_ = 0;
	delete bd;
    }
    detachAllNotifiers();
}


void uiGroup::bodyDel( CallBacker* cb )
{
    if ( body_ == cb )
	body_ = 0;
    else
        { pErrMsg("huh?"); }
}


void uiGroup::uiobjDel( CallBacker* cb )
{
    if ( cb == grpobj_ )
	grpobj_ = 0;
    else
        { pErrMsg("huh?"); }
}


void uiGroup::setShrinkAllowed( bool yn )
{
    uiObjectBody* bdy = dynamic_cast<uiObjectBody*>(mainObject()->body());
    if ( bdy ) bdy->setShrinkAllowed(yn);
}


bool uiGroup::shrinkAllowed()
{
    uiObjectBody* bdy = dynamic_cast<uiObjectBody*>(mainObject()->body());
    return bdy ? bdy->shrinkAllowed() : false;
}


void uiGroup::attach_( constraintType c, uiObject *other, int margin,
		       bool reciprocal )
{
    if ( (c == heightSameAs ) || (c == widthSameAs ) )
    {
	BufferString msg((c == heightSameAs ) ? "heightSameAs":"widthSameAs" );
	msg += " not allowed for group ";
	msg += mainObject()->name();
	pErrMsg(msg); return;
    }
    mainObject()->attach(c,other,margin,reciprocal);

}


void uiGroup::setIsMain( bool yn )
{ body_->setIsMain( yn ); }

void uiGroup::setHSpacing( int s )
{ body_->setHSpacing( s ); }

void uiGroup::setVSpacing( int s )
{ body_->setVSpacing( s ); }

void uiGroup::setBorder( int b )
{ body_->setBorder( b ); }

uiObject* uiGroup::hAlignObj()
{ return body_->hAlignObj(); }


void uiGroup::setFrame( bool yn )
{
    if ( !yn )
	setFrameStyle( QFrame::NoFrame );
    else
    {
	grpobj_->body_->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
	grpobj_->body_->setLineWidth( 1 );
	grpobj_->body_->setMidLineWidth( 0 );
    }
}


void uiGroup::setFrameStyle( int fs )
{ grpobj_->body_->setFrameStyle( fs );	}

void uiGroup::setHAlignObj( uiObject* obj )
{ body_->setHAlignObj( obj ); }

uiObject* uiGroup::hCenterObj()
{ return body_->hCenterObj(); }

void uiGroup::setHCenterObj( uiObject* obj )
{ body_->setHCenterObj( obj ); }


void uiGroup::setNoBackGround()
    { grpobj_->body_->setAttribute( Qt::WA_NoSystemBackground ); }


void uiGroup::setChildrenSensitive( bool yn )
{
    for ( int idx=0; idx<grpobj_->childList()->size(); idx++ )
	((uiObject*) (*grpobj_->childList())[idx])->setSensitive( yn );
}


void uiGroup::setSize( const uiSize& sz )
{
    if ( sz.width() <= 0 || sz.height() <= 0 )
	return;

    const int oldwidth = mainObject()->width();
    const int oldheight = mainObject()->height();
    if ( !oldwidth || !oldheight )
	return;

    const float wfac = sz.width()/(float)oldwidth;
    const float hfac = sz.height()/(float)oldheight;

    reSizeChildren( mainObject(), wfac, hfac );
    const int newwdth = mNINT32(oldwidth*wfac);
    const int newhght = mNINT32(oldheight*hfac);
    mainObject()->setMinimumWidth( newwdth );
    mainObject()->setMinimumHeight( newhght );
    mainObject()->setMaximumWidth( newwdth );
    mainObject()->setMaximumHeight( newhght );
}


void uiGroup::reSizeChildren( const uiObject* obj, float wfac, float hfac )
{
    if ( wfac <= 0 || hfac <= 0 || !obj->childList() )
	return;

    for ( int idchild=0; idchild<obj->childList()->size(); idchild++ )
    {
	const uiBaseObject* child = (*obj->childList())[idchild];
	mDynamicCastGet(const uiObject*,objchild,child)
	if ( objchild )
	    reSizeChildren( objchild, wfac, hfac );

	const int newwdth = mNINT32(objchild->width()*wfac);
	const int newhght = mNINT32(objchild->height()*hfac);
	((uiObject*)(objchild))->setMinimumWidth( newwdth );
	((uiObject*)(objchild))->setMaximumWidth( newwdth );
	((uiObject*)(objchild))->setMinimumHeight( newhght );
	((uiObject*)(objchild))->setMaximumHeight( newhght );
    }
}


uiGroup* uiGroup::gtDynamicCastToGrp( QWidget* widg )
{
    uiGroupObjBody* body = dynamic_cast<uiGroupObjBody*>( widg );
    if ( !body || !body->prntbody_ )
	return nullptr;

    return &body->prntbody_->handle_;
}



// uiGroupObj
uiGroupObj::uiGroupObj( uiGroup* bud, uiParent* parnt , const char* nm,
			bool manage )
    : uiObject( parnt, nm )
    , uigrp_( bud )
{
    body_= new uiGroupObjBody( *this, parnt, nm );
    setBody( body_ );

    mAttachCB( uigrp_->objectToBeDeleted(), uiGroupObj::grpDel );
    mAttachCB( body_->objectToBeDeleted(), uiGroupObj::bodyDel );
}


uiGroupObj::~uiGroupObj()
{
    detachAllNotifiers();
    if ( uigrp_ )
    {
	uigrp_->grpobj_ = nullptr;
	delete uigrp_;
    }
}


const ObjectSet<uiBaseObject>* uiGroupObj::childList() const
{
    return uigrp_ ? uigrp_->childList() : nullptr;
}


void uiGroupObj::bodyDel( CallBacker* cb )
{
    if ( body_ == cb )
	body_ = nullptr;
    else
	{ pErrMsg("huh?"); }
}


void uiGroupObj::grpDel( CallBacker* cb )
{
    if ( cb == uigrp_ )
	uigrp_ = nullptr;
    else
	{ pErrMsg("huh?"); }
}
