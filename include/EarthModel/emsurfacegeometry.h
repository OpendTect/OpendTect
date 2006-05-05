#ifndef emsurfacegeometry_h
#define emsurfacegeometry_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfacegeometry.h,v 1.21 2006-05-05 19:43:51 cvskris Exp $
________________________________________________________________________


-*/

#include "bufstringset.h"
#include "emposid.h"
#include "emobject.h"
#include "position.h"
#include "mathfunc.h"
#include "rowcolsurface.h"

template <class T, class AT> class TopList;


class RowCol;
template <class T> class Interval;
template <class T> class StepInterval;


namespace Geometry { class RowColSurface; };


namespace EM
{
class Surface;
class SurfaceIODataSelection;

/*!\brief 
*/


class SurfaceGeometry
{
public:
    			SurfaceGeometry(Surface&);
    virtual		~SurfaceGeometry();
    virtual void	removeAll();

    virtual bool	enableChecks(bool yn);
    virtual bool	isChecksEnabled() const;
    virtual bool	isNodeOK(const PosID&) const;

    int			nrSections() const;
    SectionID		sectionID(int idx) const;
    SectionID		sectionID(const char*) const;
    bool		hasSection(const SectionID&) const;
    int			sectionIndex(const SectionID&) const;
    const char*		sectionName(const SectionID&) const;
    bool		setSectionName( const SectionID&, const char*,
	    				bool addtohistory );
    SectionID		addSection(const char* nm,bool addtohistory);
    SectionID		addSection(const char* nm,const SectionID&,
	    			   bool addtohistory);
    			/*!<Return false if the sectionid allready exists */
    virtual bool	removeSection(const SectionID&,bool addtohistory);
    virtual SectionID	cloneSection(const SectionID&);
    //void		checkSections();
    
    virtual const Geometry::Element*	sectionGeometry(const SectionID&) const;
    virtual Geometry::Element*		sectionGeometry(const SectionID&);
    virtual int		getConnectedPos(const PosID& posid,
	    				TypeSet<PosID>* res) const;
    			/*!<\returns the number of connected pos. */
    virtual void	getLinkedPos(const PosID& posid,TypeSet<PosID>&) const;
    virtual bool	isAtEdge(const PosID&) const;

    bool		isChanged(int) const		{ return changed_; }
    void		resetChangedFlag()		{ changed_=false; }

    virtual bool	isLoaded() const;
    virtual bool	isFullResolution() const;
    virtual Executor*	loader(const SurfaceIODataSelection* s=0);
    virtual Executor*	saver(const SurfaceIODataSelection* s=0,
			      const MultiID* key=0);

    virtual int		findPos(const SectionID&,const Interval<float>& x,
	    			const Interval<float>& y,
				const Interval<float>& z,
				TypeSet<PosID>* res) const;
    virtual int		findPos(const Interval<float>& x,
	    			const Interval<float>& y,
				const Interval<float>& z,TypeSet<PosID>*) const;
    virtual int		findPos(const CubeSampling&,TypeSet<PosID>*) const;

    //bool		findClosestNodes(TopList<float,PosID>& res,
//					 const Coord3& pos) const;
 //   bool		findClosestNodes(const SectionID&,
//					 TopList<float,PosID>& res,
//					 const Coord3& pos) const;
 //   bool		computeNormal(Coord3& res,const PosID& posid,
//				      bool normalize=true) const;

 //   float		normalDistance(const Coord3&) const;
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

  //  char		whichSide(const Coord3&, float fuzzyness=0) const;
			/*!< Determies wich side of the surface the position is.
			  \retval	1	The positive side
			  \retval	0	Is on the surface
			  \retval	-1	The negative side
			  \retval	-2	Side could not be determined
			*/

    virtual EMObjectIterator*	createIterator(const EM::SectionID&) const;


    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
protected:

    SectionID			addSectionInternal(Geometry::Element*,
					   const char* nm,const SectionID&,
					   bool addtohistory);


    virtual Geometry::Element*		createSectionGeometry() const = 0;

    Surface&				surface_;
    ObjectSet<Geometry::Element>	sections_;
    TypeSet<SectionID>			sids_;
    BufferStringSet			sectionnames_;

    bool				changed_;

};


class RowColSurfaceGeometry : public SurfaceGeometry
{
public:
    			RowColSurfaceGeometry(Surface&);
    virtual		~RowColSurfaceGeometry();


    const Geometry::RowColSurface* sectionGeometry(const SectionID&) const;

    StepInterval<int>		rowRange(const SectionID&) const;
    StepInterval<int>		rowRange() const;
    StepInterval<int>		colRange(const SectionID&,int row) const;
    StepInterval<int>		colRange() const;
    StepInterval<int>		colRange(int row) const;

    EMObjectIterator*		createIterator(const EM::SectionID&) const;

    //Interval<float>	getZRange(const Interval<int>& rowrg,
	    			  //const Interval<int>& colrg) const;

protected:

//    SubID		getSurfSubID(const RowCol&,const SectionID&) const;

    //bool		insertRowOrCol( const SectionID&, int rc, bool row,
//	    				bool hist );
    			/*!<Implementation of insertRow and insertCol. */

};


}; // Namespace

#endif
