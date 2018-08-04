#

testrun: test_run331.c
	gcc -o testrun  test_run331.c
ifxad:	ifxad_v103.c
	gcc -o ifxad ifxad_v103.c

ifxdb:	func4003.c ssct4201.c dbkern4101.c
	gcc -s -o ifxdb ssct4201.c dbkern4101.c func4003.c -lpthread

ifxcreate: ifxdb_create.c
	gcc -o ifxdb_create  ifxdb_create.c

ifxutil: ifxdb_util.c
	gcc -o ifxdb_util  ifxdb_util.c
ifxs: ifxshell_v103.c
	gcc -o ifxs ifxshell_v103.c

all:
	make ifxs
	make ifxdb
	make ifxutil
	make ifxcreate
	make ifxad
