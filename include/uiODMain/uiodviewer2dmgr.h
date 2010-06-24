#ifndef uiodviewer2dmgr_h
#define uiodviewer2dmgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: uiodviewer2dmgr.h,v 1.1 2010-06-24 08:54:11 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"

#include "uiodapplmgr.h"

class uiODViewer2D;
class uiTreeFactorySet;


mClass uiODViewer2DMgr : public CallBacker
{
public:
    void			displayIn2DViewer(int visid,int attribid,
	    					  bool wva);
    void			remove2DViewer(int visid);

    uiTreeFactorySet*		treeItemFactorySet2D()	{ return tifs2d_; }
    uiTreeFactorySet*		treeItemFactorySet3D()	{ return tifs3d_; }

protected:

    				uiODViewer2DMgr(uiODMain*);
				~uiODViewer2DMgr();

    uiODViewer2D&		addViewer2D(int visid);
    uiODViewer2D*		find2DViewer(int visid);

    ObjectSet<uiODViewer2D>     viewers2d_;

    uiTreeFactorySet*		tifs2d_;
    uiTreeFactorySet*		tifs3d_;

    uiODMain&			appl_;

    inline uiODApplMgr&         applMgr()     { return appl_.applMgr(); }
    inline uiVisPartServer&     visServ()     { return *applMgr().visServer(); }


    friend class                uiODMain;
};


#endif
