/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2003
 RCS:           $Id: attribsetcreator.h,v 1.4 2009-07-23 12:33:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "multiid.h"

namespace Attrib { class Desc; class DescSet; };
class uiParent;


mClass AttributeSetCreator
{
public:
			AttributeSetCreator(uiParent*,
					    const BufferStringSet&,
					    Attrib::DescSet*);

    bool		create();
    static void		setStorageHint( const MultiID& m ) { storhint_ = m; }

protected:

    Attrib::Desc*	getDesc(const char*);
    
    uiParent*		prnt;
    Attrib::DescSet*	attrset;
    BufferStringSet	indirects;
    BufferStringSet	directs;

    static MultiID	storhint_;
};
