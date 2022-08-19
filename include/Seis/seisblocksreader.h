#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisblocks.h"
#include "ranges.h"
#include "threadlock.h"
#include "typeset.h"
#include "uistring.h"

class od_istream;
class SeisTrc;
class SeisTrcInfo;
namespace PosInfo { class CubeData; class CubeDataPos; }


namespace Seis
{

class SelData;

namespace Blocks
{

class FileColumn;
class OffsetTable;

/*!\brief Reads data from Blocks Storage. For comments see master branch. */

mExpClass(Seis) Reader : public IOClass
{ mODTextTranslationClass(Seis::Blocks::Reader);
public:

    typedef PosInfo::CubeData	    CubeData;

			Reader(const char* fnm);    //!< data or info
			Reader(od_istream&);	    //!< data or info
			~Reader();

    const uiRetVal&	state() const		    { return state_; }

    BufferString	surveyName() const	    { return survname_; }
    const CubeData&	positions() const	    { return cubedata_; }
    inline int		nrComponents() const
			{ return componentNames().size(); }
    BoolTypeSet&	compSelected()		    { return compsel_; }
    bool		depthInFeet() const	    { return depthinfeet_; }

    void		setSelData(const SelData*);

    bool		goTo(const BinID&) const;
    uiRetVal		skip(int) const;
    uiRetVal		getTrcInfo(SeisTrcInfo&) const;
    uiRetVal		getNext(SeisTrc&) const;
    uiRetVal		get(const BinID&,SeisTrc&) const;

    void		close(); //!< early retire

protected:

    typedef PosInfo::CubeDataPos    CubeDataPos;

    mutable od_istream*	strm_;
    bool		strmmine_;
    SelData*		seldata_;
    LinScaler*		scaler_;
    DataInterp*		interp_;
    OffsetTable&	offstbl_;
    CubeData&		cubedata_;
    CubeDataPos&	curcdpos_;
    BufferString	survname_;
    bool		depthinfeet_;

    BoolTypeSet		compsel_;
    uiRetVal		state_;
    const Interval<float> zrgintrace_;
    const int		nrcomponentsintrace_;
    mutable bool	lastopwasgetinfo_;

    void		closeStream() const;
    bool		reset(uiRetVal&) const;
    bool		isSelected(const CubeDataPos&) const;
    bool		advancePos(CubeDataPos&) const;
    bool		doGoTo(const BinID&,uiRetVal&) const;
    void		doGet(SeisTrc&,uiRetVal&) const;
    void		fillInfo(const BinID&,SeisTrcInfo&) const;
    FileColumn*		getColumn(const HGlobIdx&,uiRetVal&) const;
    void		readTrace(SeisTrc&,uiRetVal&) const;

    friend class	FileColumn;

private:

    void		initFromFileName(const char*);
    void		readInfoFile(od_istream&);
    bool		getGeneralSectionData(const IOPar&);
    bool		getOffsetSectionData(const IOPar&);

};


} // namespace Blocks

} // namespace Seis
