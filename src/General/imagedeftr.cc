/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "imagedeftr.h"

#include "file.h"
#include "uistrings.h"


ImageDef::ImageDef()
{}


ImageDef::~ImageDef()
{}


bool ImageDef::fillPar( IOPar& par ) const
{
    par.set( sKey::FileName(), filename_ );
    par.set( IOPar::compKey(sKey::Position(),0), tlcoord_ );
    par.set( IOPar::compKey(sKey::Position(),1), brcoord_ );
    if ( trcoord_.isDefined() )
	par.set( IOPar::compKey(sKey::Position(),2), trcoord_ );
    if ( blcoord_.isDefined() )
	par.set( IOPar::compKey(sKey::Position(),3), blcoord_ );

    return true;
}


bool ImageDef::usePar( const IOPar& par )
{
    mSetUdf( tlcoord_ );
    mSetUdf( brcoord_ );
    mSetUdf( trcoord_ );
    mSetUdf( blcoord_ );

    par.get( sKey::FileName(), filename_ );
    par.get( IOPar::compKey(sKey::Position(),0), tlcoord_ );
    par.get( IOPar::compKey(sKey::Position(),1), brcoord_ );
    par.get( IOPar::compKey(sKey::Position(),2), trcoord_ );
    par.get( IOPar::compKey(sKey::Position(),3), blcoord_ );
    return true;
}


defineTranslatorGroup(ImageDef,"Image")
mDefSimpleTranslatorSelector(ImageDef)
mDefSimpleTranslatorioContext(ImageDef,Misc)
defineTranslator(OD,ImageDef,"Image")


const char* ImageDefTranslator::sKeyImageSpecs()
{
    return "Image Specifications";
}


uiString ImageDefTranslatorGroup::sTypeName( int num )
{
    return uiStrings::sImage(num);
}

bool ODImageDefTranslator::read( ImageDef& def, const IOObj& ioobj )
{
    const BufferString specsfnm = ioobj.fullUserExpr();
    IOPar specs;
    if ( !specs.read(specsfnm,sKeyImageSpecs()) )
    {
	errmsg_ = tr("Cannot read Image metadata.");
	return false;
    }

    def.usePar( specs );
    if ( !File::exists(def.filename_) )
    {
	errmsg_ = tr("Image file '%1' does not exist.").arg(def.filename_);
	return false;
    }

    return true;
}


bool ODImageDefTranslator::write( const ImageDef& def, const IOObj& ioobj )
{
    const BufferString specsfnm = ioobj.fullUserExpr();
    IOPar specs;
    def.fillPar( specs );
    return specs.write( specsfnm, sKeyImageSpecs() );
}


bool ODImageDefTranslator::implRename( const IOObj* ioobj,
				       const char* newnm) const
{
    return Translator::implRename( ioobj, newnm );
}


bool ODImageDefTranslator::implRemove( const IOObj* ioobj, bool deep ) const
{
    return Translator::implRemove( ioobj, deep );
}


bool ODImageDefTranslator::readDef( ImageDef& def, const IOObj& ioobj )
{
    PtrMan<Translator> translator = ioobj.createTranslator();
    mDynamicCastGet(ODImageDefTranslator*,imgtr,translator.ptr())
    return imgtr ? imgtr->read( def, ioobj ) : false;
}


bool ODImageDefTranslator::writeDef( const ImageDef& def, const IOObj& ioobj )
{
    PtrMan<Translator> translator = ioobj.createTranslator();
    mDynamicCastGet(ODImageDefTranslator*,imgtr,translator.ptr())
    return imgtr ? imgtr->write( def, ioobj ) : false;
}
