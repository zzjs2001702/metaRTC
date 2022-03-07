﻿//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangzlm/YangZlmConnection.h>
#include <yangutil/yangtype.h>
#include <yangutil/sys/YangLog.h>
#include <yangwebrtc/YangRtcSession.h>
#include <yangwebrtc/YangRtcStun.h>
#include <yangsdp/YangSdp.h>
#include <yangzlm/YangZlmSdp.h>

int32_t yang_zlm_doHandleSignal(YangRtcSession* session,ZlmSdpResponseType* zlm,int32_t localport, YangStreamOptType role);

extern int32_t g_localport;//=10000;

int32_t yang_zlm_connectRtcServer(YangRtcSession* session){

	g_localport=session->context.streamConf->localPort;
	int err=Yang_Ok;
	ZlmSdpResponseType zlm;
	memset(&zlm,0,sizeof(ZlmSdpResponseType));
	g_localport+=yang_random()%15000;

	if ((err=yang_zlm_doHandleSignal(session,&zlm,g_localport,session->context.streamConf->streamOptType))  == Yang_Ok) {
		yang_stun_createStunPacket(session,zlm.id,session->remoteIcePwd);
		yang_rtcsession_startudp(session,g_localport);
		g_localport++;
	}

	yang_destroy_zlmresponse(&zlm);
    return err;
}

int32_t yang_zlm_doHandleSignal(YangRtcSession* session,ZlmSdpResponseType* zlm,int32_t localport, YangStreamOptType role) {
	int32_t err = Yang_Ok;

	char *tsdp=NULL;
	yang_sdp_genLocalSdp(session,localport, &tsdp,role);

	char apiurl[256] ;
	memset(apiurl,0,sizeof(apiurl));

	sprintf(apiurl, "index/api/webrtc?app=%s&stream=%s&type=%s", session->context.streamConf->app,session->context.streamConf->stream,role==Yang_Stream_Play?"play":"push");
	err=yang_zlm_query(session,zlm,role==Yang_Stream_Play?1:0,(char*)session->context.streamConf->serverIp,80,apiurl, tsdp);

	yang_free(tsdp);

	return err;
}
