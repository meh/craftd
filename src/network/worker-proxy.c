/*
 * Copyright (c) 2010-2011 Kevin M. Bowling, <kevin.bowling@kev009.com>, USA
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <pthread.h>

#include "network/network.h"
#include "network/network-private.h"
#include <util.h>

int workerproxy(uint8_t pkttype, size_t pktlen, struct WQ_entry *workitem)
{
      if(workitem->worktype == WQ_GAME_INPUT)
      {
	  LOG(LOG_DEBUG,"Recieved packet type: %d from client",pkttype);
	  if(process_isproxypassthrough(pkttype) && workitem->player->sev)
	  {
	    struct evbuffer *tmpbuf = evbuffer_new();
	    evbuffer_remove_buffer(bufferevent_get_input(workitem->bev),
				   tmpbuf,pktlen);
	    newOutputWq(tmpbuf,workitem->player,workitem->player->sev,
			&workitem->player->sevoutlock);
	    
	  }
          else
	  {
	    void *packet = packetdecoder(pkttype, pktlen, workitem->bev); 
	    struct WQ_process_data *pdata;
	    pdata = Malloc(sizeof(struct WQ_process_data));
	    pdata->packet = packet;
	    pdata->pktlen = pktlen;
	    pdata->pkttype = pkttype;
	    newProcessWq(workitem->player,workitem->bev,pdata);
	  }
      }
      else if(workitem->worktype == WQ_PROXY_INPUT)
      {
	LOG(LOG_DEBUG,"Recieved packet %d from server",pkttype);
	if(process_isproxyserverpassthrough(pkttype))
	{
	    struct evbuffer *tmpbuf = evbuffer_new();
	    evbuffer_remove_buffer(bufferevent_get_input(workitem->bev),
				   tmpbuf,pktlen);
	    newOutputWq(tmpbuf,workitem->player,workitem->player->bev,
		&workitem->player->outlock);
	}
	else
	{
	    void *packet = packetdecoder(pkttype, pktlen, workitem->bev);
	    struct WQ_process_data *pdata;
	    pdata = Malloc(sizeof(struct WQ_process_data));
	    pdata->packet = packet;
	    pdata->pktlen = pktlen;
	    pdata->pkttype = pkttype;
	    newProcessWq(workitem->player,workitem->bev,pdata);
	}
      }
      else if(workitem->worktype == WQ_PROCESS)
      {
	if(workitem->bev == workitem->player->bev) //Process a client packet
	{
	  struct WQ_process_data *pdata = workitem->workdata;
	  process_proxypacket(workitem->player, pdata->pkttype, pdata->packet);
	  packetfree(pdata->pkttype,pdata->packet);
	  free(pdata);
	}
	if(workitem->bev == workitem->player->sev) //Process a server packet
	{
	  struct WQ_process_data *pdata = workitem->workdata;
	  process_proxyserverpacket(workitem->player, pdata->pkttype, pdata->packet);
	  packetfree(pdata->pkttype,pdata->packet);
	  free(pdata);
	}
      }
      else {
        return -1;
      }
     return 1;
}
