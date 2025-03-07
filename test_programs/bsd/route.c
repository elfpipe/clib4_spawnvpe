/*
 * route.c - route command for AmiTCP/IP
 *
 * Author: jraja <Jarno.Rajahalme@hut.fi>
 *
 * Last modified: Tue Feb 22 02:12:47 1994 ppessi
 */

static const char version[] = "\0$VER: route 1.0 (08.08.2022)"
                              " "
                              "Copyright © 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>\n"
                              "Helsinki University of Technology, Finland.\n"
                              "@(#) Copyright © 1983, 1989 The Regents of the University of California.\n"
                              "All rights reserved.\n";

/*
 * Copyright © 1983, 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

static char sccsid[] = "@(#)route.c	5.35 (Berkeley) 6/27/91";

/****** netutil.doc/route ***************************************************
*
*   NAME
*        route - manually manipulate the routing tables
*
*   SYNOPSIS
*        route [-n] [-q] [-v] command [modifiers] destination gateway
*
*   DESCRIPTION
*        Route is a program used to manually manipulate the network routing
*        tables.
*
*        Options supported by route:
*
*        -n      Prevent attempts to print host and network names
*                symbolically when reporting actions.
*
*        -v      (verbose) Print additional details.
*
*        -q      Suppress all output.
*
*        Commands accepted by route:
*
*        add         Add a route.
*        delete      Delete a specific route.
*
*        The destination is the destination host or network, gateway is the
*        next-hop gateway to which packets should be addressed. Routes to a
*        particular host are distinguished from those to a network by
*        interpreting the Internet address associated with destination. The
*        optional modifiers -net and -host force the destination to be
*        interpreted as a network or a host, respectively.  Otherwise, if the
*        destination has a ``local address part'' of INADDR_ANY, or if the
*        destination is the symbolic name of a network, then the route is
*        assumed to be to a network; otherwise, it is presumed to be a route
*        to a host.
*
*        For example, 128.32 is interpreted as -host 128.0.0.32; 128.32.130
*        is interpreted as -host 128.32.0.130; -net 128.32 is interpreted as
*        128.32.0.0; and -net 128.32.130 is interpreted as 128.32.130.0.
*
*        To add a default route, give the destination as 'default'.
*
*        If the route is via an interface rather than via a gateway, the
*        -interface modifier should be specified; the gateway given is the
*        address of this host on the common network, indicating the interface
*        to be used for transmission.
*
*        The optional -netmask qualifier is used to specify the netmask of
*        the interface. One specifies an additional ensuing address parameter
*        (to be interpreted as a network mask).  The implicit network mask
*        generated can be overridden by making sure this option follows the
*        destination parameter.
*
*        All symbolic names specified for a destination or gateway are looked
*        up first as a host name using gethostbyname(). If this lookup fails,
*        getnetbyname() is then used to interpret the name as that of a
*        network.
*
*   DIAGNOSTICS
*        add [host | network ] %s: gateway %s flags %x
*                The specified route is being added to the tables. The values
*                printed are from the routing table entry supplied in the
*                IoctlSocket() call. If the gateway address used was not the
*                primary address of the gateway (the first one returned by
*                gethostbyname()), the gateway address is printed numerically
*                as well as symbolically.
*
*        delete [ host | network ] %s: gateway %s flags %x
*                As above, but when deleting an entry.
*
*        Network is unreachable
*                An attempt to add a route failed because the gateway listed
*                was not on a directly-connected network.  The next-hop
*                gateway must be given.
*
*        not in table
*                A delete operation was attempted for an entry which wasn't
*                present in the tables.
*
*        routing table overflow
*                An add operation was attempted, but the system was low on
*                resources and was unable to allocate memory to create the
*                new entry.
*
*   SEE ALSO
*        ifconfig, protocols/routing
*
*   HISTORY
*        The route command appeared in 4.2BSD.
*
*****************************************************************************
*
*/

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <sys/sockio.h>


struct keytab {
    char *kt_cp;
    int kt_i;
} keywords[] = {
#include "keywords.h"

        {0, 0}
};

struct ortentry route;
union sockunion {
    struct sockaddr sa;
    struct sockaddr_in sockin;
#if 0
    struct	sockaddr_ns sns;
    struct	sockaddr_iso siso;
    struct	sockaddr_dl sdl;
    struct	sockaddr_x25 sx25;
#endif
} so_dst, so_gate, so_mask, so_genmask, so_ifa, so_ifp;

union sockunion *so_addrs[] =
        {&so_dst, &so_gate, &so_mask, &so_genmask, &so_ifp, &so_ifa, 0};

typedef union sockunion *sup;
int rtm_addrs;
int s;
int forcehost, forcenet, doflush, nflag, af, qflag, tflag, Cflag,
        keyword(char *cp);
int iflag, verbose, aflen = sizeof(struct sockaddr_in);
int locking, lockrest, debugonly;
struct sockaddr_in sockin = {sizeof(sockin), AF_INET};
struct rt_metrics rt_metrics;
u_long rtm_inits;
#if 0
struct	in_addr inet_makeaddr();
#endif

char *routename(struct sockaddr *sa), *netname(struct sockaddr *sa);

void flushroutes(int argc, char *argv[]),
        newroute(int argc, char **argv), monitor(void), sockaddr();

void print_getmsg(), print_rtmsg(), pmsg_common(), sodump(), bprintf();

int getaddr(int which, char *s, struct hostent **hpp), rtmsg();

void
usage(char *cp) {
    if (cp)
        fprintf(stderr, "route: botched keyword: %s\n", cp);
    fprintf(stderr,
            "usage: route [ -Cnqv ] cmd [[ -<qualifers> ] args ]\n");
    exit(1);
    /* NOTREACHED */
}

void
quit(char *s) {
    int sverrno = errno;

    fprintf(stderr, "route: ");
    if (s)
        fprintf(stderr, "%s: ", s);
    fprintf(stderr, "%s\n", strerror(sverrno));
    exit(1);
    /* NOTREACHED */
}

#define ROUNDUP(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

int main(int argc, char **argv) {
    extern int optind;
    int ch;
    char *argvp;

    if (argc < 2)
        usage(NULL);

    while ((ch = getopt(argc, argv, "Cnqtv")) != EOF)
        switch (ch) {
            case 'C':
                Cflag = 1;    /* Use old ioctls. */
                break;
            case 'n':
                nflag = 1;
                break;
            case 'q':
                qflag = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 't':
                tflag = 1;
                break;
            case '?':
            default:
                usage(NULL);
        }
    argc -= optind;
    argv += optind;

    //Cflag = 1;    /* Force old ioctls. */

    if (Cflag)
        s = socket(AF_INET, SOCK_RAW, 0);
    if (s < 0)
        quit("socket");
    if (*argv)
        switch (keyword(*argv)) {
            case K_GET:
#if 0
                uid = 0;
#endif
                /* FALLTHROUGH */

            case K_CHANGE:
                if (Cflag)
                    usage("change or get with -C");
                /* FALLTHROUGH */

            case K_ADD:
            case K_DELETE:
                newroute(argc, argv);
                exit(0);
                /* NOTREACHED */

            case K_MONITOR:
                monitor();
                /* NOTREACHED */

            case K_FLUSH:
                flushroutes(argc, argv);
                exit(0);
                /* NOTREACHED */
        }
    usage(*argv);
    /* NOTREACHED */
}

/*
 * Purge all entries in the routing tables not
 * associated with network interfaces.
 */
void
flushroutes(argc, argv)
        int argc;
        char *argv[];
{
#if 0
    int needed, seqno, rlen;
    char *buf, *next, *lim;
    register struct rt_msghdr *rtm;

    if (uid)
        quit("must be root to alter routing table");
    shutdown(s, 0); /* Don't want to read back our messages */
    if (argc > 1) {
        argv++;
        if (argc == 2 && **argv == '-')
            switch (keyword(*argv + 1)) {
            case K_INET:
                af = AF_INET;
                break;
            case K_XNS:
                af = AF_NS;
                break;
            case K_LINK:
                af = AF_LINK;
                break;
            case K_ISO:
            case K_OSI:
                af = AF_ISO;
                break;
            case K_X25:
                af = AF_CCITT;
            default:
                goto bad;
        } else
bad:			usage(*argv);
    }
    if ((needed = getkerninfo(KINFO_RT_DUMP, 0, 0, 0)) < 0)
        quit("route-getkerninfo-estimate");
    if ((buf = malloc(needed)) == NULL)
        quit("malloc");
    if ((rlen = getkerninfo(KINFO_RT_DUMP, buf, &needed, 0)) < 0)
        quit("actual retrieval of routing table");
    lim = buf + rlen;
    seqno = 0;		/* ??? */
    for (next = buf; next < lim; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr *)next;
        if ((rtm->rtm_flags & RTF_GATEWAY) == 0)
            continue;
        if (af) {
            struct sockaddr *sa = (struct sockaddr *)(rtm + 1);

            if (sa->sa_family != af)
                continue;
        }
        rtm->rtm_type = RTM_DELETE;
        rtm->rtm_seq = seqno;
        rlen = write(s, next, rtm->rtm_msglen);
        if (rlen < (int)rtm->rtm_msglen) {
            fprintf(stderr,
                "route: write to routing socket: %s\n",
                strerror(errno));
            printf("got only %d for rlen\n", rlen);
            break;
        }
        seqno++;
        if (qflag)
            continue;
        if (verbose)
            print_rtmsg(rtm, rlen);
        else {
            struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
            printf("%-20.20s ", rtm->rtm_flags & RTF_HOST ?
                routename(sa) : netname(sa));
            sa = (struct sockaddr *)(sa->sa_len + (char *)sa);
            printf("%-20.20s ", routename(sa));
            printf("done\n");
        }
    }
#endif /* 0 */
}

char *
routename(sa)
        struct sockaddr *sa;
{
    register char *cp;
    static char line[50];
    struct hostent *hp;
    static char domain[MAXHOSTNAMELEN + 1];
    static int first = 1;
    char *ns_print();

    if (first) {
        first = 0;
        if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
            (cp = index(domain, '.')))
            strcpy(domain, cp + 1);
        else
            domain[0] = 0;
    }
    switch (sa->sa_family) {

        case AF_INET: {
            struct in_addr in;
            in = ((struct sockaddr_in *) sa)->sin_addr;

            cp = 0;
            if (in.s_addr == INADDR_ANY)
                cp = "default";
            if (cp == 0 && !nflag) {
                hp = gethostbyaddr((char *) &in, sizeof(struct in_addr),
                                   AF_INET);
                if (hp) {
                    if ((cp = index(hp->h_name, '.')) &&
                        !strcmp(cp + 1, domain))
                        *cp = 0;
                    cp = hp->h_name;
                }
            }
            if (cp)
                strcpy(line, cp);
            else {
#define C(x)    ((x) & 0xff)
                in.s_addr = ntohl(in.s_addr);
                sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
                        C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
            }
            break;
        }
#if 0
            case AF_NS:
        return (ns_print((struct sockaddr_ns *)sa));

    case AF_LINK:
        return (link_ntoa((struct sockaddr_dl *)sa));

    case AF_ISO:
        sprintf(line, "iso %s",
            iso_ntoa(&((struct sockaddr_iso *)sa)->siso_addr));
        break;
#endif
        default: {
            u_short *s = (u_short *) sa->sa_data;
            u_short *slim = s + ((sa->sa_len + 1) >> 1);
            char *cp = line + sprintf(line, "(%d)", sa->sa_family);

            while (s < slim)
                cp += sprintf(cp, " %x", *s++);
            break;
        }
    }
    return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(sa)
        struct sockaddr *sa;
{
    char *cp = 0;
    static char line[50];
    struct netent *np = 0;
    u_long net, mask;
    register u_long i;
    int subnetshift;
    char *ns_print();

    switch (sa->sa_family) {

        case AF_INET: {
            struct in_addr in;
            in = ((struct sockaddr_in *) sa)->sin_addr;

            i = in.s_addr = ntohl(in.s_addr);
            if (in.s_addr == 0)
                cp = "default";
            else if (!nflag) {
                if (IN_CLASSA(i)) {
                    mask = IN_CLASSA_NET;
                    subnetshift = 8;
                } else if (IN_CLASSB(i)) {
                    mask = IN_CLASSB_NET;
                    subnetshift = 8;
                } else {
                    mask = IN_CLASSC_NET;
                    subnetshift = 4;
                }
                /*
                 * If there are more bits than the standard mask
                 * would suggest, subnets must be in use.
                 * Guess at the subnet mask, assuming reasonable
                 * width subnet fields.
                 */
                while (in.s_addr & ~mask)
                    mask = (long) mask >> subnetshift;
                net = in.s_addr & mask;
                while ((mask & 1) == 0)
                    mask >>= 1, net >>= 1;
                np = getnetbyaddr(net, AF_INET);
                if (np)
                    cp = np->n_name;
            }
            if (cp)
                strcpy(line, cp);
            else if ((in.s_addr & 0xffffff) == 0)
                sprintf(line, "%u", C(in.s_addr >> 24));
            else if ((in.s_addr & 0xffff) == 0)
                sprintf(line, "%u.%u", C(in.s_addr >> 24),
                        C(in.s_addr >> 16));
            else if ((in.s_addr & 0xff) == 0)
                sprintf(line, "%u.%u.%u", C(in.s_addr >> 24),
                        C(in.s_addr >> 16), C(in.s_addr >> 8));
            else
                sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
                        C(in.s_addr >> 16), C(in.s_addr >> 8),
                        C(in.s_addr));
            break;
        }
#if 0
            case AF_NS:
        return (ns_print((struct sockaddr_ns *)sa));
        break;

    case AF_LINK:
        return (link_ntoa((struct sockaddr_dl *)sa));

    case AF_ISO:
        sprintf(line, "iso %s",
            iso_ntoa(&((struct sockaddr_iso *)sa)->siso_addr));
        break;
#endif
        default: {
            u_short *s = (u_short *) sa->sa_data;
            u_short *slim = s + ((sa->sa_len + 1) >> 1);
            char *cp = line + sprintf(line, "af %d:", sa->sa_family);

            while (s < slim)
                cp += sprintf(cp, " %x", *s++);
            break;
        }
    }
    return (line);
}

void
set_metric(char *value, int key) {
#if 0
    int flag = 0;
    u_long noval, *valp = &noval;

    switch (key) {
#define caseof(x, y, z)	case x: valp = &rt_metrics.z; flag = y; break
    caseof(K_MTU, RTV_MTU, rmx_mtu);
    caseof(K_HOPCOUNT, RTV_HOPCOUNT, rmx_hopcount);
    caseof(K_EXPIRE, RTV_EXPIRE, rmx_expire);
    caseof(K_RECVPIPE, RTV_RPIPE, rmx_recvpipe);
    caseof(K_SENDPIPE, RTV_SPIPE, rmx_sendpipe);
    caseof(K_SSTHRESH, RTV_SSTHRESH, rmx_ssthresh);
    caseof(K_RTT, RTV_RTT, rmx_rtt);
    caseof(K_RTTVAR, RTV_RTTVAR, rmx_rttvar);
    }
    rtm_inits |= flag;
    if (lockrest || locking)
        rt_metrics.rmx_locks |= flag;
    if (locking)
        locking = 0;
    *valp = atoi(value);
#endif
}

void
newroute(int argc, register char **argv) {
    char *cmd, *dest = "", *gateway = "", *err;
    int ishost = 0, ret, attempts, oerrno, flags = 0;
    int key;
    struct hostent *hp = 0;

    cmd = argv[0];
    if (*cmd != 'g')
        shutdown(s, 0); /* Don't want to read back our messages */
    while (--argc > 0) {
        if (**(++argv) == '-') {
            switch (key = keyword(1 + *argv)) {
#if 0
                case K_LINK:
                af = AF_LINK;
                aflen = sizeof(struct sockaddr_dl);
                break;
            case K_OSI:
            case K_ISO:
                af = AF_ISO;
                aflen = sizeof(struct sockaddr_iso);
                break;
#endif
            case K_INET:
                af = AF_INET;
                aflen = sizeof(struct sockaddr_in);
                break;
#if 0
            case K_X25:
                af = AF_CCITT;
                aflen = sizeof(struct sockaddr_x25);
                break;
            case K_SA:
                af = 0;
                aflen = sizeof(union sockunion);
                break;
            case K_XNS:
                af = AF_NS;
                aflen = sizeof(struct sockaddr_ns);
                break;
#endif
            case K_IFACE:
            case K_INTERFACE:
                iflag++;
                break;
            case K_LOCK:
                locking = 1;
                break;
            case K_LOCKREST:
                lockrest = 1;
                break;
            case K_HOST:
                forcehost++;
                break;
            case K_REJECT:
                flags |= RTF_REJECT;
                break;
            case K_PROTO1:
                flags |= RTF_PROTO1;
                break;
            case K_PROTO2:
                flags |= RTF_PROTO2;
                break;
            case K_CLONING:
                flags |= RTF_CLONING;
                break;
            case K_XRESOLVE:
                flags |= RTF_XRESOLVE;
                break;
            case K_IFA:
                argc--;
                getaddr(RTA_IFA, *++argv, 0);
                break;
            case K_IFP:
                argc--;
                getaddr(RTA_IFP, *++argv, 0);
                break;
            case K_GENMASK:
                argc--;
                getaddr(RTA_GENMASK, *++argv, 0);
                break;
            case K_GATEWAY:
                argc--;
                getaddr(RTA_GATEWAY, *++argv, 0);
                break;
            case K_DST:
                argc--;
                ishost = getaddr(RTA_DST, *++argv, &hp);
                dest = *argv;
                break;
            case K_NETMASK:
                argc--;
                getaddr(RTA_NETMASK, *++argv, 0);
                /* FALLTHROUGH */
            case K_NET:
                forcenet++;
                break;
            case K_MTU:
            case K_HOPCOUNT:
            case K_EXPIRE:
            case K_RECVPIPE:
            case K_SENDPIPE:
            case K_SSTHRESH:
            case K_RTT:
            case K_RTTVAR:
                argc--;
                set_metric(*++argv, key);
                break;
            default:
                usage(1 + *argv);
            }
        } else {
            if ((rtm_addrs & RTA_DST) == 0) {
                dest = *argv;
                ishost = getaddr(RTA_DST, *argv, &hp);
            } else if ((rtm_addrs & RTA_GATEWAY) == 0) {
                gateway = *argv;
                getaddr(RTA_GATEWAY, *argv, &hp);
            } else {
                int ret = atoi(*argv);
                if (ret == 0) {
                    printf("%s,%s", "old usage of trailing 0",
                           "assuming route to if\n");
                    iflag = 1;
                    continue;
                } else if (ret > 0 && ret < 10) {
                    printf("old usage of trailing digit, ");
                    printf("assuming route via gateway\n");
                    iflag = 0;
                    continue;
                }
                getaddr(RTA_NETMASK, *argv, 0);
            }
        }
    }
    if (forcehost)
        ishost = 1;
    if (forcenet)
        ishost = 0;
    flags |= RTF_UP;
    if (ishost)
        flags |= RTF_HOST;
    if (iflag == 0)
        flags |= RTF_GATEWAY;
    for (attempts = 1;; attempts++) {
        errno = 0;
        if (Cflag && (af == AF_INET || af == AF_NS)) {
            route.rt_flags = flags;
            route.rt_dst = so_dst.sa;
            route.rt_gateway = so_gate.sa;
            if ((ret = ioctl(s, *cmd == 'a' ? SIOCADDRT : SIOCDELRT, (caddr_t) & route)) == 0)
                break;
#if 0
            } else {
            if ((ret = rtmsg(*cmd, flags)) == 0)
                break;
#endif
        }
        if (errno != ENETUNREACH && errno != ESRCH)
            break;
        if (af == AF_INET && hp && hp->h_addr_list[1]) {
            hp->h_addr_list++;
            bcopy(hp->h_addr_list[0], (caddr_t) & so_dst.sockin.sin_addr,
                  hp->h_length);
        } else
            break;
    }
    if (*cmd == 'g')
        exit(0);
    oerrno = errno;
    printf("%s %s %s: gateway %s", cmd, ishost ? "host" : "net",
           dest, gateway);
    if (attempts > 1 && ret == 0)
        printf(" (%s)",
               inet_ntoa(((struct sockaddr_in *) &route.rt_gateway)->sin_addr));
    if (ret == 0)
        printf("\n");
    else {
        switch (oerrno) {
            case ESRCH:
                err = "not in table";
                break;
            case EBUSY:
                err = "entry in use";
                break;
            case ENOBUFS:
                err = "routing table overflow";
                break;
            default:
                err = strerror(oerrno);
                break;
        }
        printf(": %s\n", err);
    }
}

void
inet_makenetandmask(u_long net, register struct sockaddr_in *sockin) {
    u_long addr, mask = 0;
    register char *cp;

    rtm_addrs |= RTA_NETMASK;
    if (net == 0)
        mask = addr = 0;
    else if (net < 128) {
        addr = net << IN_CLASSA_NSHIFT;
        mask = IN_CLASSA_NET;
    } else if (net < 65536) {
        addr = net << IN_CLASSB_NSHIFT;
        mask = IN_CLASSB_NET;
    } else if (net < 16777216L) {
        addr = net << IN_CLASSC_NSHIFT;
        mask = IN_CLASSC_NET;
    } else {
        addr = net;
        if ((addr & IN_CLASSA_HOST) == 0)
            mask = IN_CLASSA_NET;
        else if ((addr & IN_CLASSB_HOST) == 0)
            mask = IN_CLASSB_NET;
        else if ((addr & IN_CLASSC_HOST) == 0)
            mask = IN_CLASSC_NET;
        else
            mask = -1;
    }
    sockin->sin_addr.s_addr = htonl(addr);
    sockin = &so_mask.sockin;
    sockin->sin_addr.s_addr = htonl(mask);
    sockin->sin_len = 0;
    sockin->sin_family = 0;
    cp = (char *) (&sockin->sin_addr + 1);
    while (*--cp == 0 && cp > (char *) sockin);
    sockin->sin_len = 1 + cp - (char *) sockin;
}

/*
 * Interpret an argument as a network address of some kind,
 * returning 1 if a host address, 0 if a network address.
 */
int
getaddr(which, s, hpp)
        int which;
        char *s;
        struct hostent **hpp;
{
    register sup su;
#if 0
    struct ns_addr ns_addr();
    struct iso_addr *iso_addr();
#endif
    struct hostent *hp;
    struct netent *np;
    u_long val;

    if (af == 0) {
        af = AF_INET;
        aflen = sizeof(struct sockaddr_in);
    }
    rtm_addrs |= which;
    switch (which) {
        case RTA_DST:
            su = so_addrs[0];
            su->sa.sa_family = af;
            break;
        case RTA_GATEWAY:
            su = so_addrs[1];
            su->sa.sa_family = af;
            break;
        case RTA_NETMASK:
            su = so_addrs[2];
            break;
        case RTA_GENMASK:
            su = so_addrs[3];
            break;
        case RTA_IFP:
            su = so_addrs[4];
            su->sa.sa_family = af;
            break;
        case RTA_IFA:
            su = so_addrs[5];
            su->sa.sa_family = af;
            break;
        default:
            usage("Internal Error"); /*NOTREACHED*/
    }
    su->sa.sa_len = aflen;
    if (strcmp(s, "default") == 0) {
        switch (which) {
            case RTA_DST:
                forcenet++;
                getaddr(RTA_NETMASK, s, 0);
                break;
            case RTA_NETMASK:
            case RTA_GENMASK:
                su->sa.sa_len = 0;
        }
        return 0;
    }
#if 0
    if (af == AF_NS)
        goto do_xns;
    if (af == AF_OSI)
        goto do_osi;
    if (af == AF_LINK)
        goto do_link;
    if (af == AF_CCITT)
        goto do_ccitt;
    if (af == 0)
        goto do_sa;
#endif
    if (hpp == NULL)
        hpp = &hp;
    *hpp = NULL;
    if (((val = inet_addr(s)) != -1) &&
        (which != RTA_DST || forcenet == 0)) {
        su->sockin.sin_addr.s_addr = val;
        if (inet_lnaof(su->sockin.sin_addr) != INADDR_ANY)
            return (1);
        else {
            val = ntohl(val);
            out:
            if (which == RTA_DST)
                inet_makenetandmask(val, &su->sockin);
            return (0);
        }
    }
    val = inet_network(s);
    if (val != -1) {
        goto out;
    }
    np = getnetbyname(s);
    if (np) {
        val = np->n_net;
        goto out;
    }
    hp = gethostbyname(s);
    if (hp) {
        *hpp = hp;
        su->sockin.sin_family = hp->h_addrtype;
        bcopy(hp->h_addr, (char *) &su->sockin.sin_addr, hp->h_length);
        return (1);
    }
    fprintf(stderr, "%s: bad value\n", s);
    exit(1);
#if 0
    do_xns:
    if (which == RTA_DST) {
        extern short ns_bh[3];
        struct sockaddr_ns *sms = &(so_mask.sns);
        bzero((char *)sms, sizeof(*sms));
        sms->sns_family = 0;
        sms->sns_len = 6;
        sms->sns_addr.x_net = *(union ns_net *)ns_bh;
        rtm_addrs |= RTA_NETMASK;
    }
    su->sns.sns_addr = ns_addr(s);
    return (!ns_nullhost(su->sns.sns_addr));
do_osi:
    su->siso.siso_addr = *iso_addr(s);
    if (which == RTA_NETMASK || which == RTA_GENMASK) {
        register char *cp = (char *)TSEL(&su->siso);
        su->siso.siso_nlen = 0;
        do {--cp ;} while ((cp > (char *)su) && (*cp == 0));
        su->siso.siso_len = 1 + cp - (char *)su;
    }
    return (1);
do_ccitt:
    ccitt_addr(s, &su->sx25);
    return (1);
do_link:
    link_addr(s, &su->sdl);
    return (1);
do_sa:
    su->sa.sa_len = sizeof(*su);
    sockaddr(s, &su->sa);
    return (1);
#endif
}

short ns_nullh[] = {0, 0, 0};
short ns_bh[] = {-1, -1, -1};

#if 0
char *
ns_print(sns)
    struct sockaddr_ns *sns;
{
    struct ns_addr work;
    union { union ns_net net_e; u_long long_e; } net;
    u_short port;
    static char mybuf[50], cport[10], chost[25];
    char *host = "";
    register char *p;
    register u_char *q;

    work = sns->sns_addr;
    port = ntohs(work.x_port);
    work.x_port = 0;
    net.net_e  = work.x_net;
    if (ns_nullhost(work) && net.long_e == 0) {
        if (!port)
            return ("*.*");
        sprintf(mybuf, "*.%XH", port);
        return (mybuf);
    }

    if (bcmp((char *)ns_bh, (char *)work.x_host.c_host, 6) == 0)
        host = "any";
    else if (bcmp((char *)ns_nullh, (char *)work.x_host.c_host, 6) == 0)
        host = "*";
    else {
        q = work.x_host.c_host;
        sprintf(chost, "%02X%02X%02X%02X%02X%02XH",
            q[0], q[1], q[2], q[3], q[4], q[5]);
        for (p = chost; *p == '0' && p < chost + 12; p++)
            /* void */;
        host = p;
    }
    if (port)
        sprintf(cport, ".%XH", htons(port));
    else
        *cport = 0;

    sprintf(mybuf,"%XH.%s%s", ntohl(net.long_e), host, cport);
    return (mybuf);
}
#endif

void
monitor(void) {
#if 0
    int n;
    char msg[2048];

    verbose = 1;
    for(;;) {
        n = read(s, msg, 2048);
        printf("got message of size %d\n", n);
        print_rtmsg((struct rt_msghdr *)msg);
    }
#endif
}

#if 0
struct {
    struct	rt_msghdr m_rtm;
    char	m_space[512];
} m_rtmsg;

int
rtmsg(cmd, flags)
    int cmd, flags;
{
    static int seq;
    int rlen;
    register char *cp = m_rtmsg.m_space;
    register int l;

#define NEXTADDR(w, u) \
    if (rtm_addrs & (w)) {\
        l = ROUNDUP(u.sa.sa_len); bcopy((char *)&(u), cp, l); cp += l;\
        if (verbose) sodump(&(u),"u");\
    }

    errno = 0;
    bzero((char *)&m_rtmsg, sizeof(m_rtmsg));
    if (cmd == 'a')
        cmd = RTM_ADD;
    else if (cmd == 'c')
        cmd = RTM_CHANGE;
    else if (cmd == 'g')
        cmd = RTM_GET;
    else
        cmd = RTM_DELETE;
#define rtm m_rtmsg.m_rtm
    rtm.rtm_type = cmd;
    rtm.rtm_flags = flags;
    rtm.rtm_version = RTM_VERSION;
    rtm.rtm_seq = ++seq;
    rtm.rtm_addrs = rtm_addrs;
    rtm.rtm_rmx = rt_metrics;
    rtm.rtm_inits = rtm_inits;

    if (rtm_addrs & RTA_NETMASK)
        mask_addr();
    NEXTADDR(RTA_DST, so_dst);
    NEXTADDR(RTA_GATEWAY, so_gate);
    NEXTADDR(RTA_NETMASK, so_mask);
    NEXTADDR(RTA_GENMASK, so_genmask);
    NEXTADDR(RTA_IFP, so_ifp);
    NEXTADDR(RTA_IFA, so_ifa);
    rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;
    if (verbose)
        print_rtmsg(&rtm, l);
    if (debugonly)
        return 0;
    if ((rlen = write(s, (char *)&m_rtmsg, l)) < 0) {
        perror("writing to routing socket");
        return (-1);
    }
    if (cmd == RTM_GET) {
        do {
            l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
        } while (l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid));
        if (l < 0)
            fprintf(stderr,
                "route: read from routing socket: %s\n",
                strerror(errno));
        else
            print_getmsg(&rtm, l);
    }
#undef rtm
    return (0);
}
#endif

#if 0
mask_addr() {
    register char *cp1, *cp2;
    int olen;

    if ((rtm_addrs & RTA_DST) == 0)
        return;
    switch(so_dst.sa.sa_family) {
    case AF_NS: case AF_INET: case 0:
        return;
    case AF_ISO:
        olen = MIN(so_dst.siso.siso_nlen, so_mask.sa.sa_len - 6);
    }
    cp1 = so_mask.sa.sa_len + 1 + (char *)&so_dst;
    cp2 = so_dst.sa.sa_len + 1 + (char *)&so_dst;
    while (cp2 > cp1)
        *--cp2 = 0;
    cp2 = so_mask.sa.sa_len + 1 + (char *)&so_mask;
    while (cp1 > so_dst.sa.sa_data)
        *--cp1 &= *--cp2;
    switch(so_dst.sa.sa_family) {
    case AF_ISO:
        so_dst.siso.siso_nlen = olen;
    }
}

char *msgtypes[] = {
    "",
    "RTM_ADD: Add Route",
    "RTM_DELETE: Delete Route",
    "RTM_CHANGE: Change Metrics or flags",
    "RTM_GET: Report Metrics",
    "RTM_LOSING: Kernel Suspects Partitioning",
    "RTM_REDIRECT: Told to use different route",
    "RTM_MISS: Lookup failed on this address",
    "RTM_LOCK: fix specified metrics",
    "RTM_OLDADD: caused by SIOCADDRT",
    "RTM_OLDDEL: caused by SIOCDELRT",
    0,
};

char metricnames[] =
"\010rttvar\7rtt\6ssthresh\5sendpipe\4recvpipe\3expire\2hopcount\1mtu";
char routeflags[] =
"\1UP\2GATEWAY\3HOST\4REJECT\5DYNAMIC\6MODIFIED\7DONE\010MASK_PRESENT\011CLONING\012XRESOLVE\013LLINFO\017PROTO2\020PROTO1";


void
print_rtmsg(rtm, msglen)
    register struct rt_msghdr *rtm;
    int msglen;
{
    if (verbose == 0)
        return;
    if (rtm->rtm_version != RTM_VERSION) {
        printf("routing message version %d not understood\n",
            rtm->rtm_version);
        return;
    }
    printf("%s\npid: %d, len %d, seq %d, errno %d, flags:",
        msgtypes[rtm->rtm_type], rtm->rtm_pid, rtm->rtm_msglen,
        rtm->rtm_seq, rtm->rtm_errno);
    bprintf(stdout, rtm->rtm_flags, routeflags);
    pmsg_common(rtm);
}

void
print_getmsg(rtm, msglen)
    register struct rt_msghdr *rtm;
    int msglen;
{
    if (rtm->rtm_version != RTM_VERSION) {
        (void)printf("routing message version %d not understood\n",
            rtm->rtm_version);
        return;
    }
    if (rtm->rtm_msglen > msglen) {
        (void)printf("get length mismatch, in packet %d, returned %d\n",
            rtm->rtm_msglen, msglen);
    }
    printf("RTM_GET: errno %d, flags:", rtm->rtm_errno);
    bprintf(stdout, rtm->rtm_flags, routeflags);
    printf("\nmetric values:\n  ");
#define metric(f, e)\
    printf("%s: %d%s", __STRING(f), rtm->rtm_rmx.__CONCAT(rmx_,f), e)
    metric(recvpipe, ", ");
    metric(sendpipe, ", ");
    metric(ssthresh, ", ");
    metric(rtt, "\n  ");
    metric(rttvar, ", ");
    metric(hopcount, ", ");
    metric(mtu, ", ");
    metric(expire, "\n");
#undef metric
    pmsg_common(rtm);
}

void
pmsg_common(rtm)
    register struct rt_msghdr *rtm;
{
    char *cp;
    register struct sockaddr *sa;
    int i;

    printf("\nlocks: ");
    bprintf(stdout, rtm->rtm_rmx.rmx_locks, metricnames);
    printf(" inits: ");
    bprintf(stdout, rtm->rtm_inits, metricnames);
    printf("\nsockaddrs: ");
    bprintf(stdout, rtm->rtm_addrs,
        "\1DST\2GATEWAY\3NETMASK\4GENMASK\5IFP\6IFA\7AUTHOR");
    putchar('\n');
    cp = ((char *)(rtm + 1));
    if (rtm->rtm_addrs)
        for (i = 1; i; i <<= 1)
            if (i & rtm->rtm_addrs) {
                sa = (struct sockaddr *)cp;
                printf(" %s", routename(sa));
                ADVANCE(cp, sa);
            }
    putchar('\n');
    fflush(stdout);
}

void
bprintf(fp, b, s)
    register FILE *fp;
    register int b;
    register u_char *s;
{
    register int i;
    int gotsome = 0;

    if (b == 0)
        return;
    while (i = *s++) {
        if (b & (1 << (i-1))) {
            if (gotsome == 0)
                i = '<';
            else
                i = ',';
            putc(i, fp);
            gotsome = 1;
            for (; (i = *s) > 32; s++)
                putc(i, fp);
        } else
            while (*s > 32)
                s++;
    }
    if (gotsome)
        putc('>', fp);
}
#endif

int
keyword(cp)
        char *cp;
{
    register struct keytab *kt = keywords;

    while (kt->kt_cp && strcmp(kt->kt_cp, cp))
        kt++;
    return kt->kt_i;
}
#if 0
void
sodump(su, which)
    register sup su;
    char *which;
{
    switch (su->sa.sa_family) {
    case AF_LINK:
        printf("%s: link %s; ",
            which, link_ntoa(&su->sdl));
        break;
    case AF_ISO:
        printf("%s: iso %s; ",
            which, iso_ntoa(&su->siso.siso_addr));
        break;
    case AF_INET:
        printf("%s: inet %s; ",
            which, inet_ntoa(su->sockin.sin_addr));
        break;
    case AF_NS:
        printf("%s: xns %s; ",
            which, ns_ntoa(su->sns.sns_addr));
        break;
    }
    fflush(stdout);
}
/* States*/
#define VIRGIN	0
#define GOTONE	1
#define GOTTWO	2
/* Inputs */
#define	DIGIT	(4*0)
#define	END	(4*1)
#define DELIM	(4*2)

void
sockaddr(addr, sa)
register char *addr;
register struct sockaddr *sa;
{
    register char *cp = (char *)sa;
    int size = sa->sa_len;
    char *cplim = cp + size;
    register int byte = 0, state = VIRGIN, new;

    bzero(cp, size);
    do {
        if ((*addr >= '0') && (*addr <= '9')) {
            new = *addr - '0';
        } else if ((*addr >= 'a') && (*addr <= 'f')) {
            new = *addr - 'a' + 10;
        } else if ((*addr >= 'A') && (*addr <= 'F')) {
            new = *addr - 'A' + 10;
        } else if (*addr == 0)
            state |= END;
        else
            state |= DELIM;
        addr++;
        switch (state /* | INPUT */) {
        case GOTTWO | DIGIT:
            *cp++ = byte; /*FALLTHROUGH*/
        case VIRGIN | DIGIT:
            state = GOTONE; byte = new; continue;
        case GOTONE | DIGIT:
            state = GOTTWO; byte = new + (byte << 4); continue;
        default: /* | DELIM */
            state = VIRGIN; *cp++ = byte; byte = 0; continue;
        case GOTONE | END:
        case GOTTWO | END:
            *cp++ = byte; /* FALLTHROUGH */
        case VIRGIN | END:
            break;
        }
        break;
    } while (cp < cplim);
    sa->sa_len = cp - (char *)sa;
}
#endif
