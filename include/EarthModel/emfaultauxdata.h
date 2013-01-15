#ifndef emfaultauxdata_h
#define emfaultauxdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		08-01-2012
 RCS:		$Id$
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

class Fault3D;
class PosID;

/*!
\ingroup EarthModel
\brief Fault surface data
*/

mClass(EarthModel) FaultAuxData 
{
public:
			FaultAuxData(const Fault3D&);
			~FaultAuxData();

    Executor*		dataLoader(int sdidx);
    Executor*		dataSaver(int sdidx=0,bool overwrite=false);

    int			addData(const char* name);
    void		setDataName(int,const char*);    
    void		removeData(int sdidx);
    const char*		dataName(int sdidx) const;
    int			dataIndex(const char*) const;

    const BufferStringSet& auxDataList();
    
    enum Action		{ Add=0, Remove=1, Rename=2 };
    void		updateDataInfoFile(Action,int idx,const char* newname);

    float		getDataVal(int sdidx,const PosID& posid) const;
    void		setDataVal(int sdidx,const PosID& posid,float val);

    bool		isChanged(int) const	{ return changed_; }
    void		resetChangedFlag()	{ changed_ = false; }

    Array2D<float>*	createArray2D(int sdidx) const;
    void		setArray2D(int sdidx,const Array2D<float>&);

    void		readSDInfoFile(ObjectSet<IOPar>&);

    const char*		sKeyFaultAuxData()	{ return "Fault Aux Data"; }
    const char*		sKeyExtension()		{ return "auxdatainfo"; }	

protected:

    void			init();
    BufferString		getFileName(const IOObj&,const char* attrnm);
    BufferString		getFileName(const char* fulluserexp,
	    				    const char* attrnm);

    const Fault3D&		fault_;
    BufferString		fltfullnm_;
    BinIDValueSet*		surfdata_;

    BufferStringSet		sdusernames_;
    BufferStringSet		sdfilenames_;
    bool			changed_;
};


}; // namespace EM


#endif
