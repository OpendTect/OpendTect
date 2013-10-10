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

#include "earthmodelmod.h"

#include "bufstringset.h"
#include "odmemory.h"

template <class T> class Array2D;


namespace EM
{

class Fault3D;

/*!
\brief Fault surface data
*/

mExpClass(EarthModel) FaultAuxData 
{
public:
			FaultAuxData(const Fault3D&);
			FaultAuxData(const MultiID&);
			~FaultAuxData();

    bool		init();
    int			setData(const char* sdname,const Array2D<float>* data,
	    			OD::PtrPolicy);
    void		setData(int sdidx,const Array2D<float>*,
	    			OD::PtrPolicy);
    const Array2D<float>* loadIfNotLoaded(const char* sdname);
    const Array2D<float>* loadIfNotLoaded(int sdidx);
    int			dataIndex(const char* sdname) const;

    void		setSelected(const TypeSet<int>& sl);
    const TypeSet<int>& selectedIndices() const	 { return selected_; }
    const BufferStringSet& selectedNames() const { return selattribnames_; }

    bool		storeData(int sdidx,bool binary);
    
    void		setDataName(int sdidx,const char* newname);    
    void		setDataName(const char* oldname,const char* newname);   
    void		removeData(const char* sdname);
    void		removeData(int sdidx);
    void		removeAllData();
    void		renameFault(const char* fltnewname);

    void		getAuxDataList(BufferStringSet&) const;
    const char*		errMsg() const		{ return errmsg_.buf(); }

protected:

    const char*		sKeyFaultAuxData()	{ return "Fault Aux Data"; }
    const char*		sKeyExtension()		{ return "auxinfo"; }	

    bool		loadData(int sdidx);
    enum Action		{ Remove=0, SetName=1 };
    void		updateDataFiles(Action,int idx,const char* newnme=0);
    void		readSDInfoFile(ObjectSet<IOPar>&);
    BufferString	createFltDataName(const char* base,int sdidx);

    const MultiID&	faultmid_;
    BufferString	fltfullnm_;
    BufferString	errmsg_;

    struct DataInfo
    {
				DataInfo();
				~DataInfo();
	bool			operator==(const DataInfo&);

	BufferString		username;
	BufferString		filename;
	const Array2D<float>*	data;
	OD::PtrPolicy		policy;
    };
    
    ObjectSet<DataInfo>	dataset_;
    TypeSet<int>	selected_;
    BufferStringSet	selattribnames_;    
};


}; // namespace EM


#endif
