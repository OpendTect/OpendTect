#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "basemap.h"
#include "uigroup.h"


class uiGraphicsItem;
class uiGraphicsScene;
class uiGraphicsView;
class uiWorld2Ui;

mExpClass(uiTools) uiBasemapObject : public CallBacker
{
public:
				uiBasemapObject(BasemapObject*);
    virtual			~uiBasemapObject();

    BasemapObject*		getObject();

    bool			hasChanged() const	{ return changed_; }
    void			resetChangeFlag() { changed_ = false; }
    void			setTransform(const uiWorld2Ui*);
    const char*			name() const;
    virtual void		show(bool yn);
    virtual bool		isShown() const;

    uiGraphicsItem&		graphItem()		{ return graphitem_; }
    const uiGraphicsItem&	graphItem() const	{ return graphitem_; }
    uiGraphicsItem&		labelItem()		{ return labelitem_; }
    const uiGraphicsItem&	labelItem() const	{ return labelitem_; }
    void			showLabels(bool yn);
    bool			labelsShown() const	{ return showlabels_; }

    virtual void		update();
    virtual void		updateStyle();

    virtual void		getMousePosInfo(Coord3&,TrcKey&,float& val,
						BufferString& info) const;
protected:
    friend			class uiBasemap;

    void			changedCB(CallBacker*);
    void			changedStyleCB(CallBacker*);
    void			changedZValueCB(CallBacker*);
    void			leftClickCB(CallBacker*);
    void			rightClickCB(CallBacker*);
    void			addToGraphItem(uiGraphicsItem&);

    uiGraphicsItem&		graphitem_;
    uiGraphicsItem&		labelitem_;
    bool			showlabels_;
    const uiWorld2Ui*		transform_;

    bool			changed_;
    BasemapObject*		bmobject_;

private:
    void			addLabel(uiGraphicsItem&);
};


mExpClass(uiTools) uiBasemap : public uiGroup
{
public:
				uiBasemap(uiParent*);
    virtual			~uiBasemap();

    void			setView(const uiWorldRect&);

    void			addObject(BasemapObject*);
    BasemapObject*		getObject(BasemapObjectID);
    uiBasemapObject*		getUiObject(BasemapObjectID);

    ObjectSet<uiBasemapObject>& getObjects()		{ return objects_; }

    bool			hasChanged();
    inline void			setChangeFlag()		{ changed_ = true; }
    void			resetChangeFlag();
				//!Owned by caller
    void			removeObject(const BasemapObject*);
    void			show(const BasemapObject&,bool yn);

    void			showLabels(bool yn);
    bool			labelsShown() const;

    void			addObject(uiBasemapObject*);

    const uiBasemapObject*	uiObjectAt(const Geom::Point2D<float>&) const;
    const char*			nameOfItemAt(const Geom::Point2D<float>&) const;
    void			getMousePosInfo(BufferString& name,Coord3&,
						TrcKey&,float& val,
						BufferString& info) const;

    uiGraphicsItem&		worldItem()		{ return worlditem_;}
    void			centerWorldItem(bool);
    inline uiGraphicsView&	view()			{ return view_; }
    uiGraphicsScene&		scene();
    inline const uiWorld2Ui&	getWorld2Ui() const	{ return w2ui_; }

    CNotifier<uiBasemap,BasemapObjectID>	objectAdded;
    CNotifier<uiBasemap,BasemapObjectID>	objectRemoved;

protected:

    int				indexOf(const BasemapObject*) const;

    uiGraphicsView&		view_;
    uiGraphicsItem&		worlditem_;
    ObjectSet<uiBasemapObject>	objects_;
    bool			changed_;
    bool			centerworlditem_;

    uiWorldRect			wr_;

    void			reSizeCB(CallBacker*);
    virtual void		reDraw(bool deep=true) override;
    void			updateTransform();

private:
    uiWorld2Ui&			w2ui_;
};
