#ifndef prestackmutedef_h
#define prestackmutedef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmutedef.h,v 1.2 2007-07-06 16:44:27 cvsyuancheng Exp $
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
protected:

    ObjectSet<PointBasedMathFunction>	fns_;
    TypeSet<BinID>			pos_;
};


}; //namespace

#endif
