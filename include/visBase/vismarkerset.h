#ifndef vismarkerset_h
#define vismarkerset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "trigonometry.h"
#include "draw.h"
#include "viscoord.h"
#include "visnormals.h"

namespace osgGeo { class MarkerSet; }

namespace visBase
{


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
    Coordinates*	getCoordinates()  const  { return coords_; }
    Normals*		getNormals();

    void		setMaterial(visBase::Material*);
			//!<If material is null, markers will be single color

    void		setMarkersSingleColor(const Color& singlecolor);
			//!<all markers will use the same color, the color can
			//!< also be set by markerstyle
    Color		getMarkersSingleColor() const;

    void		setMarkerStyle(const MarkerStyle3D&);
    			/*!<Sets predefined shape and size.
			    Will only use color of markerstyle if no
			    material is set.*/
    MarkerStyle3D&	getMarkerStyle() { return markerstyle_; }
    void		setType(MarkerStyle3D::Type);
    			/*!<Sets predefined shape. */
    MarkerStyle3D::Type	getType() const;
 
    void		setScreenSize(const float);
    			/*!<If a nonzero value is given, the object will
			    try to have the same size (in pixels) on the screen
			    at all times. */
    float		getScreenSize() const;
    static float	cDefaultScreenSize() { return 5; }

    void		setMarkerHeightRatio( float );
    float		getMarkerHeightRatio() const;

    void		setMinimumScale(float);
    float		getMinimumScale() const;

    void		setMaximumScale(float);
    float		getMaximumScale() const;

    void		setAutoRotateMode(AutoRotateMode);

    void		doFaceCamera(bool yn);
    			/*!<If true, the maker will always be rotated so the
			    same part of the marker always faces the camera. */
    bool		facesCamera() const;

    void		setDisplayTransformation( const mVisTrans* );
    const mVisTrans*	getDisplayTransformation() const;

    void		removeMarker(int idx);

    void		clearMarkers();
    void		turnMarkerOn(unsigned int idx,bool);

    int			findClosestMarker(const Coord3&, bool scenespace=false);
    int			findMarker(const Coord3&, const Coord3& eps, 
				   bool scenespace = false);

    void		setMarkerResolution(float res);
			/*!< The res value is between 0 and 1. It indicates the 
			quality of marker visualization. */

protected:
    void		materialChangeCB(CallBacker*);
				~MarkerSet();
    
    RefMan<Coordinates>		coords_;
    RefMan<Normals>		normals_;
    RefMan<const mVisTrans>	displaytrans_;
    osgGeo::MarkerSet*		markerset_;
    MarkerStyle3D		markerstyle_;

    //void		setArrowDir(const ::Sphere&);
};

};


#endif

