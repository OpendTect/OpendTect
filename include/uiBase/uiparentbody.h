#ifndef uiparentbody_h
#define uiparentbody_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: uiparentbody.h,v 1.20 2012-08-30 05:49:34 cvsnageswara Exp $
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

    virtual void	addChild( uiBaseObject& child )
			{ 
			    if ( children_.indexOf(&child )!=-1 )
				return;

			    children_ += &child; 
			    child.deleteNotify(mCB(this,uiParentBody,childDel));
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

    bool		finalised() const	{ return finalised_; }
    virtual void 	finalise()		{ finaliseChildren(); }
    void      		finaliseChildren();	// body: uiobj.cc
    void      		clearChildren();	// body: uiobj.cc

			//! widget to be used as parent for QWidgets
    inline const mQtclass(QWidget*) managewidg() const	{ return managewidg_();}
			//! widget to be used as parent for QWidgets
    inline mQtclass(QWidget*)	managewidg()
    		    { return const_cast<mQtclass(QWidget*)>( managewidg_() ); }

protected:
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

    bool			finalised_;
};

#endif
