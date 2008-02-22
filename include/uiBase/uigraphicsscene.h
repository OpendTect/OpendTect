#ifndef uigraphicsscene_h
#define uigraphicsscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: uigraphicsscene.h,v 1.1 2008-02-22 12:26:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

class ioPixmap;
class QGraphicsScene;

class uiGraphicsScene : public NamedObject
{
public:
				uiGraphicsScene(const char*);
				~uiGraphicsScene();

    void			addText(const char*);
    void			addPixmap(const ioPixmap&);
    void			addRect(float x,float y,float w,float h);

    QGraphicsScene*		qGraphicsScene()
    				{ return qgraphicsscene_; }
protected:

    QGraphicsScene*		qgraphicsscene_;
};

#endif
