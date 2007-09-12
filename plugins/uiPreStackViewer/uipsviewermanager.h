#ifndef psviewermanager_h
#define psviewermanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2007
 RCS:		$Id: uipsviewermanager.h,v 1.1 2007-09-12 16:04:33 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "menuhandler.h"

class uiVisPartServer;

namespace PreStackView
{

class PreStackViewer;
/*!Manages psviewers in the 3d visualization. */
class uiPSViewerMgr : public CallBacker
{
public:
				uiPSViewerMgr();
				~uiPSViewerMgr();

protected:
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
    void			removeAllCB(CallBacker*);
    void			sceneChangeCB(CallBacker*);

    ObjectSet<PreStackViewer>	viewers_;
    MenuItem			selectpsdatamenuitem_;
    MenuItem			removemenuitem_;
    MenuItem			proptymenuitem_;
    uiVisPartServer*		visserv_;
};

}; //namespace

#endif
