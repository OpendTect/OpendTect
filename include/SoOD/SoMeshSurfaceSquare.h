#ifndef SoMeshSurfaceSquare_h
#define SoMeshSurfaceSquare_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoMeshSurfaceSquare.h,v 1.8 2003-10-21 18:46:06 kristofer Exp $
________________________________________________________________________


-*/

#include "Inventor/nodekits/SoBaseKit.h"
#include "Inventor/fields/SoMFInt32.h"
#include "Inventor/fields/SoSFShort.h"
#include "Inventor/SbLinear.h"


class SbBox3f;
class SoCallbackList;
class SoCoordinate3;
class SoEventCallback;
class SoTextureCoordinate2;
class SoMeshSurfaceBrick;
class SoMeshSurfaceBrickWire;
class SoIndexedTriangleFanSet;
class SoSensor;
class SoFieldSensor;
class SoState;
class SoSwitch;
class SoNormal;
class SoMeshSurface;
class SbVec3f;
class SbVec2s;

class SoMeshSurfaceSquare;

typedef void SoMeshSurfaceSquareCB( void* data, SoMeshSurfaceSquare* );

/*!\brief
Is a part of a SoMeshSurface. A SoMeshSurfaceSquare has two parts - the
brick and the glue. The brick is avaliable in different resolutions. The
glue makes sure that the brick fits well with the neigbor one, that might
have a different resolution.
*/


class SoMeshSurfaceSquare : public SoBaseKit
{
    SO_KIT_HEADER(SoMeshSurfaceSquare);
    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(eventCatcher);
    SO_KIT_CATALOG_ENTRY_HEADER(coords);
    SO_KIT_CATALOG_ENTRY_HEADER(texturecoords);
    SO_KIT_CATALOG_ENTRY_HEADER(triResSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(wireResSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(wireSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(wireTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(glue);
    SO_KIT_CATALOG_ENTRY_HEADER(glueNormals);
    SO_KIT_CATALOG_ENTRY_HEADER(glueSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(glueGroup);


public:
    				SoMeshSurfaceSquare();
    static void			initClass();

    SoMFInt32			origo;
    SoSFShort			sizepower;

    void			setTextureRange(int firstrow, int firstcol,
						int lastrow, int lastcol );

    void			setPos(int row,int col,const SbVec3f&);
    SbVec3f			getPos(int row,int col) const;
    void			removePos(int row,int col);

    void			setNeighbor(int relrow,int relcol,
	    			    SoMeshSurfaceSquare*,bool callback=true);

    void			turnOnWireFrame(bool);
    bool			isWireFrameOn() const;

    bool			setResolution(int);
    				/*!< Returns false if the given res was 
				     unavailiable (and that management should 
				     make it) */
    int				getResolution() const;
    int				computeResolution(SoState*);
    				/*!< Returns a the selected res if it was 
				     unavailiable (and that management should 
				     make it), else, it returns -1 */

    SbBool			shouldGLRender(SoGLRenderAction* action);

    bool			hasResolutionChanged() const;
    void			updateGlue();

    void			touch( int row, int col );

    SbVec3f			getNormal(int row,int col,int res);
    void			getSide(bool row,bool start,SbList<int>&) const;

    SoMeshSurfaceBrick*		getBrick(int resolution);
    const SoMeshSurfaceBrick*	getBrick(int resolution) const;
    SoMeshSurfaceBrickWire*	getWire(int resolution);
    const SoMeshSurfaceBrickWire* getWire(int resolution) const;

    void			addPickCB( SoMeshSurfaceSquareCB*, void* = 0 );
    void			removePickCB( SoMeshSurfaceSquareCB*, void* =0);
    void			getPickedRowCol( int& row, int& col ) const;

    void			GLRender(SoGLRenderAction*);
    void			getBoundingBox(SoGetBoundingBoxAction*);

private:
				~SoMeshSurfaceSquare();

    int				getCoordIndex(int row,int col) const;
    int				getNeigborIndex(int relrow,int relcol) const;
    SbBool			getNormal(int row,int col,int res, SbVec3f&);

    void			computeBBox();
    SbBool			cullTest(SoState*);

    int				getBlockSize(int res) const;
    static int			get2Power(int);

    void			addGlueFan(int& coordindex,
	    				     int& normalindex,
					     const SbList<int>&,
	    				     const SbList<SbVec3f>&,
					     const SbList<int>&,
	    				     const SbList<SbVec3f>&,
	   				     SbBool dir );
    bool			isUndefined(int,int) const;

    SoSwitch*			wireswitchptr;
    SoSwitch*			triswitchptr;
    SoSwitch*			glueswitchptr;
    SoIndexedTriangleFanSet*	glueptr;
    SoNormal*			gluenormalptr;
    SoCoordinate3*		coordptr;
    SoTextureCoordinate2*	texturecoordptr;

    SbList<SoMeshSurfaceSquare*> neighbors;

    SbBox3f*			bboxcache;

    bool			reshaschanged;
    bool			updateglue;
    bool			showtri;
    bool			showwire;

    int				currentres;

    SoCallbackList*		pickcallbacks;
    int				pickedrow, pickedcol;
    static void			pickCB( void*, SoEventCallback* );

    SoFieldSensor*		sizePowerSensor;
    static void			sizePowerCB( void*, SoSensor* );
    int				sidesize;

    SbVec2s			extension;
    				//is updated & valid if bbox cache is valid
};

#endif
