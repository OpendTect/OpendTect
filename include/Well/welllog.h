#ifndef welllog_h
#define welllog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welllog.h,v 1.21 2009-06-17 14:03:05 cvsbert Exp $
________________________________________________________________________


-*/

#include "welldahobj.h"
#include "ranges.h"
#include "color.h"
#include "iopar.h"

namespace Well
{

mClass Log : public DahObj
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
    void		addValue(float dh,float val);
    			//!< addition must always ascend or descend
    void		ensureAscZ();
    			// Do this after adding values when Z may be reversed

    Interval<float>& valueRange() 			{ return range_; }
    void		setSelValueRange(const Interval<float>&);
    Interval<float>& selValueRange() 			{ return selrange_; }
    bool		dispLogarithmic() const		{ return displogrthm_; }
    void		setDispLogarithmic( bool yn )	{ displogrthm_ = yn; }

    const char*		unitMeasLabel() const		{ return unitmeaslbl_; }
    void		setUnitMeasLabel( const char* s ) { unitmeaslbl_ = s; }
    static const char*	sKeyUnitLbl();
    static const char*	sKeyHdrInfo();
    static const char*	sKeyStorage();

    float*		valArr()			{ return val_.arr(); }
    const float*	valArr() const			{ return val_.arr(); }

    IOPar&		pars()				{ return pars_; }
    const IOPar&	pars() const			{ return pars_; }

protected:

    TypeSet<float>	val_;
    Interval<float>	range_;
    Interval<float>	selrange_;
    BufferString	unitmeaslbl_;
    bool		displogrthm_;
    IOPar		pars_;

    void		removeAux( int idx )		{ val_.remove(idx); }
    void		eraseAux()			{ val_.erase(); }

};


mClass LogDisplayPars
{
public:
			LogDisplayPars( const char* nm=0 )
			    : name_(nm)
 			    , cliprate_(mUdf(float))
			    , range_(mUdf(float),mUdf(float))
			    , nocliprate_(false)	
			    , logarithmic_(false)
			    , repeat_(1)	
			    , repeatovlap_(mUdf(float))
			    , seisstyle_(false)	
			    , linecolor_(Color::White())	
			    , logfill_(false)
	    		    , logfillcolor_(Color::White())
			    , seqname_("")
       			    , singlfillcol_(false)				
						        {}
			~LogDisplayPars()		{}

    BufferString	name_;
    float		cliprate_;	//!< If undef, use range_
    Interval<float>	range_;		//!< If cliprate_ set, filled using it
    bool		logarithmic_;
    bool		seisstyle_;
    bool		nocliprate_;
    bool		logfill_;
    int 		repeat_;
    float		repeatovlap_;
    Color		linecolor_;
    Color		logfillcolor_;
    const char*		seqname_;
    bool 		singlfillcol_;
};


mClass LogDisplayParSet
{
public:
			LogDisplayParSet ()
			{
			    Interval<float> lrg( 0, 0 );
			    Interval<float> rrg( 0, 0 );
			    leftlogpar_ = new LogDisplayPars( "None" );
			    rightlogpar_ = new LogDisplayPars( "None" );
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
