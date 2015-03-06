#ifndef basemapzvalues_h
#define basemapzvalues_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basemapmod.h"
#include "bufstringset.h"
#include "typeset.h"

class IOPar;

namespace Basemap
{

mExpClass(Basemap) ZValueMgr
{
public:
			ZValueMgr();
			~ZValueMgr();

    void		set(const char* key,int zval);
    int			get(const char* key) const;

protected:
    IOPar&		iopar_;

    BufferStringSet	keys_;
    TypeSet<int>	zvals_;
};

mGlobal(Basemap) ZValueMgr& ZValues();

} // namespace Basemap

#endif

