#ifndef visgeomindexedshape_h
#define visgeomindexedshape_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "valseries.h"
#include "visobject.h"
#include "coltabsequence.h"
#include "coltabmapper.h"

namespace Geometry { class IndexedShape; class IndexedGeometry; }

class SoMaterial;
class SoMaterialBinding;
class SoShapeHints;
class SoIndexedShape;
class SoSwitch;
class TaskRunner;
class DataPointSet;

namespace visBase
{

class Coordinates;
class Normals;
class TextureCoords;
class ForegroundLifter;

/*!Visualisation for Geometry::IndexedShape. */

mClass GeomIndexedShape : public VisualObjectImpl
{
public:
    static GeomIndexedShape*	create()
				mCreateDataObj(GeomIndexedShape);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setSurface(Geometry::IndexedShape*,
	    				   TaskRunner* = 0);
    				//!<Does not become mine, should remain
				//!<in memory
    void			setRightHandSystem(bool);

    void			touch(bool forall,TaskRunner* =0);

    void			set3DLineRadius(float radius,
	    					bool constantonscreen=true,
						float maxworldsize=-1);
    				/*!<If radius is less than 0, a normal
				    line will be drawn. */
    void			renderOneSide(int side);
    				/*!< 0 = visisble from both sides.
				     1 = visisble from positive side
				     -1 = visisble from negative side. */

    void			createColTab();
    void			enableColTab(bool);
    bool			isColTabEnabled() const;
    void			setDataMapper(const ColTab::MapperSetup&,
	    				      TaskRunner*);
    const ColTab::MapperSetup*	getDataMapper() const;
    void			setDataSequence(const ColTab::Sequence&);
    const ColTab::Sequence*	getDataSequence() const;

    void			getAttribPositions(DataPointSet&,
	    				TaskRunner*) const;
    void			setAttribData(const DataPointSet&,
	    				TaskRunner*);

    void			setMaterial(Material*);
    void			updateMaterialFrom(const Material*);

    void			turnOnForegroundLifter(bool);

protected:
				~GeomIndexedShape();
    void			reClip();
    void			reMap(TaskRunner*);
    void			matChangeCB(CallBacker*);

    mClass			ColTabMaterial
    {
    public:
					ColTabMaterial();
					~ColTabMaterial();
	void				updatePropertiesFrom(const Material*);
	ColTab::Mapper			mapper_;
	ColTab::Sequence                sequence_;

	SoMaterialBinding*		materialbinding_;
	visBase::Material*		coltab_;
	ArrayValueSeries<float,float>	cache_;
    };

    static const char*			sKeyCoordIndex() { return "CoordIndex";}

    ColTabMaterial*				ctab_;

    SoShapeHints*				hints_;
    Coordinates*				coords_;
    Normals*					normals_;
    TextureCoords*				texturecoords_;

    ObjectSet<SoIndexedShape>			strips_;
    ObjectSet<const Geometry::IndexedGeometry>	stripgeoms_;

    float					lineradius_;
    bool					lineconstantonscreen_;
    float					linemaxsize_;
    ObjectSet<SoIndexedShape>			lines_;
    ObjectSet<const Geometry::IndexedGeometry>	linegeoms_;

    ObjectSet<SoIndexedShape>			fans_;
    ObjectSet<const Geometry::IndexedGeometry>	fangeoms_;

    Geometry::IndexedShape*			shape_;
    
    ForegroundLifter*				lifter_;
    SoSwitch*					lifterswitch_;   
};

};
	
#endif
