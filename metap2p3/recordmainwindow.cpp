﻿//
// Copyright (c) 2019-2022 yanggaofeng
//
#include "recordmainwindow.h"
#include "ui_recordmainwindow.h"
#include <yangpush/YangPushCommon.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangFile.h>
#include <yangutil/sys/YangSocket.h>
#include <yangutil/sys/YangIni.h>
#include <yangutil/sys/YangString.h>
#include <yangpush/YangPushFactory.h>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QMessageBox>

#include <QSettings>

RecordMainWindow::RecordMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::RecordMainWindow)
{

    ui->setupUi(this);

    m_context=new YangContext();
    m_context->init((char*)"yang_config.ini");
    m_context->avinfo.video.videoEncoderFormat=YangI420;
    m_context->avinfo.enc.createMeta=1;
#if Yang_GPU_Encoding
    //using gpu encode
    m_context->avinfo.video.videoEncHwType=YangV_Hw_Nvdia;//YangV_Hw_Intel,  YangV_Hw_Nvdia,
    m_context->avinfo.video.videoEncoderFormat=YangArgb;//YangI420;//YangArgb;
    m_context->avinfo.enc.createMeta=0;
#endif
#if Yang_HaveVr
    //using vr bg file name
    memset(m_context->avinfo.bgFilename,0,sizeof(m_ini->bgFilename));
    QSettings settingsread((char*)"yang_config.ini",QSettings::IniFormat);
    strcpy(m_context->avinfo.bgFilename, settingsread.value("sys/bgFilename",QVariant("d:\\bg.jpeg")).toString().toStdString().c_str());
#endif

     init();
    yang_setLogLevle(m_context->avinfo.sys.logLevel);
    yang_setLogFile(m_context->avinfo.sys.hasLogFile);

    m_context->avinfo.sys.mediaServer=Yang_Server_Srs;//Yang_Server_Srs/Yang_Server_Zlm

    m_hb0=new QHBoxLayout();
    ui->vdMain->setLayout(m_hb0);
    m_win0=new YangPlayWidget(this);
   // m_win1=new YangPlayWidget(this);
    m_hb0->addWidget(m_win0);
   // m_hb0->addWidget(m_win1);
    m_hb0->setMargin(0);
    m_hb0->setSpacing(0);
    char localip[64]={0};
    char s[128]={0};
    yang_getLocalInfo(localip);
    sprintf(s,"webrtc://%s/live/livestream",localip);
    ui->m_url->setText(s);

    m_hasAudio=true;

  //  m_isStartplay=false;
    m_isStartpushplay=false;


    //using h264 h265
    m_context->avinfo.video.videoEncoderType=Yang_VED_264;//Yang_VED_265;
    read_ip_address();
}

RecordMainWindow::~RecordMainWindow()
{
    closeAll();
}

void RecordMainWindow::closeEvent( QCloseEvent * event ){

    closeAll();
    exit(0);
}

void RecordMainWindow::read_ip_address()
{
    QString ip_address;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    for (int i = 0; i < ipAddressesList.size(); ++i)
    {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&  ipAddressesList.at(i).toIPv4Address())
        {
            ip_address = ipAddressesList.at(i).toString();
            if(strlen(m_context->avinfo.sys.localIp)==0){
                memset(m_context->avinfo.sys.localIp,0,sizeof(m_context->avinfo.sys.localIp));
                strcpy(m_context->avinfo.sys.localIp,ip_address.toLatin1().data());
                yang_trace("\nlocal ip====%s",m_context->avinfo.sys.localIp);
                qDebug()<<ip_address;  //debug
            }
            //break;
        }
    }
    if (ip_address.isEmpty())
        ip_address = QHostAddress(QHostAddress::LocalHost).toString();
    //return ip_address;
}
void RecordMainWindow::receiveSysMessage(YangSysMessage *mss, int32_t err){
    switch (mss->messageId) {
    case YangM_Push_Connect:
        {
            if(err){
                ui->m_b_pushplay->setText("推拉流");
                m_isStartpushplay=!m_isStartpushplay;
                QMessageBox::about(NULL,  "error","push error("+QString::number(err)+")!");
            }
        }
        break;
   case YangM_Push_Disconnect:
        break;
   case YangM_Push_StartVideoCapture:
    {
        m_rt->m_videoBuffer=m_p2pfactory.getPreVideoBuffer(m_message);
        qDebug()<<"message==="<<m_message<<"..prevideobuffer==="<<m_rt->m_videoBuffer<<"....ret===="<<err;
         break;
     }

    }


}
void RecordMainWindow::initVideoThread(YangRecordThread *prt){
    m_rt=prt;
    m_rt->m_local_video=m_win0;
    m_rt->initPara(m_context);
    m_rt->m_syn= m_context->streams.m_playBuffer;

}
void RecordMainWindow::closeAll(){

    if(m_context){
        m_rt->stopAll();
        m_rt=NULL;
        yang_delete(m_message);
        yang_delete(m_context);
        delete ui;
    }
}

void RecordMainWindow::initPreview(){

        yang_post_message(YangM_Push_StartVideoCapture,0,NULL);
        ui->m_b_pushplay->setEnabled(false);

}


void RecordMainWindow::init() {
    m_context->avinfo.audio.usingMono=0;
    m_context->avinfo.audio.sample=48000;
    m_context->avinfo.audio.channel=2;
    m_context->avinfo.audio.hasAec=1;
    m_context->avinfo.audio.audioCacheNum=8;
    m_context->avinfo.audio.audioCacheSize=8;
    m_context->avinfo.audio.audioPlayCacheNum=8;

    m_context->avinfo.video.videoCacheNum=10;
    m_context->avinfo.video.evideoCacheNum=10;
    m_context->avinfo.video.videoPlayCacheNum=10;

    m_context->avinfo.audio.audioEncoderType=Yang_AED_OPUS;
    m_context->avinfo.sys.rtcLocalPort=17000;
    m_context->avinfo.enc.enc_threads=4;
}

void RecordMainWindow::success(){

}
void RecordMainWindow::failure(int32_t errcode){
    //on_m_b_play_clicked();
    QMessageBox::aboutQt(NULL,  "push error("+QString::number(errcode)+")!");

}


void RecordMainWindow::on_m_b_pushplay_clicked()
{

    ui->m_b_pushplay->setEnabled(false);
    if(!m_isStartpushplay){

        ui->m_b_pushplay->setText("停止");
        m_isStartpushplay=!m_isStartpushplay;
        qDebug()<<"url========="<<ui->m_url->text().toLatin1().data();
        m_url=ui->m_url->text().toLatin1().data();
        yang_post_message(YangM_Push_Connect,0,NULL,(void*)m_url.c_str());
    }else{
        ui->m_b_pushplay->setText("推拉流");
       yang_post_message(YangM_Push_Disconnect,0,NULL);
              m_isStartpushplay=!m_isStartpushplay;
    }
}

//void RecordMainWindow::on_m_b_play_clicked()
//{
//    if(!m_isStartplay){
//         m_rt->m_syn=m_context->streams.m_playBuffer;
//         m_rt->m_syn->resetVideoClock();
//           if(m_player&&m_player->play(ui->m_url->text().toStdString(),16000)==Yang_Ok){
//               ui->m_b_play->setText("stop");

//               m_isStartplay=!m_isStartplay;
//              // m_videoThread->m_isRender=true;
//           }else{
//                QMessageBox::about(NULL, "Error", "play url error!");
//                // m_videoThread->m_isRender=false;
//           }

//    }else{
//       // m_rt->m_isRender=false;
//        ui->m_b_play->setText("play");

//       QThread::msleep(50);
//       if(m_player) m_player->stopPlay();

//        m_isStartplay=!m_isStartplay;

//    }
//}