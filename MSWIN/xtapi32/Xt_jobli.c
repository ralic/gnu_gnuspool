/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_joblist.c: created by John Collins on Tue Mar  8 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_jobli.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_jobli.c,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 22.1  1995/01/13  17:06:57  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:22:26  jmc
 * Brand new Release 21
 *
 * Revision 20.1  1994/03/24  17:25:52  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 */

#include <sys/types.h>
#include <malloc.h>
#include <winsock.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int	xt_read(const SOCKET, char *, unsigned),
			xt_rmsg(const struct api_fd *, struct api_msg *),
			xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

int	xt_joblist(const int fd, const unsigned	flags, int *nj, slotno_t **slots)
{
	int		ret;
	unsigned	numjobs;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  XT_INVALID_FD;

	msg.code = API_JOBLIST;
	msg.un.lister.flags = htonl(flags);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);

	/* Get number of jobs */

	fdp->jserial = ntohl(msg.un.r_lister.seq);
	numjobs = (unsigned) ntohl(msg.un.r_lister.nitems);
	
	if  (nj)
		*nj = (int) numjobs;

	/* Try to allocate enough space to hold the list.
	   If we don't succeed we'd better carry on reading it
	   so we don't get out of sync. */

	if  (numjobs != 0)  {
		unsigned  nbytes = numjobs * sizeof(slotno_t), cnt;
		slotno_t	*sp;
				
		if  (nbytes > fdp->bufmax)  {
			if  (fdp->bufmax != 0)  {
				free(fdp->buff);
				fdp->bufmax = 0;
				fdp->buff = (char *) 0;
			}
			if  (!(fdp->buff = malloc(nbytes)))  {
				unsigned  cnt;
				for  (cnt = 0;  cnt < numjobs;  cnt++)  {
					ULONG  slurp;
					if  ((ret = xt_read(fdp->sockfd, (char *) &slurp, sizeof(slurp))))
						return  ret;
				}
				return  XT_NOMEM;
			}
			fdp->bufmax = nbytes;
		}
		if  ((ret = xt_read(fdp->sockfd, fdp->buff, nbytes)))
			return  ret;

		sp = (slotno_t *) fdp->buff;
		for  (cnt = 0;  cnt < numjobs;  cnt++)  {
			*sp = ntohl(*sp);
			sp++;
		}
	}

	/* Set up answer */

	if  (slots)
		*slots = (slotno_t *) fdp->buff;
	return  XT_OK;
}
