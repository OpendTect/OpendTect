#ifndef emsurfaceauxdata_h
#define emsurfaceauxdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfaceauxdata.h,v 1.10 2009-06-17 16:35:48 cvskris Exp $
________________________________________________________________________


-*/

#include "typeset.h"
#include "bufstringset.h"
#include "emposid.h"

class Executor;
class IOObj;
class IOPar;
class BinIDValueSet;

template <class T> class Array2D;

namespace EM
{

class Horizon3D;
class PosID;


mClass SurfaceAuxData 
{
public:
			SurfaceAuxData(Horizon3D&);
			~SurfaceAuxData();
    Executor*		auxDataLoader(int selidx=-1);
    Executor*		auxDataSaver(int dataidx=0,bool overwrite=false);

    void		removeAll();
    void		removeSection(const SectionID&);

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
    int			addAuxData(const char* name);
    			/*!<\return The dataidx of the new data.
				    The index is persistent in runtime.  */

    void		setAuxDataName(int,const char*);    
    void		removeAuxData(int dataidx);
    float		getAuxDataVal(int dataidx,const PosID& posid) const;
    void		setAuxDataVal(int dataidx,const PosID& posid,float val);

    void		setAuxDataShift(int,float);
    float		auxDataShift(int) const;

    bool		isChanged(int) const;
    void		resetChangedFlag();

    static BufferString	getAuxDataFileName(const IOObj&,const char* attrnm);

    Array2D<float>*	createArray2D(int dataidx,SectionID) const;
    void		setArray2D(int dataidx,SectionID,const Array2D<float>&);

    virtual bool	usePar( const IOPar& );
    virtual void	fillPar( IOPar& ) const;

protected:
    Horizon3D&					horizon_;

    						//One entry per auxdata
    BufferStringSet				auxdatanames_;
    BufferStringSet				auxdatainfo_;
    TypeSet<float>				auxdatashift_;

    						//One entry per section
    ObjectSet<BinIDValueSet>			auxdata_;

    bool					changed_;
};


}; // namespace EM


#endif
