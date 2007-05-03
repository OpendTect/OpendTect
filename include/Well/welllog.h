#ifndef welllog_h
#define welllog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welllog.h,v 1.14 2007-05-03 11:26:38 cvsraman Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "welldahobj.h"
#include "ranges.h"
#include "color.h"

namespace Well
{

class Log : public DahObj
{
public:

			Log( const char* nm=0 )
			: DahObj(nm)
			, range_(mUdf(float),-mUdf(float))
			, displogrthm_(false)		{}
			Log( const Log& t )
			: DahObj("")			{ *this = t; }
    Log&		operator =(const Log&);

    float		value( int idx ) const		{ return val_[idx]; }

    float		getValue(float) const;
    void		addValue(float z,float val);
    			//!< addition must always ascend or descend
    void		ensureAscZ();
    			// Do this after adding values when Z may be reversed

    const Interval<float>& valueRange() const		{ return range_; }
    void		setSelValueRange(const Interval<float>&);
    const Interval<float>& selValueRange() const	{ return selrange_; }
    bool		dispLogarithmic() const		{ return displogrthm_; }
    void		setDispLogarithmic( bool yn )	{ displogrthm_ = yn; }

    const char*		unitMeasLabel() const		{ return unitmeaslbl_; }
    void		setUnitMeasLabel( const char* s ) { unitmeaslbl_ = s; }
    static const char*	sKeyUnitLbl;

protected:

    TypeSet<float>	val_;
    Interval<float>	range_;
    Interval<float>	selrange_;
    BufferString	unitmeaslbl_;
    bool		displogrthm_;

    void		removeAux( int idx )		{ val_.remove(idx); }
    void		eraseAux()			{ val_.erase(); }

};


class LogDisplayPars
{
public:
			LogDisplayPars( BufferString nm,
					const Interval<float>& rg, 
					bool logsc )
			    : lognm_(nm)
			    , logrange_(rg)
			    , logscale_(logsc)
			    , logcolor_(Color::White)	{}
			LogDisplayPars() {}
			~LogDisplayPars() {}

    void		setLogNm( const char* nm )	{ lognm_ = nm; }
    void		setRange( const Interval<float>& rg )
    				{ logrange_ = rg; }
    void		setLogScale( bool ls )	{ logscale_ = ls; }
    void		setColor( const Color& col )	
				{ logcolor_.setRgb( col.rgb() ); }
    BufferString	getLogNm() const	{ return lognm_; }
    Interval<float>	getRange() const	{ return logrange_; }
    bool		getLogScale() const	{ return logscale_; }
    Color		getColor() const	{ return logcolor_; }

protected:
    BufferString	lognm_;
    Interval<float>	logrange_;
    bool		logscale_;
    Color		logcolor_;
};


class LogDisplayParSet
{
public:
			LogDisplayParSet ()
			{
			    Interval<float> lrg( 0, 0 );
			    Interval<float> rrg( 0, 0 );
			    leftlogpar_ = new LogDisplayPars( "None", lrg,
							false );
			    rightlogpar_ = new LogDisplayPars( "None", rrg,
							false );  
			}
			~LogDisplayParSet()  
			{
			    delete leftlogpar_;
			    delete rightlogpar_;
			}

    LogDisplayPars*	getLeft() const { return leftlogpar_; }
    LogDisplayPars*	getRight() const { return rightlogpar_; }
    void		setLeft( LogDisplayPars* lp ) { leftlogpar_ = lp; }
    void		setRight( LogDisplayPars* rp ) { rightlogpar_ = rp; }

protected:
    LogDisplayPars*	leftlogpar_;
    LogDisplayPars*	rightlogpar_;
};

}; // namespace Well

#endif
