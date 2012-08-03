#ifndef uiseisioobjinfo_h
#define uiseisioobjinfo_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseisioobjinfo.h,v 1.17 2012-08-03 13:01:08 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "seisioobjinfo.h"


mClass(uiSeis) uiSeisIOObjInfo
{
public:

    			uiSeisIOObjInfo(const IOObj&,bool error_feedback=true);
    			uiSeisIOObjInfo(const MultiID&,bool err_feedback=true);

    bool		isOK() const		{ return sii.isOK(); }
    bool		is2D() const		{ return sii.is2D(); }
    bool		isPS() const		{ return sii.isPS(); }
    bool		isTime() const		{ return sii.isTime(); }
    bool		isDepth() const		{ return sii.isDepth(); }
    const ZDomain::Def&	zDomainDef() const	{ return sii.zDomainDef(); }

    bool		provideUserInfo() const;
    bool		checkSpaceLeft(const SeisIOObjInfo::SpaceInfo&) const;

    int			expectedMBs( const SeisIOObjInfo::SpaceInfo& s ) const
					{ return sii.expectedMBs(s); }
    bool		getRanges( CubeSampling& cs ) const
					{ return sii.getRanges( cs ); }
    bool		getBPS( int& b, int icmp=-1 ) const
					{ return sii.getBPS(b,icmp); }
    void		getDefKeys( BufferStringSet& b ,bool add=true ) const
					{ return sii.getDefKeys(b,add); }

    static const char*	sKeyEstMBs;

    const SeisIOObjInfo& ioObjInfo() const	{ return sii; }
    const IOObj*	ioObj() const		{ return sii.ioObj(); }

protected:

    SeisIOObjInfo	sii;
    bool		doerrs;

};


#endif

