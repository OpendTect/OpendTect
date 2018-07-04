#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "generalmod.h"
#include "arrayndinfo.h"
#include "factory.h"

namespace H5 { class H5File; }


namespace HDF5
{

class Reader;
class Writer;

typedef OD::DataRepType	ODDataType;


/*\brief Key to groups and data sets in HDF5 files.

  The group name and dataset key correspond to one dataset. Some attributes
  are valid for an entire group, then leave the dataset name empty. The
  reader and writer will use a special data set (called
  sGroupInfoDataSetName()).

 */

mExpClass(General) DataSetKey
{
public:
			DataSetKey( const char* grpnm=0, const char* dsnm=0 )
			    : dsnm_(dsnm)	{ setGroupName(grpnm); }

    inline const char*	groupName() const	{ return grpnm_; }
    inline void		setGroupName( const char* nm )
						{ grpnm_.set( nm ); }
    bool		hasGroup(const char* nm) const;

    inline const char*	dataSetName() const	{ return dsnm_; }
    inline void		setDataSetName( const char* nm )
						{ dsnm_.set( nm ); }
    inline bool		dataSetEmpty() const	{ return dsnm_.isEmpty(); }
    inline bool		hasDataSet( const char* nm ) const
						{ return dsnm_ == nm; }

    BufferString	fullDataSetName() const;

    static const char*	sGroupInfoDataSetName()	{ return "++info++"; }

protected:

    BufferString	grpnm_;
    BufferString	dsnm_;

};


/*\brief simple specification of the 'hyperslab' concept in HDF5.

  Basically, we offer a range + step in all dimensions, which seems enough for
  almost all normal work.

 */

mExpClass(General) SlabDimSpec
{
public:

    typedef ArrayNDInfo::IdxType	IdxType;

    ArrayNDInfo::IdxType start_=0, step_=1, count_=-1; //!< -1 == full size
    mImplSimpleEqOpers3Memb( SlabDimSpec, start_, step_, count_ )

};

mExpClass(General) SlabSpec : public TypeSet<SlabDimSpec>
{
public:

			mTypeDefArrNDTypes;

			SlabSpec()			{}
			SlabSpec( NrDimsType nrdims )	{ setNrDims( nrdims ); }

    void		setNrDims( NrDimsType nrdims )
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
    virtual bool	setScope(const DataSetKey&)	= 0;
    virtual od_int64	curGroupID() const		= 0;

    bool		isOpen() const			{ return file_; }
    H5::H5File*		getHDF5File()			{ return file_; }

    static uiString	sHDF5PackageDispName();
    static uiString	sHDF5NotAvailable();
    static uiString	sHDF5NotAvailable(const char* fnm);
    static uiString	sNotHDF5File(const char*);
    static uiString	sHDF5FileNoLongerAccessibe();
    uiString		sCantSetScope(const DataSetKey&) const;
    static uiString	sDataSetNotFound(const DataSetKey&);
    static uiString	sCannotReadDataSet(const DataSetKey&);
    static const char*	sFileExtension()	{ return "h5"; }

    static bool		isEnabled(const char* fortype=0);
    static bool		isEnvBlocked(const char* fortype=0);
    static bool		isHDF5File(const char*);
    static const char*	sSettingsEnabKey()	{ return "dTect.Use HDF5"; }

protected:

			Access();

    H5::H5File*		file_;
    bool		myfile_;

    virtual void	closeFile()				= 0;
    virtual void	openFile(const char*,uiRetVal&,bool ed)	= 0;

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

    mDefineFactory0ParamInClass(AccessProvider,factory);

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
