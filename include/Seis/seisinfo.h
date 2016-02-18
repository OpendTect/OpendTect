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
class SeisTrc;
class PosAuxInfo;


/*!\brief Information for a seismic trace, AKA trace header info */

mExpClass(Seis) SeisTrcInfo
{
public:
			SeisTrcInfo();
			SeisTrcInfo(const SeisTrcInfo&);
    SeisTrcInfo&	operator=(const SeisTrcInfo&);

    SamplingData<float>	sampling_;
    int			nr_;
    TrcKey		trckey_;
    Coord		coord_;
    float		offset_;
    float		azimuth_;
    float		refnr_;
    float		pick_;

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
			{ return sampling_.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<float>&) const;
    bool		dataPresent(float pos,int trcsize) const;

    enum Fld		{ TrcNr=0, Pick, RefNr,
			  CoordX, CoordY, BinIDInl, BinIDCrl,
			  Offset, Azimuth };
    			mDeclareEnumUtils(Fld)
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

    float		zref_;		// not stored
    bool		new_packet_;	// not stored


    float&			pick;
                                /*!<For backward compatibility. Will be
                                    removed in future releases. */

    SamplingData<float>&	sampling;
                                        /*!<For backward compatibility. Will be
                                            removed in future releases. */
    int&			nr;
                                        /*!<For backward compatibility. Will be
                                            removed in future releases. */
    BinID&                              binid;
                                        /*!<Reference into the trckey.
                                            For backward compatibility. Will be
                                            deprecated in future releases. */
    Coord&                  coord;
                                        /*!<For backward compatibility. Will be
                                            removed in future releases. */

    float&                  offset;
                                        /*!<Reference into the trckey.
                                         For backward compatibility. Will be
                                         deprecated in future releases. */
    float&                  azimuth;
                                    /*!<Reference into the trckey.
                                     For backward compatibility. Will be
                                     deprecated in future releases. */
    float&                  refnr;
                                    /*!<Reference into the trckey.
                                     For backward compatibility. Will be
                                     deprecated in future releases. */
    float&                  zref;
                                    /*!<Reference into the trckey.
                                     For backward compatibility. Will be
                                     deprecated in future releases. */
    bool&                   new_packet;
                                    /*!<Reference into the trckey.
                                     For backward compatibility. Will be
                                     deprecated in future releases. */
};


#endif
