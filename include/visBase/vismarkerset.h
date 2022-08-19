#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "color.h"
#include "draw.h"
#include "trigonometry.h"

#include "visobject.h"
#include "viscoord.h"
#include "visnormals.h"

namespace osgGeo { class MarkerSet; }

namespace visBase
{
    class PolygonOffset;

/*!\brief

MarkerSet is a set of basic pickmarker with a constant size on screen.
Size and shape are settable.

*/

mExpClass(visBase) MarkerSet : public VisualObjectImpl
{
public:

    enum AutoRotateMode
    {
	NO_ROTATION,
	ROTATE_TO_SCREEN,
	ROTATE_TO_CAMERA,
	ROTATE_TO_AXIS
    };

    static MarkerSet*	create()
			mCreateDataObj(MarkerSet);

    Coordinates*	getCoordinates()         { return coords_; }
    const Coordinates*	getCoordinates()  const  { return coords_; }
    Normals*		getNormals();

    void		setMaterial(visBase::Material*) override;
			//!<If material is null, markers will be single color

    void		setMarkersSingleColor(const OD::Color& singlecolor);
			//!<all markers will use the same color, the color can
			//!< also be set by markerstyle
    OD::Color		getMarkersSingleColor() const;
    bool		usesSingleColor() const;
    void		getColorArray(TypeSet<OD::Color>& colors) const;

    void		setMarkerStyle(const MarkerStyle3D&);
			/*!<Sets predefined shape and size.
			    Will only use color of markerstyle if no
			    material is set.*/
    MarkerStyle3D&	getMarkerStyle() { return markerstyle_; }
    const MarkerStyle3D& getMarkerStyle() const { return markerstyle_; }
    void		setType(MarkerStyle3D::Type);
			/*!<Sets predefined shape. */
    MarkerStyle3D::Type	getType() const;

    void		setScreenSize(float);
			/*!<If a nonzero value is given, the object will
			    try to have the same size (in pixels) on the screen
			    at all times. */
    float		getScreenSize() const;
    static float	cDefaultScreenSize() { return 5; }

    void		setMarkerHeightRatio(float);
    float		getMarkerHeightRatio() const;

    void		setMinimumScale(float);
    float		getMinimumScale() const;

    void		setMaximumScale(float);
    float		getMaximumScale() const;

    void		setAutoRotateMode(AutoRotateMode);

    void		setRotationForAllMarkers(const Coord3&, const float);
    void		setSingleMarkerRotation(const Quaternion&, int);
    void		applyRotationToAllMarkers(bool);


    void		doFaceCamera(bool yn);
			/*!<If true, the maker will always be rotated so the
			    same part of the marker always faces the camera. */
    bool		facesCamera() const;

    void		setDisplayTransformation(const mVisTrans*) override;
    const mVisTrans*	getDisplayTransformation() const override;

    void		setPixelDensity(float) override;
    float		getPixelDensity() const override
			{ return pixeldensity_; }

    void		removeMarker(int idx);

    void		clearMarkers();
			//!<Removes all markers
    void		turnMarkerOn(unsigned int idx,bool);
    void		turnAllMarkersOn(bool);
    bool		markerOn(unsigned int);

    int			findClosestMarker(const Coord3&, bool scenespace=false);
    int			findMarker(const Coord3&, const Coord3& eps,
				   bool scenespace = false);

    void		setMarkerResolution(float res);
			/*!< The res value is between 0 and 1. It indicates the
			quality of marker visualization. */

     int		size() const;
     int		addPos(const Coord3&, bool draw = true);
    void		setPos(int, const Coord3&,bool draw = true);
    void		insertPos(int, const Coord3&,bool draw = true);
    void		forceRedraw(bool);
    void		addPolygonOffsetNodeState();
    void		removePolygonOffsetNodeState();

protected:
    void		materialChangeCB(CallBacker*) override;
				~MarkerSet();

    RefMan<Coordinates>		coords_;
    RefMan<Normals>		normals_;
    ConstRefMan<mVisTrans>	displaytrans_;
    osgGeo::MarkerSet*		markerset_;
    MarkerStyle3D		markerstyle_;

    float			pixeldensity_;

    float			rotationangle_;
    Coord3			rotationvec_;
    PolygonOffset*		offset_;
    osg::Array*			onoffarr_;
};

} // namespace visBase
