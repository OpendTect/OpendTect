#ifndef uiodscenemgr_h
#define uiodscenemgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.h,v 1.1 2003-12-20 13:24:05 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class uiGroup;
class uiODMain;
class uiDockWin;
class uiSoViewer;
class uiListView;
class uiWorkSpace;
class uiODTreeTop;
class uiTreeFactorySet;


/*!\brief Position control elements at application top level */

class uiODSceneMgr
{
public:

			uiODSceneMgr(uiODMain*);

    void		addScene();
    void		mkScenesFrom(ODSession*);
    void		removeScene(CallBacker*);
    void		setZoomValue(float);


protected:

    class Scene
    {
    public:
			Scene(uiWorkSpace*);
			~Scene();
       
	uiListView*	lv;
	uiDockWin*	treewin;
	uiSoViewer*	sovwr;
	uiODTreeTop*	itemmanager;

	uiGroup*	group();
    };

    uiODMain&		appl;
    uiWorkSpace*	wsp;
    ObjectSet<Scene>	scenes;
    int			vwridx;
    int			lasthrot, lastvrot, lastdval;
    uiTreeFactorySet*	tifs;

    void		initTree(Scene*,int);

};



#endif
