/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltransl.h"

#include "filepath.h"
#include "iostrm.h"
#include "strmprov.h"
#include "welldata.h"
#include "wellhdf5reader.h"
#include "wellhdf5writer.h"
#include "wellioprov.h"
#include "wellodreader.h"
#include "wellodwriter.h"


// hdfWellDataIOProvider

class hdfWellDataIOProvider : public WellDataIOProvider
{
public:
			hdfWellDataIOProvider();
			~hdfWellDataIOProvider();

private:

    Well::ReadAccess*	makeReadAccess(const IOObj&,Well::Data&,
				       uiString& errmsg) const override;
    Well::WriteAccess*	makeWriteAccess(const IOObj&,const Well::Data&,
					uiString& errmsg) const override;

public:
    static void		initClass();

};


hdfWellDataIOProvider::hdfWellDataIOProvider()
    : WellDataIOProvider(hdfWellTranslator::translKey())
{}


hdfWellDataIOProvider::~hdfWellDataIOProvider()
{}


Well::ReadAccess* hdfWellDataIOProvider::makeReadAccess( const IOObj& ioobj,
				Well::Data& wd, uiString& emsg ) const
{
    return new Well::HDF5Reader( ioobj, wd, emsg );
}


Well::WriteAccess* hdfWellDataIOProvider::makeWriteAccess( const IOObj& ioobj,
				const Well::Data& wd, uiString& emsg ) const
{
    return new Well::HDF5Writer( ioobj, wd, emsg );
}


void hdfWellDataIOProvider::initClass()
{
    WDIOPF().add( new hdfWellDataIOProvider );
}


// odWellDataIOProvider

class odWellDataIOProvider : public WellDataIOProvider
{
public:
			odWellDataIOProvider();
			~odWellDataIOProvider();

private:

    Well::ReadAccess*	makeReadAccess(const IOObj&,Well::Data&,
				       uiString& errmsg) const override;
    Well::WriteAccess*	makeWriteAccess(const IOObj&,const Well::Data&,
					uiString& errmsg) const override;

public:
    static void		initClass();

};


odWellDataIOProvider::odWellDataIOProvider()
    : WellDataIOProvider(odWellTranslator::translKey())
{}


odWellDataIOProvider::~odWellDataIOProvider()
{}


Well::ReadAccess* odWellDataIOProvider::makeReadAccess( const IOObj& ioobj,
				Well::Data& wd, uiString& emsg ) const
{
    return new Well::odReader( ioobj, wd, emsg );
}


Well::WriteAccess* odWellDataIOProvider::makeWriteAccess( const IOObj& ioobj,
				const Well::Data& wd, uiString& emsg ) const
{
    return new Well::odWriter( ioobj, wd, emsg );
}


void odWellDataIOProvider::initClass()
{
    WDIOPF().add( new odWellDataIOProvider );
}


// WellTranslatorGroup

defineTranslatorGroup(Well,"Well");

uiString WellTranslatorGroup::sTypeName( int num )
{ return uiStrings::sWell( num ); }


// WellTranslator

mDefSimpleTranslatorSelector(Well);
mDefSimpleTranslatorioContext(Well,WllInf)


bool WellTranslator::implRename( const IOObj* ioobj,
				 const char* /* newnm */ ) const
{
    if ( !ioobj )
	return false;

    if ( Well::MGR().isLoaded(ioobj->key()) )
    {
	RefMan<Well::Data> wd = Well::MGR().get( ioobj->key() );
	if ( wd )
	    wd->setName( ioobj->name() );
    }

    return true;
}


const WellDataIOProvider& WellTranslator::getProv() const
{
    return *WDIOPF().provider( userName() );
}


// hdfWellTranslator

const char* hdfWellTranslator::iconName() const
{
    return HDF5::Access::sIconName();
}


const char* hdfWellTranslator::defExtension() const
{
    return HDF5::Access::sFileExtension();
}


bool hdfWellTranslator::isUserSelectable( bool forread ) const
{
    return HDF5::isEnabled( forread ? nullptr : HDF5::sWellType() );
}


bool hdfWellTranslator::implRename( const IOObj* ioobj,
				    const char* newnm ) const
{
    if ( !ioobj || ioobj->translator()!=translKey() )
	return false;

    if ( !WellTranslator::implRename(ioobj,newnm) )
	return false;

    PtrMan<HDF5::Writer> wrr = HDF5::mkWriter();
    if ( !wrr )
    {
	WellTranslator::implRename( ioobj, newnm );
	return false;
    }

    const uiRetVal uirv = wrr->open4Edit( newnm );
    if ( uirv.isOK() )
	wrr->setAttribute( "Well name", ioobj->name().str() );

    return WellTranslator::implRename( ioobj, newnm );
}


hdfWellTranslator* hdfWellTranslator::instance()
{
    return new hdfWellTranslator( "hdf", translKey() );
}


const char* hdfWellTranslator::translKey()
{
    return "HDF";
}


void hdfWellTranslator::initClass()
{
    auto* tr = new hdfWellTranslator( "hdf", translKey() );
    WellTranslatorGroup::theInst().add( tr );
    hdfWellDataIOProvider::initClass();
}


// odWellTranslator

const char* odWellTranslator::iconName() const
{
    return WellTranslator::iconName();
}


const char* odWellTranslator::defExtension() const
{
    // Not Well::odIO::sExtWell(), which is ".well"
    return "well";
}


bool odWellTranslator::isUserSelectable( bool forread ) const
{
    return forread ? true : !HDF5::isEnabled( HDF5::sWellType() );
}


#define mImplStart(fn) \
    if ( !ioobj || ioobj->translator()!=translKey() ) \
	return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
\
    BufferString pathnm = iostrm->fileSpec().fullDirName(); \
    BufferString filenm = iostrm->fileSpec().fileName(); \
    StreamProvider prov( filenm ); \
    prov.addPathIfNecessary( pathnm ); \
    if ( !prov.fn ) return false;

#define mRemove(ext,nr,extra) \
{ \
    StreamProvider sp( Well::odIO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    const bool exists = sp.exists( true ); \
    if ( exists && !sp.remove(false) ) \
	return false; \
    extra; \
}

bool odWellTranslator::implRemove( const IOObj* ioobj, bool ) const
{
    mImplStart(remove(false));

    FilePath fp( filenm );
    fp.setExtension( nullptr );
    const BufferString bnm = fp.fullPath();
    mRemove(Well::odIO::sExtMarkers(),0,)
    mRemove(Well::odIO::sExtD2T(),0,)
    mRemove(Well::odIO::sExtCSMdl(),0,)
    mRemove(Well::odIO::sExtDispProps(),0,)
    mRemove(Well::odIO::sExtDefaults(),0,)
    mRemove(Well::odIO::sExtWellTieSetup(),0,)
    for ( int idx=1; ; idx++ )
	mRemove(Well::odIO::sExtLog(),idx,if ( !exists ) break)

    return true;
}


#define mRename(ext,nr,extra) \
{ \
    StreamProvider sp( Well::odIO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    StreamProvider spnew( Well::odIO::mkFileName(newbnm,ext,nr) ); \
    spnew.addPathIfNecessary( pathnm ); \
    const bool exists = sp.exists( true ); \
    if ( exists && !sp.rename(spnew.fileName()) ) \
	return false; \
    extra; \
}

bool odWellTranslator::implRename( const IOObj* ioobj, const char* newnm ) const
{
    mImplStart(rename(newnm));

    FilePath fp( filenm );
    fp.setExtension( nullptr );
    const BufferString bnm = fp.fullPath();
    fp.set( newnm );
    fp.setExtension( nullptr );
    const BufferString newbnm = fp.fullPath();
    mRename(Well::odIO::sExtMarkers(),0,)
    mRename(Well::odIO::sExtD2T(),0,)
    mRename(Well::odIO::sExtCSMdl(),0,)
    mRename(Well::odIO::sExtDispProps(),0,)
    mRename(Well::odIO::sExtDefaults(),0,)
    mRename(Well::odIO::sExtWellTieSetup(),0,)
    for ( int idx=1; ; idx++ )
	mRename(Well::odIO::sExtLog(),idx,if ( !exists ) break)

    return WellTranslator::implRename( ioobj, newnm );
}


bool odWellTranslator::implSetReadOnly( const IOObj* ioobj, bool ro ) const
{
    mImplStart(setReadOnly(ro));
    return true;
}


odWellTranslator* odWellTranslator::instance()
{
    return new odWellTranslator( "od", translKey() );
}


const char* odWellTranslator::translKey()
{
    return "dGB";
}


void odWellTranslator::initClass()
{
    auto* tr = new odWellTranslator( "od", translKey() );
    WellTranslatorGroup::theInst().add( tr );
    odWellDataIOProvider::initClass();
}
