#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2004
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "seisioobjinfo.h"
class uiParent;


mExpClass(uiSeis) uiSeisIOObjInfo : public SeisIOObjInfo
{ mODTextTranslationClass(uiSeisIOObjInfo);
public:

		uiSeisIOObjInfo( const uiParent* p, const IOObj* ioobj )
		    : parent_(p), SeisIOObjInfo(ioobj)		{}
		uiSeisIOObjInfo( const uiParent* p, const IOObj& ioobj )
		    : parent_(p), SeisIOObjInfo(ioobj)		{}
		uiSeisIOObjInfo( const uiParent* p, const DBKey& dbky )
		    : parent_(p), SeisIOObjInfo(dbky)		{}
		uiSeisIOObjInfo( const uiParent* p, const char* ioobjnm,
				 Seis::GeomType gt)
		    : parent_(p), SeisIOObjInfo(ioobjnm,gt)		{}
		uiSeisIOObjInfo( const uiParent* p, const SeisIOObjInfo& sii )
		    : parent_(p), SeisIOObjInfo(sii)		{}
		uiSeisIOObjInfo( const uiParent* p, const uiSeisIOObjInfo& oth )
		    : parent_(p), SeisIOObjInfo(oth)		{}

    bool	provideUserInfo() const;
    bool	provideLineInfo(const GeomIDSet* ts=0) const;

    bool	checkSpaceLeft(const SeisIOObjInfo::SpaceInfo&,
				bool error_feedback=true) const;

    const uiParent* parent_;

};
