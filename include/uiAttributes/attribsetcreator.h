/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2003
 RCS:           $Id: attribsetcreator.h,v 1.2 2009-01-08 08:50:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"

namespace Attrib { class Desc; class DescSet; };
class uiParent;


mClass AttributeSetCreator
{
public:
			AttributeSetCreator(uiParent*,
					    const BufferStringSet&,
					    Attrib::DescSet*);

    bool		create();

protected:

    Attrib::Desc*	getDesc(const char*);
    
    uiParent*		prnt;
    Attrib::DescSet*	attrset;
    BufferStringSet	indirects;
    BufferStringSet	directs;
};
