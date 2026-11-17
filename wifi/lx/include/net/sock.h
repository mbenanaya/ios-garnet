/*
 * net/sock.h for ath10k's qmi.c, which includes it for one call: kernel_connect() on its QMI handle's socket, with the
 * address cast to Linux's "struct sockaddr_unsized". The call is the kernel's (bsd/sys/hakios.qmi.h), which takes the
 * address as it comes; the type only has to have a name.
 */
#ifndef LX_NET_SOCK_H
#define LX_NET_SOCK_H
struct sockaddr_unsized;
#endif
