#ifndef prestackmutedef_h
#define prestackmutedef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmutedef.h,v 1.3 2007-07-11 21:06:34 cvsyuancheng Exp $
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
    bool				isChanged() const { return ischanged_; }
    void				setChanged(bool yn) { ischanged_=yn; }

protected:

    ObjectSet<PointBasedMathFunction>	fns_;
    TypeSet<BinID>			pos_;
    bool				ischanged_;
};


}; //namespace

#endif
