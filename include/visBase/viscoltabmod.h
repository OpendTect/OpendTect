#ifndef viscoltabmod_h
#define viscoltabmod_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Nanne Hemstra
 Date:		June 2003
 RCS:		$Id: viscoltabmod.h,v 1.1 2003-06-06 14:03:50 nanne Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "ranges.h"

class LinScaler;

namespace visBase
{

/*!\brief


*/

class VisColTabMod : public DataObject
{
public:
    static VisColTabMod*	create()
				mCreateDataObj(VisColTabMod);

    float			clipRate(bool) const;
    void			setClipRate(float,float);
    void			setRange(const Interval<float>&);
    const Interval<float>&	getRange() const	{ return range; }

    void			useClipping(bool yn)	{ useclip = yn; }
    bool			clippingUsed() const	{ return useclip; }

    void			doReverse(bool yn)	{ reverse = yn; }
    bool			isReverse() const	{ return reverse; }

    void			setScale(const float*,int,int);
    const LinScaler&		getScale(int) const;

    int				usePar( const IOPar& );
    void			fillPar( IOPar&, TypeSet<int>& ) const;

protected:
    virtual			~VisColTabMod();

    TypeSet<LinScaler>		datascales;
    				/*!<\note The first entry is not used since
				          the color range is in the coltab
				*/
    
    float			cliprate0;
    float			cliprate1;
    Interval<float>		range;
    bool			useclip;
    bool			reverse;

    static const char*		cliprate0str;
    static const char*		cliprate1str;
    static const char*		rangestr;
};

}; // Namespace


#endif
