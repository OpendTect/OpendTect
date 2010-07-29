#ifndef uibasemap_h
#define uibasemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2010
 RCS:           $Id: uibasemap.h,v 1.2 2010-07-29 10:56:00 cvsraman Exp $
________________________________________________________________________

-*/


#include "basemap.h"
#include "uigraphicsitem.h"
#include "uigroup.h"


class uiGraphicsView;
class uiParent;
class uiWorld2Ui;

mClass uiBaseMapObject : public BaseMapObject
{
public:
    				uiBaseMapObject(const char*);
    virtual			~uiBaseMapObject() {}

//    float			getDepth() const;
//    void			setDepth(float);
    void			setTransform(const uiWorld2Ui*);

    uiGraphicsItemGroup*	itemGrp()		{ return itemgrp_; }
    virtual void		updateGeometry()			= 0;

protected:

    uiGraphicsItemGroup*	itemgrp_;
    const uiWorld2Ui*		transform_;

};


mClass uiBaseMap : public BaseMap,
       		   public uiGroup
{
public:
				uiBaseMap(uiParent*);
    virtual				~uiBaseMap();

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
