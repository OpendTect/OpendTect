#ifndef uibasemapwin_h
#define uibasemapwin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"

#include "uimainwin.h"

class uiBasemapTreeTop;
class uiDockWin;
class uiSurveyMap;
class uiToolBar;
class uiTreeView;

mClass(uiBasemap) uiBasemapWin : public uiMainWin
{
public:
    			uiBasemapWin(uiParent*);
			~uiBasemapWin();

protected:

private:
    void		initWin(CallBacker*);
    void		initToolBar();
    void		initTree();
    void		iconClickCB(CallBacker*);
    bool		closeOK();

    uiSurveyMap*	basemapview_;
    uiToolBar*		vwtoolbar_;
    uiToolBar*		itemtoolbar_;
    uiDockWin*		treedw_;
    uiTreeView*		tree_;
    uiBasemapTreeTop*	topitem_;

    TypeSet<int>		ids_;
};

#endif

