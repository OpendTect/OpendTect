#ifndef emsurface_h
#define emsurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurface.h,v 1.21 2003-10-17 14:19:00 bert Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "emobject.h"
#include "position.h"
#include "bufstringset.h"

/*!
Rules for surfaces.

A horizon can have many patches that can be used for reversed faults.


     ----------------------
     |                    |
     |       xxxxxx       |
     |     xxxxxxxxxx     |
     |   xx Patch 1 xxx   |
     |  XXXXXXXXXXXXXXX   |
     |                    |
     |                    |
     |     Patch 0        |
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

/*!\brief
The horizon is made up of one or more meshes (so they can overlap at faults).
The meshes are defined by knot-points in a matrix and the fillstyle inbetween
the knots.
*/

class Surface : public EMObject
{
public:
    virtual Executor*	loader(const EM::SurfaceIODataSelection* s=0,
			       int attridx=-1)			{return 0;}
    virtual Executor*	saver(const EM::SurfaceIODataSelection* s=0,
			      bool auxdataonly=false,const MultiID* key=0)
    								{return 0;}

    int			nrPatches() const;
    PatchID		patchID(int idx) const;
    PatchID		patchID(const char*) const;
    const char*		patchName(const PatchID&) const;
    PatchID		addPatch(const char* nm, bool addtohistory);
    bool		addPatch(const char* nm, PatchID, bool addtohistory);
    			/*!< Return false if the patchid allready exists */
    			
    void		removePatch(EM::PatchID, bool addtohistory);

    bool		setPos( const PatchID& patch, const SubID&,
	    			const Coord3&, bool autoconnect, bool addtoh );
    bool		setPos(const EM::PosID&, const Coord3&, bool addtohist);

    bool		isDefined( const PatchID& patch, const RowCol&) const;
    bool		isDefined( const EM::PosID& ) const;

    Coord3		getPos(const EM::PosID&) const;
    Coord3		getPos(const PatchID& patch, const RowCol&) const;
    void		getPos(const RowCol&,TypeSet<Coord3>&) const;
    			//!< Returns positions from all patches on RowCol
    
    int			findPos( const CubeSampling&,
	    			 TypeSet<EM::PosID>* res ) const;

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
    int                 findPos( const RowCol& rowcol,
                                 TypeSet<PosID>& res ) const;
    const char*		dbInfo() const			{ return dbinfo; }
    void		setDBInfo( const char* s )	{ dbinfo = s; }

    const Geometry::MeshSurface*		getSurface(PatchID) const;
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

    void		getRange(StepInterval<int>&,bool rowdir) const;
    void		getRange(const EM::PatchID&,
	    			 StepInterval<int>&,bool rowdir) const;

protected:
    bool		getMeshRowCol(const EM::SubID&,RowCol&,
	    			      const PatchID&) const;
    			/*!< Converts EM::SubID to rowcol that is used
			     on the Geometry::MeshSurface */
    bool		getMeshRowCol(const RowCol&,RowCol&,
	    			      const PatchID&) const;
    			/*!< Converts input RowCol(in surface domain)
			     to a RowCol that is used
			     on the Geometry::MeshSurface */
    EM::SubID		getSurfSubID(const RowCol&,const PatchID&) const;
    EM::SubID		getSurfSubID(const Geometry::PosID&,
	    			     const PatchID&) const;


    virtual Geometry::MeshSurface*	createPatchSurface(const PatchID&) 
								      const = 0;

//   int			findPos( const RowCol& rowcol,
//	    			 TypeSet<PosID>& res ) const;
    friend class			EMManager;
    friend class			EMObject;

    					Surface(EMManager&, const MultiID& );
    					~Surface();
    void				cleanUp();

    ObjectSet<Geometry::MeshSurface>	surfaces;
    TypeSet<PatchID>			patchids;
    BufferStringSet			patchnames;

    BufferStringSet			auxdatanames;
    BufferStringSet			auxdatainfo;
    ObjectSet<ObjectSet<TypeSet<float> > > auxdata;
    BufferString			dbinfo;

    RowCol				loadedstep;
    RowCol				step_;
    TypeSet<RowCol>			origos;

    Interval<int>*			rowinterval;
    Interval<int>*			colinterval;
};


}; // Namespace


#endif
