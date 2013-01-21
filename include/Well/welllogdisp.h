#ifndef welllogdisp_h
#define welllogdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		June 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "ranges.h"
#include "color.h"
#include "bufstring.h"

namespace Well
{

mExpClass(Well) LogDisplayPars
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


mExpClass(Well) LogDisplayParSet
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

