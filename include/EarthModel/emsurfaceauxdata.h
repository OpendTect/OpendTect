#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emcommon.h"
#include "typeset.h"
#include "bufstringset.h"
#include "uistring.h"

class BinIDValueSet;
class Executor;
class IOObj;
class UnitOfMeasure;

template <class T> class Array2D;

namespace EM
{

class Horizon3D;


/*!\brief Surface data like attribute data. */

mExpClass(EarthModel) SurfaceAuxData
{ mODTextTranslationClass(SurfaceAuxData);
public:

    typedef int		AuxID;

			SurfaceAuxData(Horizon3D&);
    virtual		~SurfaceAuxData();
    Executor*		auxDataLoader(int selidx=-1);
    Executor*		auxDataLoader(const char* nm);
    Executor*		auxDataSaver(AuxID id=0,bool overwrite=false);

    void		removeAll();

    bool		isUsable(AuxID) const;
    int			nrAuxData() const	{ return usable_.size(); }
    int			nrUsableAuxData() const;

    bool		hasAuxDataName(const char*) const;
    const char*		auxDataName(AuxID) const;
    const char*		firstUsableAuxDataName() const;
    void		getUsableAuxDataNames(BufferStringSet&) const;
    AuxID		auxDataIndex(const char*) const;
    AuxID		addAuxData(const char* name);
			    /*!<\return the AuxID of the new data. */

    void		setAuxDataName(AuxID,const char*);
    void		removeAuxData(AuxID);
    float		getAuxDataVal(AuxID,const PosID&) const;
    float		getAuxDataVal(AuxID,const TrcKey&) const;
    void		setAuxDataVal(AuxID,const PosID&,float val,
				      bool onlynewpos=false);
    void		setAuxDataVal(AuxID,const TrcKey&,float val);

    enum		AuxDataType { NoType=0, AutoShow, Tracking };
    void		setAuxDataType(AuxID,AuxDataType);
    AuxDataType		getAuxDataType(AuxID) const;

    void		setAuxDataShift(AuxID,float);
    float		auxDataShift(AuxID) const;

    void		setUnit(AuxID,const UnitOfMeasure*);
    const UnitOfMeasure* unit(AuxID) const;

    bool		isChanged(AuxID) const;
    void		resetChangedFlag();

    static bool		hasAttribute(const IOObj&,const char* attrnm);
    static BufferString	getFileName(const IOObj&,const char* attrnm);
    static BufferString	getFileName(const char* fullexp,const char* attrnm);
    static BufferString	getFreeFileName(const IOObj&);
    static bool		removeFile(const IOObj&,const char* attrnm);
    BufferString	getFileName(const char* attrnm) const;
    bool		removeFile(const char* attrnm) const;

    void		init(AuxID,bool onlynewpos=false,
			     float val=mUdf(float)); //!< auxid==-1: init all

    Array2D<float>*	createArray2D(AuxID) const;
    void		setArray2D(AuxID,const Array2D<float>&);

    const BinIDValueSet& getData() const	{ return auxdata_; }

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

protected:

    Horizon3D&		horizon_;
    BinIDValueSet&	auxdata_;

			//One entry per auxdata
    BoolTypeSet		usable_;
    BufferStringSet	auxdatanames_;
    BufferStringSet	auxdatainfo_;
    TypeSet<float>	auxdatashift_;
    TypeSet<AuxDataType> auxdatatypes_;
    ObjectSet<const UnitOfMeasure> units_;

    bool		changed_;

    int			getColIdx(AuxID) const;

};

} // namespace EM
