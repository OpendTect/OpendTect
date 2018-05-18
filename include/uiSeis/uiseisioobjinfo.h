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


mExpClass(uiSeis) uiSeisIOObjInfo : public SeisIOObjInfo
{ mODTextTranslationClass(uiSeisIOObjInfo);
public:

			uiSeisIOObjInfo( const IOObj* ioobj )
			    : SeisIOObjInfo(ioobj)		{}
			uiSeisIOObjInfo( const IOObj& ioobj )
			    : SeisIOObjInfo(ioobj)		{}
			uiSeisIOObjInfo( const DBKey& dbky )
			    : SeisIOObjInfo(dbky)		{}
			uiSeisIOObjInfo( const char* ioobjnm, Seis::GeomType gt)
			    : SeisIOObjInfo(ioobjnm,gt)		{}
			uiSeisIOObjInfo( const SeisIOObjInfo& sii )
			    : SeisIOObjInfo(sii)		{}
			uiSeisIOObjInfo( const uiSeisIOObjInfo& oth )
			    : SeisIOObjInfo(oth)		{}

    bool		provideUserInfo() const;
    bool		provideLineInfo(const TypeSet<Pos::GeomID>* ts=0) const;

    bool		checkSpaceLeft(const SeisIOObjInfo::SpaceInfo&,
					bool error_feedback=true) const;

};
