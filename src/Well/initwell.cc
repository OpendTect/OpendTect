/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "wellt2dtransform.h"
#include "wellposprovider.h"
#include "welltransl.h"


mDefModInitFn(Well)
{
    mIfNotFirstTime( return );

    WellTranslatorGroup::initClass();
    hdfWellTranslator::initClass();
    odWellTranslator::initClass();
    TranslatorGroup& welltrgrp = WellTranslatorGroup::theInst();
    for ( int idx=0; idx<welltrgrp.templates().size(); idx++ )
    {
	const Translator& transl = *welltrgrp.templates().get( idx );
	if ( transl.isUserSelectable(true) &&
	     transl.isUserSelectable(false) )
	{
	    welltrgrp.setDefTranslIdx( idx );
	    break;
	}
    }

    WellT2DTransform::initClass();
    Pos::WellProvider3D::initClass();
}
