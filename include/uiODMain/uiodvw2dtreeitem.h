#ifndef uiodvw2dtreeitem_h
#define uiodvw2dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: uiodvw2dtreeitem.h,v 1.4 2011/06/28 13:35:43 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uitreeitemmanager.h"

class CubeSampling;
class uiListView;
class uiODApplMgr;
class uiODViewer2D;

namespace Attrib { class SelSpec; }


mClass uiODVw2DTreeItem : public uiTreeItem
{
public:
    			uiODVw2DTreeItem(const char*);
			~uiODVw2DTreeItem(){}

    void		updCubeSamling(const CubeSampling&,bool);
    void		updSelSpec(const Attrib::SelSpec*,bool wva);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static bool		create(uiTreeItem*,int vwrvisid,int displayid);
    static bool		create(uiTreeItem*,const uiODViewer2D&,int displayid);

protected:

    int				displayid_;

    uiODApplMgr*		applMgr();
    uiODViewer2D*		viewer2D();
    virtual void		updateCS(const CubeSampling&,bool)	{}
    virtual void		updateSelSpec(const Attrib::SelSpec*,bool wva)
				{}
};


mClass uiODVw2DTreeItemFactory : public uiTreeItemFactory
{
    public:
	virtual uiTreeItem* createForVis(const uiODViewer2D&,int visid) const
		                                { return 0; }
};



mClass uiODVw2DTreeTop : public uiTreeTopItem
{
public:
    				uiODVw2DTreeTop(uiListView*,uiODApplMgr*,
					uiODViewer2D*,uiTreeFactorySet*);
				~uiODVw2DTreeTop();
    
    static const char*  	viewer2dptr();
    static const char*  	applmgrstr();

    void			updCubeSamling(const CubeSampling&,bool);
    void			updSelSpec(const Attrib::SelSpec*,bool wva);

protected:

    void			addFactoryCB(CallBacker*);
    void			removeFactoryCB(CallBacker*);

    virtual const char* 	parentType() const { return 0; }
    uiODApplMgr*        	applMgr();
    uiODViewer2D*               viewer2D();

    uiTreeFactorySet*   	tfs_;
    bool			selectWithKey(int);
};


#endif
