/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2003
 RCS:           $Id: attribsetcreator.h,v 1.5 2009/07/28 07:09:41 cvsranojay Exp $
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
    static void		setStorageHint( const MultiID& m );

protected:

    Attrib::Desc*	getDesc(const char*);
    
    uiParent*		prnt;
    Attrib::DescSet*	attrset;
    BufferStringSet	indirects;
    BufferStringSet	directs;

};
