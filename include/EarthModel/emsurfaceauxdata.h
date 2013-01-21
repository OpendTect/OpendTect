#ifndef emsurfaceauxdata_h
#define emsurfaceauxdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
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


/*!
\brief Surface data
*/

mExpClass(EarthModel) SurfaceAuxData 
{
public:
			SurfaceAuxData(Horizon3D&);
			~SurfaceAuxData();
    Executor*		auxDataLoader(int selidx=-1);
    Executor*		auxDataSaver(int dataid=0,bool overwrite=false);

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
    const char*		auxDataName(int dataid) const;
    			/*!<\return The name of aux-data or 0 if the data
				    is removed; */
    int			auxDataIndex(const char*) const;
    			/*!<\return The dataid of this aux data name, or -1 */
    int			addAuxData(const char* name);
    			/*!<\return The dataid of the new data.
				    The index is persistent in runtime.  */

    void		setAuxDataName(int,const char*);    
    void		removeAuxData(int dataid);
    float		getAuxDataVal(int dataid,const PosID& posid) const;
    void		setAuxDataVal(int dataid,const PosID& posid,float val);

    void		setAuxDataShift(int,float);
    float		auxDataShift(int) const;

    bool		isChanged(int) const;
    void		resetChangedFlag();

    static bool		hasAttribute(const IOObj&,const char* attrnm);
    static BufferString	getFileName(const IOObj&,const char* attrnm);
    static BufferString	getFileName(const char* fullexp,const char* attrnm);
    static BufferString	getFreeFileName(const IOObj&);
    static bool		removeFile(const IOObj&,const char* attrnm);
    BufferString	getFileName(const char* attrnm) const;
    bool		removeFile(const char* attrnm) const;

    Array2D<float>*	createArray2D(int dataid,SectionID) const;
    void		setArray2D(int dataid,SectionID,const Array2D<float>&);

    virtual bool	usePar( const IOPar& );
    virtual void	fillPar( IOPar& ) const;

protected:
    Horizon3D&		horizon_;

    			//One entry per auxdata
    BufferStringSet	auxdatanames_;
    BufferStringSet	auxdatainfo_;
    TypeSet<float>	auxdatashift_;
        			//One entry per section
    ObjectSet<BinIDValueSet>	auxdata_;
    bool		changed_;
};


}; // namespace EM


#endif

