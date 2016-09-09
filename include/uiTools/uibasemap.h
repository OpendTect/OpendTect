#ifndef uibasemap_h
#define uibasemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "basemap.h"
#include "uigroup.h"


class uiGraphicsItem;
class uiGraphicsScene;
class uiGraphicsView;
class uiWorld2Ui;

mExpClass(uiTools) uiBaseMapObject : public CallBacker
{
public:
				uiBaseMapObject(BaseMapObject*);
    virtual			~uiBaseMapObject();

    const BaseMapObject*	getObject() const	{ return bmobject_; }
    BaseMapObject*		getObject()		{ return bmobject_; }

    const char*			getType() const;

    bool			hasChanged() const	{ return changed_; }
    void			resetChangeFlag() { changed_ = false; }
    void			setTransform(const uiWorld2Ui*);
    const char*			name() const;
    virtual void		show(bool yn);
    virtual bool		isShown() const;

    uiGraphicsItem&		graphItem()		{ return graphitem_; }
    const uiGraphicsItem&	graphItem() const	{ return graphitem_; }
    uiGraphicsItem&		labelItem()		{ return labelitem_; }

    virtual void		update();
    virtual void		updateStyle();

protected:
    friend			class uiBaseMap;

    void			changedCB(CallBacker*);
    void			changedStyleCB(CallBacker*);
    void			changedZValueCB(CallBacker*);
    void			leftClickCB(CallBacker*);
    void			rightClickCB(CallBacker*);
    void			add(uiGraphicsItem&);

    uiGraphicsItem&		labelitem_;
    uiGraphicsItem&		graphitem_;
    const uiWorld2Ui*		transform_;

    bool			changed_;
    BaseMapObject*		bmobject_;

private:
    void			addLabel(uiGraphicsItem&);
};


mExpClass(uiTools) uiBaseMap : public BaseMap
{
public:
				uiBaseMap(uiParent*);
    virtual			~uiBaseMap();

    void			setView(const uiWorldRect&);

    virtual void		addObject(BaseMapObject*);
    BaseMapObject*		getObject(int id);

    ObjectSet<uiBaseMapObject>& getObjects()		{ return objects_; }

    bool			hasChanged();
    inline void			setChangeFlag()		{ changed_ = true; }
    void			resetChangeFlag();
				//!Owned by caller
    void			removeObject(const BaseMapObject*);
    void			show(const BaseMapObject&,bool yn);

    void			addObject(uiBaseMapObject*);

    const char*			nameOfItemAt(const Geom::Point2D<float>&) const;

    uiGraphicsItem&		worldItem()		{ return worlditem_;}
    void			centerWorldItem(bool);
    inline uiGroup&		attachGroup()		{ return *bmgroup_; }
    inline uiGraphicsView&	view()			{ return *view_; }
    uiGraphicsScene&		scene();
    inline const uiWorld2Ui&	getWorld2Ui() const	{ return w2ui_; }

    CNotifier<uiBaseMap,int>	objectAdded;
    CNotifier<uiBaseMap,int>	objectRemoved;

protected:

    int				indexOf(const BaseMapObject*) const;

    uiGroup*			bmgroup_;
    uiGraphicsView*		view_;
    uiGraphicsItem&		worlditem_;
    ObjectSet<uiBaseMapObject>	objects_;
    bool			changed_;
    bool			centerworlditem_;

    uiWorldRect			wr_;

    void			reSizeCB(CallBacker*);
    virtual void		reDraw(bool deep=true);
    void			updateTransform();

private:
    uiWorld2Ui&			w2ui_;
};

#endif
