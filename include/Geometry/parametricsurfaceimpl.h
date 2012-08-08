#ifndef parametricsurfaceimpl_h
#define parametricsurfaceimpl_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: parametricsurfaceimpl.h,v 1.12 2012-08-08 05:04:30 cvssalil Exp $
________________________________________________________________________

-*/

#define mInsertStart( idxvar, type, nrfunc ) \
int idxvar = type##Index(type); \
if ( idxvar<-nrtoinsert || idxvar>nrfunc-1+nrtoinsert ) \
{ \
    errmsg() = "Cannot insert row or column that is not connected to existing rows."; \
    return false; \
} \
const bool addedinfront = idxvar<0; \
if ( addedinfront ) \
{ \
    idxvar = 0; \
    origin_.type -= step_.type*nrtoinsert; \
}\
\
const int curnrrows = nrRows(); \
const int curnrcols = nrCols()


#define mReplaceVariable( variable ) \
if ( new##variable ) \
{ \
    delete variable; \
    variable = new##variable; \
} 


#define mCloneRowVariable( type, variable, interpolfunc, udf ) \
Array2D<type>* new##variable = variable \
    ?  new Array2DImpl<type>(curnrrows+nrtoinsert,curnrcols) : 0; \
for ( int idx=0; new##variable && idx<curnrrows+nrtoinsert; idx++ ) \
{ \
    for ( int idy=0; idy<curnrcols; idy++ ) \
    { \
	if ( idx>=rowidx && idx<rowidx+nrtoinsert ) \
	{ \
	    if ( idx>=curnrrows || idx<nrtoinsert ) \
		new##variable->set(idx,idy, udf ); \
	    else \
	    { \
		const float relrow = origin_.row + \
		    ((float) idx-rowidx)/(nrtoinsert+1)*step_.row; \
		const Coord param( relrow, origin_.col+idy*step_.col ); \
		new##variable->set(idx,idy,(type) interpolfunc); \
	    } \
	} \
	else \
	{ \
	    const int sourcerow = idx>rowidx ? idx-nrtoinsert : idx; \
	    new##variable->set(idx,idy,variable->get(sourcerow,idy)); \
	} \
    } \
} \
mReplaceVariable( variable )




#define mCloneColVariable( type, variable, interpolfunc, udf ) \
    Array2D<type>* new##variable = variable \
        ?  new Array2DImpl<type>(curnrrows,curnrcols+nrtoinsert) : 0; \
for ( int idx=0; new##variable && idx<curnrrows; idx++ ) \
{ \
    for ( int idy=0; idy<curnrcols+nrtoinsert; idy++ ) \
    { \
	if ( idy>=colidx && idy<colidx+nrtoinsert ) \
	{ \
	    if ( idy>=curnrcols || idy<nrtoinsert ) \
		new##variable->set(idx,idy, udf ); \
	    else \
	    { \
		const float relcol = origin_.col + \
		    ((float) idy-colidx)/(nrtoinsert+1)*step_.col; \
		const Coord param( origin_.row+idx*step_.row, relcol ); \
		new##variable->set(idx,idy,(type) interpolfunc); \
	    } \
	} \
	else \
	{ \
	    const int sourcecol = idy>colidx ? idy-nrtoinsert : idy; \
	    new##variable->set(idx,idy,variable->get(idx,sourcecol)); \
	} \
    } \
} \
mReplaceVariable( variable )


#endif
