#ifndef parametricsurfaceimpl_h
#define parametricsurfaceimpl_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: parametricsurfaceimpl.h,v 1.8 2006-04-26 21:09:27 cvskris Exp $
________________________________________________________________________

-*/

#define mInsertStart( idxvar, type, nrfunc ) \
int idxvar = type##Index(type); \
if ( idxvar<-1 || idxvar>nrfunc ) \
{ \
    errmsg() = "Cannot insert row or column that is not connected to existing rows."; \
    return false; \
} \
const bool addedinfront = idxvar==-1; \
if ( addedinfront ) \
{ \
    idxvar = 0; \
    origin_.type -= step_.type; \
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
    ?  new Array2DImpl<type>(curnrrows+1,curnrcols) : 0; \
for ( int idx=0; new##variable && idx<=curnrrows; idx++ ) \
{ \
    for ( int idy=0; idy<curnrcols; idy++ ) \
    { \
	if ( idx==rowidx ) \
	{ \
	    if ( idx==curnrrows || !idx ) \
		new##variable->set(idx,idy, udf ); \
	    else \
	    { \
		const Coord param( origin_.row+(rowidx-0.5)*step_.row, \
				   origin_.col+idy*step_.col ); \
		new##variable->set(idx,idy,interpolfunc); \
	    } \
	} \
	else \
	{ \
	    const int sourcerow = idx>rowidx ? idx-1 : idx; \
	    new##variable->set(idx,idy,variable->get(sourcerow,idy)); \
	} \
    } \
} \
mReplaceVariable( variable )




#define mCloneColVariable( type, variable, interpolfunc, udf ) \
    Array2D<type>* new##variable = variable \
        ?  new Array2DImpl<type>(curnrrows,curnrcols+1) : 0; \
for ( int idx=0; new##variable && idx<curnrrows; idx++ ) \
{ \
    for ( int idy=0; idy<=curnrcols; idy++ ) \
    { \
	if ( idy==colidx ) \
	{ \
	    if ( idy==curnrcols || !idy ) \
		new##variable->set(idx,idy, udf ); \
	    else \
	    { \
		const Coord param( origin_.row+idx*step_.row, \
				   origin_.col+(colidx-0.5)*step_.col ); \
		new##variable->set(idx,idy,interpolfunc); \
	    } \
	} \
	else \
	{ \
	    const int sourcecol = idy>colidx ? idy-1 : idy; \
	    new##variable->set(idx,idy,variable->get(idx,sourcecol)); \
	} \
    } \
} \
mReplaceVariable( variable )


#endif
