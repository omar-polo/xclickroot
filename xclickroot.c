#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>

static void
usage(void)
{
	(void)fprintf(stderr, "usage: xclickroot [-12345lmr] command [args...]\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	unsigned button;
	XEvent ev;
	Display *dpy;
	Window rootwin;

#if __OpenBSD__
	if (pledge("unix stdio rpath proc exec", NULL) == -1)
		err(EXIT_FAILURE, "pledge");
#endif

	argv++;
	argc--;
	button = Button3;
	if (*argv && (*argv)[0] == '-') {
		switch ((*argv)[1]) {
		case '1': case 'l': button = Button1; break;
		case '2': case 'm': button = Button2; break;
		case '3': case 'r': button = Button3; break;
		case '4':           button = Button4; break;
		case '5':           button = Button5; break;
		default:
			usage();
			break;
		}

		if ((*argv)[2] != '\0')
			usage();

		argv++;
		argc--;
	}

	if (argc == 0)
		usage();

	/* don't leave zombies around */
	signal(SIGCHLD, SIG_IGN);

	/* open connection to server and set X variables */
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(EXIT_FAILURE, "cannot open display");
	rootwin = DefaultRootWindow(dpy);
	XGrabButton(dpy, button, AnyModifier, rootwin, False, ButtonPressMask,
	            GrabModeSync, GrabModeSync, None, None);
	while (!XWindowEvent(dpy, rootwin, ButtonPressMask, &ev)) {
		if (ev.type == ButtonPress) {
			if (ev.xbutton.button != button || ev.xbutton.subwindow != None) {
				XAllowEvents(dpy, ReplayPointer, CurrentTime);
				continue;
			}
			XUngrabPointer(dpy, ev.xbutton.time);
			XAllowEvents(dpy, ReplayPointer, CurrentTime);
			XFlush(dpy);
			switch (fork()) {
			case -1:
				warn("can't fork");
				break;
			case 0:
				execvp(*argv, argv);
				warn("can't exec %s", *argv);
				_exit(127);
			}
		}
	}
	XCloseDisplay(dpy);
	return 0;
}
