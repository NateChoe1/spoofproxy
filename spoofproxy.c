#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/resource.h>

#define UNUSED(var) (void) var

static int accept4_fake(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
static int get_real_address(int fd, struct sockaddr *addr, socklen_t *addrlen);
static int fdgetline(int fd, char *line, size_t bufflen);

int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
	static int (*real_accept4)(int, struct sockaddr *, socklen_t *, int flags) = NULL;
	int fd;

	if (real_accept4 == NULL) {
		void *sym;
		if ((sym = dlsym(RTLD_NEXT, "accept4")) == NULL) {
			real_accept4 = accept4_fake;
		}
		else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
			real_accept4 = sym;
#pragma GCC diagnostic pop
		}
	}
	if ((fd = real_accept4(sockfd, addr, addrlen, flags)) < 0) {
		return fd;
	}
	if (get_real_address(fd, addr, addrlen) != 0) {
		close(fd);
		errno = EPROTO;
		return -1;
	}

	return fd;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
	return accept4(sockfd, addr, addrlen, 0);
}

static int accept4_fake(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
	static int (*real_accept)(int, struct sockaddr *, socklen_t *) = NULL;

	UNUSED(flags);

	if (real_accept == NULL) {
		void *sym;
		if ((sym = dlsym(RTLD_NEXT, "accept")) == NULL) {
			fprintf(stderr, "spoofproxy: dlsym() failed: %s\n", dlerror());
			errno = ESOCKTNOSUPPORT;
			return -1;
		}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
		real_accept = sym;
#pragma GCC diagnostic pop
	}
	return real_accept(sockfd, addr, addrlen);
}

static int get_real_address(int fd, struct sockaddr *addr, socklen_t *addrlen) {
	int oldflags;
	char line[256];
	char *lineptr;
	char *scratch;
	int af;
	struct pollfd pfd;
	void *addrloc;

	/* 100ms timeout */
	pfd.fd = fd;
	pfd.events = POLLIN;
	poll(&pfd, 1, 100);

	oldflags = fcntl(fd, F_GETFL);
	if (!(oldflags & O_NONBLOCK) &&
	    fcntl(fd, F_SETFL, oldflags | O_NONBLOCK) == -1) {
		return -1;
	}

	/* get the first line */
	if (fdgetline(fd, line, sizeof(line)) != 0) {
		return -1;
	}

	lineptr = line;

	/* Make sure we're using the PROXY protocol */
	if (memcmp("PROXY ", lineptr, sizeof("PROXY ") - 1) != 0) {
		return -1;
	}
	lineptr += sizeof("PROXY ") - 1;

	/* Check IP version, or fail if invalid */
	if (memcmp("TCP4 ", lineptr, sizeof("TCP4 ") - 1) == 0) {
		if (*addrlen < sizeof(struct sockaddr_in)) {
			return -1;
		}
		*addrlen = sizeof(struct sockaddr_in);
		addrloc = &((struct sockaddr_in *) addr)->sin_addr;
		af = AF_INET;
	}
	else if (memcmp("TCP6 ", lineptr, sizeof("TCP6") - 1) == 0) {
		if (*addrlen < sizeof(struct sockaddr_in6)) {
			return -1;
		}
		*addrlen = sizeof(struct sockaddr_in6);
		addrloc = &((struct sockaddr_in6 *) addr)->sin6_addr;
		af = AF_INET6;
	}
	else {
		return -1;
	}
	if (addr->sa_family != af) {
		return -1;
	}
	lineptr += sizeof("TCPx ") - 1;

	/* get the visitor's ip */
	if ((scratch = strchr(lineptr, ' ')) == NULL) {
		return -1;
	}
	scratch[0] = '\0';
	if (inet_pton(af, lineptr, addrloc) != 1) {
		return -1;
	}

	if (fcntl(fd, F_SETFL, oldflags) == -1) {
		return -1;
	}
	return 0;
}

static int fdgetline(int fd, char *line, size_t bufflen) {
	size_t i;
	for (i = 0; i < bufflen; ++i) {
		ssize_t readlen;

		/* error at premature EOF */
		if ((readlen = read(fd, line + i, 1)) < 1) {
			if (readlen == 0) {
				return -1;
			}
			if (errno == EINTR) {
				--i;
				continue;
			}
			return -1;
		}

		/* end at CRLF */
		if (i != 0 && line[i-1] == '\r') {
			if (line[i] == '\n') {
				line[i-1] = '\0';
				return 0;
			}
			return -1;
		}

		/* end at LF without CR */
		if (line[i] == '\n') {
			return -1;
		}
	}
	return -1;
}
