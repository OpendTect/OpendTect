#ifndef flatviewaxesdrawer_h
#define flatviewaxesdrawer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.h,v 1.1 2007-03-09 12:28:44 cvsbert Exp $
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

    void		setAuxNr( int nr )	{ auxnr_ = nr; }
    int			auxNr() const		{ return auxnr_; }

    void		draw(uiRect,uiWorldRect);

protected:

    Viewer&		vwr_;
    int			auxnr_;

    virtual double      getAnnotTextAndPos(bool,double,BufferString*) const;

};

} // namespace

#endif
