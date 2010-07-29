#ifndef uiseisioobjinfo_h
#define uiseisioobjinfo_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseisioobjinfo.h,v 1.13 2010-07-29 13:49:43 cvsbert Exp $
________________________________________________________________________

-*/

#include "seisioobjinfo.h"


mClass uiSeisIOObjInfo
{
public:

    			uiSeisIOObjInfo(const IOObj&,bool error_feedback=true);
    			uiSeisIOObjInfo(const MultiID&,bool err_feedback=true);

    bool		isOK() const		{ return sii.isOK(); }
    bool		is2D() const		{ return sii.is2D(); }
    bool		isPS() const		{ return sii.isPS(); }
    bool		isTime() const		{ return sii.isTime(); }
    bool		isDepth() const		{ return sii.isDepth(); }

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

    			// 2D only
    void		getLineNames(BufferStringSet& b, bool add=true,
	    				const BinIDValueSet* bvs=0 ) const
				{ sii.getLineNames(b,add,bvs); }
    void		getAttribNames( BufferStringSet& b, bool add=true,
	    				const BinIDValueSet* bvs=0,
	   				const char* datatyp=0,
	   				bool allowcnstabsent=false,
	   				bool incl=true ) const
				{ sii.getAttribNames(b,add,bvs,datatyp,
					             allowcnstabsent,incl); }
    void		getAttribNamesForLine( const char* nm,
					BufferStringSet& b, bool add=true,
					const char* datatyp=0,
	    				bool allowcnstabsent=false,
	   				bool incl=true ) const
				{ sii.getAttribNamesForLine(nm,b,add,datatyp,
							    allowcnstabsent,
							    incl); }
    void		getLineNamesWithAttrib( const char* nm,
					BufferStringSet& b, bool add=true) const
				{ sii.getLineNamesWithAttrib(nm,b,add); }

    static const char*	sKeyEstMBs;

    const IOObj*	ioObj() const		{ return sii.ioObj(); }

protected:

    SeisIOObjInfo	sii;
    bool		doerrs;

};


#endif
