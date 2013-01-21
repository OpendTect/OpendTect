#ifndef seisinfo_h
#define seisinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		25-10-1996
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "seisposkey.h"
#include "samplingdata.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"
class IOPar;
class SeisTrc;
class PosAuxInfo;
template <class T> class TypeSet;


/*!\brief Information for a seismic trace, AKA trace header info */

mExpClass(Seis) SeisTrcInfo
{
public:
			SeisTrcInfo()
			: sampling(0,defaultSampleInterval()), nr(0)
			, refnr(mUdf(float)), pick(mUdf(float))
			, offset(0), azimuth(0), zref(0), new_packet(false)
			{}

    SamplingData<float>	sampling;
    int			nr;
    BinID		binid;
    Coord		coord;
    float		offset;
    float		azimuth;
    float		refnr;
    float		pick;

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
			{ return sampling.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<float>&) const;
    bool		dataPresent(float pos,int trcsize) const;

    enum Fld		{ TrcNr=0, Pick, RefNr,
			  CoordX, CoordY, BinIDInl, BinIDCrl,
			  Offset, Azimuth };
    			DeclareEnumUtils(Fld)
    double		getValue(Fld) const;
    static void		getAxisCandidates(Seis::GeomType,TypeSet<Fld>&);
    int			getDefaultAxisFld(Seis::GeomType,
					  const SeisTrcInfo* next) const;
    void		getInterestingFlds(Seis::GeomType,IOPar&) const;
    void		setPSFlds(const Coord& rcvpos,const Coord& srcpos,
	    			  bool setpos=false);

    static const char*	sSamplingInfo;
    static const char*	sNrSamples;
    static float	defaultSampleInterval(bool forcetime=false);

    Seis::PosKey	posKey(Seis::GeomType) const;
    void		setPosKey(const Seis::PosKey&);
    void		putTo(PosAuxInfo&) const;
    void		getFrom(const PosAuxInfo&);
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    float		zref;		// not stored
    bool		new_packet;	// not stored

    void		handlePossibleFeetConversion(bool conv_back=false,
						 bool other_domain=false);
			/*!< Use Seis::InternalUnitsEnsurer class (see below).
			     Needed when seismic data meets the 'real world'.
			     Note that this function most often does nothing.
			     The other_domain is for time traces in depth
			     surveys and vv. */

};


namespace Seis
{

/* Ensures depth seismics are in meters (in this scope).

   Depth seismic data is the *only* exception to the rule that everything
   inside OpendTect is always in SI units (internal units).
   To make the connection with other objects, you need to make sure seismic
   data is compatible. This object makes this easy. Usage is almost certainly
   via one of the macros, probably mSeisTrcEnsureInternalUnits. Usage like:

	    void doSomething( const SeisTrc& trc )
	    {
		mSeisTrcEnsureInternalUnits( trc );
		// use trace without fear for feet here
		// after function, trc won't have changed
	    }

   */

mExpClass(Seis) InternalUnitsEnsurer
{
public:
    			InternalUnitsEnsurer( SeisTrcInfo& ti,
					      bool othdom=false )
			    : ti_(ti), othdom_(othdom)	{ convTo(); }
    			InternalUnitsEnsurer( const SeisTrcInfo& ti,
					      bool othdom=false )
			    : ti_(const_cast<SeisTrcInfo&>(ti))
			    , othdom_(othdom)		{ convTo(); }
			~InternalUnitsEnsurer()		{ convBack(); }


    SeisTrcInfo&	ti_;
    bool		othdom_;

    inline void		convTo()
    			{ ti_.handlePossibleFeetConversion(false,othdom_); }
    inline void		convBack()
    			{ ti_.handlePossibleFeetConversion(true,othdom_); }
};


//! ensures depth seismics are in meters (in this scope).
#define mSeisTrcEnsureInternalUnits(trc) \
    Seis::InternalUnitsEnsurer trc##_intunits_ensurer( trc.info() )
//! ensures depth seismics are in meters (in this scope).
#define mSeisTrcInfoEnsureInternalUnits(inf) \
    Seis::InternalUnitsEnsurer inf##_intunits_ensurer( inf )

}


#endif

