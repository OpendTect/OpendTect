#ifndef emsurface_h
#define emsurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurface.h,v 1.7 2003-06-17 12:51:37 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "emobject.h"
#include "sets.h"
#include "position.h"

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
template <class T>
class Interval;

namespace Geometry
{
    class GridSurface;
};


namespace EM
{
class EMManager;

/*!\brief
The horizon is made up of one or more grids (so they can overlap at faults).
The grids are defined by knot-points in a matrix and the fillstyle inbetween
the knots.
*/

class Surface : public EMObject
{
public:
    int			nrPatches() const;
    PatchID		patchID(int idx) const;
    const char*		patchName(const PatchID&) const;
    PatchID		addPatch(const char* nm, bool addtohistory);
    bool		addPatch(const char* nm, PatchID, bool addtohistory);
    			/*!< Return false if the patchid allready exists */
    			
    void		removePatch(EM::PatchID, bool addtohistory);

    bool		setPos( PatchID patch, const SubID&, const Coord3&,
	    			bool autoconnect, bool addtohistory );
    Coord3		getPos(const EM::PosID&) const;
    bool		setPos(const EM::PosID&, const Coord3&,
	    		       bool addtohistory);
    
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
				    is removed;
			*/
    const char*		auxDataInfo(int dataidx) const;
    			/*!<\return The info of aux-data or 0 if the data
				    is removed;
			*/
    int			addAuxData( const char* name, const char* info );
    			/*!<\return The dataidx of the new data.
				    The index is persistent in runtime.
			*/
				
    void		removeAuxData( int dataidx);
    float		getAuxDataVal(int dataidx,const EM::PosID& posid) const;
    void		setAuxDataVal(int dataidx, const EM::PosID& posid,
				      float value );


    const Geometry::GridSurface*		getSurface(PatchID) const;
    bool		getGridRowCol( const EM::SubID&, RowCol& ) const;
    EM::SubID		getSurfSubID( const RowCol& ) const;
    EM::SubID		getSurfSubID( const Geometry::PosID& ) const;

    RowCol		loadedStep() const;
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

protected:
    virtual Geometry::GridSurface*		createPatchSurface() const = 0;

    int			findPos( const RowCol& rowcol,
	    			 TypeSet<PosID>& res ) const;
    friend class	EMManager;
    friend class	EMObject;

    			Surface(EMManager&, const MultiID& );
    			~Surface();
    void		cleanUp();

    ObjectSet<Geometry::GridSurface>	surfaces;
    TypeSet<PatchID>			patchids;
    ObjectSet<BufferString>		patchnames;

    ObjectSet<BufferString>			auxdatanames;
    ObjectSet<BufferString>			auxdatainfo;
    ObjectSet<ObjectSet<TypeSet<float> > >	auxdata;

    RowCol					loadedstep;
    RowCol					step;
    RowCol					origo;

    Interval<int>*				rowinterval;
    Interval<int>*				colinterval;

};


}; // Namespace


#endif
