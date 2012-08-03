#ifndef uiemobjeditor_h
#define uiehobjeditor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uiemobjeditor.h,v 1.4 2012-08-03 13:01:02 cvskris Exp $
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "menuhandler.h"
#include "uimpe.h"


class uiMPEPartServer;


namespace MPE
{
class HorizonEditor;

mClass(uiMPE) uiEMObjectEditor : public uiEMEditor
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

