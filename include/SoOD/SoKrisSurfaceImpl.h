#ifndef MeshSurfacePart_h
#define MesSurfacePart_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoKrisSurfaceImpl.h,v 1.3 2005-03-10 11:50:37 cvskris Exp $
________________________________________________________________________


-*/

#include "SoKrisSurface.h"

#include "Inventor/lists/SbList.h"

class SbRWMutex;
class MeshSurfacePartResolution;

class MeshSurfacePartPart
{
public:
    				MeshSurfacePartPart( SoKrisSurface&, int, int );
    				~MeshSurfacePartPart();
    void			setStart( int row, int col );
    void			touch( int, int, bool undef );
    void			invalidateCaches();
    void			rayPick(SoRayPickAction*, bool );
    void			computeBBox(SoState*, SbBox3f&, SbVec3f&,
	    				    bool useownvalidation );

    static int			sideSize() { return 16; }

protected:
    bool			isInside( int, int ) const;
    const int32_t*		getMatIndexPtr(int,int) const;

    const SoKrisSurface&	meshsurface;
    int				rowstart, colstart;

    SoBoundingBoxCache*		cache;
    bool			ownvalidation;
};


class MeshSurfaceTesselationCache
{
public:
    				MeshSurfaceTesselationCache(
					const SoKrisSurface&, bool isstrip );
    				~MeshSurfaceTesselationCache();

    void			reset(bool all);

    void			GLRender(SoGLRenderAction*);
    bool			isValid() const { return isvalid; }
    void			setValid(bool yn=true) { isvalid=yn; }

    SbList<int>			ci;
    SbList<int>			ni;

    SbList<SbVec3f>		normals;
    SbRWMutex*			buildmutex;

protected:
    const SoKrisSurface&	meshsurface;
    bool			isstrip;

    bool			isvalid;
};


class MeshSurfacePart
{
public:
    		MeshSurfacePart( SoKrisSurface&, int start0, int start1,
				 int sidesize );
    		~MeshSurfacePart();
    void	setStart( int row, int col );
    int		getRowStart() const { return start0; }
    int		getColStart() const { return start1; }
    void	touch( int, int, bool undef );
    void	computeBBox(SoState*, SbBox3f&, SbVec3f&,bool useownvalidation);
    void	rayPick( SoRayPickAction*, bool useownvalidation);
    void	GLRender(SoGLRenderAction*,bool useownvalidation);
    void	GLRenderGlue(SoGLRenderAction*,bool useownvalidation);

    void	invalidateCaches();

    int		computeResolution( SoState*, bool useownvalidatoin );
    bool	setResolution( int desiredres, bool useownvalidatoin);
    bool	hasResChangedSinceLastRender() const { return reshaschanged; }

    void	setNeighbor( int, MeshSurfacePart*, bool callback=false );

    int		nrResolutions() const { return resolutions.getLength(); }
    int		getResolution() const { return resolution; }
    MeshSurfacePartResolution*	getResolution(int i) { return resolutions[i]; }

protected:
    int 	getSpacing( int res ) const;
    int		nrRows() const;
    int		nrCols() const;

    SbVec3f	getNormal( int, int, int, bool );
    SbBool	getNormal( int, int, int, bool, SbVec3f& );
    void	addGlueFan( const SbList<int>&,
			    const SbList<SbVec3f>&,
			    const SbList<int>&,
			    const SbList<SbVec3f>&,
			    SbBool dir );

    int					start0, start1;
    int					sidesize;
    int					resolution;
    bool				reshaschanged;
    SbList<MeshSurfacePartResolution*>	resolutions;
    SbList<MeshSurfacePartPart*>	bboxes;
    SbList<MeshSurfacePart*>		neighbors;
    SoKrisSurface&			meshsurface;
    SoBoundingBoxCache*			bboxcache;
    bool				bboxvalidation;

    MeshSurfaceTesselationCache*	gluecache;
    bool				gluevalidation;
};


class MeshSurfacePartResolution 
{
public:
		MeshSurfacePartResolution( SoKrisSurface&,
			    int s0, int s1, int ssz0, int ssz1, int spacing);
    		~MeshSurfacePartResolution();
    void	setStart( int row, int col );
    void	GLRender(SoGLRenderAction*,bool overridetessel);
    void	touch( int, int, bool undef );
    bool	canDisplay(bool ownvalidation) const;
    bool	needsUpdate(bool ownvalidation) const;
    bool	getNormal( int row, int col,
	    		   bool useownvalidation, SbVec3f& );

    int		getSpacing() const { return spacing; }

    void	tesselate();
    void	invalidateCaches();
protected:
    void	startNewStrip( int, int, int&, int&,
					 int&, int&, int&, bool&);
    void	expandStrip( int, int, int&, int&,
	    			       int&, int&, int&, bool&);
    bool	computeNormal( int, int, SbVec3f* =0 );
    bool	getBestDiagonal( int, int, int, int, bool ) const; 
    int		nrCols() const;
    int		nrRows() const;
    int		getCoordIndex( int, int ) const;
    int		getFillType(int,int) const;

    int		start0, start1;
    int		spacing;
    int		sidesize0;
    int		sidesize1;

    int		cachestatus;
    		//0=OK, 1=need retesselation, 2=invalid

    MeshSurfaceTesselationCache*	cache;
    const SoKrisSurface&		meshsurface;
};



#endif
    
