#ifndef uiparentbody_h
#define uiparentbody_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: uiparentbody.h,v 1.12 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uilayout.h"
#include "sets.h"
#include "uiobj.h"
#include "uibody.h"

#include "uigroup.h"

class uiParentBody : public uiBody, public UserIDObject
{
friend class uiObjectBody;
public:
				//uiParentBody( const char* nm = "uiParentBody")
				uiParentBody( const char* nm )
				    : UserIDObject( nm )
				    , finalised_( false )
				    , destructing( 0xdeadbeef )
				{}

    virtual			~uiParentBody()	
				{
				    destructing = 1;
				    deepErase( children );
				}

    virtual void		addChild( uiObjHandle& child )
				    { 
					if( children.indexOf(&child ) < 0 )
					{
					    children += &child; 

					    child.deleteNotify( mCB(this,
							uiParentBody,childDel));
					}
				    }

				//! child becomes mine.
    void			manageChld( uiObjHandle& child, uiObjectBody& b)
				{
				    addChild( child );
				    manageChld_(child,b);
				}

    virtual void		attachChild ( constraintType tp, 
					      uiObject* child, 
					      uiObject* other, int margin,
					      bool reciprocal ) =0;

    bool			finalised() const	{ return finalised_; }
    virtual void 		finalise()		
				{ 
				    finaliseChildren(); 
				}
    void      			finaliseChildren();	// body: uiobj.cc
    void      			clearChildren();	// body: uiobj.cc

				//! widget to be used as parent for QWidgets
    inline const QWidget*	managewidg() const	{ return managewidg_();}
				//! widget to be used as parent for QWidgets
    inline QWidget*		managewidg()
                                   {return const_cast<QWidget*>(managewidg_());}

protected:

    void			childDel( CallBacker* cb )
				{
				    if( destructing != 0xdeadbeef ) return;
				    uiObjHandle* obj =
					    static_cast<uiObjHandle*>( cb );
				    if( obj ) children -= obj;
				}

    virtual const QWidget*	managewidg_() const		=0;
    virtual void		manageChld_(uiObjHandle&, uiObjectBody& ){}

    ObjectSet<uiObjHandle>		children;



private:

    bool			finalised_;
    int				destructing;
//    bool			restored_position;

};

#endif
