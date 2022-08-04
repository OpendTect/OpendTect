#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/06/2001
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uilayout.h"
#include "sets.h"
#include "uiobj.h"
#include "uibody.h"

#include "uigroup.h"

mClass(uiBase) uiParentBody : public uiBody, public NamedCallBacker
{
friend class uiObjectBody;
public:
    virtual		~uiParentBody()
			{ sendDelNotif(); deleteAllChildren(); }

    virtual void	addChild( uiBaseObject& child )
			{
			    if ( children_.isPresent(&child ) )	return;
			    children_ += &child;
			    mAttachCB( child.objectToBeDeleted(),
				       uiParentBody::childDel );
			}

			//! child becomes mine.
    void		manageChld( uiBaseObject& child, uiObjectBody& b)
			{
			    addChild( child );
			    manageChld_(child,b);
			}

    virtual void	attachChild( constraintType tp, uiObject* child,
				     uiObject* other, int margin,
				     bool reciprocal ) =0;

    const ObjectSet<uiBaseObject>* childList() const	{ return &children_; }

    bool		finalized() const override	{ return finalized_; }
    void		finalize() override		{ finalizeChildren(); }
    void		finalizeChildren();	// body: uiobj.cc
    void		clearChildren();	// body: uiobj.cc

			//! widget to be used as parent for QWidgets
    inline const mQtclass(QWidget*) managewidg() const	{ return managewidg_();}
			//! widget to be used as parent for QWidgets
    inline mQtclass(QWidget*)	managewidg()
		    { return const_cast<mQtclass(QWidget*)>( managewidg_() ); }

protected:
			uiParentBody( const char* nm )
			    : NamedCallBacker( nm )
			    , finalized_( false )
			{}

    void	deleteAllChildren()
		{
		    //avoid the problems from childDel() removal from
		    //children_
		    ObjectSet<uiBaseObject> childrencopy = children_;
		    children_.erase();
		    deepErase( childrencopy );
		}

    void	childDel( CallBacker* cb )
		{
		    uiBaseObject* obj = static_cast<uiBaseObject*>( cb );
		    if ( obj ) children_ -= obj;
		}

    virtual const mQtclass(QWidget*)	managewidg_() const = 0;
    virtual void		manageChld_(uiBaseObject&,uiObjectBody&) {}

    ObjectSet<uiBaseObject>	children_;

private:

    bool			finalized_;
};


mExpClass(uiBase) uiCentralWidgetBody : public uiParentBody
{
public:
    virtual		~uiCentralWidgetBody();

    uiGroup*		uiCentralWidg() { return centralwidget_; }
    void		addChild(uiBaseObject&) override;
    void		manageChld_(uiBaseObject&,uiObjectBody&) override;
    void		attachChild(constraintType,uiObject* child,
				    uiObject* other,int margin,
				    bool reciprocal) override;

protected:
			uiCentralWidgetBody(const char* nm);

    const QWidget*	managewidg_() const override;

    bool		initing_;
    uiGroup*		centralwidget_;
};

