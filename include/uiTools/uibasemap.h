#ifndef uibasemap_h
#define uibasemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
 RCS:           $Id: uibasemap.h,v 1.4 2010-08-09 20:00:56 cvskris Exp $
________________________________________________________________________

-*/


#include "basemap.h"
#include "uigraphicsitem.h"
#include "uigroup.h"


class uiGraphicsView;
class uiParent;
class uiWorld2Ui;

mClass uiBaseMapObject
{
public:
    				uiBaseMapObject(BaseMapObject*);
    virtual			~uiBaseMapObject();

    void			setTransform(const uiWorld2Ui*);

    uiGraphicsItemGroup*	itemGrp()		{ return itemgrp_; }
    void			updateGeometry()	{} //TODO: Implement: Read geometry from object_

protected:
    friend			class uiBaseMap;

    uiGraphicsItemGroup*	itemgrp_;
    const uiWorld2Ui*		transform_;

    BaseMapObject*		bmobject_;
};


mClass uiBaseMap : public BaseMap,
       		   public uiGroup
{
public:
				uiBaseMap(uiParent*);
    virtual			~uiBaseMap();

    void			addObject(BaseMapObject*);
    void			removeObject(const BaseMapObject*);

protected:

    uiGraphicsView&		view_;
    ObjectSet<uiBaseMapObject>	objects_;

    uiWorld2Ui&			w2ui_;

    void			reSizeCB(CallBacker*)		{ reDraw(); }
    virtual void		reDraw();

};

#endif
