#ifndef uiparent_h
#define uiparent_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id: uiparent.h,v 1.5 2002-08-14 10:30:02 arend Exp $
________________________________________________________________________

-*/

#include <uidobj.h>
#include <uihandle.h>
#include <uilayout.h>

class uiObjHandle;
class uiObjectBody;
class uiObject;
class uiMainWin;


class uiParent : public uiObjHandle
{
friend class uiParentBody;
public:
			uiParent( const char* nm, uiParentBody* );

    void		addChild( uiObjHandle& );
    void		manageChld( uiObjHandle&, uiObjectBody& );
    void                attachChild ( constraintType tp, uiObject* child,
				      uiObject* other, int margin );

			//! persists current widget position
    void		storePosition(CallBacker* cb=0);

    virtual uiMainWin*	mainwin()		{ return 0; }

    uiParentBody*	pbody();
};

#endif
