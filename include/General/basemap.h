#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
________________________________________________________________________

-*/

#include "generalmod.h"
#include "draw.h"
#include "notify.h"
#include "namedmonitorable.h"
#include "odpresentationmgr.h"

namespace OD
{
    class RGBImage;
    class LineStyle;
    class MarkerStyle2D;
}


class MouseEvent;
class TaskRunnerProvider;
template <class T> class ODPolygon;

/*!Object that can be painted in a basemap. */


mExpClass(General) BaseMapObject : public NamedMonitorable
{
public:

    typedef Geom::PosRectangle<double>	BoundingBox;
    typedef OD::Alignment	Alignment;
    typedef OD::LineStyle	LineStyle;
    typedef OD::MarkerStyle2D	MarkerStyle;

				BaseMapObject(const char* nm);
				~BaseMapObject();

    int				ID() const		{ return id_; }

    Threads::Lock		lock_;
    virtual void		updateGeometry()	{}

    virtual const char*		type() const		{ return typenm_; }
    virtual void		setType( const char* t ) { typenm_ = t;}

    virtual int			depth() const	{ return depth_; }
    virtual void		setDepth(int);
				    /*!< Determines what should be painted on
					 top of what */

    virtual int			nrShapes() const;
    virtual const char*		shapeName(int shapeidx) const;
    virtual void		getPoints(int shapeidx,TypeSet<Coord>&) const {}
				    /*!<Returns a number of coordinates that
					may form a be connected or filled. */
    virtual bool		getBoundingBox(BoundingBox&) const;
    virtual Alignment		alignment(int shapeidx) const;
    virtual float		textRotation() const { return 0; }
    virtual Color		color() const;

    virtual void		setMarkerStyle(int idx,const MarkerStyle&) {}
    virtual const MarkerStyle*	markerStyle(int shapeidx) const
					{ return 0; }
    virtual BufferString	imageFileName( int ) const
					{ return ""; }
    virtual void		getXYScale(int,float& scx,float& scy) const
					{ scx = scy = 1.f; }
    virtual void		setXYScale(int,float,float)	{}

    virtual void		setLineStyle(int shapeidx,
					const OD::LineStyle&) {}
    virtual const LineStyle*	lineStyle(int shapeidx) const { return 0; }

    virtual void		setFillColor(int idx,const Color&)	    {}
    virtual const Color		fillColor(int idx) const
						    { return Color::NoColor();}

    virtual bool		fill(int shapeidx) const	{ return false;}
    virtual bool		close(int shapeidx) const	{ return false;}
    virtual bool		multicolor(int shapeidx) const	{ return false;}

    const OD::RGBImage*		createImage(Coord& origin,Coord& p11) const;
				/*!<Returns image in xy plane. p11 is the
				    coordinate of the corner opposite of the
				    origin. */
    const OD::RGBImage*		createPreview(int approxdiagonal) const;
				/*!<Returns a preview image that has
				    approximately the size of the specified
				    diagonal. */
    virtual bool		allowHoverEvent(int) const	{ return true; }

    virtual bool		canRemoveWithPolygon() const   { return false; }
    virtual void		removeWithPolygon(const ODPolygon<double>&) {}

    virtual void		getMousePosInfo(Coord3&,TrcKey&,float& val,
						BufferString& info) const     {}

    bool			fillPar(IOPar&) const;
    bool			usePar(const IOPar&,const TaskRunnerProvider&);

    CNotifier<BaseMapObject,const MouseEvent&>	leftClicked;
    CNotifier<BaseMapObject,const MouseEvent&>	rightClicked;
    Notifier<BaseMapObject>			changed;
    Notifier<BaseMapObject>			styleChanged;
    Notifier<BaseMapObject>			zvalueChanged;
    CNotifier<BaseMapObject,const uiString&>	nameChanged;

protected:

    int				depth_;
    int				id_;
    BufferString		typenm_;

    virtual bool		doFillPar(IOPar&) const			= 0;
    virtual bool		doUsePar(const IOPar&,
					const TaskRunnerProvider&)	= 0;

};


/*!Base class for a Basemap. */
mExpClass(General) BaseMap : public Presentation::ManagedViewer
{
public:

    typedef Presentation::ViewerTypeID	ViewerTypeID;

			BaseMap();
    virtual void	addObject(BaseMapObject*)		= 0;
				/*!<Object maintained by caller. Adding an
				    existing will trigger update */
    virtual void	removeObject(const BaseMapObject*)	= 0;

    static ViewerTypeID	theViewerTypeID() { return ViewerTypeID::get(2); }

protected:

    virtual ViewerTypeID vwrTypeID() const
				{ return theViewerTypeID(); }

};
