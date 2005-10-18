#ifndef emsurfacegeometry_h
#define emsurfacegeometry_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfacegeometry.h,v 1.17 2005-10-18 20:16:43 cvskris Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "emobject.h"
#include "position.h"
#include "bufstringset.h"
#include "mathfunc.h"

template <class T, class AT> class TopList;


class RowCol;
template <class T> class Interval;
template <class T> class StepInterval;


namespace Geometry { class MeshSurface; class ParametricSurface; };


namespace EM
{
class Surface;
class SurfaceIODataSelection;

/*!\brief 
*/


class SurfaceGeometry : public CallBackClass
{
public:
    			SurfaceGeometry(Surface&);
    virtual		~SurfaceGeometry();
    Executor*		loader(const SurfaceIODataSelection* s=0);
    Executor*		saver(const SurfaceIODataSelection* s=0,
			      const MultiID* key=0);

    void		removeAll();

    int			nrSections() const;
    SectionID		sectionID(int idx) const;
    SectionID		sectionID(const char*) const;
    bool		hasSection(const SectionID&) const;
    int			sectionNr(const SectionID&) const;
    const char*		sectionName(const SectionID&) const;
    bool		setSectionName( const SectionID&, const char*,
	    				bool addtohistory );
    SectionID		addSection(const char* nm,bool addtohistory);
    SectionID		addSection(const char* nm,const SectionID&,
	    			   bool addtohistory);
    			/*!< Return false if the sectionid allready exists */
    void		removeSection(const SectionID&,bool addtohistory);
    SectionID		cloneSection(const SectionID&);
    void		checkSections();

    virtual bool	insertRow( const SectionID&, int newrow, bool hist );
    virtual bool	insertCol( const SectionID&, int newcol, bool hist );

    virtual bool	isDefined(const SectionID&,const RowCol&) const;
    virtual bool	isDefined(const PosID&) const;

    void		getPos(const RowCol&,TypeSet<Coord3>&) const;
    			//!< Returns positions from all sections on RowCol
   
    virtual PosID	getNeighbor( const PosID& posid,
	   			     const RowCol& dir ) const;
       			/*!<If the node has a neigbor in the given direction
		  	    it is returned. If not, the PosID of the neighbor
		  	    node on the same section is returned
			*/	    
    int			getNeighbors( const PosID& posid, 
	    			      TypeSet<PosID>* res,
	   			      int size=1, bool circle=false ) const;
    			/*!<\param posid	The posid that we want the
						neigbors to
			    \param res		A pointer where the result is
			    			stored
			    \param size		Specifies how far from posid the
						search should continue
			    \param circle	Specifies wether size should
			    			be seen as a radius or the
						half sidelength of a square
			    \return		The number of neigbors found
			*/


    void		getLinkedPos( const PosID& posid,
	    			      TypeSet<PosID>& ) const;

    bool		isLoaded() const;
    bool		isFullResolution() const;

    void                setShift(float sh_)		{ shift = sh_; }
    float               getShift() const		{ return shift; }

    const Geometry::ParametricSurface* getSurface(const SectionID&) const;
    RowCol		loadedStep() const;
    RowCol		step() const;
    void		setStep(const RowCol& step,const RowCol& loadedstep);
    			/*!< Sets subselection data
			    \param step		The step that the surface
						is defined in
			    \param loadedstep	The step that is loaded
			*/
						
    StepInterval<int>	rowRange(const SectionID& =SectionID(-1)) const;
    			/*< If SectionID is -1, the overall range is returned */
    StepInterval<int>	colRange(const SectionID& =SectionID(-1)) const;
    			/*< If SectionID is -1, the overall range is returned */

    Interval<float>	getZRange(const Interval<int>& rowrg,
	    			  const Interval<int>& colrg) const;

    virtual int		findPos(const SectionID&,const Interval<float>& x,
	    			const Interval<float>& y,
				const Interval<float>& z,
				TypeSet<PosID>* res) const;
    int			findPos(const Interval<float>& x,
	    			const Interval<float>& y,
				const Interval<float>& z,TypeSet<PosID>*) const;
    int			findPos(const CubeSampling&,TypeSet<PosID>*) const;
    int			findPos(const RowCol&,TypeSet<PosID>&) const;
    bool		findClosestNodes(TopList<float,PosID>& res,
					 const Coord3& pos,
					 const FloatMathFunction* t2d=0) const;
    bool		findClosestNodes(const SectionID&,
					 TopList<float,PosID>& res,
					 const Coord3& pos,
					 const FloatMathFunction* t2d=0) const;
    bool		findClosestMesh(PosID& res,const Coord3& pos,
					const FloatMathFunction* t2d=0) const;

    bool		computeMeshNormal(Coord3& res,const PosID&, 
					  const FloatMathFunction* t2d=0) const;
    bool		computeNormal(Coord3& res,const PosID& posid,
				      const FloatMathFunction* t2d=0,
				      bool normalize=true) const;
    bool		computeNormal(Coord3& res,const TypeSet<PosID>& nodes,
				      const FloatMathFunction* t2d=0,
				      bool normalize=true) const;
			/*!< Computes an aproximation of the orientation of a
			     part of a surface
			     \param nodes	orientation is computed on the
						connections surrounding these
						nodes.
			     \param depthconv	Convert the depth before
						computing. This can be handy
						if z is given in timedomain wich
						will give problems in pca. If
						ommitted, the z coords will not
						be converted.
			*/

    bool		computeNormal(Coord3& res,const CubeSampling* cs=0,
				      const FloatMathFunction* t2d=0,
				      bool normalize=true) const;
			/*!< Computes an aproximation of the surface's
			     orientation
			     \param cs		Compute only within this cube.
						If ommitted, the depth will be
						computed on the entire surface.
			     \param depthconv	Convert the depth before
						computing. This can be handy
						if z is given in timedomain wich
						will give problems in pca. If
						ommitted, the z coords will not
						be converted.
			*/

    float		normalDistance(const Coord3&,
				       const FloatMathFunction* t2d=0,
				       Interval<float>* meshvariation=0) const;
			/*!< Computes the distance along normal of the closest
			     mesh's plane.
			  \returns the distance. Note that the distance is
			   negative on the back side of the surface.
			  \param meshvariation
				      If set, the variation of the mesh's
				      coordinates' own distance along the
				      normal is set. If the returned value
				      lies inside this range, it cannot be
				      said on which side of the surface
				      the given coord is.
			*/

    char		whichSide(const Coord3&,
				  const FloatMathFunction* t2d=0,
				  float fuzzyness=0) const;
			/*!< Determies wich side of the surface the position is.
			  \retval	1	The positive side
			  \retval	0	Is on the surface
			  \retval	-1	The negative side
			  \retval	-2	Side could not be determined
			*/

    bool		checkSupport(bool yn);
    			/*!<\returns previous status */
    bool		checksSupport() const;

    bool		isAtEdge(const PosID&) const;

    bool		isChanged(int) const		{ return changed; }
    void		resetChangedFlag()		{ changed=false; }

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

protected:

    SectionID		addSection(Geometry::ParametricSurface*,
				   const char* nm,const SectionID&,
				   bool addtohistory);

    virtual Geometry::ParametricSurface* createSectionSurface() const = 0;

    SubID		getSurfSubID(const RowCol&,const SectionID&) const;

    void		getMeshCoords(const PosID&,
				      Coord3& c00, Coord3& c10,
				      Coord3& c01, Coord3& c11,
				      bool& c00def, bool& c10def,
				      bool& c01def, bool& c11def,
				      const FloatMathFunction* t2d=0) const;

    Surface&			surface;
    ObjectSet<Geometry::ParametricSurface>	meshsurfaces;
    TypeSet<SectionID>		sectionids;
    BufferStringSet		sectionnames;

    RowCol			loadedstep_;
    RowCol			step_;

    float 			shift;
    bool			changed;
};


}; // Namespace

#endif
