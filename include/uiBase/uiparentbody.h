#ifndef uiparentbody_h
#define uiparentbody_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: uiparentbody.h,v 1.6 2002-08-14 10:30:02 arend Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uilayout.h"
#include "sets.h"
#include "uiobj.h"
#include "uibody.h"

#include "uigroup.h"

class uiParentBody : public uiBody
{
friend class uiObjectBody;
public:
				uiParentBody()
				    : finalised( false )
				{}

    virtual			~uiParentBody()		{ deepErase(children);}

    virtual void		addChild( uiObjHandle& child )
				    { 
					if( children.indexOf(&child ) < 0 )
					    children += &child; 
				    }

				//! child becomes mine.
    void			manageChld( uiObjHandle& child, uiObjectBody& b)
				{
				    addChild( child );
				    manageChld_(child,b);
				}

    virtual void		attachChild ( constraintType tp, 
					      uiObject* child, 
					      uiObject* other, int margin ) =0;

    virtual void 		finalise(bool t=false)		
				{ 
				    finaliseChildren(); 
				    restorePosition(); 
				}
    void      			finaliseChildren();	// body: uiobj.cc
    void      			clearChildren();	// body: uiobj.cc

				//! widget to be used as parent for QWidgets
    inline const QWidget*	managewidg() const	{ return managewidg_();}
				//! widget to be used as parent for QWidgets
    inline QWidget*		managewidg()
                                   {return const_cast<QWidget*>(managewidg_());}

    void			storePosition();
    void			restorePosition();

protected:

    virtual const QWidget*	managewidg_() const		=0;
    virtual void		manageChld_(uiObjHandle&, uiObjectBody& ){}

    ObjectSet<uiObjHandle>		children;

private:

    bool			finalised;
//    bool			restored_position;

};

#endif
