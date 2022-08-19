#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "bufstringset.h"
#include "multiid.h"
#include "uistring.h"

namespace Attrib { class Desc; class DescSet; };
class uiParent;

/*!
\brief Creates attribute set.
*/

mExpClass(uiAttributes) AttributeSetCreator
{ mODTextTranslationClass(AttributeSetCreator);
public:
			AttributeSetCreator(uiParent*,
					    const BufferStringSet&,
					    Attrib::DescSet*);

    bool		create();
    static void		setStorageHint( const MultiID& m );

protected:

    Attrib::Desc*	getDesc(const char*);
    
    uiParent*		prnt;
    Attrib::DescSet*	attrset;
    BufferStringSet	indirects;
    BufferStringSet	directs;

};
