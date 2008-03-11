#ifndef uiparentbody_h
#define uiparentbody_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: uiparentbody.h,v 1.15 2008-03-11 20:49:44 cvskris Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uilayout.h"
#include "sets.h"
#include "uiobj.h"
#include "uibody.h"

#include "uigroup.h"

class uiParentBody : public uiBody, public NamedObject
{
friend class uiObjectBody;
public:
			//uiParentBody( const char* nm = "uiParentBody")
			uiParentBody( const char* nm )
			    : NamedObject( nm )
			    , finalised_( false )
			{}

    virtual		~uiParentBody()		{ deleteAllChildren(); }

    virtual void	addChild( uiObjHandle& child )
			{ 
			    if ( children_.indexOf(&child )!=-1 )
				return;

			    children_ += &child; 
			    child.deleteNotify(mCB(this,uiParentBody,childDel));
			}

			//! child becomes mine.
    void		manageChld( uiObjHandle& child, uiObjectBody& b)
			{
			    addChild( child );
			    manageChld_(child,b);
			}

    virtual void	attachChild( constraintType tp, uiObject* child, 
				     uiObject* other, int margin,
				     bool reciprocal ) =0;

    const ObjectSet<uiObjHandle>* childList() const	{ return &children_; }

    bool		finalised() const	{ return finalised_; }
    virtual void 	finalise()		{ finaliseChildren(); }
    void      		finaliseChildren();	// body: uiobj.cc
    void      		clearChildren();	// body: uiobj.cc

			//! widget to be used as parent for QWidgets
    inline const QWidget* managewidg() const	{ return managewidg_();}
			//! widget to be used as parent for QWidgets
    inline QWidget*	managewidg()
    			{ return const_cast<QWidget*>( managewidg_() ); }

protected:
    void	deleteAllChildren()
		{
		    //avoid the problems from childDel() removal from
		    //children_
		    ObjectSet<uiObjHandle> childrencopy = children_;
		    children_.erase(); 
		    deepErase( childrencopy );
		}

    void	childDel( CallBacker* cb )
		{
		    uiObjHandle* obj = static_cast<uiObjHandle*>( cb );
		    if ( obj ) children_ -= obj;
		}

    virtual const QWidget*	managewidg_() const			= 0;
    virtual void		manageChld_(uiObjHandle&,uiObjectBody&)	{}

    ObjectSet<uiObjHandle>	children_;

private:

    bool			finalised_;
};

#endif
