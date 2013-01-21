#ifndef seismulticubeps_h
#define seismulticubeps_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Aug 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "seispsread.h"
#include "seispsioprov.h"
class SeisTrcReader;
class MultiID;


/*!\brief PS data store reader based on multiple 3D CBVS cubes */

mExpClass(Seis) MultiCubeSeisPSReader : public SeisPS3DReader
{
public:

    			MultiCubeSeisPSReader(const char* fnm);
			// Check errMsg() to see failure
			~MultiCubeSeisPSReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.str(); }

    const PosInfo::CubeData& posData() const	{ return posdata_; }
    bool		getSampleNames(BufferStringSet&) const
			{ return false; }

    void		usePar(const IOPar&);

    void		addReader( SeisTrcReader* rdr, float offs )
			{ rdrs_ += rdr; offs_ += offs; }

    bool		getFrom(const char* fnm);
    bool		putTo(const char* fnm) const;

    static bool		writeData(const char* fnm,const ObjectSet<MultiID>&,
	    			  const TypeSet<float>&,const TypeSet<int>&,
				  BufferString& emsg);

protected:

    PosInfo::CubeData&		posdata_;
    ObjectSet<SeisTrcReader>	rdrs_;
    TypeSet<float>		offs_;
    TypeSet<int>		comps_;
    mutable BufferString	errmsg_;

    void			getCubeData(const SeisTrcReader&,
	    				    PosInfo::CubeData&) const;

};


mExpClass(Seis) MultiCubeSeisPS3DTranslator : public SeisPS3DTranslator
{			       isTranslator(MultiCube,SeisPS3D)
public:
    			mDefEmptyTranslatorConstructor(MultiCube,SeisPS3D)

    virtual bool	isReadDefault() const	{ return true; }
    virtual const char*	defExtension() const    { return "mcps"; }
};


#endif

