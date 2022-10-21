#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uibody.h"
#include "uigroup.h"
#include "uilayout.h"
#include "uiobj.h"
#include "uiparent.h"

mClass(uiBase) uiParentBody : public uiBody, public NamedCallBacker
{
friend class uiObjectBody;
public:
    virtual		~uiParentBody();

    virtual void	addChild(uiBaseObject& child);

			//! child becomes mine.
    void		manageChild(uiBaseObject& child,uiObjectBody&);
    virtual void	attachChild(constraintType,uiObject* child,
				    uiObject* other,int margin,
				    bool reciprocal) =0;
    const ObjectSet<uiBaseObject>*
			childList() const		{ return &children_; }

    bool		finalized() const override	{ return finalized_; }
    void		finalize() override		{ finalizeChildren(); }
    void		finalizeChildren();
    void		clearChildren();

			//! widget to be used as parent for QWidgets
    inline const mQtclass(QWidget*) managewidg() const	{ return managewidg_();}
			//! widget to be used as parent for QWidgets
    inline mQtclass(QWidget*)	managewidg()
		    { return const_cast<mQtclass(QWidget*)>( managewidg_() ); }

protected:
			uiParentBody(const char* nm);

    void		deleteAllChildren();
    void		childDel(CallBacker*);

    virtual const mQtclass(QWidget*)	managewidg_() const = 0;
    virtual void		manageChld_(uiBaseObject&,uiObjectBody&) {}

    ObjectSet<uiBaseObject>	children_;

private:

    bool			finalized_		= false;
};


mExpClass(uiBase) uiCentralWidgetBody : public uiParentBody
{
public:
    virtual		~uiCentralWidgetBody();

    uiGroup*		uiCentralWidg() { return centralwidget_; }
    void		addChild(uiBaseObject&) override;
    void		attachChild(constraintType,uiObject* child,
				    uiObject* other,int margin,
				    bool reciprocal) override;

protected:
			uiCentralWidgetBody(const char* nm);

    void		manageChld_(uiBaseObject&,uiObjectBody&) override;
    const QWidget*	managewidg_() const override;

    bool		initing_;
    uiGroup*		centralwidget_;
};
