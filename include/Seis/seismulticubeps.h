#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2008
________________________________________________________________________

-*/

#include "seispsread.h"
#include "seispsioprov.h"
#include "uistring.h"

namespace Seis { class Provider; }


/*!\brief PS data store reader based on multiple 3D CBVS cubes */

mExpClass(Seis) MultiCubeSeisPSReader : public SeisPS3DReader
{ mODTextTranslationClass(MultiCubeSeisPSReader);
public:

			MultiCubeSeisPSReader(const char* fnm);
			// Check errMsg() to see failure
			~MultiCubeSeisPSReader();

    SeisTrc*		getTrace(const TrcKey&,int) const;
    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const TrcKey&,SeisTrcBuf&) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    uiString		errMsg() const		{ return errmsg_; }

    const PosInfo::CubeData& posData() const	{ return posdata_; }
    bool		getSampleNames(BufferStringSet&) const
			{ return false; }

    void		usePar(const IOPar&);

    void		addProvider( Seis::Provider* prov, float offs )
			{ provs_ += prov; offs_ += offs; }

    bool		getFrom(const char* fnm);
    bool		putTo(const char* fnm) const;

    static bool		readData(const char* fnm,DBKeySet&,
				  TypeSet<float>&,TypeSet<int>&,
				  uiString& emsg);
    static bool		writeData(const char* fnm,const DBKeySet&,
				  const TypeSet<float>&,const TypeSet<int>&,
				  uiString& emsg);

protected:

    PosInfo::CubeData&		posdata_;
    ObjectSet<Seis::Provider>	provs_;
    TypeSet<float>		offs_;
    TypeSet<int>		comps_;
    mutable uiString		errmsg_;
};


mExpClass(Seis) MultiCubeSeisPS3DTranslator : public SeisPS3DTranslator
{			       isTranslator(MultiCube,SeisPS3D)
public:
			mDefEmptyTranslatorConstructor(MultiCube,SeisPS3D)

    virtual bool	isUserSelectable( bool fr ) const { return fr; }
    virtual const char*	defExtension() const		  { return "mcps"; }
};
