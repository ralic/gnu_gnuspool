/* xspq.c -- xspq main module

   Copyright 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
static	char	rcsid2[] = "@(#) $Revision: 1.1 $";
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <gtk/gtk.h>
#include "incl_sig.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "xfershm.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xsq_ext.h"
#include "displayopt.h"
#include "gtk_lib.h"

#define	DEFAULT_WIDTH	400
#define	DEFAULT_HEIGHT	400

void	load_optfile(void);
void	job_redisplay(void);
void	openjfile(void);
void	openpfile(void);
void	ptr_redisplay(void);

FILE	*Cfile;

#define	IPC_MODE	0600
int	Ctrl_chan = -1;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;

extern	struct	macromenitem	jobmacs[], ptrmacs[];

DEF_DISPOPTS;

unsigned	Pollinit,	/* Initial polling */
		Pollfreq;	/* Current polling frequency */

uid_t	Daemuid,
	Realuid,
	Effuid;

char	confabort = 1;
char	*Realuname;

struct	spdet	*mypriv;

struct	spr_req	jreq,
		preq,
		oreq;
struct	spq	JREQ;
struct	spptr	PREQ;

char	*ptdir,
	*spdir,
	*Curr_pwd;

int	Dirty;

/* X Stuff */

GtkWidget	*toplevel,	/* Main window */
		*jwid,		/* Job scroll list */
		*pwid;		/* Printer scroll list */

GtkUIManager 	*ui;

GtkListStore	*jlist_store,
		*unsorted_plist_store;

GtkTreeModelSort	*sorted_plist_store;

static void	cb_about(void);
static void	cb_quit(void);
extern void	cb_viewopt(void);
extern void	cb_saveopts(void);
extern void	cb_syserr(void);
extern void	cb_jact(void);
extern void	cb_onemore(void);
extern void	cb_pact(GtkAction *);
extern void	cb_view(void);
extern void	cb_jform(void);
extern void	cb_jpages(void);
extern void	cb_juser(void);
extern void	cb_jretain(void);
extern void	cb_jclass(void);
extern void	cb_unqueue(void);
extern void	cb_pform(void);
extern void	cb_pclass(void);
extern void	cb_pdev(void);
extern void	cb_padd(void);
extern void	cb_search(void);
extern void	cb_rsearch(GtkAction *);
extern void	cb_jmac(void);
extern void	cb_pmac(void);
extern void	cb_jmacedit(void);
extern void	cb_pmacedit(void);

extern void	loadmacs(const char, struct macromenitem *);

static GtkActionEntry entries[] = {
	{ "FileMenu", NULL, "_File"  },
	{ "ActMenu", NULL, "_Action"  },
	{ "JobMenu", NULL, "_Jobs"  },
	{ "PtrMenu", NULL, "_Printers" },
	{ "SrchMenu", NULL, "_Search"  },
	{ "jmacMenu", NULL, "Job _macros" },
	{ "pmacMenu", NULL, "Pt_r macros" },
	{ "HelpMenu", NULL, "_Help"  },
	{ "tb", NULL, "_TB"  },
	{ "Viewopts", NULL, "_View Options", "equal", "Select view", G_CALLBACK(cb_viewopt) },
	{ "Saveopts", NULL, "_Save Options", NULL, "Remember view options", G_CALLBACK(cb_saveopts) },
	{ "Syserr", NULL, "Display _Error Log", NULL, "Display system error log file", G_CALLBACK(cb_syserr) },
	{ "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit program", G_CALLBACK(cb_quit)},
	{ "Jab", NULL, "_Abort Job", "a", "Abort job if printing and delete", G_CALLBACK(cb_jact) },
	{ "Onemore", NULL, "_Plus one",  "plus", "One more copy of job", G_CALLBACK(cb_onemore)  },
	{ "Go", NULL, "_Go printer", "<shift>G", "Start printer running", G_CALLBACK(cb_pact)  },
	{ "Heoj", NULL, "_Halt printer", "H", "Halt printer at end of current job", G_CALLBACK(cb_pact) },
	{ "Halt", NULL, "_Stop printer", "<shift>H", "Halt printer at once", G_CALLBACK(cb_pact) },
	{ "Ok", NULL, "_OK alignment", "Y", "Approve alignment page", G_CALLBACK(cb_pact) },
	{ "NOk", NULL, "_Not OK alignment", "N", "Disapprove alignment page", G_CALLBACK(cb_pact) },
	{ "View", NULL, "_View job", "<shift>I", "View job as text", G_CALLBACK(cb_view) },
	{ "Formj", NULL, "Job _form", "F", "Set job form, title, printer, priority", G_CALLBACK(cb_jform)  },
	{ "Pages", NULL, "Job _pages", NULL, "Set page range for printing", G_CALLBACK(cb_jpages)  },
	{ "User", NULL, "_User, mail", NULL, "Set User and mail options", G_CALLBACK(cb_juser)  },
	{ "Retain", NULL, "_Retain opts", NULL, "Set retention on queue and similar", G_CALLBACK(cb_jretain)  },
	{ "Classj", NULL, "_Class codes", NULL, "Set job class codes", G_CALLBACK(cb_jclass)  },
	{ "Unqueue", NULL, "Un_queue", NULL, "Copy job to file and shell script", G_CALLBACK(cb_unqueue)  },
	{ "Interrupt", NULL, "_Interrupt", "exclam", "Interrupt current printer job and pick higher priority", G_CALLBACK(cb_pact)  },
	{ "Abortp", NULL, "_Abort", NULL, "Abort whatever job is being printed", G_CALLBACK(cb_pact)  },
	{ "Restart", NULL, "_Restart", NULL, "Restart job currently being printed", G_CALLBACK(cb_pact)  },
	{ "Formp", NULL, "Set printer _form", NULL, "Select form type for printer", G_CALLBACK(cb_pform)  },
	{ "Classp", NULL, "Set printer _classcode", NULL, "Select class code for printer", G_CALLBACK(cb_pclass)  },
	{ "Devicep", NULL, "Set printer _device", NULL, "Select device or host/IP for printer", G_CALLBACK(cb_pdev)  },
	{ "Add", NULL, "_Add printer", NULL, "Add a new printer", G_CALLBACK(cb_padd)  },
	{ "Delete", NULL, "_Delete printer", NULL, "Delete selected printer", G_CALLBACK(cb_pact)  },
	{ "Search", NULL, "_Search for ...", NULL, "Search for a job or printer", G_CALLBACK(cb_search)  },
	{ "Fsearch", NULL, "Search _forward", "F3", "Repeat last search going forward", G_CALLBACK(cb_rsearch)  },
	{ "Rsearch", NULL, "Search _backward", "F4", "Repeat last search going backward", G_CALLBACK(cb_rsearch)  },
	{ "jrunmac", NULL, "_Run macro command", NULL, "Run macro for job", G_CALLBACK(cb_jmac)  },
	{ "Jmacedit", NULL, "_Edit macro list", NULL, "Edit list of job macros", G_CALLBACK(cb_jmacedit)  },
	{ "prunmac", NULL, "_Run macro command", NULL, "Run printer for printer", G_CALLBACK(cb_pmac)  },
	{ "Pmacedit", NULL, "_Edit macro list", NULL, "Edit list of printer maros", G_CALLBACK(cb_jmacedit)  },
	{ "About", NULL, "About xspq", NULL, "About xspq", G_CALLBACK(cb_about)}  };

extern void	init_jlist_win(void);
extern void	init_plist_win(void);
extern void	init_jdisplay(void);
extern void	init_pdisplay(void);

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

/* If we get a message error die appropriately */

static void	msg_error(const int ret)
{
	doerror(ret);
	exit(E_SETUP);
}

/* Write messages to scheduler.  */

void	womsg(const int act)
{
	oreq.spr_un.o.spr_act = (USHORT) act;
	if  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)
		msg_error(errno == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error});
}

void	my_wjmsg(const int act)
{
	int	ret;
	jreq.spr_un.j.spr_act = (USHORT) act;
	if  ((ret = wjmsg(&jreq, &JREQ)))
		msg_error(ret);
}

void	my_wpmsg(const int act)
{
	int	ret;
	preq.spr_un.p.spr_act = (USHORT) act;
	if  ((ret = wpmsg(&preq, &PREQ)))
		msg_error(ret);
}

char	*authlist[] =  { "John M Collins", NULL  };

static void	cb_about(void)
{
	GtkWidget  *dlg = gtk_about_dialog_new();
	char	*cp = strchr(rcsid2, ':');
	char	vbuf[20];

	if  (!cp)
		strcpy(vbuf, "Initial version");
	else  {
		char  *ep;
		cp++;
		ep = strchr(cp, '$');
		int  n = ep - cp;
		strncpy(vbuf, cp, n);
		vbuf[n] = '\0';
	}
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), vbuf);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dlg), "Xi Software Ltd 2008");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), "http://www.xisl.com");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg), (const char **) authlist);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

static void	cb_quit(void)
{
	if  (Dirty  &&  !Confirm($PH{xspq changes not saved ok}))
		return;
	gtk_main_quit();
}

gboolean	check_dirty(void)
{
	if  (Dirty  &&  !Confirm($PH{xspq changes not saved ok}))
		return  TRUE;
	return  FALSE;
}

/* Possibly redisplay if something has changed.  */

gboolean	poll_changes(void)
{
	if  (Ptr_seg.Last_ser != Ptr_seg.dptr->ps_serial)
		ptr_redisplay();
	if  (Job_seg.Last_ser != Job_seg.dptr->js_serial)
		job_redisplay();
	return  TRUE;
}

static void	winit(void)
{
	GError *err;
	char	*fn;

	toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(toplevel), DEFAULT_WIDTH, DEFAULT_HEIGHT);
	fn = gprompt($P{xspq app title});
	gtk_window_set_title(GTK_WINDOW(toplevel), fn);
	free(fn);
	gtk_container_set_border_width(GTK_CONTAINER(toplevel), 5);
	fn = envprocess(XSPQ_ICON);
	gtk_window_set_default_icon_from_file(fn, &err);
	free(fn);
	gtk_window_set_resizable(GTK_WINDOW(toplevel), TRUE);
	g_signal_connect(G_OBJECT(toplevel), "delete_event", G_CALLBACK(check_dirty), NULL);
	g_signal_connect(G_OBJECT(toplevel), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

GtkWidget *	wstart(void)
{
	char	*mf;
	GError *err;
	GtkActionGroup *actions;
	GtkWidget  *vbox;

	actions = gtk_action_group_new("Actions");
	gtk_action_group_add_actions(actions, entries, G_N_ELEMENTS(entries), NULL);
	ui = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui, actions, 0);
	gtk_window_add_accel_group(GTK_WINDOW(toplevel), gtk_ui_manager_get_accel_group(ui));
	mf = envprocess(XSPQ_MENU);
	if  (!gtk_ui_manager_add_ui_from_file(ui, mf, &err))  {
		g_message("Menu build failed");
		exit(99);
	}
	free(mf);
	loadmacs('j', jobmacs);
	loadmacs('p', ptrmacs);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(toplevel), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui, "/MenuBar"), FALSE, FALSE, 0);
	init_jlist_win();
	init_plist_win();
	return  vbox;
}

void	view_popup_menu(GtkWidget * treeview, GdkEventButton * event, gpointer userdata)
{
	gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(ui, (const char *) userdata)), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time());
}

gboolean	view_clicked(GtkWidget * treeview, GdkEventButton * event, gpointer userdata)
{
	if  (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)  {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		GtkTreePath *path;
		if  (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint) event->x, (gint) event->y, &path, NULL, NULL, NULL))  {
			gtk_tree_selection_unselect_all(selection);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_path_free(path);
			view_popup_menu(treeview, event, userdata);
			return  TRUE;
		}
	}
	return  FALSE;
}

static void	wcomplete(GtkWidget * vbox)
{
	GtkWidget  *paned, *scroll1, *scroll2;

	scroll1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scroll1), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll1), jwid);
	scroll2 = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scroll2), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll2), pwid);
	paned = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
	gtk_paned_pack1(GTK_PANED(paned), scroll1, TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(paned), scroll2, TRUE, TRUE);
	g_signal_connect(jwid, "button-press-event", (GCallback) view_clicked, "/jpop");
	g_signal_connect(jwid, "popup-menu", (GCallback) view_popup_menu, "/jpop");
	g_signal_connect(pwid, "button-press-event", (GCallback) view_clicked, "/ppop");
	g_signal_connect(pwid, "popup-menu", (GCallback) view_popup_menu, "/ppop");
	gtk_widget_show_all(toplevel);
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	GtkWidget  *vbox;
	int	ret;

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	/* If we haven't got a directory, use the current */

	if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
		Curr_pwd = runpwd();

	Realuid = getuid();
	Effuid = geteuid();
	if  ((LONG) (Daemuid = lookup_uname(SPUNAME)) == UNKNOWN_UID)
		Daemuid = ROOTID;
	Cfile = open_cfile("XSPQCONF", "xmspq.help");

	gtk_chk_uid();

#ifdef	HAVE_SETREUID
	setreuid(Daemuid, Daemuid);
#else
	setuid(Daemuid);
#endif
	load_optfile();
	gtk_init(&argc, &argv);
	mypriv = getspuser(Realuid);

	Displayopts.opt_classcode = mypriv->spu_class;

	if  ((mypriv->spu_flgs & (PV_OTHERJ|PV_VOTHERJ)) != (PV_OTHERJ|PV_VOTHERJ))
		Realuname = prin_uname(Realuid);

	spdir = envprocess(SPDIR);

	if  (chdir(spdir) < 0)  {
		print_error($E{Cannot chdir});
		exit(E_NOCHDIR);
	}
	ptdir = envprocess(PTDIR);

	hash_hostfile();	/* To get host names first */

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		print_error($E{Spooler not running});
		exit(E_NOTRUN);
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	if  ((ret = init_xfershm(1)))  {
		print_error(ret);
		exit(E_SETUP);
	}
	openjfile();
	openpfile();
	oreq.spr_mtype = jreq.spr_mtype = preq.spr_mtype = MT_SCHED;
	oreq.spr_un.o.spr_pid = preq.spr_un.p.spr_pid = jreq.spr_un.j.spr_pid = getpid();

	winit();
	vbox = wstart();
	init_jdisplay();
	init_pdisplay();
	wcomplete(vbox);
	g_timeout_add(1000, (GSourceFunc) poll_changes, NULL);

	gtk_main();
	return  0;
}
