//#define _CRT_SECURE_NO_WARNINGS
//#include <stdio.h>
//#include <iostream>
//#include <conio.h>
//#include "Windows.h"
//#include "HCNetSDK.h"
//#define SDL_MAIN_HANDLED
//#include <SDL.h>
//#include <SDL_syswm.h>
//using namespace std;
//
//int iNum = 0; //已保存的图片个数
//
//void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser)
//{
//    //每次收到报警先打一行头部，便于区分是一次新的上报还是同一事件的多条结构
//    printf("\n==================== 收到报警 ====================\n");
//    if (pAlarmer != NULL)
//    {
//        printf("命令号[0x%x], 设备用户ID[%d], 设备IP[%s], 设备名[%s], 报警socket端口[%d], 数据长度[%d]\n",
//            lCommand, pAlarmer->lUserID, pAlarmer->sDeviceIP, pAlarmer->sDeviceName, pAlarmer->wSocketPort, dwBufLen);
//    }
//
//    switch (lCommand)
//    {
//    case COMM_ALARM_ACS: //门禁主机报警信息
//    {
//        NET_DVR_ACS_ALARM_INFO struAcsAlarmInfo = { 0 };
//        memcpy(&struAcsAlarmInfo, pAlarmInfo, sizeof(NET_DVR_ACS_ALARM_INFO));
//        NET_DVR_ACS_EVENT_INFO* pEventInfo = &struAcsAlarmInfo.struAcsEventInfo;
//
//        printf("门禁主机报警信息[0x5002]: 时间[%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d], 主类型[0x%x], 次类型[0x%x], 感应事件类型[%d], 事件流水号[%d]\n",
//            struAcsAlarmInfo.struTime.dwYear, struAcsAlarmInfo.struTime.dwMonth, struAcsAlarmInfo.struTime.dwDay,
//            struAcsAlarmInfo.struTime.dwHour, struAcsAlarmInfo.struTime.dwMinute, struAcsAlarmInfo.struTime.dwSecond,
//            struAcsAlarmInfo.dwMajor, struAcsAlarmInfo.dwMinor, struAcsAlarmInfo.wInductiveEventType,
//            pEventInfo->dwSerialNo);
//
//        //详细事件信息(NET_DVR_ACS_EVENT_INFO)：卡号、门禁、读卡器等基础字段
//        printf("  卡/门禁信息: 卡号[%s], 卡类型[%d], 允许名单号[%d], 上传通道[%d], 读卡器类型[%d], 读卡器号[%d], 门号[%d]\n",
//            pEventInfo->byCardNo, pEventInfo->byCardType, pEventInfo->byAllowListNo, pEventInfo->byReportChannel,
//            pEventInfo->byCardReaderKind, pEventInfo->dwCardReaderNo, pEventInfo->dwDoorNo);
//        printf("  卡/门禁信息: 工号[%d], 核验号[%d], 报警输入号[%d], 报警输出号[%d], 事件传感器号[%d], RS485通道号[%d], 群组号[%d]\n",
//            pEventInfo->dwEmployeeNo, pEventInfo->dwVerifyNo, pEventInfo->dwAlarmInNo, pEventInfo->dwAlarmOutNo,
//            pEventInfo->dwCaseSensorNo, pEventInfo->dwRs485No, pEventInfo->dwMultiCardGroupNo);
//        printf("  卡/门禁信息: 人员通道号[%d], 设备编号[%d], 分控器号[%d], 本地控制器编号[%d], 网络ID[%d], 触发类型[%d], MAC[%02x:%02x:%02x:%02x:%02x:%02x], 刷卡类型[%d]\n",
//            pEventInfo->wAccessChannel, pEventInfo->byDeviceNo, pEventInfo->byDistractControlNo, pEventInfo->wLocalControllerID,
//            pEventInfo->byInternetAccess, pEventInfo->byType, pEventInfo->byMACAddr[0], pEventInfo->byMACAddr[1],
//            pEventInfo->byMACAddr[2], pEventInfo->byMACAddr[3], pEventInfo->byMACAddr[4], pEventInfo->byMACAddr[5],
//            pEventInfo->bySwipeCardType);
//        //人脸/防护相关：0-未识别，1-未知，2-有/戴，3-无/未戴
//        printf("  [人脸相关] 是否戴口罩[%d], 是否戴安全帽[%d]\n", pEventInfo->byMask, pEventInfo->byHelmet);
//
//        //扩展信息，包含以人为中心下发人员触发事件中的工号参数
//        if (struAcsAlarmInfo.byAcsEventInfoExtend == 1)
//        {
//            NET_DVR_ACS_EVENT_INFO_EXTEND  struAcsEventInfoExtend = { 0 };
//            memset(&struAcsEventInfoExtend, 0, sizeof(struAcsEventInfoExtend));
//            memcpy(&struAcsEventInfoExtend, struAcsAlarmInfo.pAcsEventInfoExtend, sizeof(struAcsEventInfoExtend));
//
//            printf("  扩展事件信息: 前流水号[%d], 用户类型[%d], 当前认证方式[%d], 是否实时事件[%d], 纯密码核验[%d], 工号[%s], 考勤状态[%d], 状态值[%d]\n",
//                struAcsEventInfoExtend.dwFrontSerialNo, struAcsEventInfoExtend.byUserType, struAcsEventInfoExtend.byCurrentVerifyMode,
//                struAcsEventInfoExtend.byCurrentEvent, struAcsEventInfoExtend.byPurePwdVerifyEnable,
//                (char*)struAcsEventInfoExtend.byEmployeeNo, struAcsEventInfoExtend.byAttendanceStatus, struAcsEventInfoExtend.byStatusValue);
//            printf("  扩展事件信息: UUID[%s], 设备序列号[%s]\n",
//                (char*)struAcsEventInfoExtend.byUUID, (char*)struAcsEventInfoExtend.byDeviceName);
//        }
//
//        //扩展信息，包含人体测温温度数据
//        if (struAcsAlarmInfo.byAcsEventInfoExtendV20 == 1)
//        {
//            NET_DVR_ACS_EVENT_INFO_EXTEND_V20 struAcsEventInfoExtendV20 = { 0 };
//            memcpy(&struAcsEventInfoExtendV20, struAcsAlarmInfo.pAcsEventInfoExtendV20, sizeof(struAcsEventInfoExtendV20));
//
//            printf("  扩展事件信息V20: 远程核验[%d], 测温单位[%d], 温度异常[%d], 当前温度[%f], 测温坐标[%f,%f], 二维码长度[%d], 可见光图长度[%d], 热成像图长度[%d]\n",
//                struAcsEventInfoExtendV20.byRemoteCheck, struAcsEventInfoExtendV20.byThermometryUnit,
//                struAcsEventInfoExtendV20.byIsAbnomalTemperature, struAcsEventInfoExtendV20.fCurrTemperature,
//                struAcsEventInfoExtendV20.struRegionCoordinates.fX, struAcsEventInfoExtendV20.struRegionCoordinates.fY,
//                struAcsEventInfoExtendV20.dwQRCodeInfoLen, struAcsEventInfoExtendV20.dwVisibleLightDataLen,
//                struAcsEventInfoExtendV20.dwThermalDataLen);
//            printf("  扩展事件信息V20: 人脸坐标[x=%d,y=%d,w=%d,h=%d], 健康码[%d], 核酸检测[%d], 行程码[%d], 疫苗状态[%d], 考勤自定义标签[%s]\n",
//                struAcsEventInfoExtendV20.wXCoordinate, struAcsEventInfoExtendV20.wYCoordinate,
//                struAcsEventInfoExtendV20.wWidth, struAcsEventInfoExtendV20.wHeight,
//                struAcsEventInfoExtendV20.byHealthCode, struAcsEventInfoExtendV20.byNADCode, struAcsEventInfoExtendV20.byTravelCode,
//                struAcsEventInfoExtendV20.byVaccineStatus, (char*)struAcsEventInfoExtendV20.byAttendanceLabel);
//        }
//
//        //保存报警抓拍图片
//        if (struAcsAlarmInfo.dwPicDataLen > 0 && struAcsAlarmInfo.pPicData != NULL)
//        {
//            char cFilename[256] = { 0 };
//
//            char chTime[128];
//            sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", struAcsAlarmInfo.struTime.dwYear, struAcsAlarmInfo.struTime.dwMonth,
//                struAcsAlarmInfo.struTime.dwDay, struAcsAlarmInfo.struTime.dwHour, struAcsAlarmInfo.struTime.dwMinute,
//                struAcsAlarmInfo.struTime.dwSecond);
//
//            sprintf(cFilename, "COMM_ALARM_ACS_CapPic[%d][%s].jpg", pAlarmer->lUserID, chTime);
//
//            FILE* fSnapPicPlate = fopen(cFilename, "wb");
//            fwrite(struAcsAlarmInfo.pPicData, struAcsAlarmInfo.dwPicDataLen, 1, fSnapPicPlate);
//            fclose(fSnapPicPlate);
//            iNum++;
//            printf("  [抓拍图] 已保存: %s, 图片长度[%d]字节\n", cFilename, struAcsAlarmInfo.dwPicDataLen);
//        }
//        else
//        {
//            printf("  [抓拍图] 本次无抓拍图片, dwPicDataLen[%d], pPicData[%p]\n", struAcsAlarmInfo.dwPicDataLen, struAcsAlarmInfo.pPicData);
//        }
//    }
//    break;
//    case COMM_ID_INFO_ALARM: //门禁主机报警信息
//    {
//        NET_DVR_ID_CARD_INFO_ALARM struIDCardInfo = { 0 };
//        memcpy(&struIDCardInfo, pAlarmInfo, sizeof(NET_DVR_ID_CARD_INFO_ALARM));
//        printf("门禁身份证刷卡信息[0x5200]: 时间[%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d], 主类型[0x%x], 次类型[0x%x], 姓名[%s], 身份证号[%s], 读卡器号[%d]\n",
//            struIDCardInfo.struSwipeTime.wYear, struIDCardInfo.struSwipeTime.byMonth, struIDCardInfo.struSwipeTime.byDay, struIDCardInfo.struSwipeTime.byHour,
//            struIDCardInfo.struSwipeTime.byMinute, struIDCardInfo.struSwipeTime.bySecond, struIDCardInfo.dwMajor, struIDCardInfo.dwMinor,
//            (char*)struIDCardInfo.struIDCardCfg.byName, (char*)struIDCardInfo.struIDCardCfg.byIDNum, struIDCardInfo.dwCardReaderNo);
//
//        //身份证芯片里的详细信息
//        printf("  身份证信息: 性别[%d], 出生日期[%4.4d-%2.2d-%2.2d], 住址[%s], 签发机关[%s], 长期有效[%d]\n",
//            struIDCardInfo.struIDCardCfg.bySex, struIDCardInfo.struIDCardCfg.struBirth.wYear,
//            struIDCardInfo.struIDCardCfg.struBirth.byMonth, struIDCardInfo.struIDCardCfg.struBirth.byDay,
//            (char*)struIDCardInfo.struIDCardCfg.byAddr, (char*)struIDCardInfo.struIDCardCfg.byIssuingAuthority,
//            struIDCardInfo.struIDCardCfg.byTermOfValidity);
//
//        //刷卡事件详细信息
//        printf("  刷卡明细: 门号[%d], 卡类型[%d], 设备编号[%d], 读卡器类型[%d], 是否戴口罩[%d], 是否戴安全帽[%d], 是否实时事件[%d], 事件流水号[%d]\n",
//            struIDCardInfo.dwDoorNo, struIDCardInfo.byCardType, struIDCardInfo.byDeviceNo, struIDCardInfo.byCardReaderKind,
//            struIDCardInfo.byMask, struIDCardInfo.byHelmet, struIDCardInfo.byCurrentEvent, struIDCardInfo.dwSerialNo);
//        printf("  图片/指纹数据: 身份证图长度[%d], 抓拍图长度[%d], 证件抓拍图长度[%d], 指纹数据长度[%d]\n",
//            struIDCardInfo.dwPicDataLen, struIDCardInfo.dwCapturePicDataLen,
//            struIDCardInfo.dwCertificatePicDataLen, struIDCardInfo.dwFingerPrintDataLen);
//
//        //扩展信息，包含人体测温温度数据
//        if (struIDCardInfo.byIDCardInfoExtend == 1)
//        {
//            NET_DVR_ID_CARD_INFO_EXTEND struIDCardExtendV20 = { 0 };
//            memcpy(&struIDCardExtendV20, struIDCardInfo.pIDCardInfoExtend, sizeof(struIDCardExtendV20));
//
//            printf("  扩展事件信息: 远程核验[%d], 测温单位[%d], 温度异常[%d], 当前温度[%f], 测温坐标[%f,%f]\n",
//                struIDCardExtendV20.byRemoteCheck, struIDCardExtendV20.byThermometryUnit,
//                struIDCardExtendV20.byIsAbnomalTemperature, struIDCardExtendV20.fCurrTemperature,
//                struIDCardExtendV20.struRegionCoordinates.fX, struIDCardExtendV20.struRegionCoordinates.fY);
//        }
//
//        //保存身份证图片数据大小
//        if (struIDCardInfo.dwPicDataLen > 0 && struIDCardInfo.pPicData != NULL)
//        {
//            char cCardFilename[256] = { 0 };
//
//            char chTime[128];
//            sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", struIDCardInfo.struSwipeTime.wYear, struIDCardInfo.struSwipeTime.byMonth,
//                struIDCardInfo.struSwipeTime.byDay, struIDCardInfo.struSwipeTime.byHour,
//                struIDCardInfo.struSwipeTime.byMinute, struIDCardInfo.struSwipeTime.bySecond);
//
//            sprintf(cCardFilename, "COMM_ID_INFO_ALARM_IDCardPic[%d][%s].jpg", pAlarmer->lUserID, chTime);
//
//            FILE* fSnapPicPlate = fopen(cCardFilename, "wb");
//            fwrite(struIDCardInfo.pPicData, struIDCardInfo.dwPicDataLen, 1, fSnapPicPlate);
//            fclose(fSnapPicPlate);
//            iNum++;
//            printf("  [身份证图] 已保存: %s, 图片长度[%d]字节\n", cCardFilename, struIDCardInfo.dwPicDataLen);
//        }
//
//        //保存抓拍图片数据大小
//        if (struIDCardInfo.dwCapturePicDataLen > 0 && struIDCardInfo.pCapturePicData != NULL)
//        {
//            char cFilename[256] = { 0 };
//
//            char chTime[128];
//            sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", struIDCardInfo.struSwipeTime.wYear, struIDCardInfo.struSwipeTime.byMonth,
//                struIDCardInfo.struSwipeTime.byDay, struIDCardInfo.struSwipeTime.byHour,
//                struIDCardInfo.struSwipeTime.byMinute, struIDCardInfo.struSwipeTime.bySecond);
//
//            sprintf(cFilename, "COMM_ID_INFO_ALARM_CapPic[%d][%s].jpg", pAlarmer->lUserID, chTime);
//
//            FILE* fSnapPicPlate = fopen(cFilename, "wb");
//            fwrite(struIDCardInfo.pCapturePicData, struIDCardInfo.dwCapturePicDataLen, 1, fSnapPicPlate);
//            fclose(fSnapPicPlate);
//            iNum++;
//            printf("  [抓拍图] 已保存: %s, 图片长度[%d]字节\n", cFilename, struIDCardInfo.dwCapturePicDataLen);
//        }
//    }
//    break;
//    case COMM_UPLOAD_FACESNAP_RESULT: //人脸抓拍(识别结果)上传
//    {
//        NET_VCA_FACESNAP_RESULT struFaceSnap = { 0 };
//        memcpy(&struFaceSnap, pAlarmInfo, sizeof(NET_VCA_FACESNAP_RESULT));
//
//        printf("人脸抓拍结果[0x1112]: 人脸图ID[%d], 人脸评分[%d], 人脸图质量[%d], 活体状态[%d], 重复报警次数[%d], 停留画面时长[%.2f秒], 绝对时标[%d]\n",
//            struFaceSnap.dwFacePicID, struFaceSnap.dwFaceScore, struFaceSnap.byFacePicQuality,
//            struFaceSnap.byLivenessDetectionStatus, struFaceSnap.byRepeatTimes, struFaceSnap.fStayDuration, struFaceSnap.dwAbsTime);
//        //人体属性：0-未知，1-男/不戴/不微笑，2-女/戴/微笑，0xff-算法未识别
//        printf("  人体属性: 性别[%d], 是否戴眼镜[%d], 是否戴口罩[%d], 是否微笑[%d], 是否戴帽[%d], 表情[%d]\n",
//            struFaceSnap.struFeature.bySex, struFaceSnap.struFeature.byEyeGlass, struFaceSnap.struFeature.byMask,
//            struFaceSnap.struFeature.bySmile, struFaceSnap.struFeature.byHat, struFaceSnap.struFeature.byFaceExpression);
//        printf("  目标信息: 目标ID[%d], 人脸区域[x=%.3f,y=%.3f,w=%.3f,h=%.3f], 设备IP[%s], 端口[%d], 通道[%d], Ivms通道[%d]\n",
//            struFaceSnap.struTargetInfo.dwID, struFaceSnap.struRect.fX, struFaceSnap.struRect.fY,
//            struFaceSnap.struRect.fWidth, struFaceSnap.struRect.fHeight, struFaceSnap.struDevInfo.struDevIP.sIpV4,
//            struFaceSnap.struDevInfo.wPort, struFaceSnap.struDevInfo.byChannel, struFaceSnap.struDevInfo.byIvmsChannel);
//        printf("  图片信息: 人脸子图长度[%d], 背景图长度[%d], 上传方式[%d], 是否有附加信息[%d]\n",
//            struFaceSnap.dwFacePicLen, struFaceSnap.dwBackgroundPicLen, struFaceSnap.byUploadEventDataType, struFaceSnap.byAddInfo);
//
//        //附加信息(NET_VCA_FACESNAP_ADDINFO)：瞳距、人脸角度、遮挡、测温等
//        if (struFaceSnap.byAddInfo == 1 && struFaceSnap.pAddInfoBuffer != NULL)
//        {
//            NET_VCA_FACESNAP_ADDINFO struAddInfo = { 0 };
//            memcpy(&struAddInfo, struFaceSnap.pAddInfoBuffer, sizeof(struAddInfo));
//            printf("  附加信息: 旋转角[%d], 俯仰角[%d], 瞳距[%d], 遮挡状态[%d], 测温使能[%d], 温度异常[%d], 人脸温度[%f], 报警阈值[%f]\n",
//                struAddInfo.iSwingAngle, struAddInfo.iTiltAngle, struAddInfo.dwPupilDistance, struAddInfo.byBlockingState,
//                struAddInfo.byFaceSnapThermometryEnabled, struAddInfo.byIsAbnomalTemperature, struAddInfo.fFaceTemperature,
//                struAddInfo.fAlarmTemperature);
//        }
//
//        //保存人脸子图
//        if (struFaceSnap.dwFacePicLen > 0 && struFaceSnap.pBuffer1 != NULL)
//        {
//            char cFilename[256] = { 0 };
//            sprintf(cFilename, "COMM_UPLOAD_FACESNAP_RESULT_FacePic[%d][%d].jpg", pAlarmer->lUserID, struFaceSnap.dwFacePicID);
//
//            FILE* fFacePic = fopen(cFilename, "wb");
//            fwrite(struFaceSnap.pBuffer1, struFaceSnap.dwFacePicLen, 1, fFacePic);
//            fclose(fFacePic);
//            iNum++;
//            printf("  [人脸图] 已保存: %s, 图片长度[%d]字节\n", cFilename, struFaceSnap.dwFacePicLen);
//        }
//        else
//        {
//            printf("  [人脸图] 本次无人脸子图, dwFacePicLen[%d], pBuffer1[%p]\n", struFaceSnap.dwFacePicLen, struFaceSnap.pBuffer1);
//        }
//    }
//    break;
//    default:
//        printf("其他报警，报警信息类型(命令号): %d (0x%x)\n", lCommand, lCommand);
//        break;
//    }
//
//    return;
//}
//
//int main() {
//    SetConsoleOutputCP(CP_UTF8);   // 让控制台按 UTF-8 解释输出
//    SetConsoleCP(CP_UTF8);         // 输入也设为 UTF-8（可选）
//    //---------------------------------------
//    // 初始化
//    NET_DVR_Init();
//    //设置连接时间与重连时间
//    NET_DVR_SetConnectTime(2000, 1);
//    NET_DVR_SetReconnect(10000, true);
//
//    //---------------------------------------
//    // 注册设备
//    LONG lUserID;
//
//    //登录参数，包括设备地址、登录用户、密码等
//    NET_DVR_USER_LOGIN_INFO struLoginInfo = { 0 };
//    struLoginInfo.bUseAsynLogin = 0; //同步登录方式
//    strcpy(struLoginInfo.sDeviceAddress, "10.90.10.22"); //设备IP地址
//    struLoginInfo.wPort = 8000; //设备服务端口
//    strcpy(struLoginInfo.sUserName, "admin"); //设备登录用户名
//    strcpy(struLoginInfo.sPassword, "Tczn@2026"); //设备登录密码
//
//    //设备信息, 输出参数
//    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = { 0 };
//
//    lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
//    if (lUserID < 0)
//    {
//        printf("Login failed, error code: %d\n", NET_DVR_GetLastError());
//        NET_DVR_Cleanup();
//        return 0;
//    }
//
//    //设备登录成功，获取设备字符集编码类型
//    printf("Login successfully, byCharEncodeType: %d\n", struDeviceInfoV40.byCharEncodeType);
//
//
//    //设置报警回调函数
//    NET_DVR_SetDVRMessageCallBack_V50(0, MessageCallback, NULL);
//
//    //启用布防
//    LONG lHandle;
//    NET_DVR_SETUPALARM_PARAM  struAlarmParam = { 0 };
//    struAlarmParam.dwSize = sizeof(struAlarmParam);
//    struAlarmParam.byDeployType = 1; //布防类型：0-客户端布防（门禁设备只支持1路），1-实时布防（支持4路）
//    //不需要设置其他报警布防参数，不支持
//
//    lHandle = NET_DVR_SetupAlarmChan_V41(lUserID, &struAlarmParam);
//    if (lHandle < 0)
//    {
//        printf("NET_DVR_SetupAlarmChan_V41 failed, error code:  %d\n", NET_DVR_GetLastError());
//        NET_DVR_Logout(lUserID);
//        NET_DVR_Cleanup();
//        return 0;
//    }
//
//    //---------------------------------------
//    //实时预览：创建SDL2窗口，把窗口句柄交给SDK，画面由SDK自己解码并渲染
//    SDL_SetMainReady();
//    if (SDL_Init(SDL_INIT_VIDEO) != 0)
//    {
//        printf("SDL_Init failed: %s\n", SDL_GetError());
//    }
//
//    SDL_Window* sdlWindow = SDL_CreateWindow("海康设备实时监控",
//        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
//    if (sdlWindow == NULL)
//    {
//        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
//        NET_DVR_Logout(lUserID);
//        NET_DVR_Cleanup();
//        return 0;
//    }
//
//    //取出SDL窗口的Win32句柄(HWND)，交给海康SDK作为预览播放窗口
//    HWND hPlayWnd = NULL;
//    SDL_SysWMinfo wmInfo;
//    SDL_VERSION(&wmInfo.version);
//    if (SDL_GetWindowWMInfo(sdlWindow, &wmInfo))
//    {
//        hPlayWnd = wmInfo.info.win.window;
//    }
//    else
//    {
//        printf("SDL_GetWindowWMInfo failed: %s\n", SDL_GetError());
//    }
//
//    //启动实时预览
//    NET_DVR_PREVIEWINFO struPreviewInfo = { 0 };
//    struPreviewInfo.lChannel = 1;        //预览通道号，门禁一体机一般为1
//    struPreviewInfo.dwStreamType = 0;    //码流类型，0-主码流，1-子码流(带宽/延迟更低)
//    struPreviewInfo.dwLinkMode = 0;      //0-TCP方式取流
//    struPreviewInfo.hPlayWnd = hPlayWnd; //画面渲染窗口，为NULL则只取流不显示
//    struPreviewInfo.bBlocked = 0;        //0-非阻塞取流
//
//    LONG lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPreviewInfo, NULL, NULL);
//    if (lRealPlayHandle < 0)
//    {
//        printf("NET_DVR_RealPlay_V40 failed, error code: %d\n", NET_DVR_GetLastError());
//    }
//    else
//    {
//        printf("实时预览已开启，按 q 或关闭窗口退出\n");
//    }
//
//    //事件信息在回调函数里面获取
//    //主循环：一边处理SDL窗口事件(关闭按钮)，一边接收控制台按键，报警事件照常在回调里打印
//    char c = 0;
//    bool bQuit = false;
//    while (!bQuit && 'q' != c)
//    {
//        SDL_Event sdlEvent;
//        while (SDL_PollEvent(&sdlEvent))
//        {
//            if (sdlEvent.type == SDL_QUIT)
//            {
//                bQuit = true;
//            }
//        }
//        if (_kbhit())
//        {
//            c = _getch();
//        }
//        SDL_Delay(10);
//    }
//
//    //---------------------------------------
//    //退出程序
//
//    //停止实时预览并销毁SDL窗口
//    if (lRealPlayHandle >= 0)
//    {
//        NET_DVR_StopRealPlay(lRealPlayHandle);
//    }
//    SDL_DestroyWindow(sdlWindow);
//    SDL_Quit();
//
//    //撤销布防上传通道
//    if (!NET_DVR_CloseAlarmChan_V30(lHandle))
//    {
//        printf("NET_DVR_CloseAlarmChan_V30 failed, error code: %d\n", NET_DVR_GetLastError());
//        NET_DVR_Logout(lUserID);
//        NET_DVR_Cleanup();
//        return 0;
//    }
//
//    //注销用户
//    NET_DVR_Logout(lUserID);
//
//    //释放SDK资源
//    NET_DVR_Cleanup();
//    return 0;
//}
