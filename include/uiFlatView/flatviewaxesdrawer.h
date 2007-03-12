#ifndef flatviewaxesdrawer_h
#define flatviewaxesdrawer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.h,v 1.3 2007-03-12 10:59:35 cvsbert Exp $
________________________________________________________________________

-*/

#include "drawaxis2d.h"

namespace FlatView
{

class Viewer;

/*!\brief Axis drawer for flat viewers */

class AxesDrawer : public ::DrawAxis2D
{
public:
    			AxesDrawer(Viewer&,ioDrawArea&);

    void		draw(uiRect,uiWorldRect);

    int			altdim0_;

protected:

    Viewer&		vwr_;

    virtual double      getAnnotTextAndPos(bool,double,BufferString*) const;

};

} // namespace

#endif
