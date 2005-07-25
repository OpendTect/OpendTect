#ifndef uiemobjeditor_h
#define uiehobjeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uiemobjeditor.h,v 1.1 2005-07-25 12:23:22 cvskris Exp $
________________________________________________________________________

-*/

#include "menuhandler.h"
#include "uimpe.h"


class uiMPEPartServer;


namespace MPE
{
class HorizonEditor;

class uiEMObjectEditor : public uiEMEditor
{

public:
    			uiEMObjectEditor( uiParent*, MPE::ObjectEditor* );

    virtual void	createNodeMenus(CallBacker*);
    virtual void	handleNodeMenus(CallBacker*);

protected:
    MenuItem		snapmenuitem;

    MPE::ObjectEditor*	editor;
};

};


#endif
