/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uigroup.cc,v 1.39 2002-11-01 12:29:32 arend Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
#include <uiobjbody.h>
#include <i_layout.h>
#include <i_layoutitem.h>
#include <qwidget.h>
#include <qframe.h>

#include <iostream>
#include "errh.h"

#include <uitabgroup.h>

class uiGroupObjBody;
class uiGroupParentBody;



class i_uiGroupLayoutItem : public i_uiLayoutItem
{
public:
			i_uiGroupLayoutItem( i_LayoutMngr& mngr, 
					     uiGroupObjBody& obj, 
					     uiGroupParentBody& par );

    virtual int		horAlign(layoutMode) const;
    virtual int		horCentre(layoutMode) const;

    virtual void	invalidate();
    virtual void	updatedAlignment(layoutMode);
    virtual void	initChildLayout(layoutMode);
//#define grp_layout__
#ifdef grp_layout__
    virtual void	layout( layoutMode m, const int, bool*, bool );
#endif

protected:

    i_LayoutMngr* 	loMngr();

    uiGroupParentBody&	grpprntbody;

    int			horalign[ nLayoutMode ];

};


class uiGroupObjBody  : public uiObjectBody, public QFrame
{ 	
    friend class 		uiMainWin;
    friend class 		uiDialog;
    friend class 		i_LayoutMngr;
    friend class		i_uiGroupLayoutItem;
    friend			uiGroup* gtDynamicCastToGrp( QWidget*);
public:

				uiGroupObjBody( uiGroupObj& handle, 
						uiParent* parnt,
						const char* nm )
				    : uiObjectBody( parnt )
				    , QFrame( parnt && parnt->pbody() ?  
					parnt->pbody()->managewidg() : 0, nm )
				    , handle_( handle )
				    , prntbody_( 0 )			
				{}

				uiGroupObjBody( uiGroupObj& handle, 
						uiTabGroup* parnt,
						const char* nm )
				    : uiObjectBody( 0 )
				    , QFrame( parnt && parnt->body() ?  
					parnt->body()->qwidget() : 0, nm )
				    , handle_( handle )
				    , prntbody_( 0 )			
				{}

#define mHANDLE_OBJ     	uiGroupObj
#define mQWIDGET_BASE		QFrame
#define mQWIDGET_BODY   	QFrame
#include               		"i_uiobjqtbody.h"

public:

    virtual void        	reDraw( bool deep );
    void			setPrntBody (uiGroupParentBody* pb)
				    { prntbody_ = pb; }

    virtual int			stretch( bool hor, bool ) const;

protected:

    uiGroupParentBody*		prntbody_;

    virtual i_LayoutItem*	mkLayoutItem_( i_LayoutMngr& mngr );

    virtual void		finalise_();

};


class uiGroupParentBody : public uiParentBody
{ 	
    friend class 	uiMainWin;
    friend class 	uiDialog;
    friend class 	i_LayoutMngr;
    friend class	i_uiGroupLayoutItem;
    friend class	uiGroupObjBody;
    friend		uiGroup* gtDynamicCastToGrp( QWidget*);

public:
			uiGroupParentBody( uiGroup& handle, 
					 uiGroupObjBody& objbdy,
					 uiParent* parnt=0,
					 const char* nm="uiGroupObjBody" )
                            : uiParentBody()
                            , handle_( handle )
			    , loMngr( 0 ) , halignobj( 0 ), hcentreobj( 0 )
			    , objbody_( objbdy )
			{ 
			    loMngr = new i_LayoutMngr( objbdy.qwidget(), 
			       nm, objbdy );
			}
public:


    virtual		~uiGroupParentBody()		{ delete loMngr; }

    void		setHSpacing( int space )
			    { loMngr->setHSpacing( space ); }
    void		setVSpacing( int space )
			    { loMngr->setVSpacing( space ); }
    void		setBorder( int b )
			    { loMngr->setMargin( b ); }

    uiObject*		hAlignObj()			{ return halignobj; }
    void		setHAlignObj( uiObject* o );
    uiObject*		hCentreObj()			{ return hcentreobj; }
    void		setHCentreObj( uiObject* o );


    void		setIsMain( bool yn ) 
			    { if( loMngr ) loMngr->setIsMain( yn ); }

    void		updatedAlignment(layoutMode m )
			    { if( loMngr ) loMngr->updatedAlignment(m); }


    void		layoutChildren(layoutMode m )
			    { if( loMngr ) loMngr->layoutChildren(m); }

protected:

    i_LayoutMngr* 	loMngr;
    uiGroup&		handle_;

    uiObject*		hcentreobj;
    uiObject*		halignobj;

    virtual void        manageChld_( uiObjHandle& o, uiObjectBody& b )
			    { loMngr->addItem( b.mkLayoutItem( *loMngr ) ); }

    virtual void	attachChild ( constraintType tp,
				      uiObject* child,
				      uiObject* other, int margin,
				      bool reciprocal )
			{ 
			    if ( !child  ) return;

			    loMngr->attach( tp, *child->body()->qwidget(),
				other ? other->body()->qwidget() : 0, margin,
				reciprocal );
			}

    virtual const QWidget* qwidget_() const    { return objbody_.qwidget(); }
    virtual const QWidget* managewidg_() const { return objbody_.qwidget();}

private:

    uiGroupObjBody&	objbody_;

};

void uiGroupParentBody::setHCentreObj( uiObject* obj )
{ 
    if( !obj ||( loMngr && loMngr->isChild(obj)) ) { hcentreobj = obj; return; }

#ifdef __debug__
    if( objbody_.layoutItem() && objbody_.layoutItem()->isAligned() ) 
    {
	BufferString msg;
	msg = "Set ";
	msg += obj->name();
	msg += "\nas hcentre for ";
	msg += handle_.name();
	msg += ", but attachments were already made!";

	pErrMsg(msg);
    }
#endif

/*
    Ok. So, someone is trying to set an object as HCentreObj, which is not
    a child of this group.
    Let's try to see if the parent of the object is a child and solve the 
    problem.
*/

    uiGroup* objpar = dynamic_cast<uiGroup*>(obj->parent());
    if( objpar && loMngr && loMngr->isChild(objpar->uiObj()) )
    { // good. the object's parent is a child of this group ;-))
	if( !objpar->hCentreObj() )
	    objpar->setHCentreObj(obj);

	if( obj == objpar->hCentreObj() )
	{
	    hcentreobj = objpar->uiObj();
	    return;
	}
    }

    BufferString msg;
    msg = "Cannot set ";
    msg += obj->name();
    msg += " as hcentre for ";
    msg += handle_.name();
    msg += ". Must be child or child-of-child.";

    pErrMsg(msg);
}

void uiGroupParentBody::setHAlignObj( uiObject* obj )
{ 
    if( !obj || (loMngr && loMngr->isChild(obj)) ) { halignobj = obj; return; }

#ifdef __debug__
    if( objbody_.layoutItem() && objbody_.layoutItem()->isAligned() ) 
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
    if( objpar && loMngr && loMngr->isChild(objpar->uiObj()) )
    { // good. the object's parent is a child of this group ;-))
	if( !objpar->hAlignObj() )
	    objpar->setHAlignObj(obj);

	if( obj == objpar->hAlignObj() )
	{
	    halignobj = objpar->uiObj();
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


void uiGroupObjBody::reDraw( bool deep )
{ 
    prntbody_->handle_.reDraw_(deep);
    prntbody_->loMngr->forceChildrenRedraw( this, deep ); 
    uiObjectBody::reDraw( deep ); // calls qWidget().update()
}

int uiGroupObjBody::stretch( bool hor, bool ) const
{
#if 0
    int s = uiObjectBody::stretch( hor, true ); // true: can be undefined
    return s != mUndefIntVal ? s : 
	( prntbody_->loMngr ? prntbody_->loMngr->childStretch( hor ) : 0 );
#else
    int s = uiObjectBody::stretch( hor, true ); // true: can be undefined
    if( s != mUndefIntVal ) return s;

    if( prntbody_->loMngr )
    {
	s = prntbody_->loMngr->childStretch( hor );
	if( s )
	{
	    if( hor )	const_cast<uiGroupObjBody*>(this)->hStretch = s; 
	    else	const_cast<uiGroupObjBody*>(this)->vStretch = s;
	}
    }
    return 0;
#endif
}

i_LayoutItem* uiGroupObjBody::mkLayoutItem_( i_LayoutMngr& mngr )
{ 
#ifdef __debug__
    if( !prntbody_ ) 
	{ pErrMsg("Yo. No parentbody yet."); return 0; }
#endif
    i_uiGroupLayoutItem* loitm = 
			new i_uiGroupLayoutItem( mngr, *this, *prntbody_ );

//    prntbody_->loMngr->setLayoutPosItm( itm );

    return loitm ;
}

void uiGroupObjBody::finalise_()	{ prntbody_->finalise(); }



i_uiGroupLayoutItem::i_uiGroupLayoutItem( i_LayoutMngr& mngr, 
					  uiGroupObjBody& obj, 
					  uiGroupParentBody& par )
    : i_uiLayoutItem( mngr, obj )
    , grpprntbody( par ) 
{
    for( int idx=0; idx<nLayoutMode; idx++ )
	horalign[idx]=-1;
}

void i_uiGroupLayoutItem::invalidate()
{ 
    i_uiLayoutItem::invalidate(); 
#if 0
    for( int idx=0; idx<nLayoutMode; idx++ )
        horalign[idx]=-1;
#else
        horalign[setGeom]=-1;
#endif
}


void i_uiGroupLayoutItem::updatedAlignment( layoutMode m )
{ 
    horalign[m]=-1;
    grpprntbody.updatedAlignment(m);
}


void i_uiGroupLayoutItem::initChildLayout( layoutMode m )
{ 
     if( loMngr() ) loMngr()->initChildLayout(m); 
}

#ifdef grp_layout__
void i_uiGroupLayoutItem::layout( layoutMode m, const int iteridx, bool* chupd,
				  bool finalLoop )
{ 
    i_uiLayoutItem::layout(m,iteridx, chupd, finalLoop ); 

    uiRect mPos = loMngr()->curpos( m );
    QRect geom( mPos.left(), mPos.top(),
                                        mPos.hNrPics(), mPos.vNrPics() );

    //if( bodyLayouted() && bodyLayouted()->isHidden() && loMngr() )
    if( loMngr() )
	loMngr()->doLayout( m, geom );
}
#endif

i_LayoutMngr* i_uiGroupLayoutItem::loMngr() 
    { return grpprntbody.loMngr; } 

int i_uiGroupLayoutItem::horAlign( layoutMode m ) const 
{
    int myleft = curpos(m).left();

    if( grpprntbody.halignobj )
    {
	const i_LayoutItem* halignitm = 0;
	mDynamicCastGet(uiObjectBody*,halobjbody,grpprntbody.halignobj->body());

	if( halobjbody ) halignitm = halobjbody->layoutItem();

	if( halignitm )
	{
	    if( horalign[m] < 0 )
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


int i_uiGroupLayoutItem::horCentre(layoutMode m) const 
{ 
    if( grpprntbody.hcentreobj )
    {
	const i_LayoutItem* hcentreitm = 0;
	mDynamicCastGet(uiObjectBody*,hcobjbody,grpprntbody.hcentreobj->body());

	if( hcobjbody ) hcentreitm = hcobjbody->layoutItem();

	if( hcentreitm ) return hcentreitm->horCentre(m);
    }

    return ( curpos(m).left() + curpos(m).right() ) / 2;
}

uiGroup::uiGroup( uiParent* p, const char* nm, bool manage )
    : uiParent( nm, 0 )
    , grpobj_( 0 )
    , body_( 0 )
{
    grpobj_ =  new uiGroupObj( this,p,nm,manage );
    uiGroupObjBody* grpbdy = dynamic_cast<uiGroupObjBody*>( grpobj_->body() );

#ifdef __debug__
    if( !grpbdy ) { pErrMsg("Huh") ; return; }
#endif

    body_ =  new uiGroupParentBody(*this,*grpbdy, p, nm );
    setBody( body_ );

    grpobj_->body_->setPrntBody( body_ );
    if( p )
    {
	if( manage ) p->manageChld( *grpobj_, *grpobj_->body_ );
	else	 p->addChild( *this );
    }
}


uiGroup::uiGroup( uiTabGroup* p, const char* nm )
    : uiParent( nm, 0 )
    , grpobj_( 0 )
    , body_( 0 )
{
    grpobj_ =  new uiGroupObj( this,p,nm );
    uiGroupObjBody* grpbdy = dynamic_cast<uiGroupObjBody*>( grpobj_->body() );

#ifdef __debug__
    if( !grpbdy ) { pErrMsg("Huh") ; return; }
#endif

    body_ =  new uiGroupParentBody(*this,*grpbdy, 0, nm );
    setBody( body_ );

    grpobj_->body_->setPrntBody( body_ );
}

uiGroup::~uiGroup()
    { if( grpobj_ ) { grpobj_->uigrp_ = 0; delete grpobj_; } }

void uiGroup::display( bool yn, bool shrink, bool maximize )
{ 
    finalise();
    uiObj()->display( yn, shrink, maximize );
}

void uiGroup::setFocus()		{ uiObj()->setFocus(); }

void uiGroup::setSensitive(bool yn)	{ uiObj()->setSensitive(yn); }
bool uiGroup::sensitive() const	{ return uiObj()->sensitive(); }
int  uiGroup::prefHNrPics() const{ return uiObj()->prefHNrPics(); }
void uiGroup::setPrefWidth( int w )	{ uiObj()->setPrefWidth(w); }
int  uiGroup::prefVNrPics() const{ return uiObj()->prefVNrPics(); }
void uiGroup::setPrefHeight( int h )	{ uiObj()->setPrefHeight(h); }
void uiGroup::setFont( const uiFont& f)	{ uiObj()->setFont(f); }
void uiGroup::setCaption(const char* c)	{ uiObj()->setCaption(c); }
void uiGroup::reDraw( bool deep )		{ uiObj()->reDraw( deep ); }

void uiGroup::setShrinkAllowed(bool yn)
{ 
    uiObjectBody* bdy = dynamic_cast<uiObjectBody*>(uiObj()->body());
    if( bdy ) bdy->setShrinkAllowed(yn); 
}

bool uiGroup::shrinkAllowed()
{ 
    uiObjectBody* bdy = dynamic_cast<uiObjectBody*>(uiObj()->body());
    return bdy ? bdy->shrinkAllowed() : false; 
}


const uiFont* uiGroup::font() const		{ return uiObj()->font(); }

void uiGroup::setPrefWidthInChar( float w ) 
    { uiObj()->setPrefWidthInChar(w); }

void uiGroup::setPrefHeightInChar( float h )
    { uiObj()->setPrefHeightInChar(h); }

void uiGroup::setStretch( int hor, int ver )
    { uiObj()->setStretch( hor, ver ); }

Color uiGroup::backgroundColor() const
    { return uiObj()->backgroundColor(); }

void uiGroup::setBackgroundColor(const Color& c)
    { uiObj()->setBackgroundColor(c); }

void uiGroup::attach ( constraintType c, int margin )
    { uiObj()->attach(c,margin); }

void uiGroup::attach ( constraintType c, uiObject *other, int margin,
		       bool reciprocal )
{
    if( (c == heightSameAs ) || (c == widthSameAs ) ) 
    {
	BufferString msg((c == heightSameAs ) ? "heightSameAs":"widthSameAs" );
	msg += " not allowed for group ";
	msg += uiObj()->name();
	pErrMsg(msg); 
	return;
    }
    uiObj()->attach(c,other,margin,reciprocal);

}


uiSize uiGroup::actualSize( bool include_border) const	
    { return uiObj()->actualSize(include_border); }


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
    if( yn )
	grpobj_->body_->setFrameStyle( QFrame::Panel | QFrame::Sunken ); 
    else
	grpobj_->body_->setFrameStyle( QFrame::NoFrame );
}

void uiGroup::setHAlignObj( uiObject* o )
    { body_->setHAlignObj(o); }


uiObject* uiGroup::hCentreObj()
    { return body_->hCentreObj(); }


void uiGroup::setHCentreObj( uiObject* o )
    { body_->setHCentreObj( o ); }


uiGroupObj::uiGroupObj( uiGroup* bud, uiParent* parnt , const char* nm,
			bool manage )
    : uiObject( parnt, nm )
    , uigrp_( bud )
{
    body_= new uiGroupObjBody( *this, parnt, nm );
    setBody( body_ );
}

uiGroupObj::uiGroupObj( uiGroup* bud, uiTabGroup* parnt , const char* nm )
    : uiObject( 0, nm )
    , uigrp_( bud )
{
    body_= new uiGroupObjBody( *this, parnt, nm );
    setBody( body_ );
}

uiGroupObj::~uiGroupObj()
    { if(uigrp_) { uigrp_->grpobj_ =0; delete uigrp_; }  }

uiGroup* uiGroup::gtDynamicCastToGrp( QWidget* widg )
{ 
    uiGroupObjBody* body = dynamic_cast<uiGroupObjBody*>( widg );
    if( !body || !body->prntbody_ ) return 0;

    return &body->prntbody_->handle_;
}
