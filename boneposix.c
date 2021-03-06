/* boneposix.c -- POSIX bindings for Bone Lisp.
 * Copyright (C) 2016 Wolfgang Jaehrling
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <locale.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "bone.h"

my int my_errno; // b/c we might alloc mem to store result after syscall
my void ses() { my_errno = errno; } // Safe Error Status
DEFSUB(errno) { bone_result(int2any(my_errno)); }
DEFSUB(errname) {
  switch (my_errno) {
#define x(n) case n: bone_result(intern(#n)); break
    // C99 + POSIX
    x(E2BIG);         x(EACCES);        x(EADDRINUSE);       x(EADDRNOTAVAIL);
    x(EAFNOSUPPORT);  x(EAGAIN);        x(EALREADY);         x(EBADF);
    x(EBADMSG);       x(EBUSY);         x(ECANCELED);        x(ECHILD);
    x(ECONNABORTED);  x(ECONNREFUSED);  x(ECONNRESET);       x(EDEADLK);
    x(EDESTADDRREQ);  x(EDOM);          x(EDQUOT);           x(EEXIST);
    x(EFAULT);        x(EFBIG);         x(EHOSTUNREACH);     x(EIDRM);
    x(EILSEQ);        x(EINPROGRESS);   x(EINTR);            x(EINVAL);
    x(EIO);           x(EISCONN);       x(EISDIR);           x(ELOOP);
    x(EMFILE);        x(EMLINK);        x(EMSGSIZE);         x(EMULTIHOP);
    x(ENAMETOOLONG);  x(ENETDOWN);      x(ENETRESET);        x(ENETUNREACH);
    x(ENFILE);        x(ENOBUFS);       x(ENODATA);          x(ENODEV);
    x(ENOENT);        x(ENOEXEC);       x(ENOLCK);           x(ENOLINK);
    x(ENOMEM);        x(ENOMSG);        x(ENOPROTOOPT);      x(ENOSPC);
    x(ENOSR);         x(ENOSTR);        x(ENOSYS);           x(ENOTCONN);
    x(ENOTDIR);       x(ENOTEMPTY);     x(ENOTSOCK);         x(ENOTSUP);
    x(ENOTTY);        x(ENXIO);         x(EOVERFLOW);        x(EPERM);
    x(EPIPE);         x(EPROTO);        x(EPROTONOSUPPORT);  x(EPROTOTYPE);
    x(ERANGE);        x(EROFS);         x(ESPIPE);           x(ESRCH);
    x(ESTALE);        x(ETIME);         x(ETIMEDOUT);        x(ETXTBSY);
    x(EXDEV);
#ifndef __linux__   // the following are duplicates on GNU/Linux:
    x(EOPNOTSUPP);  // same as ENOTSUPP
    x(EWOULDBLOCK); // same as EDOM
#else               // GNU/Linux specific:
    x(EBADE);         x(EBADFD);       x(EBADR);            x(EBADRQC);
    x(EBADSLT);       x(ECHRNG);       x(ECOMM);            x(EHOSTDOWN);
    x(EISNAM);        x(EKEYEXPIRED);  x(EKEYREJECTED);     x(EKEYREVOKED);
    x(EL2HLT);        x(EL2NSYNC);     x(EL3HLT);           x(EL3RST);
    x(ELIBACC);       x(ELIBBAD);      x(ELIBEXEC);         x(ELIBMAX);
    x(ELIBSCN);       x(EMEDIUMTYPE);  x(ENOKEY);           x(ENOMEDIUM);
    x(ENONET);        x(ENOPKG);       x(ENOTBLK);          x(ENOTUNIQ);
    x(EPFNOSUPPORT);  x(EREMCHG);      x(EREMOTE);          x(EREMOTEIO);
    x(ERESTART);      x(ESHUTDOWN);    x(ESOCKTNOSUPPORT);  x(ESTRPIPE);
    x(EUCLEAN);       x(EUNATCH);      x(EUSERS);           x(EXFULL);
    // x(EDEADLOCK); // same as EDEADLK
#endif
#undef x
    default:
    bone_result(BFALSE);
  }
}

DEFSUB(getpid) { bone_result(int2any(getpid())); }
DEFSUB(getuid) { bone_result(int2any(getuid())); }
DEFSUB(geteuid) { bone_result(int2any(geteuid())); }
DEFSUB(getgid) { bone_result(int2any(getgid())); }
DEFSUB(getegid) { bone_result(int2any(getegid())); }

my void getenv_any(char *name) {
  char *res = getenv(name);
  ses();
  bone_result(res ? charp2str(res) : BFALSE);
}

my void getenv_str(any x) {
  char *name = str2charp(x);
  getenv_any(name);
  free(name);
}

my void getenv_sym(any x) { getenv_any(symtext(x)); }
DEFSUB(getenv) {
  if (is_str(args[0]))
    getenv_str(args[0]);
  else
    getenv_sym(args[0]);
}

my void setenv_any(char *name, char *val, any ow) {
  bone_result(to_bool(!setenv(name, val, is(ow))));
  ses();
}

my void setenv_str(any x, char *val, any ow) {
  char *name = str2charp(x);
  setenv_any(name, val, ow);
  free(name);
}

my void setenv_sym(any x, char *val, any ow) {
  setenv_any(symtext(x), val, ow);
}

DEFSUB(setenv) {
  char *val = str2charp(args[1]);
  if (is_str(args[0]))
    setenv_str(args[0], val, args[2]);
  else
    setenv_sym(args[0], val, args[2]);
  free(val);
}

DEFSUB(chdir) {
  char *d = str2charp(args[0]);
  bone_result(to_bool(!chdir(d)));
  ses();
  free(d);
}

// FIXME: 1024
DEFSUB(getcwd) {
  char d[1024], *r = getcwd(d, 1024);
  ses();
  bone_result((r == d) ? charp2str(d) : BFALSE);
}

DEFSUB(time) {
  time_t t = time(NULL);
  ses();
  bone_result((t != -1) ? int2any(t) : BFALSE);
}

DEFSUB(ctime) {
  char buf[32]; // ctime_r(3) says it should be "at least 26"
  time_t t = any2int(args[0]);
  char *ok = ctime_r(&t, buf);
  ses();
  bone_result(ok ? charp2str(buf) : BFALSE);
}

DEFSUB(gettimeofday) {
  struct timeval tv;
  int res = gettimeofday(&tv, NULL);
  ses();
  bone_result((res != -1) ? list2(int2any(tv.tv_sec), int2any(tv.tv_usec)) : BFALSE);
}

DEFSUB(mkdir) {
  char *d = str2charp(args[0]);
  int res = mkdir(d, any2int(args[1]));
  ses();
  free(d);
  bone_result(to_bool(!res));
}

DEFSUB(rmdir) {
  char *d = str2charp(args[0]);
  int res = rmdir(d);
  ses();
  free(d);
  bone_result(to_bool(!res));
}

DEFSUB(link) {
  char *old = str2charp(args[0]);
  char *new = str2charp(args[1]);
  int res = link(old, new);
  ses();
  free(new);
  free(old);
  bone_result(to_bool(!res));
}

DEFSUB(symlink) {
  char *old = str2charp(args[0]);
  char *new = str2charp(args[1]);
  int res = symlink(old, new);
  ses();
  free(new);
  free(old);
  bone_result(to_bool(!res));
}

DEFSUB(rename) {
  char *old = str2charp(args[0]);
  char *new = str2charp(args[1]);
  int res = rename(old, new);
  ses();
  free(new);
  free(old);
  bone_result(to_bool(!res));
}

DEFSUB(unlink) {
  char *f = str2charp(args[0]);
  int res = unlink(f);
  ses();
  free(f);
  bone_result(to_bool(!res));
}

DEFSUB(chmod) {
  char *f = str2charp(args[0]);
  int res = chmod(f, any2int(args[1]));
  ses();
  free(f);
  bone_result(to_bool(!res));
}

DEFSUB(umask) { bone_result(int2any(umask(any2int(args[0])))); }

DEFSUB(dir_entries) {
  char *d = str2charp(args[0]);
  struct dirent **ents;

  int n = scandir(d, &ents, NULL, alphasort);
  ses();
  free(d);
  if (n == -1) {
    bone_result(BFALSE);
    return;
  }

  listgen lg = listgen_new();
  for (int i = 0; i < n; i++) {
    listgen_add(&lg, charp2str(ents[i]->d_name));
    free(ents[i]);
  }
  free(ents);
  bone_result(lg.xs);
}

DEFSUB(kill) {
  int res = kill(any2int(args[0]), any2int(args[1]));
  ses();
  bone_result(to_bool(!res));
}

DEFSUB(exit) { exit(any2int(args[0])); }

DEFSUB(fork) {
  int res = fork();
  ses();
  bone_result((res != -1) ? int2any(res) : BFALSE);
}

DEFSUB(waitpid) { // FIXME: flags as syms
  int status, res = waitpid(any2int(args[0]), &status, any2int(args[1]));
  ses();
  bone_result((res != -1) ? list2(int2any(res), int2any(status)) : BFALSE);
}

// With these you can analyze the status returned by waitpid:
DEFSUB(w_exitstatus) {
  int x = any2int(args[0]);
  bone_result(WIFEXITED(x) ? int2any(WEXITSTATUS(x)) : BFALSE);
}
DEFSUB(w_termsig) {
  int x = any2int(args[0]);
  bone_result(WIFSIGNALED(x) ? int2any(WTERMSIG(x)) : BFALSE);
}
DEFSUB(w_stopsig) {
  int x = any2int(args[0]);
  bone_result(WIFSTOPPED(x) ? int2any(WSTOPSIG(x)) : BFALSE);
}
DEFSUB(w_continued) { bone_result(to_bool(WIFCONTINUED(any2int(args[0])))); }

DEFSUB(random) { bone_result(int2any(random() % any2int(args[0]))); }

DEFSUB(src_open) {
  char *fname = str2charp(args[0]);
  FILE *fp = fopen(fname, "r");
  ses();
  free(fname);
  bone_result(fp ? fp2src(fp, args[0]) : BFALSE);
}

DEFSUB(src_close) {
  bool res = (fclose(src2fp(args[0])) == 0);
  ses();
  bone_result(to_bool(res));
}

DEFSUB(dst_open) {
  char *fname = str2charp(args[0]);
  FILE *fp = fopen(fname, "w");
  ses();
  free(fname);
  bone_result(fp ? fp2dst(fp, args[0]) : BFALSE);
}

DEFSUB(dst_close) {
  bool res = (fclose(dst2fp(args[0])) == 0);
  ses();
  bone_result(to_bool(res));
}

DEFSUB(execvp) {
  char *prog = str2charp(args[0]);
  check(args[1], t_cons); // avoid invalid syscall
  int64_t argc = len(args[1]);
  char **argv = malloc((argc+1) * sizeof(char *));
  int64_t i = 0;
  foreach(arg, args[1])
    argv[i++] = str2charp(arg);
  argv[i] = NULL;
  execvp(prog, argv);
  ses();
  free(prog);
  while(i--)
    free(argv[i]);
  free(argv);
  bone_result(BFALSE);
}

DEFSUB(strerror) {
  locale_t loc = newlocale(LC_MESSAGES_MASK, "", NULL);
  char *msg = strerror_l(any2int(args[0]), loc);
  freelocale(loc);
  any res = charp2str(msg);
  bone_result(res);
}

void bone_posix_init() {
  bone_register_csub(CSUB_errno, "sys.errno", 0, 0);
  bone_register_csub(CSUB_errname, "sys.errname?", 0, 0);
  bone_register_csub(CSUB_getpid, "sys.getpid", 0, 0);
  bone_register_csub(CSUB_getuid, "sys.getuid", 0, 0);
  bone_register_csub(CSUB_geteuid, "sys.geteuid", 0, 0);
  bone_register_csub(CSUB_getgid, "sys.getgid", 0, 0);
  bone_register_csub(CSUB_getegid, "sys.getegid", 0, 0);
  bone_register_csub(CSUB_getenv, "sys.getenv?", 1, 0);
  bone_register_csub(CSUB_setenv, "sys.setenv?", 3, 0); // FIXME: last arg optional
  bone_register_csub(CSUB_chdir, "sys.chdir?", 1, 0);
  bone_register_csub(CSUB_getcwd, "sys.getcwd?", 0, 0);
  bone_register_csub(CSUB_time, "sys.time?", 0, 0);
  bone_register_csub(CSUB_gettimeofday, "sys.gettimeofday?", 0, 0);
  bone_register_csub(CSUB_mkdir, "sys.mkdir?", 2, 0);
  bone_register_csub(CSUB_rmdir, "sys.rmdir?", 1, 0);
  bone_register_csub(CSUB_link, "sys.link?", 2, 0);
  bone_register_csub(CSUB_symlink, "sys.symlink?", 2, 0);
  bone_register_csub(CSUB_rename, "sys.rename?", 2, 0);
  bone_register_csub(CSUB_unlink, "sys.unlink?", 1, 0);
  bone_register_csub(CSUB_chmod, "sys.chmod?", 2, 0);
  bone_register_csub(CSUB_umask, "sys.umask", 1, 0);
  bone_register_csub(CSUB_dir_entries, "sys.dir-entries?", 1, 0);
  bone_register_csub(CSUB_kill, "sys.kill?", 2, 0);
  bone_register_csub(CSUB_exit, "sys.exit", 1, 0);
  bone_register_csub(CSUB_fork, "sys.fork?", 0, 0);
  bone_register_csub(CSUB_waitpid, "sys.waitpid?", 2, 0);
  bone_register_csub(CSUB_w_exitstatus, "sys.exitstatus?", 1, 0);
  bone_register_csub(CSUB_w_termsig, "sys.termsig?", 1, 0);
  bone_register_csub(CSUB_w_stopsig, "sys.stopsig?", 1, 0);
  bone_register_csub(CSUB_w_continued, "sys.continued?", 1, 0);
  bone_register_csub(CSUB_random, "sys.random", 1, 0);
  bone_register_csub(CSUB_src_open, "sys.src-open?", 1, 0);
  bone_register_csub(CSUB_src_close, "sys.src-close?", 1, 0);
  bone_register_csub(CSUB_dst_open, "sys.dst-open?", 1, 0);
  bone_register_csub(CSUB_dst_close, "sys.dst-close?", 1, 0);
  bone_register_csub(CSUB_ctime, "sys.ctime?", 1, 0);
  bone_register_csub(CSUB_execvp, "sys.execvp?", 2, 0);
  bone_register_csub(CSUB_strerror, "sys.strerror", 1, 0);

  srandom(time(NULL));
  bone_info_entry("posix", 0);
}
