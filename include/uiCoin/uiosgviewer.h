#ifndef uiosgviewer_h
#define uiosgviewer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
 RCS:           $Id: uiosgviewer.h,v 1.2 2011/12/14 15:27:41 cvskris Exp $
________________________________________________________________________

-*/

#include "general.h"

namespace osgViewer { class CompositeViewer; class View; }


class uiOsgViewer;

/*! All OSG based views must be coordinated by a viewer. That is
    arranged by storing the osgViewer::View in this class, which
    takes care of the rest. */

mClass uiOsgViewHandle
{
public:
				uiOsgViewHandle();
    virtual			~uiOsgViewHandle()	{ detachView(); }

    void			detachView();
    void			setViewer(uiOsgViewer* v){ viewer_ = v; }
    osgViewer::View*		getOsgView()		{ return osgview_; }
    void			setOsgView(osgViewer::View*);

protected:
    uiOsgViewer*			viewer_;
private:
    osgViewer::View*			osgview_;
};


#endif
