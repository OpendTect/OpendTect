#ifndef viscoltabmod_h
#define viscoltabmod_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		June 2003
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "ranges.h"

class LinScaler;

namespace visBase
{

/*!\brief


*/

mClass(visBase) VisColTabMod : public DataObject
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

    void			setScale(const float*,int);
    const LinScaler&		getScale() const;

    int				usePar( const IOPar& );
    void			fillPar( IOPar&, TypeSet<int>& ) const;

protected:
    virtual			~VisColTabMod();

    LinScaler&			datascale;
    
    float			cliprate0;
    float			cliprate1;
    Interval<float>		range;
    bool			useclip;
    bool			reverse;

    static const char*		clipratestr();
    static const char*		rangestr();
    static const char*		reversestr();
    static const char*		useclipstr();
};

}; // Namespace


#endif

