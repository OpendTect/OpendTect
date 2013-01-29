#ifndef prestackmutedef_h
#define prestackmutedef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "namedobj.h"
#include "position.h"
#include "samplingdata.h"
#include "multiid.h"

class PointBasedMathFunction;

namespace PreStack
{

/*!
\brief NamedObject for definition of a mute function.
*/

mExpClass(PreStackProcessing) MuteDef : public NamedObject
{
public:
					MuteDef(const char* nm=0);
					MuteDef(const MuteDef&);
					~MuteDef();
    MuteDef&				operator=(const MuteDef&);

    int					size() const;
    int					indexOf(const BinID&) const;
    PointBasedMathFunction&		getFn(int idx);
    BinID&				getPos(int idx);
    const PointBasedMathFunction&	getFn(int idx) const;
    const BinID&			getPos(int idx) const;

    void				add(PointBasedMathFunction*,
	    				    const BinID& pos );
    					//!<Function becomes mine.
    void				remove(int idx);
    float				value(float offs,const BinID&) const;
					//!< Interpolates between defined
					//!< positions
    void				computeIntervals(float offs,
					    const BinID&,
					    TypeSet<Interval<float> >&) const;
					/*!<Interpolates between 
					  defined positions. */

    bool				isChanged() const { return ischanged_; }
    void				setChanged(bool yn) { ischanged_=yn; }


    void				setReferenceHorizon(const MultiID&);
    const MultiID&			getReferenceHorizon() const;

protected:

    ObjectSet<PointBasedMathFunction>	fns_;
    TypeSet<BinID>			pos_;
    MultiID				refhor_;

    bool				ischanged_;

    void				getAllZVals(TypeSet<float>&) const;
};


}; //namespace

#endif

