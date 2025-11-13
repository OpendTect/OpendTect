/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltransl.h"

#include "file.h"
#include "filepath.h"
#include "iostrm.h"
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
				 const char* newnm ) const
{
    if ( !ioobj )
	return false;

    if ( !Translator::implRename(ioobj,newnm) )
	return false;

    if ( Well::MGR().isLoaded(ioobj->key()) )
    {
	RefMan<Well::Data> wd = Well::MGR().get( ioobj->key() );
	if ( wd )
	    wd->setName( ioobj->name() );
    }

    return true;
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
	return false;

    const uiRetVal uirv = wrr->open4Edit( newnm );
    if ( uirv.isOK() )
	wrr->setAttribute( "Well name", ioobj->name().str() );

    return true;
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


bool removeAuxFile( const char* basenm, const char* ext, int nr=0 )
{
    const BufferString auxfnm = Well::odIO::mkFileName( basenm, ext, nr );
    return File::exists( auxfnm ) && File::remove( auxfnm );
}

bool odWellTranslator::implRemove( const IOObj* ioobj, bool deep ) const
{
    if ( !ioobj || ioobj->translator()!=translKey() )
	return false;

    if ( !WellTranslator::implRemove(ioobj,deep) )
	return false;

    // Main file has been removed, no return false, as there is no going back
    FilePath fp( ioobj->mainFileName() );
    fp.setExtension( nullptr );
    const BufferString basenm = fp.fullPath();
    removeAuxFile( basenm, Well::odIO::sExtMarkers() );
    removeAuxFile( basenm, Well::odIO::sExtD2T() );
    removeAuxFile( basenm, Well::odIO::sExtCSMdl() );
    removeAuxFile( basenm, Well::odIO::sExtDispProps() );
    removeAuxFile( basenm, Well::odIO::sExtDefaults() );
    removeAuxFile( basenm, Well::odIO::sExtWellTieSetup() );
    int logidx = 1;
    while ( removeAuxFile(basenm,Well::odIO::sExtLog(),logidx) )
	logidx++;

    return true;
}


bool renameAuxFile( const char* basenm, const char* newbasenm,
		    const char* ext, int nr=0 )
{
    const BufferString auxfnm = Well::odIO::mkFileName( basenm, ext, nr );
    const BufferString newauxfnm = Well::odIO::mkFileName( newbasenm, ext, nr );
    return File::exists( auxfnm ) && File::rename( auxfnm, newauxfnm );
}

bool odWellTranslator::implRename( const IOObj* ioobj, const char* newnm ) const
{
    if ( !ioobj || ioobj->translator()!=translKey() )
	return false;

    if ( !WellTranslator::implRename(ioobj,newnm) )
	return false;

    // Main file has been renamed, no return false, as there is no going back
    FilePath fp( ioobj->mainFileName() );
    fp.setExtension( nullptr );
    const BufferString basenm = fp.fullPath();
    fp.set( newnm );
    fp.setExtension( nullptr );
    const BufferString newbasenm = fp.fullPath();

    renameAuxFile( basenm, newbasenm, Well::odIO::sExtMarkers() );
    renameAuxFile( basenm, newbasenm, Well::odIO::sExtD2T() );
    renameAuxFile( basenm, newbasenm, Well::odIO::sExtCSMdl() );
    renameAuxFile( basenm, newbasenm, Well::odIO::sExtDispProps() );
    renameAuxFile( basenm, newbasenm, Well::odIO::sExtDefaults() );
    renameAuxFile( basenm, newbasenm, Well::odIO::sExtWellTieSetup() );
    int logidx = 1;
    while ( renameAuxFile(basenm,newbasenm,Well::odIO::sExtLog(),logidx) )
	logidx++;

    return true;
}


bool odWellTranslator::implSetReadOnly( const IOObj* ioobj, bool ro ) const
{
    if ( !ioobj || ioobj->translator()!=translKey() )
	return false;

    return WellTranslator::implSetReadOnly( ioobj, ro );
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
