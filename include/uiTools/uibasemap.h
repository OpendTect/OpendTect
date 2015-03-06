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
class uiGraphicsView;
class uiWorld2Ui;

mExpClass(uiTools) uiBaseMapObject : public CallBacker
{
public:
    				uiBaseMapObject(BaseMapObject*);
    virtual			~uiBaseMapObject();

    bool			hasChanged() const	{ return changed_; }
    void			resetChangeFlag() { changed_ = false; }
    const char*			name() const;
    void			setTransform(const uiWorld2Ui*);
    virtual void		show(bool yn);

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

    void			addObject(BaseMapObject*);
    bool			hasChanged();
    inline void			setChangeFlag() { changed_ = true; }
    void			resetChangeFlag();
    				//!Owned by caller
    void			removeObject(const BaseMapObject*);
    void			show(const BaseMapObject&,bool yn);

    void			addObject(uiBaseMapObject*);
    				//! object becomes mine, obviously.

    const char*			nameOfItemAt(const Geom::Point2D<int>&) const;

    inline uiGraphicsView&	view()			{ return view_; }
    inline const uiWorld2Ui&	transform() const	{ return w2ui_; }

protected:

    int				indexOf(const BaseMapObject*) const;

    uiGraphicsView&		view_;
    ObjectSet<uiBaseMapObject>	objects_;
    bool			changed_;

    uiWorld2Ui&			w2ui_;

    void			reSizeCB(CallBacker*)		{ reDraw(); }
    virtual void		reDraw(bool deep=true);
};

#endif
