#ifndef emsurfaceauxdata_h
#define emsurfaceauxdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfaceauxdata.h,v 1.1 2004-08-09 14:09:31 kristofer Exp $
________________________________________________________________________


-*/

#include "bufstringset.h"

#include "emposid.h"

class Executor;
class IOPar;

namespace EM
{

class Surface;
class PosID;


class SurfaceAuxData 
{
public:
			SurfaceAuxData(Surface&);
			~SurfaceAuxData();
    Executor*		auxDataLoader(int selidx=-1);
    Executor*		auxDataSaver(int dataidx=0,int fileidx=-1);

    void		removeAll();
    void		removeSection( const SectionID& sectionid );

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
    float		getAuxDataVal(int dataidx,const PosID& posid) const;
    void		setAuxDataVal(int dataidx, const PosID& posid,
				      float value );

    bool		isChanged(int) const;
    void		resetChangedFlag();

    virtual bool	usePar( const IOPar& );
    virtual void	fillPar( IOPar& ) const;

protected:
    Surface&					surface;

    BufferStringSet				auxdatanames;
    BufferStringSet				auxdatainfo;
    ObjectSet<ObjectSet<TypeSet<float> > >	auxdata;

    bool					changed;
};


}; // Namespace


#endif
