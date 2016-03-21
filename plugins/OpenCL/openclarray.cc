/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/

#include "openclmod.h"

/*

mExpClass(Basic) ArrayAccessBase
{
public:
    void 			touchMemArray() { memdirtycount_++; }
    enum Mode			{ ReadOnly, WriteOnly, ReadWrite };

    void			setMode(Mode mode)	{ mode_ = mode; }
    Mode			getMode() const		{ return mode_; }

protected:
    Threads::Atomic<int>	memdirtycount_;
    Mode			mode_;
};


mExpClass(Basic) Array1DAccess : public ArrayAccessBase
{
public:
    Array1D<float>*		getMemArray()		{ return array_; }
    const Array1D<float>*	getMemArray() const	{ return array_; }

protected:
    Array1D<float>*		array_;
};

namespace OpenCL
{
    mExpClass(OPenCL) Platform
    {
    public:
	static void				initClass();
	static const ObjectSet<Platform>&	getPlatforms();
    };
}
	
	
*/

