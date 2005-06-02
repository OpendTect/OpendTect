#ifndef seisioobjinfo_h
#define seisioobjinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id: seisioobjinfo.h,v 1.1 2005-06-02 14:11:52 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "samplingdata.h"
class IOObj;
class MultiID;
class CubeSampling;
class BinIDValueSet;
class BufferStringSet;


/*!\brief Info on IOObj for seismics */

class SeisIOObjInfo
{
public:

			SeisIOObjInfo(const IOObj*);
			SeisIOObjInfo(const IOObj&);
			SeisIOObjInfo(const MultiID&);
			SeisIOObjInfo(const SeisIOObjInfo&);
    SeisIOObjInfo&	operator =(const SeisIOObjInfo&);

    inline bool		isOK() const	{ return type_ != Bad; }
    inline bool		is2D() const	{ return type_ > VolPS; }
    inline bool		isPS() const	{ return type_ == VolPS
					      || type_ == LinePS; }

    enum Type		{ Bad, Vol, VolPS, Line, LinePS };
    Type		type() const		{ return type_; }
    const IOObj*	ioObj() const		{ return ioobj_; }

    struct SpaceInfo
    {
			SpaceInfo(int ns=-1,int ntr=-1,int bps=4);
	int		expectednrsamps;
	int		expectednrtrcs;
	int		maxbytespsamp;
    };

    int			expectedMBs(const SpaceInfo&) const;
    bool		getRanges(CubeSampling&) const;
    bool		getBPS(int&,int icomp) const;
    			//!< max bytes per sample, component -1 => add all
    void		getDefKeys(BufferStringSet&,bool add=false) const;
    			//!< For 3D: IOObj ID, for 2D: list of ID|attrnm

    // 2D only
    void		getLineNames( BufferStringSet& b, bool add=true,
	    				const BinIDValueSet* bvs=0 ) const
				{ getNms(b,add,false,bvs); }
    void		getAttribNames( BufferStringSet& b, bool add=true,
	    				const BinIDValueSet* bvs=0 ) const
				{ getNms(b,add,true,bvs); }
    void		getAttribNamesForLine( const char* nm,
	    				       BufferStringSet& b,
					       bool add=true ) const
				{ getNmsSubSel(nm,b,add,false); }
    void		getLineNamesWithAttrib( const char* nm,
	    				       BufferStringSet& b,
					       bool add=true ) const
				{ getNmsSubSel(nm,b,add,true); }

protected:

    Type		type_;
    IOObj*		ioobj_;

    void		setType();

    void		getNms(BufferStringSet&,bool,bool,
	    			const BinIDValueSet*) const;
    void		getNmsSubSel(const char*,BufferStringSet&,
	    				bool,bool) const;

};


#endif
