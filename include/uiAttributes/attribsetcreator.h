/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2003
 RCS:           $Id: attribsetcreator.h,v 1.3 2009-07-22 16:01:20 cvsbert Exp $
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
