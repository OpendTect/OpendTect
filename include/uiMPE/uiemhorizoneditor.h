#ifndef uiemhorizoneditor_h
#define uiehorizonmeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		July 2005
 RCS:		$Id: uiemhorizoneditor.h,v 1.1 2005-07-15 13:54:57 cvskris Exp $
________________________________________________________________________

-*/

#include "menuhandler.h"
#include "uimpe.h"


class uiMPEPartServer;


namespace MPE
{
class HorizonEditor;

class uiEMHorizonEditor : public uiEMEditor
{

public:
    static void		initClass();
    static uiEMEditor*	create(uiParent*, MPE::ObjectEditor* );

    			uiEMHorizonEditor( uiParent*, MPE::HorizonEditor* );

    virtual void	createNodeMenus(CallBacker*);
    virtual void	handleNodeMenus(CallBacker*);

protected:
    MenuItem		editsettingsmnuitem;

    MPE::HorizonEditor*	editor;
};

};


#endif
