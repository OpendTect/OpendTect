/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uiparentbody.h"

#include "uimain.h"

mUseQtnamespace

// uiParent
uiParent::uiParent( const char* nm, uiParentBody* b )
    : uiBaseObject( nm, b )
{}


uiParent::~uiParent()
{}


uiParentBody* uiParent::pbody()
{
    return dCast(uiParentBody*,body());
}


void uiParent::addChild( uiBaseObject& child )
{
    mDynamicCastGet(uiBaseObject*,thisuiobj,this);
    if ( thisuiobj && &child == thisuiobj )
	return;

    if ( !body() )
    {
	pErrMsg("uiParent has no body!");
	return;
    }

    auto* pb = dCast(uiParentBody*,body());
    if ( !pb )
    {
	pErrMsg("uiParent has a body, but it's no uiParentBody");
	return;
    }

    pb->addChild( child );
}


void uiParent::manageChild( uiBaseObject& child, uiObjectBody& bdy )
{
    if ( &child == static_cast<uiBaseObject*>(this) )
	return;

    auto* pb = dCast(uiParentBody*,body());
    if ( !pb )
	return;

    pb->manageChild( child, bdy );
}


void uiParent::attachChild ( constraintType tp, uiObject* child,
			     uiObject* other, int margin, bool reciprocal )
{
    if ( child == static_cast<uiBaseObject*>(this) )
	return;

    if ( !body() )
    {
	pErrMsg("uiParent has no body!");
	return;
    }

    auto* pb = dCast(uiParentBody*,body());
    if ( !pb )
    {
	pErrMsg("uiParent has a body, but it's no uiParentBody");
	return;
    }

    pb->attachChild ( tp, child, other, margin, reciprocal );
}


const ObjectSet<uiBaseObject>* uiParent::childList() const
{
    auto* pb = dCast(const uiParentBody*,body());
    return pb ? pb->childList(): nullptr;
}


OD::Color uiParent::backgroundColor() const
{
    return mainObject() ? mainObject()->backgroundColor()
			: uiMain::instance().windowColor();
}


void uiParent::translateText()
{
    uiBaseObject::translateText();

    if ( !childList() )
	return;

    for ( auto* child : *childList() )
    {
	//Workaround for missing function on uiGroupObj
	mDynamicCastGet(uiGroupObj*,groupobj,child)
	if ( groupobj && groupobj->group() )
	    groupobj->group()->translateText();

	child->translateText();
    }
}


// uiParentBody
uiParentBody::uiParentBody( const char* nm )
    : NamedCallBacker(nm)
{}


uiParentBody::~uiParentBody()
{
    sendDelNotif();
    detachAllNotifiers();
    deleteAllChildren();
}


void uiParentBody::addChild( uiBaseObject& child )
{
    if ( children_.isPresent(&child) )
	return;

    children_ += &child;
    mAttachCB( child.objectToBeDeleted(), uiParentBody::childDel );
}


void uiParentBody::manageChild( uiBaseObject& child, uiObjectBody& body )
{
    addChild( child );
    manageChld_( child, body );
}


void uiParentBody::finalizeChildren()
{
    if ( !finalized_ )
    {
	finalized_ = true;
	for ( auto* child : children_ )
	    child->finalize();
    }
}


void uiParentBody::clearChildren()
{
    for ( auto* child : children_ )
	child->clear();
}


void uiParentBody::deleteAllChildren()
{
    //avoid the problems from childDel() removal from
    //children_
    ObjectSet<uiBaseObject> childrencopy = children_;
    children_.erase();
    deepErase( childrencopy );
}


void uiParentBody::childDel( CallBacker* cb )
{
    uiBaseObject* obj = static_cast<uiBaseObject*>( cb );
    if ( obj )
	children_ -= obj;
}



// uiCentralWidgetBody
uiCentralWidgetBody::uiCentralWidgetBody( const char* nm )
    : uiParentBody(nm)
    , initing_(true)
    , centralwidget_(nullptr)
{}


uiCentralWidgetBody::~uiCentralWidgetBody()
{}


void uiCentralWidgetBody::addChild( uiBaseObject& child )
{
    if ( !initing_ && centralwidget_ )
	centralwidget_->addChild( child );
    else
	uiParentBody::addChild( child );
}


void uiCentralWidgetBody::manageChld_( uiBaseObject& o, uiObjectBody& b )
{
    if ( !initing_ && centralwidget_ )
	centralwidget_->manageChild( o, b );
}


void uiCentralWidgetBody::attachChild ( constraintType tp,
					uiObject* child,
					uiObject* other, int margin,
					bool reciprocal )
{
    if ( !centralwidget_ || !child || initing_ )
	return;

    centralwidget_->attachChild( tp, child, other, margin, reciprocal);
}


const QWidget* uiCentralWidgetBody::managewidg_() const
{
    return initing_ ? qwidget_() : centralwidget_->pbody()->managewidg();
}
