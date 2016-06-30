/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "MQTTLinux.h"

char expired(Timer* timer)
{
  struct timeval now, res;
  gettimeofday(&now, NULL);
  timersub(&timer->end_time, &now, &res);		
  return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}


void countdown_ms(Timer* timer, unsigned int timeout)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
  timeradd(&now, &interval, &timer->end_time);
}


void countdown(Timer* timer, unsigned int timeout)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  struct timeval interval = {timeout, 0};
  timeradd(&now, &interval, &timer->end_time);
}


int left_ms(Timer* timer)
{
  struct timeval now, res;
  gettimeofday(&now, NULL);
  timersub(&timer->end_time, &now, &res);
  //TRACE_LOG("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
  return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}


void InitTimer(Timer* timer)
{
  timer->end_time = (struct timeval){0, 0};
}


int linux_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
  struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
  if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
  {
    interval.tv_sec = 0;
    interval.tv_usec = 100;
  }
  setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

  Timer timer;
  InitTimer(&timer);    
  if (timeout_ms < 100) { //give chance to call write()
    timeout_ms = 100;
  }
  countdown_ms(&timer, timeout_ms);

  int bytes = 0;
  TRACE_LOG("linux_read to=%dms len=%d\n", timeout_ms, len);
  while (!expired(&timer) && bytes < len) {
    TRACE_LOG("SSL_read> len=%d\n", len - bytes);
    int rc = SSL_read(n->my_ssl, &buffer[bytes], (size_t)(len - bytes));
    TRACE_LOG("SSL_read< rc=%d\n", rc);
    if (rc < 0) {
      if (SSL_get_error(n->my_ssl, rc) != SSL_ERROR_WANT_READ && SSL_get_error(n->my_ssl, rc) != SSL_ERROR_WANT_WRITE)
      {
        TRACE_LOG("linux_read rc=%d, error=%d\n", rc, SSL_get_error(n->my_ssl, rc));
        bytes = -1;
        break;
      } else {
        TRACE_LOG("linux_read non-blocking rc=%d, error=%d\n", rc, SSL_get_error(n->my_ssl, rc));
      }
    } else {
      bytes += rc;
    }
  }
  return bytes;
}


int linux_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
  struct timeval tv;

  tv.tv_sec = 0;  /* 30 Secs Timeout */
  tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

  Timer timer;
  InitTimer(&timer);    
  if (timeout_ms < 100) { //give chance to call write()
    timeout_ms = 100;
  }
  countdown_ms(&timer, timeout_ms);

  setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

  int bytes = 0;
  TRACE_LOG("linux_write to=%dms len=%d\n", timeout_ms, len);
  while (!expired(&timer) && bytes < len)
  {
    int	rc = SSL_write(n->my_ssl, buffer, len);
    TRACE_LOG("linux write SSL_write rc=%d\n", rc);
    if (rc < 0) {
      if (SSL_get_error(n->my_ssl, rc) != SSL_ERROR_WANT_READ && SSL_get_error(n->my_ssl, rc) != SSL_ERROR_WANT_WRITE)
      {
        ERROR_LOG("linux_write rc=%d, error=%d\n", rc, SSL_get_error(n->my_ssl, rc));
        bytes = -1;
        break;
      } else {
        TRACE_LOG("linux_write non-blocking rc=%d, error=%d\n", rc, SSL_get_error(n->my_ssl, rc));
      }
    } else {
      bytes += rc;
    }
  }
  return bytes;
}


void linux_disconnect(Network* n)
{
  close(n->my_socket);
  if (NULL != n->my_ssl) {
    SSL_free(n->my_ssl);
    n->my_ssl = NULL;
  } 
  if (NULL != n->my_ctx) {
    SSL_CTX_free(n->my_ctx);
    n->my_ctx = NULL;
  } 
}


void NewNetwork(Network* n)
{
  n->my_socket = 0;
  n->my_ssl = NULL;
  n->my_ctx = NULL;
  n->mqttread = linux_read;
  n->mqttwrite = linux_write;
  n->disconnect = linux_disconnect;
}


int ConnectNetwork(Network* n, char* addr, int port)
{
  int type = SOCK_STREAM;
  struct sockaddr_in address;
  int rc = -1;
  sa_family_t family = AF_INET;
  struct addrinfo *result = NULL;
  struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

  if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0)
  {
    struct addrinfo* res = result;

    /* prefer ip4 addresses */
    while (res)
    {
      if (res->ai_family == AF_INET)
      {
        result = res;
        break;
      }
      res = res->ai_next;
    }

    if (result->ai_family == AF_INET)
    {
      address.sin_port = htons(port);
      address.sin_family = family = AF_INET;
      address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
    }
    else
      rc = -1;

    freeaddrinfo(result);
  }

  if (rc == 0)
  {
    n->my_socket = socket(family, type, 0);
    if (n->my_socket != -1)
    {
      int opt = 1;			
      rc = connect(n->my_socket, (struct sockaddr*)&address, sizeof(address));
    }
  }

  return rc;
}

SSL_CTX* TlsClientNew(void) {
  SSL_CTX *ctx;

  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ctx = SSL_CTX_new(TLSv1_client_method());
  if (ctx == NULL) {
    ERR_print_errors_fp(stderr);
    assert(0);
  }
  return ctx;
}

int ConnectTlsNetwork(Network* n, char* addr, int port, char* cafile, char* hostname)
{

  SSL_CTX *ctx;
  int server;
  SSL *ssl;
  int rc = -1;
  char buf[1024];
  int bytes;

  SSL_library_init();

  n->my_ctx = ctx = TlsClientNew();
  if (SSL_CTX_load_verify_locations(ctx, cafile, NULL) != 1) {
    ERR_print_errors_fp(stderr);
    return -1;
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

  rc = ConnectNetwork(n, addr, port);
  ssl = SSL_new(ctx);
  n->my_ssl = ssl;
  SSL_set_fd(ssl, n->my_socket);
  if (hostname) {
    SSL_set_tlsext_host_name(ssl, hostname);
  }
  rc = SSL_connect(ssl);
  if (rc < 0) {
    ERR_print_errors_fp(stderr);
    ERROR_LOG("SSL_connect error=%d\n", rc);
    linux_disconnect(n);
  }
  return rc;
}
