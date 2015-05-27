#ifndef uibasemap_h
#define uibasemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "basemap.h"
#include "uigroup.h"


class uiGraphicsItemGroup;
class uiGraphicsScene;
class uiGraphicsView;
class uiWorld2Ui;

mExpClass(uiTools) uiBaseMapObject : public CallBacker
{
public:
				uiBaseMapObject(BaseMapObject*);
    virtual			~uiBaseMapObject();

    BaseMapObject*		getObject();

    bool			hasChanged() const	{ return changed_; }
    void			resetChangeFlag() { changed_ = false; }
    void			setTransform(const uiWorld2Ui*);
    const char*			name() const;
    virtual void		show(bool yn);
    virtual bool		isShown() const;

    uiGraphicsItemGroup&	itemGrp()		{ return itemgrp_; }
    const uiGraphicsItemGroup&	itemGrp() const		{ return itemgrp_; }
    virtual void		update();
    virtual void		updateStyle();

protected:
    friend			class uiBaseMap;

    void			changedCB(CallBacker*);
    void			changedStyleCB(CallBacker*);

    uiGraphicsItemGroup&	itemgrp_;
    const uiWorld2Ui*		transform_;

    bool			changed_;
    BaseMapObject*		bmobject_;
};


mExpClass(uiTools) uiBaseMap : public BaseMap, public uiGroup
{
public:
				uiBaseMap(uiParent*);
    virtual			~uiBaseMap();

    void			setView(const uiWorldRect&);

    virtual void		addObject(BaseMapObject*);
    void			addStaticObject(BaseMapObject*);
    BaseMapObject*		getObject(int id);

    bool			hasChanged();
    inline void			setChangeFlag() { changed_ = true; }
    void			resetChangeFlag();
				//!Owned by caller
    void			removeObject(const BaseMapObject*);
    void			show(const BaseMapObject&,bool yn);

    void			addObject(uiBaseMapObject*);
				//! object becomes mine, obviously.
    void			addStaticObject(uiBaseMapObject*);

    const char*			nameOfItemAt(const Geom::Point2D<int>&) const;

    uiGraphicsItemGroup&	worldItemGroup()	{ return worlditemgrp_;}
    inline uiGraphicsView&	view()			{ return view_; }
    uiGraphicsScene&		scene();
    inline const uiWorld2Ui&	getWorld2Ui() const	{ return w2ui_; }

    CNotifier<uiBaseMap,int>	objectAdded;
    CNotifier<uiBaseMap,int>	objectRemoved;

protected:

    int				indexOf(const BaseMapObject*) const;

    uiGraphicsView&		view_;
    uiGraphicsItemGroup&	worlditemgrp_;
    uiGraphicsItemGroup&	staticitemgrp_;
    ObjectSet<uiBaseMapObject>	objects_;
    bool			changed_;

    uiWorldRect			wr_;

    void			reSizeCB(CallBacker*);
    virtual void		reDraw(bool deep=true);
    void			updateTransform();

private:
    uiWorld2Ui&			w2ui_;
};

#endif
