#ifndef vistristripset_h
#define vistristripset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistristripset.h,v 1.7 2002-10-23 09:41:55 nanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class Coord3;
class SoCoordinate3;
class SoSwitch;
class SoMaterial;
class SoIndexedTriangleStripSet;
class CallBacker;


namespace Geometry
{ class TriangleStripSet; };

namespace visBase
{
class VisColorTab;


/*!\brief


*/

class TriangleStripSet : public VisualObjectImpl
{
public:
    static TriangleStripSet*	create()
				mCreateDataObj0arg( TriangleStripSet );

    void		setStrips( Geometry::TriangleStripSet*, bool connect );
    			/*!< If connect is true, the object will use the
			     stripsets callbacks to keep itself updated.
			     stripset must then stay alive.
			*/

    void		setValues( const float* data );
    			/*!< The data must be arranged in the same order as
			     the coords, i.e. data[n] should correspond to
			     coord[n]
			*/

    void		getPositions( TypeSet<Coord3>& ) const;

    void		setColorTab( VisColorTab* );
    const VisColorTab*	getColorTab() const { return colortable; }
    VisColorTab*	getColorTab() { return colortable; }

    void		setAutoscale(bool yn) { autoscale = yn; }
    bool		autoScale() const { return autoscale; }

    void		setClipRate( float n );
    			/*!< Should be between 0 and 0.5 */
    float		clipRate() const { return cliprate; }

    void		useTexture(bool);
    bool		usesTexture() const;
			     
protected:
			~TriangleStripSet();
    void		clipData();

    void		updateCoords(CallBacker* =0);
    void		updateIndexes(CallBacker* =0);
    void		updateTexture();

    bool 		autoscale;
    float		cliprate;

    VisColorTab*	colortable;

    float*		data;

    SoCoordinate3*		coords;
    SoMaterial*			ctmaterial;
    SoSwitch*			textureswitch;
    SoIndexedTriangleStripSet*	strips;

    Geometry::TriangleStripSet*	stripset;

};
};


#endif
