#ifndef MeshSurfacePart_h
#define MesSurfacePart_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoKrisSurfaceImpl.h,v 1.1 2004-10-02 12:29:58 kristofer Exp $
________________________________________________________________________


-*/

#include "SoKrisSurface.h"

#include "Inventor/caches/SoCache.h"
#include "Inventor/lists/SbList.h"

class SbRWMutex;
class MeshSurfacePartResolution;

class MeshSurfacePartPart
{
public:
    				MeshSurfacePartPart( SoKrisSurface&, int, int );
    				~MeshSurfacePartPart();
    void			touch( int, int, bool undef );
    void			rayPick(SoRayPickAction*, bool );
    void			computeBBox(SoState*, SbBox3f&, SbVec3f&,
	    				    bool useownvalidation );

    static int			sideSize() { return 16; }

protected:
    bool			isInside( int, int ) const;
    const int32_t*		getCoordIndexPtr(int,int) const;
    const int32_t*		getMatIndexPtr(int,int) const;

    const SoKrisSurface&	meshsurface;
    int				start0,start1;

    SoBoundingBoxCache*		cache;
    bool			ownvalidation;
};


class MeshSurfacePart
{
public:
    		MeshSurfacePart( SoKrisSurface&, int start0, int start1,
				 int sidesize );
    		~MeshSurfacePart();
    void	touch( int, int, bool undef );
    void	computeBBox(SoState*, SbBox3f&, SbVec3f&,bool useownvalidation);
    void	rayPick( SoRayPickAction*, bool useownvalidation);
    void	GLRender(SoGLRenderAction*,bool useownvalidation);

    int		computeResolution( SoState*, bool useownvalidatoin );
    bool	setResolution( SoState*, int desiredres, bool useownvalidatoin);
    bool	hasResChangedSinceLastRender() const { return reshaschanged; }

    void	setNeighbor( int, MeshSurfacePart*, bool callback=false );

    void	updateGlue() {}
    int		nrResolutions() const { return resolutions.getLength(); }
    MeshSurfacePartResolution*	getResolution( int i) { return resolutions[i]; }

protected:
    int		nrRows() const;
    int		nrCols() const;
    int		start0,start1;
    int		sidesize;

    int					resolution;
    bool				reshaschanged;
    SbList<MeshSurfacePartResolution*>	resolutions;
    SbList<MeshSurfacePartPart*>	bboxes;
    SbList<MeshSurfacePart*>		neighbors;
    SoKrisSurface&			meshsurface;
    SoBoundingBoxCache*			bboxcache;
    bool				bboxvalidation;
};


class MeshSurfaceTesselationCache : public SoCache
{
public:
    				MeshSurfaceTesselationCache(SoState*,
							const SoKrisSurface&);
    				~MeshSurfaceTesselationCache();

    void			call(SoGLRenderAction*);

    SbList<int>			cii;
    SbList<int>			ni;
    SbList<SbVec3f>		normals;
    SbRWMutex*			buildmutex;

    const SoKrisSurface&	meshsurface;
};


class MeshSurfacePartResolution 
{
public:
    				MeshSurfacePartResolution( SoKrisSurface&,
					int s0, int s1, int ssz0, int ssz1,
					int spacing);
    				~MeshSurfacePartResolution();
    void			GLRender(SoGLRenderAction*,bool overridetessel);
    void			touch( int, int, bool undef );
    bool			canDisplay(SoState*,bool ownvalidation) const;
    bool			needsUpdate(SoState*,bool ownvalidation) const;

    void				tesselate(SoState*);
protected:
    void				startNewStrip( SoState*,
	    					       int, int, int&, int&,
	    					       int&, int&, int&, bool&);
    void				expandStrip( SoState*,
	    					     int, int, int&, int&,
	    					     int&, int&, int&, bool&);
    const SoKrisSurface&		meshsurface;
    bool				computeNormal( SoState*, int, int );
    bool				getBestDiagonal( SoState*, int, int,
	    						 int, int, bool ) const;

    int					nrCols() const;
    int					nrRows() const;
    int					getCoordIndexIndex( int, int ) const;
    int					getFillType(int,int) const;

    const int				start0, start1;
    int					spacing;
    int					sidesize0;
    int					sidesize1;

    int					cachestatus;
    					//0=OK, 1=need retesselation, 2=invalid
    MeshSurfaceTesselationCache*	cache;
};



#endif
    
