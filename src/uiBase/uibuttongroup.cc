/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id: uibuttongroup.cc,v 1.2 2001-08-27 12:58:50 windev Exp $
________________________________________________________________________

-*/

#include <uibuttongroup.h>
#include <uiobjbody.h>
#include <i_layout.h>
#include <i_layoutitem.h>
#include <qbuttongroup.h>

#include <iostream>
#include "errh.h"

class uiButtonGroupObjBody;
class uiButtonGroupParentBody;


class uiButtonGroupObjBody : public uiObjectBody  , public QButtonGroup
{
public:
			uiButtonGroupObjBody(uiButtonGroupObj& handle, 
					uiParent* parnt, const char* txt, 
                                        bool vertical, int strips )
			: uiObjectBody( parnt )
			, QButtonGroup( strips, 

//					vertical ? Vertical : Horizontal, txt,
// Qt seems to have a different notion of "Horizontal" then I have....

					vertical ? Horizontal : Vertical, txt,
					parnt && parnt->body() ?
					  parnt->body()->managewidg() : 0, txt )
			, handle_( handle )
			{}

    virtual		~uiButtonGroupObjBody()	{}

#define mHANDLE_OBJ	uiButtonGroupObj
#define mQWIDGET_BASE	QWidget
#define mQWIDGET_BODY	QButtonGroup
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
                            : uiParentBody()
                            , handle_( handle )
			    , objbody_( objbdy )
			{}

virtual void            attachChild ( constraintType tp,
                                              uiObject* child,
                                              uiObject* other, int margin )
			{ pErrMsg("Cannot do attachments in uiButtonGroups "); }

protected:

    uiButtonGroup&		handle_;

    virtual const QWidget* qwidget_() const    { return objbody_.qwidget(); }
    virtual const QWidget* managewidg_() const { return objbody_.managewidg();}

private:

    uiButtonGroupObjBody&	objbody_;

};


uiButtonGroup::uiButtonGroup( uiParent* p, const char* nm,
			      bool vertical, int strips )
    : uiParent( nm, 0 )
    , grpobj_( 0 )
    , body_( 0 )
{
    grpobj_ =  new uiButtonGroupObj( p,nm,vertical,strips );
    uiButtonGroupObjBody* grpbdy = 
	    dynamic_cast<uiButtonGroupObjBody*>( grpobj_->body() );

#ifdef __debug__
    if( !grpbdy ) { pErrMsg("Huh") ; return; }
#endif

    body_ =  new uiButtonGroupParentBody(*this,*grpbdy, p, nm);
    setBody( body_ );

    grpobj_->body_->setPrntBody( body_ );

    p->manageChld( *grpobj_, *grpobj_->body_ );
}


uiButtonGroupObj:: uiButtonGroupObj( uiParent* parnt, const char* nm,
                                          bool vertical, int strips )
: uiObject( parnt, nm )
{
    body_= new uiButtonGroupObjBody( *this, parnt, nm, vertical, strips);
    setBody( body_ );
}

