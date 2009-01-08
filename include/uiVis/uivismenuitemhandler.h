#ifndef uivismenuitemhandler_h
#define uivismenuitemhandler_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uivismenuitemhandler.h,v 1.2 2009-01-08 10:37:54 cvsranojay Exp $
________________________________________________________________________


-*/

#include "menuhandler.h"

class uiVisPartServer;

/*!Adds a menu-item to a visual object's right-click menu. Menu item
will be added to all visual objects of a certain type.

Usage example:
\code
    uiVisMenuItemHandler( visSurvey::WellDisplay::getStaticClassName(),
		  vispartserver_, "My menu text, mCB(this,MyClass,myFuncCB) );
\endcode
*/


mClass uiVisMenuItemHandler : public MenuItemHandler
{
public:
    		uiVisMenuItemHandler(const char* classnm,uiVisPartServer&,
				     const char* mnutext, const CallBack& cb,
				     int placement = -1);

    int		getDisplayID() const;
    		/*!<Does only give a valid answer if called from cb. */

protected:
    bool		shouldAddMenu() const;

    const char*		classnm_;
    uiVisPartServer&	visserv_;
};



#endif
