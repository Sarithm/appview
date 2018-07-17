#
ifxad:	ifxad_v103.c
	gcc -o ifxad ifxad_v103.c

ifxdb:	func4003.c ssct4201.c dbkern4101.c
	gcc -s -o ifxdb ssct4201.c dbkern4101.c func4003.c -lpthread


