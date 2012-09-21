#ifndef uiosgviewer_h
#define uiosgviewer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uicoinmod.h"
#include "general.h"

namespace osgViewer { class CompositeViewer; class View; }


class uiOsgViewer;

/*! All OSG based views must be coordinated by a viewer. That is
    arranged by storing the osgViewer::View in this class, which
    takes care of the rest. */

mClass(uiCoin) uiOsgViewHandle
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

