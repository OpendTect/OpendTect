#ifndef uistratdispdata_h
#define uistratdispdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdispdata.h,v 1.6 2010-07-14 10:05:13 cvsbruno Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "callback.h"
#include "color.h"
#include "ranges.h"
#include "bufstring.h"


namespace Strat{ class UnitRef; class NodeUnitRef; class RefTree; }
class uiStratRefTree;
class uiStratMgr;
class uiListViewItem;

mClass AnnotData
{
public:
			AnnotData() {};
			~AnnotData() { eraseData(); };

    mStruct Marker
    {
			Marker( const char* nm, float pos )
			    : zpos_(pos)
			    , name_(nm)
			    , colidx_(0)       
			    , isdotted_(false)	
			    {}

	float 		zpos_;
	Color 		col_;
	BufferString	name_;
	int		id_;
	Color 		nmcol_;
	int 		colidx_;	
	bool		isdotted_;
    };

    mStruct Unit : public Marker
    {
			Unit(const char* nm,float zpostop,float zposbot)
			    : Marker(nm,zpostop)
			    , zposbot_(zposbot)
			    {}

	float 		zposbot_;
	BufferStringSet annots_;
    };
    
    mStruct Column 
    {
			Column( const char* nm )
			    : name_(nm)
			    , iseditable_(true)  
			    {}

	BufferString	name_;
	bool		iseditable_;
	ObjectSet<Marker> markers_;  
	ObjectSet<Unit>	units_; 
    };

    void		eraseData() 
			{ 
			    for ( int idx=0; idx<columns_.size(); idx++ )
			    {
				deepErase( columns_[idx]->markers_ );
				deepErase( columns_[idx]->units_ );
				deepErase( columns_ );
			    }
			}
    const Column*	getCol( int idx ) const { return columns_[idx]; }
    Column*		getCol( int idx ) 	{ return columns_[idx]; }
    int			nrCols() const 		{ return columns_.size(); }
    void		addCol( Column* col) 	{ columns_ += col; }
    
protected:

    ObjectSet<Column> 	columns_;  
};



/*!\brief used to gather all units and tied levels from a tree for display*/

mClass uiStratAnnotGather : public CallBacker
{
public:
	                        uiStratAnnotGather(AnnotData&,
						    const uiStratMgr&);
	                        ~uiStratAnnotGather(){};

    Notifier<uiStratAnnotGather> newtreeRead;

protected:

    const uiStratMgr&	uistratmgr_;
    AnnotData& 		data_;

    void		addUnits(const Strat::NodeUnitRef&,int);
    void		addUnit(const Strat::UnitRef&,int);
    void 		readFromTree();				

    void		triggerDataChange(CallBacker*);
};


/*!brief used to write directly in the listView of the uiStratRefTree as if we were handling the uiTree directly*/

mClass uiStratTreeWriter : public CallBacker
{
public:
	                        uiStratTreeWriter(uiStratRefTree&);
	                        ~uiStratTreeWriter(){};

    uiListViewItem*		getItemFromTree(const char*);
    void			handleUnitMenu(const char*);
    void			handleLvlMenu(int);
    void 			fillUndef(CallBacker*);

protected:

    uiStratRefTree&     uitree_;
};

#endif
