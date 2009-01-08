#ifndef uiemobjeditor_h
#define uiehobjeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uiemobjeditor.h,v 1.2 2009-01-08 09:11:18 cvsranojay Exp $
________________________________________________________________________

-*/

#include "menuhandler.h"
#include "uimpe.h"


class uiMPEPartServer;


namespace MPE
{
class HorizonEditor;

mClass uiEMObjectEditor : public uiEMEditor
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
