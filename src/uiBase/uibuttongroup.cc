/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id: uibuttongroup.cc,v 1.16 2008-02-01 06:18:44 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uibuttongroup.h"
#include "uiobjbody.h"

#include <Q3ButtonGroup>
#include <QAbstractButton>

#include "errh.h"

class uiButtonGroupObjBody;
class uiButtonGroupParentBody;


class uiButtonGroupObjBody : public uiObjectBody  , public Q3ButtonGroup
{
public:
			uiButtonGroupObjBody(uiButtonGroupObj& handle, 
					uiParent* parnt, const char* txt, 
                                        bool vertical, int strips )
			: uiObjectBody( parnt, txt )
			, Q3ButtonGroup(strips, vertical ? Qt::Horizontal
							 : Qt::Vertical,
					txt,
					parnt && parnt->pbody() ?
					parnt->pbody()->managewidg() : 0, txt )
			, handle_( handle )	{}

    virtual		~uiButtonGroupObjBody()	{}

#define mHANDLE_OBJ	uiButtonGroupObj
#define mQWIDGET_BASE	QWidget
#define mQWIDGET_BODY	Q3ButtonGroup
#include		"i_uiobjqtbody.h"

public:

    void		setPrntBody (uiButtonGroupParentBody* pb)
			{ prntbody_ = pb; }

protected:

    uiButtonGroupParentBody*	prntbody_;

};


class uiButtonGroupParentBody : public uiParentBody
{ 	
    friend class	uiButtonGroupObjBody;

public:
			uiButtonGroupParentBody( uiButtonGroup& handle, 
					 uiButtonGroupObjBody& objbdy,
					 uiParent* parnt=0,
					 const char* nm="uiButtonGroupObjBody") 
                            : uiParentBody(nm)
                            , handle_(handle)
			    , objbody_(objbdy) {}

    virtual void	attachChild( constraintType tp,
				     uiObject* child,
				     uiObject* other, int margin,
				     bool reciprocal )
			{ pErrMsg("Cannot do attachments in uiButtonGroups"); }

protected:

    uiButtonGroup&		handle_;

    virtual const QWidget* 	qwidget_() const    
    				{ return objbody_.qwidget(); }
    virtual const QWidget* 	managewidg_() const 
    				{ return objbody_.qwidget();}

private:

    uiButtonGroupObjBody&	objbody_;

};


uiButtonGroupObj::uiButtonGroupObj( uiButtonGroup* uibg, uiParent* p, 
				    const char* nm, bool vertical, int strips )
    : uiObject( p, nm )
    , uibutgrp_( uibg )
{
    body_ = new uiButtonGroupObjBody( *this, p, nm, vertical, strips );
    setBody( body_ );

    uibutgrp_->deleteNotify( mCB(this,uiButtonGroupObj,grpDel) );
    body_->deleteNotify( mCB(this,uiButtonGroupObj,bodyDel) );
}


uiButtonGroupObj::~uiButtonGroupObj()
{ if(uibutgrp_) { uibutgrp_->grpobj_ =0; delete uibutgrp_; }  }


const ObjectSet<uiObjHandle>* uiButtonGroupObj::childList() const
{ return uibutgrp_ ? uibutgrp_->childList() : 0; }


void uiButtonGroupObj::bodyDel( CallBacker* cb )
{
    if( body_ == cb ) body_ = 0;
    else pErrMsg("huh?");
}


void uiButtonGroupObj::grpDel( CallBacker* cb )
{
    if( cb == uibutgrp_ ) uibutgrp_ = 0;
    else pErrMsg("huh?");
}


uiButtonGroup::uiButtonGroup( uiParent* p, const char* nm,
			      bool vertical, int strips )
    : uiParent( nm, 0 )
    , grpobj_( 0 )
    , body_( 0 )
{
    grpobj_ = new uiButtonGroupObj( this, p, nm, vertical, strips );
    uiButtonGroupObjBody* grpbdy = 
	    dynamic_cast<uiButtonGroupObjBody*>( grpobj_->body() );

#ifdef __debug__
    if( !grpbdy ) { pErrMsg("Huh") ; return; }
#endif

    body_ =  new uiButtonGroupParentBody( *this, *grpbdy, p, nm);
    setBody( body_ );

    grpobj_->body_->setPrntBody( body_ );

    p->manageChld( *grpobj_, *grpobj_->body_ );
    displayFrame( false );
}


uiButtonGroup::~uiButtonGroup()
{
    if ( grpobj_ ) { grpobj_->uibutgrp_ = 0; delete grpobj_; }
	if ( body_ )
	{
	    uiButtonGroupParentBody* bd = body_;
	    body_ = 0;
	    delete bd;
	}
}


void uiButtonGroup::selectButton( int id )
{
    grpobj_->body_->setButton( id );
}


int uiButtonGroup::selectedId() const
{
    QAbstractButton* selbut = grpobj_->body_->selected();
    return grpobj_->body_->id( selbut );
}


int uiButtonGroup::nrButtons() const
{
    return grpobj_->body_->count();
}


void uiButtonGroup::setSensitive( int id, bool yn )
{
    QAbstractButton* but = grpobj_->body_->find( id );
    if ( but ) but->setEnabled( yn );
}


void uiButtonGroup::displayFrame( bool yn )
{ grpobj_->body_->setFlat( !yn ); }


bool uiButtonGroup::isFrameDisplayed() const
{ return grpobj_->body_->isFlat(); }


void uiButtonGroup::setRadioButtonExclusive( bool yn )
{ grpobj_->body_->setRadioButtonExclusive( yn ); }


bool uiButtonGroup::isRadioButtonExclusive() const
{ return grpobj_->body_->isRadioButtonExclusive(); }
