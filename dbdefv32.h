
#define	ALLOC_MAX	64
#define	ALLOC_MAXL	256
#define	T_MAX		448
#define T_P_SIZE	2048
#define	PAGES_MAX	945


struct  table   
{
char    name[20];
int     nr;
char	nc,poolid,type,access;
unsigned short int attrs[50];
short	int     lc[T_MAX]; 
};


struct  attr
{
        char    name[20];
	short	int len,qty,index;
	char	type,ald,pool,access;
        short	int     al[ALLOC_MAX];
	short	int	lc[PAGES_MAX];
};

struct  pool
{
unsigned char id,ihi,hi,r;
int     ipages,mpages,map;
FILE	*f;
struct  attr alloc;
};

struct	initdata
{
int	attr_qty,attr_offset;
int	t1_offset,t2_offset,t1_qty,p_qty;
int	acc_qty;
};

struct	XT
{
char	name[20];
int	ts;
unsigned char	id,pool,type,access;
unsigned short int	qa,in;
struct	table	*tbase;
unsigned int	ST[5];
};
struct	TF {
	int		s,code;
	short int	size;
	unsigned char	t,owner,*p;
};


struct	table *find_table(char *buf);
char	*read_attr(struct attr *, int, char *);
int	write_attr(struct attr *, char *);
int	free_attr(struct attr *, int );
char	*update_attr(struct attr *, int, char *);
int	create_attr(char *, char , int, char,char);
char	*translate_addr(struct attr *, int, int );

char	*map_segment(int ,int ,int);
int	vmark_used(struct attr *);
int	mark_used(int *,int );
