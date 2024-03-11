# spoofproxy

As far as I can tell, it's not possible to expose real client IP addresses to
rootless Docker services. This program attempts to solve that.

Usage:

in docker-compose.yml:

    environment:
      LD_PRELOAD=/spoofproxy.so
    volumes:
      ./spoofproxy.so:/spoofproxy.so

Then, set up an nginx stream reverse proxy using the PROXY protocol which points
to your container.
