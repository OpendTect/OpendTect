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

/*!\brief Parameters for the colortable Mapper.  */

mExpClass(General) MapperSetup : public SharedObject
{
public:

    typedef ColTab::SeqUseMode	SeqUseMode;

				MapperSetup();
				MapperSetup(Interval<float> fixed_range);
				mDeclMonitorableAssignment(MapperSetup);

    // How do we determine the mapping value -> position in Sequence?
    mImplSimpleMonitoredGetSet(inline,isFixed,setIsFixed,
				bool,isfixed_,cIsFixedChange());
    mImplSimpleMonitoredGetSet(inline,doHistEq,setDoHistEq,
				bool,dohisteq_,cDoHistEqChange());

    // Do we, and how do we segment?
    mImplSimpleMonitoredGetSet(inline,nrSegs,setNrSegs,
				int,nrsegs_,cSegChange());
    // How do we use the Sequence?
    mImplSimpleMonitoredGetSet(inline,seqUseMode,setSeqUseMode,
				SeqUseMode,sequsemode_,cUseModeChange());

    // The scaling when fixed
    mImplSimpleMonitoredGetSet(inline,range,setRange,
				Interval<float>,range_,cRangeChange());

    // The parameters for auto-scaling
    mImplSimpleMonitoredGetSet(inline,clipRate,setClipRate,
				Interval<float>,cliprate_,cAutoScaleChange());
    mImplSimpleMonitoredGetSet(inline,guessSymmetry,setGuessSymmetry,
				bool,guesssymmetry_,cAutoScaleChange());
    mImplSimpleMonitoredGetSet(inline,symMidVal,setSymMidVal,
				float,symmidval_,cAutoScaleChange());

    bool			hasSegmentation() const
				{ return nrSegs() != 0; }
    bool			needsReClip(const MapperSetup&) const;

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    static const char*		sKeyRange()	{ return "Range"; }
    static const char*		sKeyClipRate()	{ return "Clip Rate"; }
    static const char*		sKeyAutoSym()	{ return "Auto Sym"; }
    static const char*		sKeySymMidVal()	{ return "Sym Mid Value"; }
    static const char*		sKeyFlipSeq()	{ return "Flip seq"; }
    static const char*		sKeyCycleSeq()	{ return "Cycle seq"; }

    static ChangeType		cIsFixedChange()	{ return 2; }
    static ChangeType		cDoHistEqChange()	{ return 3; }
    static ChangeType		cRangeChange()		{ return 4; }
    static ChangeType		cAutoScaleChange()	{ return 5; }
    static ChangeType		cSegChange()		{ return 6; }
    static ChangeType		cUseModeChange()	{ return 7; }

protected:

    virtual			~MapperSetup();

    bool			isfixed_;
    bool			dohisteq_;
    Interval<float>		range_;
    Interval<float>		cliprate_;
    bool			guesssymmetry_;
    float			symmidval_;
    int				nrsegs_;
    SeqUseMode			sequsemode_;

};

} // namespace ColTab
