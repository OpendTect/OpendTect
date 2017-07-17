#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007 / Feb 2017
________________________________________________________________________

-*/

#include "coltab.h"
#include "sharedobject.h"


namespace ColTab
{

/*!\brief Parameters for the colortable Mapper.

  Note that the range can be set at the end of the auto-scaling. Then, no
  objectChanged is emitted but you can use the rangeCalculated Notifier.

*/

mExpClass(General) MapperSetup : public SharedObject
{
public:

    typedef Interval<ValueType>	RangeType;

				MapperSetup();
				MapperSetup(const RangeType&);
				mDeclMonitorableAssignment(MapperSetup);

    // Use a fixed range, or obtain range by some form of clipping?
    mImplSimpleMonitoredGet(	isFixed,bool,isfixed_);
    mImplSimpleMonitoredGet(	range,RangeType,range_);
    void			setNotFixed();
    void			setFixedRange(RangeType);
    // When auto-scaling, need a clip rate
    mImplSimpleMonitoredGetSet(inline,clipRate,setClipRate,
				ClipRatePair,cliprate_,cAutoScaleChange());
    void			setNoClipping()
				{ setClipRate( ClipRatePair(0.f,0.f) ); }

    // Use Histogram equalisation?
    mImplSimpleMonitoredGetSet(inline,doHistEq,setDoHistEq,
				bool,dohisteq_,cDoHistEqChange());

    // Do we, and how do we segment?
    mImplSimpleMonitoredGetSet(inline,nrSegs,setNrSegs,
				int,nrsegs_,cSegChange());

    // How do we use the Sequence?
    mImplSimpleMonitoredGetSet(inline,seqUseMode,setSeqUseMode,
				SeqUseMode,sequsemode_,cUseModeChange());

    bool			hasSegmentation() const
				{ return nrSegs() != 0; }

    bool			isMappingChange(ChangeType) const;

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    static const char*		sKeyRange()	{ return "Range"; }
    static const char*		sKeyNrSegs()	{ return "Nr Segments"; }
    static const char*		sKeyClipRate()	{ return "Clip Rate"; }
    static const char*		sKeyFlipSeq()	{ return "Flip seq"; }
    static const char*		sKeyCycleSeq()	{ return "Cycle seq"; }

    static ChangeType		cIsFixedChange()	{ return 2; }
    static ChangeType		cRangeChange()		{ return 3; }
    static ChangeType		cDoHistEqChange()	{ return 4; }
    static ChangeType		cAutoScaleChange()	{ return 5; }
    static ChangeType		cSegChange()		{ return 6; }
    static ChangeType		cUseModeChange()	{ return 7; }

    Notifier<MapperSetup>	rangeCalculated;

protected:

    virtual			~MapperSetup();

    bool			isfixed_;
    bool			dohisteq_;
    RangeType			range_;
    ClipRatePair		cliprate_;
    int				nrsegs_;
    SeqUseMode			sequsemode_;

    friend class		Mapper;
    void			setCalculatedRange(const RangeType&) const;

};

} // namespace ColTab
