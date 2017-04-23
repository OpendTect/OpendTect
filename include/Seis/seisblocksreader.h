#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocks.h"
#include "ranges.h"
#include "threadlock.h"
#include "typeset.h"
#include "uistring.h"

class SeisTrc;
namespace PosInfo { class CubeData; class CubeDataPos; }


namespace Seis
{

class SelData;

namespace Blocks
{

class FileColumn;

/*!\brief Reads data from Blocks Storage.

  if ( state().isError() ) do not attempt anything.

  The get() and getNext() should be MT-safe, but reading is not parallel.

*/

mExpClass(Seis) Reader : public IOClass
{ mODTextTranslationClass(Seis::Blocks::Reader);
public:

    typedef PosInfo::CubeData	    CubeData;

			Reader(const char* fnm_or_dirnm);
			~Reader();

    const uiRetVal&	state() const		    { return state_; }

    const HGeom&	hGeom() const		    { return *hgeom_; }
    BufferString	surveyName() const	    { return survname_; }
    const CubeData&	positions() const	    { return cubedata_; }
    Interval<int>	inlRange() const	    { return inlrg_; }
    Interval<int>	crlRange() const	    { return crlrg_; }
    inline int		nrComponents() const
			{ return componentNames().size(); }
    BoolTypeSet		compSelected()		    { return compsel_; }

    void		setSelData(const SelData*);

    uiRetVal		get(const BinID&,SeisTrc&) const;
    uiRetVal		getNext(SeisTrc&) const;

protected:

    typedef PosInfo::CubeDataPos    CubeDataPos;

    HGeom*		hgeom_;
    SelData*		seldata_;
    BufferString	survname_;
    Interval<IdxType>	globinlidxrg_;
    Interval<IdxType>	globcrlidxrg_;
    CubeData&		cubedata_;
    CubeDataPos&	curcdpos_;
    Interval<int>	inlrg_;
    Interval<int>	crlrg_;
    int			maxnrfiles_;
    BoolTypeSet		compsel_;
    uiRetVal		state_;

    mutable ObjectSet<FileColumn> activitylist_;

    void		readMainFile();
    bool		getGeneralSectionData(const IOPar&);
    bool		reset(uiRetVal&) const;
    bool		isSelected(const CubeDataPos&) const;
    bool		advancePos(CubeDataPos&) const;
    void		doGet(SeisTrc&,uiRetVal&) const;
    FileColumn*		getColumn(const HGlobIdx&,uiRetVal&) const;
    void		readTrace(SeisTrc&,uiRetVal&) const;
    bool		activateColumn(FileColumn*,uiRetVal&) const;

    friend class	FileColumn;

};


} // namespace Blocks

} // namespace Seis
