#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.2 2002-09-17 12:13:01 kristofer Exp $
________________________________________________________________________


-*/
#include "emobject.h"

namespace Geometry
{
    class Pos;
    class GridSurfaceImpl;
};

namespace EarthModel
{

/*!\brief

*/
class Fault : public EMObject
{
public:
			Fault( EarthModel::EMManager&, const MultiID &);
			~Fault();

    PosID		setPos( int row, int col, const Geometry::Pos& );
			/*!<
			 */
    Geometry::Pos	getPos( int row, int col ) const;
    PosID		addPosOnRow( int row, bool start,
	    			     const Geometry::Pos& );
			/*!< If start==true, adding pos first on row,
			     else last */
        
    PosID		insertPosOnRow( int row, int column, bool moveup,
					const Geometry::Pos& );
			/*!< If moveup==true, shifting upwards and inserting,
			  else downwards. */
		 
    void		insertRow( int row, bool moveup );
			/*!< If moveup==true, shifting up. */

    Geometry::GridSurfaceImpl&		getSurface() { return surface; }
    const Geometry::GridSurfaceImpl&	getSurface() const { return surface; }
    
protected:
    Geometry::GridSurfaceImpl&	surface;
};

}; // Namespace


#endif
