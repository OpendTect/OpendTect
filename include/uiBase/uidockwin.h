#ifndef uidockwin_h
#define uidockwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.h,v 1.1 2002-02-13 16:33:37 arend Exp $
________________________________________________________________________

-*/

#include <uiparent.h>
#include <uihandle.h>

class uiDockWinBody;
class uiObject;
class uiGroup;

class uiDockWin : public uiParent
{

public:
			uiDockWin( uiParent* parnt=0, 
				   const char* nm="uiDockWin" );

    virtual		~uiDockWin();


    uiObject*		uiObj();
    const uiObject*	uiObj() const;

    uiGroup* 		topGroup();

protected:

    uiDockWinBody*	body_;
};

#endif
