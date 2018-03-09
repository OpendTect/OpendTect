#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "generalmod.h"
#include "factory.h"

class ArrayNDInfo;
template <class T> class ArrayND;
namespace H5 { class H5File; }


namespace HDF5
{

class Reader;
class Writer;

typedef OD::DataRepType	ODDataType;
typedef ArrayND<float> FloatArrND;


/*\brief Key to the actual data in HDF5 files. */

mExpClass(General) DataSetKey
{
public:
			DataSetKey( const char* grpnm=0, const char* dsnm=0 )
			    : dsnm_(dsnm)	{ setGroupName(grpnm); }

    const char*		groupName() const	{ return grpnm_; }
    void		setGroupName( const char* nm )
			{
			    grpnm_.set( nm );
			    if ( !grpnm_.startsWith("/") )
				grpnm_.insertAt( 0 , "/" );
			}

    const char*		dataSetName() const	{ return dsnm_; }
    void		setDataSetName( const char* nm )
			{ dsnm_.set( nm ); }

    BufferString	fullDataSetName() const
			{ return BufferString(grpnm_,"/",dsnm_); }

protected:

    BufferString	grpnm_;
    BufferString	dsnm_;

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

  */


mExpClass(General) Access
{ mODTextTranslationClass(HDF5::Access);
public:

			Access();
    virtual		~Access();

    uiRetVal		open(const char*);
    virtual const char*	fileName() const			= 0;

    H5::H5File*		getHDF5File()	{ return file_; }

    static uiString	sHDF5PackageDispName();

protected:

    H5::H5File*		file_;

    virtual void	closeFile()				= 0;
    virtual void	openFile(const char*,uiRetVal&)		= 0;

    static uiString	sHDF5Err();
    static uiString	sFileNotOpen();

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

inline bool isAvailable() { return !AccessProvider::factory().isEmpty(); }
inline Reader* mkReader() { return AccessProvider::mkReader(); }
inline Writer* mkWriter() { return AccessProvider::mkWriter(); }


} // namespace HDF5
