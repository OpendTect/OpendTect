#ifndef flatviewaxesdrawer_h
#define flatviewaxesdrawer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.h,v 1.2 2007-03-10 12:13:46 cvsbert Exp $
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

    BufferString	xioparkey_; //!< default empty => use positioning
    BufferString	yioparkey_; //!< default empty => use positioning

protected:

    Viewer&		vwr_;

    virtual double      getAnnotTextAndPos(bool,double,BufferString*) const;

};

} // namespace

#endif
