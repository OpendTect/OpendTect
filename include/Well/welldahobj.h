#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "namedobj.h"
#include "ranges.h"

namespace Well
{

/*!
\brief Depth/Distance along hole object.
*/

mExpClass(Well) DahObj : public ::NamedCallBacker
{
public:

    virtual		~DahObj();

    inline int		size() const			{ return dah_.size(); }
    inline float	dah(int idx) const		{ return dah_[idx]; }
    virtual float	value(int idx) const		= 0;
    virtual bool	insertAtDah(float dah, float val) = 0;
    int			indexOf(float dah) const;
    virtual void	remove( int idx )
			{ dah_.removeSingle(idx); removeAux(idx); }
    virtual void	setEmpty()
			{ dah_.erase(); eraseAux(); }
    inline bool		isEmpty() const			{ return size() == 0; }
    Interval<float>&	dahRange();
    Interval<float>	dahRange() const;
			//!< returns Udf for empty dah_
    void		updateDahRange();

    float		dahStep(bool min_else_average) const;

    void		addToDahFrom(int fromidx,float extradah);
    void		removeFromDahFrom(int fromidx,float extradah);

    void		deInterpolate();
			//!< Remove unnecessary points
    float*		dahArr()			{ return dah_.arr(); }
    const float*	dahArr() const			{ return dah_.arr(); }

    static Notifier<DahObj>&	instanceCreated();

protected:

			DahObj(const char* nm=nullptr);
			DahObj(const DahObj&);

    DahObj&		operator =(const DahObj&);

    TypeSet<float>	dah_;
    Interval<float>	dahrange_;

    virtual void	removeAux(int)			= 0;
    virtual void	eraseAux()			= 0;
};


#define mWellDahObjInsertAtDah(dh,v,vals,ascvalonly)\
{\
    if ( mIsUdf(v) ) return false;\
    const bool ascendingvalonly = (ascvalonly); \
    if ( dah_.isEmpty() || dh >= dah_[dah_.size()-1] )\
    {\
	if ( !dah_.isEmpty() && ascendingvalonly && v <= vals[dah_.size()-1] )\
	    return false;\
	dah_ += dh; vals += val;\
	return true;\
    }\
    if ( dh < dah_[0] )\
    {\
	if ( ascendingvalonly && v >= vals[0] )\
	    return false;\
	dah_.insert( 0, dh ); vals.insert( 0, v );\
	return true; \
    }\
    const int insertidx = indexOf( dh );\
    if ( insertidx<0 ) return false;\
    if ( ascendingvalonly && (v <= vals[insertidx] || v >= vals[insertidx+1]) )\
	return false;\
    dah_.insert( insertidx+1, dh ); vals.insert( insertidx+1, v );\
}



mExpClass(Well) DahObjIter
{
public:
    virtual		~DahObjIter();
			mOD_DisableCopy(DahObjIter)

    enum Direction	{ Forward, Backward };
    bool		next();
    int			size() const;
    inline bool		isEmpty() const		{ return size() < 1; }

    inline bool		isForward() const	{ return dir_ == Forward; }
    inline bool		isValid() const		{ return isPresent(curidx_); }
    inline bool		atFirst() const		{ return curidx_ == startidx_; }
    inline bool		atLast() const		{ return curidx_ == stopidx_; }
    inline int		curIdx() const		{ return curidx_; }
    bool		isPresent(int) const;
    virtual void	reInit();
    void		retire();

    const DahObj&	dahObj() const		{ return obj_; }
    float		dah() const;
    float		value() const;

protected:
			DahObjIter(const DahObj&,bool start_at_end=false);

    const DahObj&	obj_;
    const Direction	dir_;
    const int		startidx_;
    const int		stopidx_;

    int			curidx_;
};

} // namespace Well
