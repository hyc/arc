u_int	getbuf PROTO ((FILE *f));
u_int	getb_ucr PROTO ((FILE *f));
u_int	getb_usq PROTO ((FILE *f));
u_int	getb_unp PROTO ((FILE *f));
VOID	hufb_tab PROTO ((u_char *buf, u_int len));
VOID	 lzw_buf PROTO ((u_char *buf, u_int len, FILE *f));
VOID	putb_unp PROTO ((u_char *buf, u_int len, FILE *f));
VOID	putb_ncr PROTO ((u_char *buf, u_int len, FILE *f));
VOID	putb_pak PROTO ((u_char *buf, u_int len, FILE *f));
