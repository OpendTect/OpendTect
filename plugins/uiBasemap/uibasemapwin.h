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

class uiDockWin;
class uiTreeView;
class uiSurveyMap;

mClass(uiBasemap) uiBasemapWin : public uiMainWin
{
public:
    			uiBasemapWin(uiParent*);
			~uiBasemapWin();

protected:

private:
    void		initWin(CallBacker*);
    bool		closeOK();

    uiSurveyMap*	basemapview_;
    uiDockWin*		treedw_;
    uiTreeView*		tree_;
};

#endif

