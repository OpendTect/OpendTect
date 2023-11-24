#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "typeset.h"
#include "bufstringset.h"
#include "emposid.h"
#include "uistring.h"

class Executor;
class IOObj;
class BinIDValueSet;
class TrcKeySampling;

template <class T> class Array2D;
namespace Pos { class Filter; }

namespace EM
{

class Horizon3D;
class PosID;


/*!
\brief Surface data
*/

mExpClass(EarthModel) SurfaceAuxData
{ mODTextTranslationClass(SurfaceAuxData)
public:
			SurfaceAuxData(Horizon3D&);
    virtual		~SurfaceAuxData();

    Executor*		auxDataLoader(int selidx=-1);
    Executor*		auxDataLoader(const char* nm);
    Executor*		auxDataSaver(int dataidx=0,bool overwrite=false);

    void		removeAll();

    bool		validIdx(int idx) const;
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
    bool		hasAuxDataName(const char*) const;
    const char*		auxDataName(int dataidx) const;
			/*!<\return The name of aux-data or 0 if the data
				    is removed; */
    int			auxDataIndex(const char*) const;
			/*!<\return The dataidx of this aux data name, or -1 */
    int			addAuxData(const char* name);
			/*!<\return The dataidx of the new data.
				    The index is persistent in runtime.  */

    void		setAuxDataName(int dataidx,const char*);
    void		removeAuxData(int dataidx);
    float		getAuxDataVal(int dataidx,const PosID& posid) const;
    float		getAuxDataVal(int dataidx,const TrcKey&) const;
    float		getAuxDataVal(int dataidx,const BinID&) const;
    void		setAuxDataVal(int dataidx,const PosID& posid,float val);
    void		setAuxDataVal(int dataidx,const PosID& posid,float val,
				      bool onlynewpos);
    void		setAuxDataVal(int dataidx,const TrcKey&,float val);
    void		setAuxDataVal(int dataidx,const BinID&,float val);

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

    void		init(int dataidx,bool onlynewpos=false,
			     float val=mUdf(float));
			/*!<dataidx==-1: init all*/

    Array2D<float>*	createArray2D(int dataidx) const;
    void		setArray2D(int dataidx,const Array2D<float>&,
				   const TrcKeySampling* tks=nullptr);
			/*!tks=nullptr assumes that array has same origin
			   as horizon*/

    const ObjectSet<BinIDValueSet>& getData() const	{ return auxdata_; }

    bool		usePar( const IOPar& );
    void		fillPar( IOPar& ) const;
    void		init(int dataidx,float val=mUdf(float));
			/*!<dataidx==-1: init all*/
    enum		AuxDataType { NoType=0, AutoShow, Tracking };
    void		setAuxDataType(int dataidx,AuxDataType);
    AuxDataType		getAuxDataType(int dataidx) const;

    void		applyPosFilter(const Pos::Filter&,int dataidx=-1);

    void		addAuxData(const BufferStringSet&,
							const BinIDValueSet&);


// Deprecated public functions

    mDeprecated("Use without SectionID")
    Array2D<float>*	createArray2D(int dataidx,SectionID) const
			{ return createArray2D(dataidx); }
    mDeprecated("Use without SectionID")
    void		setArray2D(int dataidx,SectionID,
				   const Array2D<float>& arr,
				   const TrcKeySampling* tks=nullptr)
			{ setArray2D(dataidx,arr,tks); }
//    mDeprecatedObs
    void		removeSection(const SectionID&);

protected:
    Horizon3D&		horizon_;

			//One entry per auxdata
    BufferStringSet	auxdatanames_;
    BufferStringSet	auxdatainfo_;
    TypeSet<float>	auxdatashift_;
			//One entry per section
    ObjectSet<BinIDValueSet>	auxdata_;
    bool		changed_;
    TypeSet<AuxDataType> auxdatatypes_;
};

} // namespace EM
