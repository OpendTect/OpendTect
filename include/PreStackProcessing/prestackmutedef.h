#ifndef prestackmutedef_h
#define muting_hprestackmutedef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmutedef.h,v 1.1 2007-03-15 17:28:52 cvskris Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "position.h"
#include "samplingdata.h"

class PointBasedMathFunction;


namespace PreStack
{

class MuteDef : public NamedObject
{
public:

					MuteDef( const char* nm=0 );
					~MuteDef();

    int					size() const;
    PointBasedMathFunction&		getFn(int idx);
    BinID&				getPos(int idx);
    const PointBasedMathFunction&	getFn(int idx) const;
    const BinID&			getPos(int idx) const;

    void				add(PointBasedMathFunction*,
	    				    const BinID& pos );

    float				value(float offs,const BinID&) const;
					//!< Interpolates between defined
					//!< positions
protected:

    ObjectSet<PointBasedMathFunction>	fns_;
    TypeSet<BinID>			pos_;
};


}; //namespace

#endif
