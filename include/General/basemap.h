#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "draw.h"
#include "fontdata.h"
#include "integerid.h"
#include "namedobj.h"
#include "threadlock.h"

namespace OD { class RGBImage; }

class MarkerStyle2D;
class MouseEvent;
class TaskRunner;
template <class T> class ODPolygon;

/*!\brief Unique ID for a basemap. */
mExpClass(General) BasemapID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline BasemapID		udf()	{ return BasemapID(); }
};


/*!\brief Unique ID for a basemap object. */
mExpClass(General) BasemapObjectID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline BasemapObjectID	udf()	{ return BasemapObjectID(); }
};


/*!\brief Object that can be painted in a basemap. */
mExpClass(General) BasemapObject : public NamedCallBacker
{
public:

    typedef Geom::PosRectangle<double>	BoundingBox;

				BasemapObject(const char* nm);
				~BasemapObject();

    BasemapObjectID		ID() const		{ return id_; }

    Threads::Lock		lock_;
    virtual void		updateGeometry()			{}

    virtual void		setType(const char* tp) { typenm_ = tp;}
    virtual const char*		getType() const		{ return typenm_; }

    virtual void		setDepth(int val);
    virtual int			getDepth() const	{ return depth_; }
				/*!<Determines what should be painted ontop of
				    what */

    virtual int			nrShapes() const;
    virtual const char*		getShapeName(int shapeidx) const;
    virtual void		getPoints(int shapeidx,TypeSet<Coord>&) const;
				/*!<Returns a number of coordinates that
				    may form a be connected or filled. */
    virtual bool		getBoundingBox(BoundingBox&) const;
    virtual Coord		getTextPos(int shapeidx) const;
    virtual Alignment		getAlignment(int shapeidx) const;
    virtual FontData		getFont(int) const;
    virtual Coord		getTextOffset(int) const;
    virtual float		getTextRotation() const { return 0; }
    virtual OD::Color		getColor() const;

    virtual void		setMarkerStyle(int idx,const MarkerStyle2D&) {}
    virtual const MarkerStyle2D* getMarkerStyle(int shapeidx) const { return 0;}
    virtual BufferString	getImageFileName(int idx) const	{ return ""; }
    virtual void		getXYScale(int idx,float& scx,float& scy) const
				{ scx = scy = 1.f; }
    virtual void		setXYScale(int idx,float scx,float scy)	{}

    virtual void		setLineStyle(int idx,const OD::LineStyle&)    {}
    virtual const OD::LineStyle* getLineStyle(int shapeidx) const { return 0; }

    virtual void		setFillColor(int idx,const OD::Color&)	    {}
    virtual const OD::Color	getFillColor(int idx) const
						{ return OD::Color::NoColor();}

    virtual bool		fill(int shapeidx) const	{ return false;}
    virtual bool		close(int shapeidx) const	{ return false;}
    virtual bool		multicolor( int shapeidx ) const
				{ return false; }

    virtual void		setImage(int idx,OD::RGBImage*)     {}
    virtual const OD::RGBImage* getImage(int shapeidx) const	{ return 0;}

    virtual const OD::RGBImage* createImage(Coord& origin,Coord& p11) const;
				/*!<Returns image in xy plane. p11 is the
				    coordinate of the corner opposite of the
				    origin. */
    virtual const OD::RGBImage* createPreview(int approxdiagonal) const;
				/*!<Returns a preview image that has
				    approximately the size of the specified
				    diagonal. */
    virtual bool		allowHoverEvent(int) const	{ return true; }

    virtual bool		canRemoveWithPolygon() const   { return false; }
    virtual void		removeWithPolygon(const ODPolygon<double>&) {}

    virtual void		getMousePosInfo(Coord3&,TrcKey&,float& val,
						BufferString& info) const     {}

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&,TaskRunner* taskr=0);

    CNotifier<BasemapObject,const MouseEvent&>	leftClicked;
    CNotifier<BasemapObject,const MouseEvent&>	rightClicked;
    Notifier<BasemapObject>			changed;
    Notifier<BasemapObject>			stylechanged;
    Notifier<BasemapObject>			zvalueChanged;
    CNotifier<BasemapObject,BufferString>	nameChanged;

protected:
    int				depth_;
    BasemapObjectID		id_;
    BufferString		typenm_;
};
