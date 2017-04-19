#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocks.h"
#include "uistring.h"

class SeisTrc;
namespace PosInfo { class CubeData; }


namespace Seis
{

class SelData;

namespace Blocks
{

/*!\brief Reads data from Blocks Storage.

  if ( state().isError() ) do not attempt anything.

*/

mExpClass(Seis) Reader : public IOClass
{ mODTextTranslationClass(Seis::Blocks::Reader);
public:

			Reader(const char* fnm_or_dirnm);
			~Reader();

    const uiRetVal&	state() const		    { return state_; }

    const SurvGeom&	survGeom() const	    { return *survgeom_; }
    const PosInfo::CubeData& positions() const	    { return cubedata_; }

    void		setSelData(const SelData*);

    uiRetVal		get(const BinID&,SeisTrc&) const;
    uiRetVal		getNext(SeisTrc&) const;

protected:

    SurvGeom*		survgeom_;
    SelData*		seldata_;
    uiRetVal		state_;
    PosInfo::CubeData&	cubedata_;

    void		readMainFile();
    bool		getGeneralSectionData(const IOPar&);

};


} // namespace Blocks

} // namespace Seis
