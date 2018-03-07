#_______________________Pmake___________________________________________________
#
#	Makefile : 	DataBase access utilities
# 	July 1995	Bert
#_______________________________________________________________________________


set(OD_MODULE_DEPS General )
set(OD_MODULE_SOURCES 
	sqldatabase.cc sqlquery.cc
	mantisdatabase.cc mantistables.cc
	databaseobject.cc )

# set(USESQL 1)

if(USESQL)
    set(OD_USEQT Sql)
    add_definitions("-D__have_qsql__")
else()
    set(OD_USEQT Core)
endif()

OD_INIT_MODULE()

