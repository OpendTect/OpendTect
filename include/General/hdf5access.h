#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "arrayndinfo.h"
#include "factory.h"

namespace H5 { class DataSet; class Group; class H5File; class H5Location;
	       class H5Object; }

#define mDefHDF5FileExt "h5"


namespace HDF5
{

class Reader;
class Writer;

typedef OD::DataRepType	ODDataType;


/*\brief Key to groups and data sets in HDF5 files.

  The group name and dataset key correspond to one dataset. Some attributes
  are valid for an entire group, then leave the dataset name empty.
  Leave the group name empty (null) for a top level DataSet

  One can set edition and chunk sizes properties, which will be
  used when creating the dataset and writing to it.

 */

mExpClass(General) DataSetKey
{
public:
			DataSetKey(const char* grpnm=nullptr,
				   const char* dsnm=nullptr);

    inline const char*	groupName() const	{ return grpnm_; }
    inline DataSetKey&	setGroupName( const char* nm )
					{ grpnm_.set( nm ); return *this; }
    bool		hasGroup(const char* nm) const;

    inline const char*	dataSetName() const	{ return dsnm_; }
    inline DataSetKey&	setDataSetName( const char* nm )
					{ dsnm_.set( nm ); return *this; }
    inline bool		dataSetEmpty() const	{ return dsnm_.isEmpty(); }
    inline bool		hasDataSet( const char* nm ) const
						{ return dsnm_ == nm; }

    inline bool		isEditable() const	{ return editable_; }
    inline void		setEditable( bool yn )	{ editable_ = yn; }

    BufferString	fullDataSetName() const;

    static DataSetKey	groupKey(const DataSetKey& parentgrp,
				 const char* subgrpnm);
    static DataSetKey	groupKey(const char* parentfulldsnm,const char* grpnm);

    void		setChunkSize(int idim,int sz);
    void		setChunkSize(const int* szs,int nrdims=1,
				     int from=0);
			//<! Pass nullptr to disable
    void		setMaximumSize(int idim,int maxsz=256);
			/*!< Always switches on the editability
			     Pass mUdf(int) to disable for a given dimension */

    int			chunkSz(int idim) const;
    int			maxDimSz(int idim) const;

protected:

    BufferString	grpnm_;
    BufferString	dsnm_;

    bool		editable_ = false;
    TypeSet<int>	chunkszs_;
    TypeSet<int>	maxsizedim_;

};


/*\brief simple specification of the 'hyperslab' concept in HDF5.

  Basically, we offer a range + step in all dimensions, which seems enough for
  almost all normal work.

 */

mExpClass(General) SlabDimSpec
{
public:

    typedef ArrayNDInfo::idx_type	idx_type;

    ArrayNDInfo::idx_type start_=0, step_=1, count_=-1; //!< -1 == full size
    bool		operator==( const SlabDimSpec& oth ) const
			{ return start_==oth.start_ && step_==oth.step_
						    && count_==oth.count_; }

};

mExpClass(General) SlabSpec : public TypeSet<SlabDimSpec>
{
public:

			mTypeDefArrNDTypes;

			SlabSpec()			{}
			SlabSpec( nr_dims_type nrdims )	{ setNrDims( nrdims ); }

    void		setNrDims( nr_dims_type nrdims )
			{
			    for ( int idim=nrdims; idim<size(); idim++ )
				removeSingle( size()-1 );
			    for ( int idim=size(); idim<nrdims; idim++ )
				*this += SlabDimSpec();
			}
};


/*\brief baseclass for reader and writer of HDF5 files.

  HDF5 can be seen as a (simple) database in a file. As usual with this sort of
  general-purpose utilities, the data model is bloated by many options that are
  only interesting for some. As usual, we take our pick of the core stuff:

  * The database is essentially a tree of nodes that are called 'Group'.
  You can see this structure as a file system and it does use names like in a
  UNIX file system. The root group '/' is always there, and it is the 'H5File'
  itself. Thus, the Group is a 'directory'. Actually, it maps to a class
  called H5::CommonFG, the base class for H5::H5File and H5::Group.

  * Where 'Group' is like a directory, the files are 'DataSet' objects. Every
  'DataSet' can be seen as an N-dimensional array of data. The dimensionalities
  are covered by the 'DataSpace' object, which maps to our ArrayNDInfo.
  Actually, as you will most likely get/put ArrayND data there, you can
  see a DataSet as an ArrayND with accompanying properties.

  * Each DataSet can have properties attached, like 'file header values'.
  These are key-value pairs. The value can be anything - but our interface
  will only read and write strings. This maps the set of properties to one
  IOPar.

  Notes:
  * HDF5 'stores' can refer to more than 1 file. We do not use or support this
    facilty.
  * If you read HDF5 files not produced by us, there may be 'advanced' stuff
    in there that we don't handle. The strategy is to do as if they do not
    exist, and handle everything gracefully.
  * The HDF5 data type system is richer than ours. We'll try to map to the
    'nearest'.
  * HDF5 and this interface are *not* MT-safe. Build your own locking.

  */


mExpClass(General) Access
{ mODTextTranslationClass(HDF5::Access);
public:
			mTypeDefArrNDTypes;

    virtual		~Access();

    uiRetVal		open(const char*);
    virtual const char*	fileName() const		= 0;
    virtual bool	isReader() const		= 0;

    virtual DataSetKey	scope() const			= 0;
    virtual od_int64	curGroupID() const		= 0;

    bool		isOpen() const			{ return file_; }
    H5::H5File*		getHDF5File()			{ return file_; }
    bool		hasGroup(const char* grpnm) const;
    bool		hasDataSet(const DataSetKey&) const;

    static uiString	sHDF5PackageDispName();
    static uiString	sHDF5NotAvailable();
    static uiString	sHDF5NotAvailable(const char* fnm);
    static uiString	sNotHDF5File(const char*);
    static uiString	sHDF5FileNoLongerAccessibe();
    uiString		sCantSetScope(const DataSetKey&) const;
    static uiString	sDataSetNotFound(const DataSetKey&);
    static uiString	sCannotReadDataSet(const DataSetKey&);
    static const char*	sFileExtension()	{ return mDefHDF5FileExt; }

    static bool		isEnabled(const char* fortype=0);
    static bool		isEnvBlocked(const char* fortype=0);
    static bool		isHDF5File(const char*);
    static const char*	sSettingsEnabKey()	{ return "dTect.Use HDF5"; }

protected:

			Access();

    H5::H5File*		file_;
    bool		myfile_;

    virtual void	closeFile()					= 0;
    virtual void	openFile(const char*,uiRetVal&,bool ed)		= 0;

    virtual H5::H5Location*	setLocation(const DataSetKey*)		= 0;
    virtual H5::H5Location*	getLocation(const DataSetKey*) const	= 0;
    virtual H5::H5Object*	setScope(const DataSetKey*)		= 0;
    virtual H5::H5Object*	getScope(const DataSetKey*) const	= 0;
    virtual H5::Group*		setGrpScope(const DataSetKey*)		= 0;
    virtual H5::Group*		getGrpScope(const DataSetKey*) const	= 0;
    virtual H5::DataSet*	setDSScope(const DataSetKey&)		= 0;
    virtual H5::DataSet*	getDSScope(const DataSetKey&) const	= 0;
			//!< Returns (new) scope. null for root scope

    static uiString	sHDF5Err(const uiString&);
    static uiString	sFileNotOpen();

    static const char*	sOpenFileFirst();
    static const char*	sNeedScope();
    static const char*	sNoDataPassed();

    friend class	AccessImpl;

};


mExpClass(General) AccessProvider
{
public:

    virtual			~AccessProvider()			{}

    mDefineFactoryInClass(AccessProvider,factory)

    virtual Reader*		getReader() const			= 0;
    virtual Writer*		getWriter() const			= 0;

    static Reader*		mkReader(int n=-1);
    static Writer*		mkWriter(int n=-1);

protected:

    static AccessProvider*	mkProv(int);

};

inline Reader* mkReader() { return AccessProvider::mkReader(); }
inline Writer* mkWriter() { return AccessProvider::mkWriter(); }
inline const char* sFileExtension() { return Access::sFileExtension(); }

inline bool isAvailable()
{ return !AccessProvider::factory().isEmpty(); }
inline bool isEnabled( const char* typ=0 )
{ return Access::isEnabled(typ); }
inline bool isEnvBlocked( const char* typ=0 )
{ return Access::isEnvBlocked(typ);}
inline bool isHDF5File( const char* fnm )
{ return Access::isHDF5File(fnm); }

inline const char* sSeismicsType()	{ return "Seismics"; }
inline const char* sPickSetType()	{ return "PickSets"; }
inline const char* sWellType()		{ return "Wells"; }

} // namespace HDF5
