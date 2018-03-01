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

typedef BufferStringSet	GroupPath;


mExpClass(General) Access
{ mODTextTranslationClass(HDF5::Access);
public:

			Access();
    virtual		~Access();

    uiRetVal		open(const char*);
    virtual const char*	fileName() const			= 0;

    virtual int		chunkSize() const			= 0;

    H5::H5File*		getHDF5File()	{ return file_; }

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

    mDefineFactory0ParamInClass(AccessProvider,factory);

    virtual Reader*		getReader() const			= 0;
    virtual Writer*		getWriter() const			= 0;

};

inline bool isAvailable() { return !AccessProvider::factory().isEmpty(); }

} // namespace HDF5
