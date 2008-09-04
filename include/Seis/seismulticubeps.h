#ifndef seismulticubeps_h
#define seismulticubeps_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Aug 2008
 RCS:		$Id: seismulticubeps.h,v 1.4 2008-09-04 14:06:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsread.h"
#include "seispsioprov.h"
class SeisTrcReader;
class MultiID;


/*!\brief PS data store reader based on multiple 3D CBVS cubes */

class MultiCubeSeisPSReader : public SeisPS3DReader
{
public:

    			MultiCubeSeisPSReader(const char* fnm);
			// Check errMsg() to see failure
			~MultiCubeSeisPSReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    const PosInfo::CubeData& posData() const	{ return posdata_; }
    bool		getSampleNames(BufferStringSet&) const
			{ return false; }

    void		usePar(const IOPar&);

    void		addReader( SeisTrcReader* rdr, float offs )
			{ rdrs_ += rdr; offs_ += offs; }

    bool		getFrom(const char* fnm);
    bool		putTo(const char* fnm) const;

    static bool		writeData(const char* fnm,const ObjectSet<MultiID>&,
	    			  const TypeSet<float>&,BufferString& emsg);

protected:

    PosInfo::CubeData&		posdata_;
    ObjectSet<SeisTrcReader>	rdrs_;
    TypeSet<float>		offs_;
    mutable BufferString	errmsg_;

    void			getCubeData(const SeisTrcReader&,
	    				    PosInfo::CubeData&) const;

};


class MultiCubeSeisPS3DTranslator : public SeisPS3DTranslator
{			       isTranslator(MultiCube,SeisPS3D)
public:
    			mDefEmptyTranslatorConstructor(MultiCube,SeisPS3D)

    virtual bool	isReadDefault() const		{ return true; }
};


#endif
