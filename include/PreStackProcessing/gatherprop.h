#ifndef gatherprop_h
#define gatherprop_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jan 2008
 RCS:		$Id: gatherprop.h,v 1.6 2010/08/04 14:49:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "stattype.h"
#include "enums.h"
#include "ranges.h"
class BinID;
class SeisTrcBuf;
class SeisPSReader;

/*!\brief calculates 'post-stack' properties of a Pre-Stack data store */

mClass SeisPSPropCalc
{
public:

    enum CalcType	{ Stats, LLSQ };
    			DeclareEnumUtils(CalcType)
    enum AxisType	{ Norm, Log, Exp, Sqr, Sqrt, Abs };
    			DeclareEnumUtils(AxisType)
    enum LSQType	{ A0, Coeff, StdDevA0, StdDevCoeff, CorrCoeff };
    			DeclareEnumUtils(LSQType)

    mClass Setup
    {
    public:
			Setup()
			    : calctype_(Stats)
			    , stattype_(Stats::Average)
			    , lsqtype_(A0)
			    , offsaxis_(Norm)
			    , valaxis_(Norm)
			    , offsrg_(0,mUdf(float))
			    , useazim_(false)
			    , component_(0)
			    , aperture_(0)	{}

	mDefSetupMemb(CalcType,calctype)
	mDefSetupMemb(Stats::Type,stattype)
	mDefSetupMemb(LSQType,lsqtype)
	mDefSetupMemb(AxisType,offsaxis)
	mDefSetupMemb(AxisType,valaxis)
	mDefSetupMemb(Interval<float>,offsrg)
	mDefSetupMemb(bool,useazim)
	mDefSetupMemb(int,component)
	mDefSetupMemb(int,aperture)
    };
			SeisPSPropCalc(const SeisPSReader&,const Setup&);
    virtual		~SeisPSPropCalc();

    Setup&		setup()				{ return setup_; }
    const Setup&	setup() const			{ return setup_; }

    bool		goTo(const BinID&);
    float		getVal(int sampnr) const;
    float		getVal(float z) const;

    const SeisPSReader&	reader() const			{ return rdr_; }

    static float	getVal(const Setup&,TypeSet<float>& vals,
	    			TypeSet<float>& offs);

protected:

    const SeisPSReader&	rdr_;
    SeisTrcBuf&		tbuf_;
    Setup		setup_;


};


#endif
