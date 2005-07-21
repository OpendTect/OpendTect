#ifndef uiemhorizoneditor_h
#define uiehorizonmeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uiemhorizoneditor.h,v 1.2 2005-07-21 20:59:14 cvskris Exp $
________________________________________________________________________

-*/

#include "menuhandler.h"
#include "uiemobjeditor.h"


class uiMPEPartServer;


namespace MPE
{
class HorizonEditor;

class uiEMHorizonEditor : public uiEMObjectEditor
{

public:
    static void		initClass();
    static uiEMEditor*	create(uiParent*, MPE::ObjectEditor* );

    			uiEMHorizonEditor( uiParent*, MPE::HorizonEditor* );

    virtual void	createNodeMenus(CallBacker*);
    virtual void	handleNodeMenus(CallBacker*);

    MPE::HorizonEditor*	getEditor();

protected:
    MenuItem		editsettingsmnuitem;
};

};


#endif
