/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uigroup.cc,v 1.13 2001-09-19 12:16:04 nanne Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
#include <uiobjbody.h>
#include <i_layout.h>
#include <i_layoutitem.h>
#include <qwidget.h>

#include <iostream>
#include "errh.h"

class uiGroupObjBody;
class uiGroupParentBody;

//! Wrapper around QLayoutItems that have been wrapped by a i_QObjWrp wrapper and therefore have a reference to a uiObject.
class i_uiGroupLayoutItem : public i_uiLayoutItem
{
public:
			i_uiGroupLayoutItem( i_LayoutMngr& mngr, 
					     uiGroupObjBody& obj, 
					     uiGroupParentBody& par );

    virtual int		horAlign() const;
    virtual int		horCentre() const;

protected:

    uiGroupParentBody&	grpprntbody;

};


class uiGroupObjBody  : public uiObjectBody,
			public QWidget
{ 	
    friend class 	uiMainWin;
    friend class 	uiDialog;
    friend class 	i_LayoutMngr;
    friend class	i_uiGroupLayoutItem;
public:
			uiGroupObjBody( uiGroupObj& handle, uiParent* parnt,
					const char* nm)
			    : uiObjectBody( parnt )
                            , QWidget( parnt && parnt->body() ?  
					parnt->body()->managewidg() : 0, nm )
                            , handle_( handle )
			    , prntbody_( 0 )			{}

#define mHANDLE_OBJ     uiGroupObj
#define mQWIDGET_BASE   QWidget
#define mQWIDGET_BODY   QWidget
#include                "i_uiobjqtbody.h"

public:

    virtual void        reDraw( bool deep );
    void		setPrntBody (uiGroupParentBody* pb)
			    { prntbody_ = pb; }

//  virtual int		stretch( bool hor );

protected:

    uiGroupParentBody*	prntbody_;

    virtual i_LayoutItem* mkLayoutItem_( i_LayoutMngr& mngr )
			    { 
#ifdef __debug__
			    if( !prntbody_ ) 
				{ pErrMsg("Yo. No parentbody yet."); return 0; }
#endif
				return new i_uiGroupLayoutItem
						( mngr, *this, *prntbody_ );
			    }

    virtual void	finalise_();
};


class uiGroupParentBody : public uiParentBody
{ 	
    friend class 	uiMainWin;
    friend class 	uiDialog;
    friend class 	i_LayoutMngr;
    friend class	i_uiGroupLayoutItem;
    friend class	uiGroupObjBody;

public:
			uiGroupParentBody( uiGroup& handle, 
					 uiGroupObjBody& objbdy,
					 uiParent* parnt=0,
					 const char* nm="uiGroupObjBody", 
					 int border=0, int spacing=10)
                            : uiParentBody()
                            , handle_( handle )
			    , loMngr( 0 ) , halignobj( 0 ), hcentreobj( 0 )
			    , objbody_( objbdy )
			{ 
			    loMngr = new i_LayoutMngr( objbdy.qwidget(), 
						       border, spacing ); 
			}

public:


    virtual		~uiGroupParentBody()		{ delete loMngr; }

    void		setSpacing( int space )
			    { loMngr->setSpacing( space ); }
    void		setBorder( int b )
			    { loMngr->setMargin( b ); }

    uiObject*		hAlignObj()			{ return halignobj; }
    void		setHAlignObj( uiObject* o ) 	{ halignobj = o;}
    uiObject*		hCentreObj()			{ return hcentreobj; }
    void		setHCentreObj( uiObject* o ) 	{ hcentreobj = o;}

    int			sumChildrenStretch( bool hor )
			{
			    int sum=0;
			    for( int idx=0; idx<children.size(); idx++ )
			    {
				mDynamicCastGet(uiObjectBody*,chbody, 
							children[idx]->body());

				if ( chbody ) sum += chbody->stretch(hor);
			    }

			    return sum;
			}

    virtual int		minTextWidgetHeight() const
			{
			    return loMngr ? loMngr->minTxtWidgHgt() 
					: uiParentBody::minTextWidgetHeight();
			}

protected:

    i_LayoutMngr* 	loMngr;
    uiGroup&		handle_;

    uiObject*		hcentreobj;
    uiObject*		halignobj;

    virtual void        manageChld_( uiObjHandle& o, uiObjectBody& b )
			    { loMngr->addItem( b.mkLayoutItem( *loMngr ) ); }

    virtual void	attachChild ( constraintType tp,
				      uiObject* child,
				      uiObject* other, int margin )
			{ 
			    if ( !child  ) return;
			    loMngr->attach( tp, *child->body()->qwidget(),
				other ? other->body()->qwidget() : 0, margin );
			}

    virtual const QWidget* qwidget_() const    { return objbody_.qwidget(); }
    virtual const QWidget* managewidg_() const { return objbody_.managewidg();}

private:

    uiGroupObjBody&	objbody_;

};



void uiGroupObjBody::reDraw( bool deep )
{ 
    prntbody_->handle_.reDraw_(deep);
    prntbody_->loMngr->forceChildrenRedraw( this, deep ); 
    uiObjectBody::reDraw( deep ); // calls qWidget().update()
}

/*
int uiGroupObjBody::stretch( bool hor )
{
    int s = uiObjectBody::stretch( hor );
    return s ? s : prntbody_->sumChildrenStretch( hor );
}
*/

void uiGroupObjBody::finalise_()	{ prntbody_->finalise(); }



i_uiGroupLayoutItem::i_uiGroupLayoutItem( i_LayoutMngr& mngr, 
					  uiGroupObjBody& obj, 
					  uiGroupParentBody& par )
    : i_uiLayoutItem( mngr, obj )
    , grpprntbody( par ) 
    {}



int i_uiGroupLayoutItem::horAlign() const 
{
    int offs = mngr().pos().left() + pos().left();
    int border = grpprntbody.loMngr->borderSpace();

    if( grpprntbody.halignobj )
    {
	const i_LayoutItem* halignitm = 0;
	mDynamicCastGet(uiObjectBody*,halobjbody, grpprntbody.halignobj->body());

	if( halobjbody ) halignitm = halobjbody->layoutItem();

	if( halignitm ) return halignitm->horAlign() + offs + border;
    }

    return offs;
}


int i_uiGroupLayoutItem::horCentre() const 
{ 
    int offs = mngr().pos().left() + pos().left();
    int border = grpprntbody.loMngr->borderSpace();

    if( grpprntbody.hcentreobj )
    {
	const i_LayoutItem* hcentreitm = 0;
	mDynamicCastGet(uiObjectBody*,hcobjbody,grpprntbody.hcentreobj->body());

	if( hcobjbody ) hcentreitm = hcobjbody->layoutItem();

	if( hcentreitm ) return hcentreitm->horCentre() + offs + border;
    }

    return ( mngr().pos().left() + mngr().pos().right() ) / 2;
}




uiGroup::uiGroup( uiParent* p, const char* nm, int border, int spacing, 
		  bool manage)
    : uiParent( nm, 0 )
    , grpobj_( 0 )
    , body_( 0 )
{
    grpobj_ =  new uiGroupObj( p,nm,manage );
    uiGroupObjBody* grpbdy = dynamic_cast<uiGroupObjBody*>( grpobj_->body() );

#ifdef __debug__
    if( !grpbdy ) { pErrMsg("Huh") ; return; }
#endif

    body_ =  new uiGroupParentBody(*this,*grpbdy, p, nm, border, spacing);
    setBody( body_ );

    grpobj_->body_->setPrntBody( body_ );

    if( manage ) p->manageChld( *grpobj_, *grpobj_->body_ );
    else	 p->addChild( *this );
}

void uiGroup::show()		{ uiObj()->show(); }
void uiGroup::hide(bool shrink)		{ uiObj()->hide(shrink); }
void uiGroup::setFocus()		{ uiObj()->setFocus(); }

void uiGroup::setSensitive(bool yn)	{ uiObj()->setSensitive(yn); }
bool uiGroup::sensitive() const	{ return uiObj()->sensitive(); }
int  uiGroup::preferredWidth() const{ return uiObj()->preferredWidth(); }
void uiGroup::setPrefWidth( int w )	{ uiObj()->setPrefWidth(w); }
int  uiGroup::preferredHeight() const{ return uiObj()->preferredHeight(); }
void uiGroup::setPrefHeight( int h )	{ uiObj()->setPrefHeight(h); }
void uiGroup::setFont( const uiFont& f)	{ uiObj()->setFont(f); }
void uiGroup::setCaption(const char* c)	{ uiObj()->setCaption(c); }
void uiGroup::reDraw( bool deep )		{ uiObj()->reDraw( deep ); }

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

void uiGroup::attach ( constraintType c, uiObject *other, int margin )
    { uiObj()->attach(c,other,margin); }

uiSize uiGroup::actualSize( bool include_border) const	
    { return uiObj()->actualSize(include_border); }


void uiGroup::setSpacing( int s )
    { body_->setSpacing( s ); }


void uiGroup::setBorder( int b )
    { body_->setBorder( b ); }


uiObject* uiGroup::hAlignObj()
    { return body_->hAlignObj(); }


void uiGroup::setHAlignObj( uiObject* o )
    { body_->setHAlignObj(o); }


uiObject* uiGroup::hCentreObj()
    { return body_->hCentreObj(); }


void uiGroup::setHCentreObj( uiObject* o )
    { body_->setHCentreObj( o ); }


uiGroupObj::uiGroupObj( uiParent* parnt , const char* nm, bool manage )
: uiObject( parnt, nm )
{
    body_= new uiGroupObjBody( *this, parnt, nm);
    setBody( body_ );
}

