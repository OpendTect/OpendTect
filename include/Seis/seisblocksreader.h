#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocksaccess.h"
#include "ranges.h"
#include "threadlock.h"
#include "typeset.h"
#include "uistring.h"

class od_istream;
class SeisTrc;
class SeisTrcInfo;
class TraceData;
namespace PosInfo { class CubeData; class LineCollDataPos; }


namespace Seis
{

class SelData;

namespace Blocks
{

class FileColumn;
class HDF5Column;
class FileIDTable;
class HDF5ReadBackEnd;
class ReadBackEnd;
class StreamReadBackEnd;

/*!\brief Reads data from Blocks Storage.

  if ( state().isError() ) do not attempt anything.

  The get() and getNext() should be MT-safe, but reading is not parallel. For
  parallel reads, simply make multiple readers.

*/

mExpClass(Seis) Reader : public Access
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
    uiRetVal		getTrcData(TraceData&) const;
    uiRetVal		getNext(SeisTrc&) const;
    uiRetVal		get(const BinID&,SeisTrc&) const;

    void		close(); //!< early retire

protected:

    typedef PosInfo::LineCollDataPos    CubeDataPos;

    ReadBackEnd*	backend_;
    SelData*		seldata_;
    LinScaler*		scaler_;
    DataInterp*		interp_;
    FileIDTable&	fileidtbl_;
    CubeDataPos&	curcdpos_;
    BufferString	survname_;
    bool		depthinfeet_;

    BoolTypeSet		compsel_;
    uiRetVal		state_;
    const StepInterval<float> zrgintrace_;
    const int		nrcomponentsintrace_;
    mutable bool	lastopwasgetinfo_;

    bool		reset(uiRetVal&) const;
    bool		isSelected(const CubeDataPos&) const;
    bool		advancePos(CubeDataPos&) const;
    bool		doGoTo(const BinID&,uiRetVal&) const;
    void		doGet(SeisTrcInfo*,TraceData&,uiRetVal&) const;
    void		fillInfo(const BinID&,SeisTrcInfo&) const;
    Column*		getColumn(const HGlobIdx&,uiRetVal&) const;
    void		readTrace(SeisTrcInfo*,TraceData&,uiRetVal&) const;
    float		scaledVal(float) const;

    friend class	FileColumn;
    friend class	HDF5Column;
    friend class	StreamReadBackEnd;
    friend class	HDF5ReadBackEnd;

private:

    void		closeBackEnd();
    BufferString	findDataFileName(const char*) const;
    void		initFromFileName(const char*);
    void		readInfoFile(od_istream&);
    bool		getGeneralSectionData(const IOPar&);
    bool		getOffsetSectionData(const IOPar&);

};


mExpClass(Seis) ReadBackEnd
{
public:

			ReadBackEnd( Reader& rdr ) : rdr_(rdr)		{}
    virtual		~ReadBackEnd()					{}

    virtual void	reset(const char*,uiRetVal&)			= 0;
    virtual Column*	createColumn(const HGlobIdx&,uiRetVal&)		= 0;
    virtual void	fillTraceData(Column&,const BinID&,
				      TraceData&,uiRetVal&) const	= 0;
    virtual void	close()						= 0;

    Reader&		rdr_;

};



} // namespace Blocks

} // namespace Seis
