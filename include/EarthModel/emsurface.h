#ifndef emsurface_h
#define emsurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurface.h,v 1.41 2004-07-23 12:54:54 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "emobject.h"
#include "position.h"
#include "bufstringset.h"

template <class T, class AT> class TopList;

/*!
Rules for surfaces.

A horizon can have many patches that can be used for reversed faults.


     ----------------------
     |                    |
     |       xxxxxx       |
     |     xxxxxxxxxx     |
     |   xx Section 1 xxx   |
     |  XXXXXXXXXXXXXXX   |
     |                    |
     |                    |
     |     Section 0        |
     |                    |
     |                    |
     |                    |
     ----------------------

The area marked with x is an area with an reversed fault, and the area with x
is an own patch, while the white area is another patch. In the border between
the patches, the nodes are defined on both patches, with the same coordinate.
In addition, they are also linked together. 
*/

class BinID;
class RowCol;
class CubeSampling;
template <class T> class MathFunction;
template <class T> class Interval;
template <class T> class StepInterval;

namespace Geometry
{
    class MeshSurface;
};


namespace EM
{
class EMManager;
class SurfaceIODataSelection;
class HingeLine;
class EdgeLineManager;

class SurfaceRelations;

/*!\brief Base class for surfaces
  This is the base class for surfaces like horizons and faults. A surface is made up by one or more segments or patches, so they can overlap. 
*/


class Surface : public EMObject
{
public:
    virtual Executor*	loader(const EM::SurfaceIODataSelection* s=0,
			       int attridx=-1);
    virtual Executor*	saver(const EM::SurfaceIODataSelection* s=0,
			      bool auxdataonly=false,const MultiID* key=0);

    int			nrSections() const;
    SectionID		sectionID(int idx) const;
    SectionID		sectionID(const char*) const;
    bool		hasSection(const SectionID&) const;
    int			sectionNr(const SectionID&) const;
    const char*		sectionName(const SectionID&) const;
    SectionID		addSection(const char* nm, bool addtohistory);
    bool		addSection(const char* nm, SectionID, bool addtohistory);
    			/*!< Return false if the sectionid allready exists */
    void		removeSection(EM::SectionID, bool addtohistory);
    SectionID		cloneSection(EM::SectionID);
    CNotifier<Surface,const SectionID&>	sectionchnotifier;

    bool		setPos( const SectionID& section, const RowCol&,
	    			const Coord3&, bool autoconnect, bool addtoh );
    bool		setPos(const EM::PosID&, const Coord3&, bool addtohist);

    bool		isDefined( const SectionID& section, const RowCol&) const;
    bool		isDefined( const EM::PosID& ) const;

    Coord3		getPos(const EM::PosID&) const;
    Coord3		getPos(const SectionID& section, const RowCol&) const;
    void		getPos(const RowCol&,TypeSet<Coord3>&) const;
    			//!< Returns positions from all sections on RowCol
   
    EM::PosID		getNeighbor( const EM::PosID& posid,
	   			     const RowCol& dir ) const;
       			/*!<If the node has a neigbor in the given direction
		  	    it is returned. If not, the PosID of the neighbor
		  	    node on the same section is returned
			*/	    
    int			getNeighbors( const EM::PosID& posid, 
	    			      TypeSet<EM::PosID>* res,
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


    void		getLinkedPos( const EM::PosID& posid,
	    			      TypeSet<EM::PosID>& ) const;

    bool		isLoaded() const;
    bool		isFullResolution() const;

    int			nrAuxData() const;
    			/*!<\return	The number of data per node.
			    \note	Some of the data might have been
			    		removed, so the result might be
					misleading. Query by doing:
					\code
					for ( int idx=0; idx<nrAuxData(); idx++)
					    if ( !auxDataName(idx) )
					\endcode
			*/
    const char*		auxDataName(int dataidx) const;
    			/*!<\return The name of aux-data or 0 if the data
				    is removed; */
    int			auxDataIndex(const char*) const;
    			/*!<\return The dataidx of this aux data name, or -1 */
    int			addAuxData( const char* name );
    			/*!<\return The dataidx of the new data.
				    The index is persistent in runtime.  */

    void		setAuxDataName(int,const char*);    
    void		removeAuxData( int dataidx);
    void		removeAllAuxdata();
    float		getAuxDataVal(int dataidx,const EM::PosID& posid) const;
    void		setAuxDataVal(int dataidx, const EM::PosID& posid,
				      float value );
    const char*		dbInfo() const			{ return dbinfo; }
    void		setDBInfo( const char* s )	{ dbinfo = s; }

    void                setShift(float sh_)		{ shift = sh_; }
    float               getShift() const		{ return shift; }

    const Geometry::MeshSurface* getSurface(SectionID) const;
    RowCol		loadedStep() const;
    RowCol		step() const;
    void		setTranslatorData( const RowCol& step,
	    				   const RowCol& loadedstep,
					   const RowCol& origo,
					   const Interval<int>* rowrange,
					   const Interval<int>* colrange );
    			/*!< Sets subselection data
			    \param step		The step that the surface
						is defined in
			    \param loadedstep	The step that is loaded
			    \param origo	The origo that is used with
						step
			    \param rowrange	Should be set if a subselection
		       				is done
			    \param colrange	Should be set if a subselection
		       				is done
			*/
						

    static RowCol	subID2RowCol( const EM::SubID& );
    static EM::SubID	rowCol2SubID( const RowCol& );

    void	getRange(StepInterval<int>&,bool rowdir) const;
    void	getRange(const EM::SectionID&,
	    		 StepInterval<int>&,bool rowdir) const;

    bool	getMeshRowCol(const EM::SubID&,RowCol&, const SectionID&) const;
		/*!< Converts EM::SubID to rowcol that is used
		     on the Geometry::MeshSurface */
    bool	getMeshRowCol(const RowCol&,RowCol&, const SectionID&) const;
		/*!< Converts input RowCol(in surface domain)
		     to a RowCol that is used on the Geometry::MeshSurface
		*/
    int		findPos( const EM::SectionID& sectionid,
			 const Interval<float>& x, const Interval<float>& y,
			 const Interval<float>& z,
			 TypeSet<EM::PosID>* res ) const;
    int		findPos( const Interval<float>& x, const Interval<float>& y,
			 const Interval<float>& z,
			 TypeSet<EM::PosID>* res ) const;
    int		findPos( const CubeSampling&, TypeSet<EM::PosID>* res ) const;
    int		findPos( const RowCol& rowcol, TypeSet<PosID>& res ) const;
    bool	findClosestNodes(TopList<float,EM::PosID>& res,
	    			const Coord3& pos,
				const MathFunction<float>* depthconv=0) const;
    bool	findClosestNodes(const EM::SectionID&,
	    			TopList<float,EM::PosID>& res,
	    			const Coord3& pos,
				const MathFunction<float>* depthconv=0) const;
    bool	findClosestMesh(EM::PosID& res, const Coord3& pos,
				const MathFunction<float>* depthconv=0) const;

    bool	computeMeshNormal( Coord3& res, const EM::PosID&, 
	    			const MathFunction<float>* dconv=0) const;
    bool	computeNormal( Coord3& res, const EM::PosID& posid,
	    			const MathFunction<float>* dconv=0) const;
    bool	computeNormal( Coord3& res, const TypeSet<EM::PosID>& nodes,
				const MathFunction<float>* depthconv=0) const;
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
    bool	computeNormal( Coord3& res, const CubeSampling* cs=0,
			       const MathFunction<float>* depthconv=0) const;
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

    float	normalDistance( const Coord3&,
	    		   const MathFunction<float>* depthconv=0,
			   Interval<float>* meshvariation=0) const;
		/*!< Computes the distance along normal of the closest mesh's
		     plane.
		  \returns 	the distance. Note that the distance is
		  		negative on the back side of the surface.
		  \param	meshvariation
		  		      If set, the variation of the mesh's
		  		      coordinates' own distance along the
				      normal is set. If the returned value
				      lies inside this range, it cannot be
				      said on which side of the surface
				      the given coord is.
		*/

    char	whichSide( const Coord3&,
	    		   const MathFunction<float>* depthconv=0,
			   float fuzzyness=0 ) const;
		/*!< Determies wich side of the surface the position is.
		  \retval	1	The positive side
		  \retval	0	Is on (within precision) the surface
		  \retval	-1	The negative side
		  \retval	-2	Side could not be determined
		*/


    bool			isAtEdge(const EM::PosID&) const;

    bool			isChanged(int) const { return changed; }
    void			resetChangedFlag() { changed=false; }

    virtual bool		createFromStick(const TypeSet<Coord3>&,float)
    				{ return false; }

    virtual bool		usePar( const IOPar& );
    virtual void		fillPar( IOPar& ) const;

    SurfaceRelations&		relations;
    EdgeLineManager&		edgelinesets;

protected:
    friend class		EMManager;
    friend class		EMObject;

    				Surface(EMManager&,const EM::ObjectID&);
    				~Surface();
    void			cleanUp();
    void			removeAuxData();

    EM::SubID			getSurfSubID(const RowCol&,
	    				     const SectionID&) const;
    EM::SubID			getSurfSubID(const Geometry::PosID&,
					     const SectionID&) const;

    void			getMeshCoords( const EM::PosID&,
	    			    Coord3& c00, Coord3& c10,
				    Coord3& c01, Coord3& c11,
				    bool& c00def, bool& c10def,
				    bool& c01def, bool& c11def,
				   const MathFunction<float>* depthconv=0)const;

    virtual Geometry::MeshSurface*
				createSectionSurface(const SectionID&) const =0;
    ObjectSet<Geometry::MeshSurface>	surfaces;
    TypeSet<SectionID>			sectionids;
    BufferStringSet		sectionnames;

    BufferStringSet		auxdatanames;
    BufferStringSet		auxdatainfo;
    ObjectSet<ObjectSet<TypeSet<float> > > auxdata;
    BufferString		dbinfo;

    RowCol			loadedstep;
    RowCol			step_;
    TypeSet<RowCol>		origos;

    Interval<int>*		rowinterval;
    Interval<int>*		colinterval;
    float 			shift;

    bool			changed;
};


}; // Namespace


#endif
