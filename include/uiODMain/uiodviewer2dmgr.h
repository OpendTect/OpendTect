#ifndef uiodviewer2dmgr_h
#define uiodviewer2dmgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "callback.h"

#include "uiodapplmgr.h"

class uiODViewer2D;
class uiTreeFactorySet;


mClass(uiODMain) uiODViewer2DMgr : public CallBacker
{
public:
    uiODViewer2D*		find2DViewer(int visid);

    void			displayIn2DViewer(int visid,int attribid,
	    					  bool wva);
    void			remove2DViewer(int visid);

    uiTreeFactorySet*		treeItemFactorySet2D()	{ return tifs2d_; }
    uiTreeFactorySet*		treeItemFactorySet3D()	{ return tifs3d_; }

    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }

    static const char*		sKeyVisID()		{ return "VisID"; }
    static const char*		sKeyAttrID()		{ return "Attrib ID"; }
    static const char*		sKeyWVA()		{ return "WVA"; }

protected:

    				uiODViewer2DMgr(uiODMain*);
				~uiODViewer2DMgr();

    uiODViewer2D&		addViewer2D(int visid);
    ObjectSet<uiODViewer2D>     viewers2d_;

    uiTreeFactorySet*		tifs2d_;
    uiTreeFactorySet*		tifs3d_;

    uiODMain&			appl_;

    inline uiODApplMgr&         applMgr()     { return appl_.applMgr(); }
    inline uiVisPartServer&     visServ()     { return *applMgr().visServer(); }

    void			viewer2DWinClosedCB(CallBacker*);

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    friend class                uiODMain;
};


#endif

